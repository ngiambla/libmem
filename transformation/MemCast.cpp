/*===- MemCast.cpp - Linking in HLS-friendly malloc() and free() SW cores-----------------===//
 *
 * This file is distributed under the LegUp license. See LICENSE for details.
 *
 *===-------------------------------------------------------------------------------------===//
 * Author: Nicholas V. Giamblanco
 * Last Revision: May 28/19
 * 
 * This Pass changes the linkage for malloc(), free() and related calls to 
 * a user specified memory allocation scheme, suitable for high-level synthesis
 * i.e:
 *
 *		{malloc,calloc,realloc,free}() -->[MemCast.cpp]--> [x_]{malloc,calloc,realloc,free}()
 *
 * Where x is a prefix from 1 of 5 allocaton schemes.
 * Five allocation schemes are available:
 * [gnu_] dlmalloc() (used by GNU unix-like OS)
 * [lin_] linear allocator.
 * [bit_] bitmap allocator.
 * [lut_] LUT allocator. 
 * [bud_] Buddy allocator.
 *
 *
 * This pass uses Valgrind to dynamically profile the C application, and determine the total
 * heap usage required for the program to run. Valgrind was modified to 'spit-out' a Makefile
 * which can be used to recompile our [de]allocators with a pretty good 'estimate' of the total
 * heap required (we pad the heap with extra bytes for safety.)
 *
 * The entire process looks like:
 * 
 * [src.c] --> (gcc -o src.exe src.c -pthread) --> (valgrind -opts[] ./src.exe) --> [Makefile]
 *
 * We then recompile our [de]allocators with this Makefile.
 *
 * We can replicate each [de]allocator to achieve multi-heap designs, such that 
 * it can improve performance for concurrent accesses to heaps in hardware (for multithreaded 
 * applications) or to reduce cycles spent on searching through lists.
 *
 * We designed an analysis which can connect {malloc,calloc,realloc}* calls to free calls,
 * This constructs a bi-partite graph which we traverse using a DFS to identify connected 
 * {malloc,calloc,realloc}-free groupings. Each grouping is assigned to one of the 
 * N-1 available heaps. Each {m,c,re}alloc() and free() call in the instruction stream is assigned a unique ID
 * (e.g. %1 = tail call noalias i8* @malloc(i32 8) is assigned M1). 
 *
 *     ex)
 *             [HEAP 0]    ...    [HEAP n]    ...  [Heap N-1]  
 *            
 *                (M1)           (M2)   (M3)        (M4)
 *               /   \     ...    |  ___/     ...    |
 * 				/     \           | /                |
 *           (F1)     (F2)       (F3)               (F4)
 *
 * ######################################################################################
 * NOTE: LegUp doesn't support type casting well -> stick to casting to int32 words...
 * ######################################################################################
 *
 *===-------------------------------------------------------------------------------------===*/

// C/C++-Libs
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>

// LLVM-libs
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Attributes.h"

#include "llvm/Linker/Linker.h"

#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "utils.h"


#define NUMFUNCS 3
#define MAX_INSPECTION_THRESH 1000	// Stops this analysis from blowing up on large files.

using namespace llvm;
namespace legup{

	typedef struct fnode_t {
		int color;
		int start;
		int end;
		std::string name;
		struct fnode_t * parent;
	} fnode_t;	

	// These 4 classes (mNode, mGraph, iNode, iPath) are used in our analysis to generate our
	// malloc() and free() bi-partite graph.
	class mNode {
		private:
			Value * mem;
			std::vector<mNode *> mNodes;
		public:
			mNode(Value * mem) {
				this->mem = mem;
			}

			void addmNode(mNode * m) {
				mNodes.push_back(m);
			}

			Value * getValue() {
				return this->mem;
			}

			std::vector<mNode *> getmNodes() {
				return mNodes;
			}

	};

	class mGraph {
		private:
			std::unordered_map<Value *, mNode *> val_2_mNode;

		public:

			mNode * getOrInsertmNode(Value * v) {
				// doesn't exist!
				if(val_2_mNode.find(v) == val_2_mNode.end()) {
					mNode * n = new mNode(v);
					val_2_mNode[v] = n;
				}
				return val_2_mNode[v];
			}
	};


	class iNode {
		private:
			iNode * parent;
			Value * value; 
		public:
			iNode(iNode * parent, Value * value) {
				this->value = value;
				this->parent = parent;
			}

			Value * getValue() {
				return this->value;
			}

			iNode * getParent() {
				return this->parent;
			}
	};		

	class iPath {
		private:
			Value * free;
			std::unordered_map<Value *, bool> mallocs;
			std::unordered_map<Value *, iNode *> i_2_iNode;

		public:
			iPath(iNode * free) {
				this->free = free->getValue();
				iNode * tmp = free;
				while(tmp) {
					i_2_iNode[tmp->getValue()]=tmp;
					if(tmp->getParent() == NULL) {
						mallocs[tmp->getValue()]=true;
					}
					tmp=tmp->getParent();
				}
			}

			std::vector<Value *> getMallocs() {
				std::vector<Value *> v;
				for(auto item : mallocs) {
					v.push_back(item.first);
				}
				return v;
			}

			void addMalloc(Value * malloc) {
				mallocs[malloc] = true;
			}

			iNode * getTailiNode() {
				return i_2_iNode[free];
			}

			Value * getFree() {
				return this->free;
			}

			iNode * getiNodeFromInst(Value * inst) {
				if(i_2_iNode.find(inst) == i_2_iNode.end())
					return NULL;
				return i_2_iNode[inst];
			}

			bool instInPath(Value * inst) {
				return i_2_iNode.find(inst) != i_2_iNode.end();
			}
	};

	struct MemCast : public ModulePass {

		static char ID; 													// ID of pass.
		
		MemCast() : ModulePass(ID) {}

		bool bitcodeModified = false; 											// notify LLVM if we actually replaced malloc and free.

		//==-- AllocaReplacer Stat Variables
		unsigned max_heap_size = 0;
		unsigned max_reserved_size = 0;
		unsigned time = 0;
		//==-- AllocaReplacer Node Variables
		std::unordered_map<std::string, fnode_t > fnodes;
		enum colors{WHITE, GREY, BLACK};

		//==-- All LegUp Config Parameters...
		int sar2DymasCall 		= LEGUP_CONFIG->getParameterInt("ALLOCA_REPLACE");
 		int malloc_replace 		= LEGUP_CONFIG->getParameterInt("AUTO_MALLOC");
 		int profile_heaps 		= LEGUP_CONFIG->getParameterInt("PROFILE_HEAPS");
 		int heap_size 			= LEGUP_CONFIG->getParameterInt("HEAP_SIZE");
 		int heap2thr 			= LEGUP_CONFIG->getParameterInt("ASSIGN_HEAP_2_THREAD");
 		int min_req_size 		= LEGUP_CONFIG->getParameterInt("MIN_REQ_SIZE");    	
		int allocator 			= LEGUP_CONFIG->getParameterInt("ALLOC_SCHEME");
		int isLazy 				= LEGUP_CONFIG->getParameterInt("LAZY_FREE");
		unsigned MAX_PARTITION 	= LEGUP_CONFIG->getParameterInt("NUM_HEAPS");
    	
    	//==-- Allocator Keywords.
    	const std::string allocators[NUMFUNCS] = { "malloc", "realloc", "calloc" };
    	const std::string free = "free";

    	//==-- Path to allocator library.
    	std::string abs_path_to_alloc = std::getenv("DIRLIBMEM");

		std::vector<iPath *> all_free_paths;		

    	std::unordered_map<Value *, bool> seen_insts_map;
		std::unordered_map<Value *, bool> all_frees_map;
		std::unordered_map<Value *, bool> all_allocators_map;
		std::vector<std::vector<mNode *> > partitionedNodes;
		std::unordered_map<Function * , std::vector<Value * > > memfunc_to_func_map;

		int getArrayParameters(Type * arrayType, Type ** dataType) {
			int size = 1;
			Type * tty = arrayType;
			while(tty->getArrayElementType()->isArrayTy()) {		
				size *= tty->getArrayElementType()->getArrayNumElements();
				tty=tty->getArrayElementType();
				*dataType = tty->getContainedType(0);
				if(tty == NULL) {
					assert(0);
				}
			}
			return size;
		}

		Type * getCastType(Type * ty, Function * F) {
			if(ty->isFloatingPointTy()) {
				return NULL;
			} else if(ty->isIntegerTy()) {
				return llvm::Type::getIntNPtrTy(F->getContext(), ty->getIntegerBitWidth());
			} else {
				return NULL;
			}
		}		

		void RecursiveDFSSearchForSAR(Module &M) {
			for(Function &F : M) {
				fnode_t fnode;
				fnode.color = WHITE;
				fnode.name = F.getName();
				fnode.parent = NULL;
				fnodes[F.getName()] = fnode;
			}

			if(M.getFunction("main") != NULL && fnodes["main"].color == WHITE) {
				visit_node(M, *(M.getFunction("main")), 0, 0);
			}

			//==-- If the replacement heap is larger than or equal to the statically reserved size, there is
			// no benefit to using this.
			if((float)max_heap_size/((float)max_reserved_size) >= 1) {
				errs() << "Skipping SAr Replacement.\n";
				max_heap_size = 0;
				return;
			}

			errs() << "Max Heap Size: "<< max_heap_size << "\n";
			errs() << "Max Rsrv Size: "<< max_reserved_size << "\n"; 

			//==-- Otherwise, reset this, and continue processing, replacing statically allocated arrays with
			//     malloc calls.
			for(auto key : fnodes) {
				fnode_t * fnode = &fnodes[key.first];
				fnode->color = WHITE;
			} 

			if(M.getFunction("main") != NULL && fnodes["main"].color == WHITE) {
				visit_and_replace_node(M, *(M.getFunction("main")), 0);
			}

		}


		void visit_node(Module &M, Function  &F, int depth, int cur_heap_size) {
			fnode_t *fnode = &fnodes[F.getName()];
			fnode->start = ++time;

			CallGraph C = CallGraph(M);
			CallGraphNode * cgn = C[&F];
			unsigned numElems = cur_heap_size;

			for(BasicBlock &B : F) {
				for(Instruction &I : B) {
					if(isa<AllocaInst>(&I)) {
						AllocaInst * AI = dyn_cast<AllocaInst>(&I);
						if(AI->isStaticAlloca()) {
							if(AI->getType()->isPointerTy()) {
						 		if(AI->getType()->getElementType()->isArrayTy() && AI->getAlignment() >=4) {
									Type * AllocaArrayPointerType;  // Pointer Element Type for Alloca Array
	 								unsigned cNumElems = getArrayParameters(AI->getType(), &AllocaArrayPointerType);			 								
	 								numElems += cNumElems;
	 								if(fnode->color == WHITE) {
		 								max_reserved_size +=cNumElems;
		 								errs() << *AI << "\n";
	 								}

						 		}
						 	}
						}
					}
				}
			}	


			fnode->color = BLACK;
			if(max_heap_size < numElems) {
				max_heap_size = numElems;
			}

			for(CallGraphNode::iterator fn = cgn->begin(); fn != cgn->end(); ++fn ) {
				CallGraphNode * cgn_t = (*fn).second; 
				Function * cf = cgn_t->getFunction();
				if(cf != NULL) {
					fnodes[cf->getName()].parent = fnode;
					visit_node(M, *cf, depth++, numElems);
				}
			}
			fnode->end=++time;
		}	


		void visit_and_replace_node(Module &M, Function  &F, int depth) {
			fnode_t *fnode = &fnodes[F.getName()];
			fnode->color = GREY;

			CallGraph C = CallGraph(M);
			CallGraphNode * cgn = C[&F];

			std::vector<Instruction *> memallocs;
			std::vector<Instruction *> bitcasts;
			std::vector<Instruction *> allocaInsts;

			for(BasicBlock &B : F) {
				for(Instruction &I : B) {
					if(isa<AllocaInst>(&I)) {
						AllocaInst * AI = dyn_cast<AllocaInst>(&I);
						if(AI->isStaticAlloca()) {
							if(AI->getType()->isPointerTy()) {
						 		if(AI->getType()->getElementType()->isArrayTy() && AI->getAlignment() >=4) {

						 			Type * StdInt32 = llvm::Type::getInt32Ty(F.getContext());
									Type * AllocaArrayPointerType;  // Pointer Element Type for Alloca Array
	 								unsigned numElems = getArrayParameters(AI->getType(), &AllocaArrayPointerType);			 								
									Constant * AllocSize = llvm::ConstantInt::get(llvm::Type::getInt32Ty(F.getContext()), AllocaArrayPointerType->getPrimitiveSizeInBits()/8);
									Constant * ArraySize = llvm::ConstantInt::get(llvm::Type::getInt32Ty(F.getContext()), numElems);
									AllocSize = ConstantExpr::getTruncOrBitCast(AllocSize, StdInt32);
									Instruction* memalloc = CallInst::CreateMalloc(&I,
									                                             	StdInt32, 
									                                             	AI->getType()->getArrayElementType(), 
									                                             	AllocSize,
									                                             	ArraySize, 
									                                             	nullptr, 
									                                             	""
									                                               );
									

									if(isa<BitCastInst>(memalloc)) {
										memallocs.push_back(memalloc->getPrevNode());
										bitcasts.push_back(memalloc);
									} else {
										memallocs.push_back(memalloc);
									}

									allocaInsts.push_back(&I);
						 		}
						 	}
						}
					}
				}
			}	

			for(unsigned int i = 0; i < allocaInsts.size(); ++i) {									
				allocaInsts[i]->replaceAllUsesWith(bitcasts[i]);
				allocaInsts[i]->eraseFromParent();			
			}
			
			for(BasicBlock &B : F) {
				for(Instruction &I : B) {
					if(isa<ReturnInst>(&I)) {
						ReturnInst *RI = dyn_cast<ReturnInst>(&I);
						for(auto MI : memallocs) {
							CallInst::CreateFree(MI, RI);
						}
					}						
				}
			}			

			for(CallGraphNode::iterator fn = cgn->begin(); fn != cgn->end(); ++fn ) {
				CallGraphNode * cgn_t = (*fn).second; 
				Function * cf = cgn_t->getFunction();
				if(cf != NULL && fnodes[cf->getName()].color == WHITE) {
					fnodes[cf->getName()].parent = fnode;
					visit_and_replace_node(M, *cf, depth++);
				}
			}
			fnode->color = BLACK;
		}	


		bool runOnModule(Module &M) override {

			if(!malloc_replace) {
				if(populateAllocatorMaps(M)) {
					// Notify User that malloc and free exist in the app
					// and that synthesis will fail
					errs() << "NOTE: {m,c,re}alloc() and/or free() are used in this application.\n";
					errs() << "Synthesis will fail unless you replace these calls with one of\n";
					errs() << "allocation schemes from libmem.";
				}				
				return false;
			}

			if(sar2DymasCall) {
				errs() << "About to replace Stack-Allocated Arrays...\n";
				RecursiveDFSSearchForSAR(M);
			}

			if(!populateAllocatorMaps(M)) {
				return false;
			}

			setHeapSize(M);

			linkAllocatorsToFrees(M);

			std::string tag= "";
			switch(allocator) {
				case 0:
					tag = "gnu";
					break;
				case 1:
					tag = "lin";
					break;
				case 2:
					tag = "bit";
					break;
				case 3:
					tag = "lut";
					break;
				case 4:
					tag = "bud";
					break;
				default:
					tag = "gnu";
			}
			performSpecializedCasting(M, tag);
			return true;
		}


		// We automatically profile a program using Valgrind, as Valgrind reports the maximum heap usage over the program lifetime.
		// Valgrind was modified to produce a Makefile which sets a -D (define) for the required arena size for our allocators.
		// Different command line options are available and can be inspected by going to legup_dir/valgrind/build/bin/
		// and running ./valgrind --help.
		// Here, we automatically run valgrind during this Pass Execution.
		void setHeapSize(Module &M) {
			std::string clo_builder="";

			if(profile_heaps <= 0) {
				clo_builder += "--legup-profile=no ";
				if(heap_size > 0) {
					clo_builder += "--legup-arena="+std::to_string(heap_size)+" ";
				}
			}

			if(heap2thr <= 0) {
				clo_builder += "--legup-arena2thr=no ";
			}

			if(min_req_size > 0) {
				clo_builder += "--legup-mrs="+std::to_string(min_req_size)+" ";
			} else {
				clo_builder += "--legup-mrs=1 ";
			}

			if(max_heap_size > 0) {
				clo_builder += "--legup-bumpup="+std::to_string(max_heap_size*4)+ " ";
			}

			std::string source_file = sanitizeModuleName(M.getModuleIdentifier())[0];
			std::string gcc_builder = "gcc -o "+source_file+".exe "+source_file+".c -pthread -DPRINTF_HIDE -O0";
			std::string rm_exe_builder = "rm "+source_file+".exe";
			std::string valgrind_builder = std::getenv("LEGUP_VALGRIND");
			valgrind_builder+="./valgrind "+clo_builder + "./"+source_file+".exe";

			//updated absolute
			std::string recomp_builder_clean	= "make clean -C "+ abs_path_to_alloc;
			std::string recomp_builder__make	= "make -C "+ abs_path_to_alloc;

			errs() << "    [MEMCAST] Compiling source with gcc\n";
			errs() << "        [gcc]-> " << runCommand(gcc_builder);
			errs() << "    [MEMCAST] Generating Makefile with Valgrind\n";
			errs() << "        [valgrind]-> " << runCommand(valgrind_builder);
			errs() << "    [MEMCAST] Cleaning Up Intermediates\n";
			errs() << "        [valgrind]-> " << runCommand(rm_exe_builder);						
			errs() << "    [MEMCAST] Recompiling Allocators with heap-config.\n";
			errs() << "        [Makefile]-> " << runCommand(recomp_builder_clean);					
			errs() << "        [Makefile]-> " << runCommand(recomp_builder__make);					
		}

		std::vector<std::string> sanitizeModuleName(std::string s) {
			vector<std::string> names;
			std::string delimiter = ".";
			size_t pos = 0;
			std::string token;
			while ((pos = s.find(delimiter)) != std::string::npos) {
				token = s.substr(0, pos);
				names.push_back(token);
				s.erase(0, pos + delimiter.length());
			}
			names.push_back(s);
			return names;
		}

		std::string runCommand(std::string cmd) {
			char buffer[128];
			std::string result = "";
			cmd += " 2>&1";
			FILE* pipe = popen(cmd.c_str(), "r");
			if (!pipe){
				return "";
			} 
			
			while (fgets(buffer, sizeof buffer, pipe) != NULL) {
				result += buffer;
			}
			pclose(pipe);
			if(result == "") {
				return cmd+" = OK.\n";
			}
			return result;
		}		


		bool usingPthreads(Module &M) {
			for(auto &MF : M) {
				if(MF.getName().find("pthread") != std::string::npos) {
					return true;
				}
			}
			return false;
		}

		/*=--------------------------------------------------------------------------------------------
		Parameters: Module &M : bitcode module.
		** This functions locates dynamic memory calls and stores their locations in a map 
		and differentiates frees and and [m|re|c]allocs() into two different maps.
		--------------------------------------------------------------------------------------------=*/
		bool populateAllocatorMaps(Module &M) {
			// Reset all maps.
			all_frees_map.clear();
			all_allocators_map.clear();

			for(Function &F: M) {
				for(auto &B : F) {
					for(BasicBlock::iterator Iptr = B.begin(), E = B.end(); Iptr != E; Iptr++) {
						Instruction * I = &(*Iptr);
						if(isa<CallInst>(I)) {
							CallInst * CI = dyn_cast<CallInst>(I);
							if(CI->getCalledFunction()->getName().compare(free) == 0) {
								Value * vi = dyn_cast<Value>(I);
								all_frees_map[vi] = false;
							}
							for(int i = 0; i < NUMFUNCS; ++i) {
								if(CI->getCalledFunction()->getName().compare(allocators[i]) == 0) {
									Value * vi = dyn_cast<Value>(I);
									all_allocators_map[vi] = false;
								}	
							}
						}						
					}
				}
			}
			if(all_frees_map.size() == 0 && all_allocators_map.size() == 0) {
				return false;
			}
			return true;
		}


		// As the title suggests, this function determines the connectivity of 
		// malloc and frees within the program structure/
		// i.e.
		//
		//  void fun1() { 
		//  	int * a = (int *)malloc(BYTES); // malloc_id =2
		//		/* Do things */
		//      if(cond)
		//			fun2(a);
		//      else
		//      	free(a); // free_id =1
		//  }
		//
		//  ...
		//
		//  void fun2(int * a) {
		//  	free(a); // free_id = 2
		//	}
		//
		//  malloc with malloc_id=1 can touch frees with id = 1 AND 2. 
		//  hence, we search the instruction graph and search for these connections
		//
		//  NOTE: These connections form a bipartite graph.
		void linkAllocatorsToFrees(Module &M) {

			// First collect each memfunc call.
			for(Function &F: M) {
				for(auto &B : F) {
					for(BasicBlock::iterator Iptr = B.begin(), E = B.end(); Iptr != E; Iptr++) {
						Instruction * I = &(*Iptr);
						if(isa<CallInst>(I)) {
							CallInst * CI = dyn_cast<CallInst>(I);
							for(int i = 0; i < NUMFUNCS; ++i) {
								if(CI->getCalledFunction()->getName().compare(allocators[i]) == 0) {
									Value * vi = dyn_cast<Value>(I);
									memfunc_to_func_map[&F].push_back(vi);
								}
							}
						}						
					}
				}
			}

			std::vector<Value *> mallocs_2_revisit;

			for(auto item : memfunc_to_func_map) {
				errs() << "  +Checking memory functions within: " << item.first->getName() << "\n";
				std::vector<Value *> V = item.second;
				for(unsigned int i = 0; i < V.size(); ++i) {
					Value * v = V[i];

					errs() << "  +-- [NEW] Inspecting-> "<< *(v) << "\n";
					seen_insts_map.clear();
					std::vector<iPath *> free_paths;

					iNode * top = new iNode(NULL, v);
					findAllFrees(v, 0, top, free_paths);

					if(free_paths.size() == 0) {
						mallocs_2_revisit.push_back(v);
					}
					
					for(auto path : free_paths) {
						all_free_paths.push_back(path);
					}

					errs() << "\n";
				}
			}

			// For unpaired mallocs() -> it could be part of a n-d allocationl
			// e.g.
			//	int ** a;
			//	a = (int**)malloc(ROWS*sizeof(int *));
			//  for(int i = 0; i < ROWS; ++i) {
			// 		a[i] = (int *)malloc(COLS*sizeof(int));
			//	}
			// so search back UP the instruction graph.
			// 
			for(Value * v : mallocs_2_revisit) {
				seen_insts_map.clear();
				errs() << "  +-- [NEW] Revisiting "<< *v << "\n";
				retraceMalloc(v,v,0);
			}


			errs() << "  +-- [NEW] Constructing Bipartite Graph.\n";

			mGraph * mgr = new mGraph();
			for(iPath * path : all_free_paths) {
				mNode * freeNode = mgr->getOrInsertmNode(path->getFree());
				for(Value * malloc : path->getMallocs()) {
					mNode * mallocNode = mgr->getOrInsertmNode(malloc);
					freeNode->addmNode(mallocNode);
					mallocNode->addmNode(freeNode);
				}
			}


			std::unordered_map<mNode *,bool> visitedNodes;
			

			errs() << "  +- - - -+==== Parsing Graph ====+- - - -+\n";
			for(auto item : memfunc_to_func_map) {
				std::vector<Value *> V = item.second;
				for(unsigned int i = 0; i < V.size(); ++i) {
					Value * v = V[i];
					mNode * mn = mgr->getOrInsertmNode(v);
					if(visitedNodes.find(mn)==visitedNodes.end()) {
						std::vector<mNode *> found = searchConnectedSubgraph(mn);
						errs() << "\n";
						partitionedNodes.push_back(found);
						for(mNode * k : found) {
							visitedNodes[k] = true;
							all_allocators_map.erase(k->getValue());
							all_frees_map.erase(k->getValue());
						}
					}
				}
			}			

			if(all_allocators_map.size() > 0) {
				errs() << "  +-- Unpaired Mallocs --+\n";
				for(auto item : all_allocators_map) {
					errs() << "  +--> "<< *(item.first) << "\n";
				}
				errs() << "  +----------------------+\n\n";
				partitionedNodes.clear();
			}

			if(all_frees_map.size() > 0) {
				errs() << "  +-- Unpaired Frees   --+\n";
				for(auto item : all_frees_map) {
					errs() << "  +--> "<< *(item.first) << "\n";
				}
				errs() << "  +----------------------+\n\n";
				partitionedNodes.clear();
			}


		}

		std::vector<mNode *> searchConnectedSubgraph(mNode * m) {
			std::unordered_map<mNode *,bool> visitedNodes;
			std::stack<mNode *> nodesToVisit;

			visitedNodes[m] = true;

			for(mNode * mn : m->getmNodes()) {
				nodesToVisit.push(mn);
			}

			while(!nodesToVisit.empty()) {
				mNode * mn = nodesToVisit.top();
				nodesToVisit.pop();

				if(visitedNodes.find(mn)==visitedNodes.end()) {
					visitedNodes[mn] == true;
					for(mNode * mnn : mn->getmNodes()) {
						nodesToVisit.push(mnn);
					}	
				} 
			}

			std::vector<mNode *> v;
			for(auto thing : visitedNodes) {
				errs() << *(thing.first->getValue()) << "\n";
				v.push_back(thing.first);
			}
			return v;
		}

		void retraceUp(Value * malloc, Value * v) {
			for(iPath * path : all_free_paths) {
				iNode * tail = path->getTailiNode();
				iNode * tmp = tail;
				while(tmp) {
					Value * uv = tmp->getValue();
					if(isa<StoreInst>(uv)) {
						User * uu = dyn_cast<User>(uv);
						if(uu->getOperand(1) == v) {
							path->addMalloc(malloc);
						}
					}
					tmp = tmp->getParent();
				}
			}

			User * u = dyn_cast<User>(v);
			if(u && u->getNumOperands() > 0) {
				retraceUp(malloc, u->getOperand(0));
			}
		}

		void retraceMalloc(Value * malloc, Value * v, int level) {

			if(level > MAX_INSPECTION_THRESH) {
				errs() << "Recursive depth seems unreasonable.\n";
				errs() << "If you'd like to override, please recompile this pass with a larger threshold\n";
				errs() << "by setting MAX_INSPECTION_THRESH to your desired value.\n";
				assert(0);				
			}

			for(auto U : v->users()) {
				if(auto I = dyn_cast<Instruction>(U)) {
					// If we've seen the instruction before, go to next avail instruction
					// else, remember the instruction and process.
					if(seen_insts_map.count(I)) {												
						continue;
					} else {
						seen_insts_map[I] = true;
					}
					// Begin Instruction Search Specialization procedure.
					if(isa<StoreInst>(U)) { 		// For store, look into uses of second operand.	
						if(v == U->getOperand(0)) {
							// begin upwardSearch
							retraceUp(malloc, U->getOperand(1));
						}

						retraceMalloc(malloc, dyn_cast<Value>(U->getOperand(1)), ++level);

					} else if(isa<CallInst>(U)) { 	
						// For CallInst, first check if it is free... otherwise, go to function with correct operand.
						CallInst * CI = dyn_cast<CallInst>(U);
						Function * F = CI->getCalledFunction();
						int numArgOp = CI->getNumArgOperands();
						int whichVarArg = 0;
						int whichArgOp = -1;
						Value * argument;

						for(int i = 0; i < numArgOp; ++i) {
							if(CI->getArgOperand(i) == v) {
								whichArgOp = i;
								break;
							}
						}

						for (Function::arg_iterator AI = F->arg_begin(), E = F->arg_end(); AI != E; ++AI) {
							if(whichArgOp == whichVarArg) {
								argument = &(*AI);
								break;
							}
							++whichVarArg;
						}
						if(whichArgOp >=0 && !F->isDeclaration()) {
							retraceMalloc(malloc, argument, ++level);	
						} else {
							retraceMalloc(malloc, dyn_cast<Value>(U), ++level);
						}
					} else if(isa<ReturnInst>(U)) {
						// Checking where all function calls with this ret are used.
						retraceMalloc(malloc, dyn_cast<Instruction>(U)->getParent()->getParent(), ++level); 
					} else {
						// Just proceed with normally traversal.
						retraceMalloc(malloc, dyn_cast<Value>(U), ++level);
					}
				} else {
					errs() << "Error: Unexpected Traversal through instruction stream.\n";
					errs() << *U << "\n";
				}
			}
		}

		// This function searches an instruction graph for all users of a malloc() until we reach a free() or terminate;
		// We push each user of a malloc on a stack, and search for those users... until a free() is found;
		// We search the graph using a DFS.
		// e.g.
		//		
		// 		%1 = tail call noalias i8* @malloc(i32 8) 	[QUEUE users of %1, which are %2 and %3, and search];
		// 		%2 = bitcast i8* %1 to %struct.HashTable* 	[QUEUE users of %2, which are ...]
		// 			[Searching for free()]
		//				...
		//			[Did not Find a free(), proceeds to search %3's users]			
		// 		%3 = icmp eq i8* %1, null 					[QUEUE users of %3, which ]
		// 	 		[Searching for free()]
		//				...
		//
		void findAllFrees(Value * v, int level, iNode * n, std::vector<iPath *> &freePaths) {

			if(level > MAX_INSPECTION_THRESH) {
				errs() << "Recursive depth seems unreasonable.\n";
				errs() << "If you'd like to override, please recompile this pass with a larger threshold\n";
				errs() << "by setting MAX_INSPECTION_THRESH to your desired value.\n";
				assert(0);				
			}

			for(auto U : v->users()) {

				if(auto I = dyn_cast<Instruction>(U)) {
					
				 	iNode * in = new iNode(n, I);
				 	if(!in) {
				 		errs() << "Error. NULL inode.\n";
				 		assert(0);
				 	}

					// If we've seen the instruction before, go to next avail instruction
					// else, remember the instruction and process. This avoids infinite loops.
					if(seen_insts_map.count(I)) {												
						continue;
					} else {
						seen_insts_map[I] = true;
					}

					// Begin Instruction Search Specialization procedure.
					if(isa<StoreInst>(U)) { 		// For store, look into uses of second operand.
						if(isa<GetElementPtrInst>(dyn_cast<Value>(U->getOperand(1)))) {
							findAllFrees(dyn_cast<GetElementPtrInst>(U->getOperand(1))->getPointerOperand(), ++level, in, freePaths);
						} else
							findAllFrees(dyn_cast<Value>(U->getOperand(1)), ++level, in, freePaths);
					} else if(isa<CallInst>(U)) { 	// For CallInst, first check if it is free... otherwise, go to function with correct operand.
						CallInst * CI = dyn_cast<CallInst>(U);
						// If free, save node and map out instructions.
						if(CI->getCalledFunction()->getName().compare(free) == 0) {
							freePaths.push_back(new iPath(in));
						}

						Function * F = CI->getCalledFunction();
						int numArgOp = CI->getNumArgOperands();
						int whichVarArg = 0;
						int whichArgOp = -1;
						Value * argument;

						for(int i = 0; i < numArgOp; ++i) {
							if(CI->getArgOperand(i) == v) {
								whichArgOp = i;
								break;
							}
						}


						for (Function::arg_iterator AI = F->arg_begin(), E = F->arg_end(); AI != E; ++AI) {
							if(whichArgOp == whichVarArg) {
								argument = &(*AI);
								break;
							}
							++whichVarArg;
						}
						if(whichArgOp >=0 && !F->isDeclaration()) {
							findAllFrees(argument, ++level, in, freePaths);											
						} else {
							findAllFrees(dyn_cast<Value>(U), ++level, in, freePaths);
						}
					} else if(isa<ReturnInst>(U)) {
						findAllFrees(dyn_cast<Instruction>(U)->getParent()->getParent(), ++level, in,freePaths);
					} else if(isa<BranchInst>(U)) {
						BranchInst * BI = dyn_cast<BranchInst>(U);
						for(unsigned int i = 0; i < BI->getNumSuccessors(); ++i) {
							Value * vv = &(*BI->getSuccessor(i)->begin());
							findAllFrees(vv, ++level, in,freePaths);
						}
					} else {
						findAllFrees(dyn_cast<Value>(U), ++level, in, freePaths);
					}
				} else if(isa<ConstantExpr>(U) && dyn_cast<ConstantExpr>(U)->isGEPWithNoNotionalOverIndexing()) {

					if(seen_insts_map.count(U)) {												
						continue;
					} else {
						seen_insts_map[U] = true;
					}

					iNode * in = new iNode(n, U);
					findAllFrees(dyn_cast<ConstantExpr>(U)->getOperand(0), ++level, in, freePaths);
					//assert(0);
				} else {
					errs() << "Error: Unexpected Traversal through instruction stream.\n";
					errs() << *U << "\n";
				}
			}
		}	

		void attachMetadata(Value * V, const char * metadataName, std::string metadataString) {
			Instruction * instr = dyn_cast<Instruction>(V);
			LLVMContext &C = instr->getContext();
			MDNode * N = MDNode::get(C, MDString::get(C, metadataString));
			instr->setMetadata(metadataName, N);
		}		

		void performSpecializedCasting(Module &M, std::string tag) {

			Linker *L = new Linker(&M, false);	
			LLVMContext &Context = getGlobalContext();
			SMDiagnostic Err;

			Module * alloc_scheme;
			Module * pthread_utils;

			// First, fetch the allocator the user selected.
			std::string allocator_loc = abs_path_to_alloc+"/lib"+tag+"mem.bc";
			alloc_scheme = ParseIRFile(allocator_loc.c_str(), Err, Context);
			if (!alloc_scheme) {
				Err.print(("Could not find the allocation library: lib"+tag+"mem\n").c_str(), errs());
				assert(false);
			}


			// Next, if pthreads are used, get pthread API for malloc/free.
			if(usingPthreads(M)) {
				std::string pthread_loc = abs_path_to_alloc+"/pthread_utils.bc";
				pthread_utils = ParseIRFile(pthread_loc.c_str(), Err, Context);
				if (!pthread_utils) {
					Err.print("Could not find the pthread_utility function, pthread_utils.bc\n", errs());
					assert(false);
				}
			}

			// If the user has selected to use multiple heaps, allow 
			// up to the N heaps the user specified to be paired with
			// the connected components of the biparitite malloc and free graph.
			if(partitionedNodes.size() > 1 && MAX_PARTITION > 1 ) {
				errs() << "==> Maximum  # of Heaps: "<< MAX_PARTITION << "\n";
				errs() << "==> Possible # of Heaps: "<< partitionedNodes.size() << "\n";
				int maxHeaps = (partitionedNodes.size() < MAX_PARTITION) ? partitionedNodes.size() : MAX_PARTITION;
				errs() << "==> Heaps Assigned:      "<< maxHeaps << "\n";

				std::vector<Module *> alloc_clones;
				std::vector<Module *> pthread_utils_clones;

				for(int i = 0; i < maxHeaps; ++i) {
					Module * MA = CloneModule(alloc_scheme);
					if(!MA) {
						errs() << "Cloned module is NULL\n";
						assert(0);
					}
					for(Module::iterator bf = MA->begin(), ef = MA->end(); bf != ef; ++bf) {
						Function * FF = &(*bf); 
						if(FF->isDeclaration()) {
							continue;
						}
						FF->setName(FF->getName()+"_"+std::to_string(i));
					}					
					alloc_clones.push_back(MA);
				}

				for(unsigned int i = 0; i < alloc_clones.size(); ++i) {
					std::string * Merr = new std::string;
					bool failed = L->linkInModule(alloc_clones[i], 1, Merr);
					
					if(failed) {
						errs() << "Link Error.\n";
						errs() << *Merr << "\n";
						assert(0);
					}
				}

				// If a malloc() is used within a threaded function, wrap the call in a mutex.
				// We do NOT specifiy locks inside the malloc calls, since their internal operation is
				// not paralleziable (data dependent flow.)
				if(usingPthreads(M)) {
					for(int i = 0; i < maxHeaps; ++i) {
						Module *PA = CloneModule(pthread_utils);
						if(!PA) {
							errs() << "Cloned module is NULL\n";
							assert(0);
						}
						GlobalVariable * mutex = PA->getGlobalVariable("allocmut");
						if(mutex)
							mutex->setName("allocmut_"+std::to_string(i));

						for(Module::iterator bf = PA->begin(), ef = PA->end(); bf != ef; ++bf) {
							Function * FF = &(*bf); 
							if(FF->isDeclaration()) {
								continue;
							}
							FF->setName(FF->getName()+"_"+std::to_string(i));
						}					
						pthread_utils_clones.push_back(PA);						
					}
					for(unsigned int i = 0; i < pthread_utils_clones.size(); ++i) {
						std::string * Merr = new std::string;
						bool failed = L->linkInModule(pthread_utils_clones[i], 1, Merr);
						
						if(failed) {
							errs() << "Link Error.\n";
							errs() << *Merr << "\n";
							assert(0);
						}
					}					
				}

				errs() << "==> Casting PartitionedNodes\n";
				for(unsigned int i = 0; i < partitionedNodes.size(); ++i) {
					for(unsigned int j = 0; j < partitionedNodes[i].size(); ++j) {
						mNode * mn = partitionedNodes[i][j];
						Value * mnv = mn->getValue();
						CallInst * MCI = dyn_cast<CallInst>(mnv);
						errs() << "Which Partition: "<< i << "\n";
						errs() << "Replacing: " << *MCI << "\n";
						std::string funName = MCI->getCalledFunction()->getName();

						if(isLazy && funName == "free") {
							funName= "lazyfree";
						}

						Function * Fnew = M.getFunction(tag+"_"+funName+"_"+std::to_string(i%maxHeaps));
						MCI->setCalledFunction(Fnew);
						errs() << "Replaced : "<< *MCI << "\n\n";

						//surround malloc call with locks;
						if(usingPthreads(M)) {
							Function * lock = M.getFunction("alloc_lock_"+std::to_string(i%maxHeaps));
							Function * unlock = M.getFunction("alloc_unlock_"+std::to_string(i%maxHeaps));

							Instruction * alloc_inst = dyn_cast<Instruction>(mnv);
							CallInst::Create(lock, "", alloc_inst);
							CallInst::Create(unlock, "", alloc_inst->getNextNode());

						}

					}
				}

			} else {

				if(usingPthreads(M)) {
					populateAllocatorMaps(M);
					std::string * Merr = new std::string;					

					bool failed = L->linkInModule(pthread_utils, 1, Merr);
					
					if(failed) {
						errs() << "Link Error.\n";
						errs() << *Merr << "\n";
						assert(0);
					}					
					Function * lock = M.getFunction("alloc_lock");
					Function * unlock = M.getFunction("alloc_unlock");

					for(auto item : all_allocators_map) {
						Instruction * alloc_inst = dyn_cast<Instruction>(item.first);
						CallInst::Create(lock, "", alloc_inst);
						CallInst::Create(unlock, "", alloc_inst->getNextNode());
					}

					for(auto item : all_frees_map) {
						Instruction * alloc_inst = dyn_cast<Instruction>(item.first);
						CallInst::Create(lock, "", alloc_inst);
						CallInst::Create(unlock, "", alloc_inst->getNextNode());
					}
				}				

				Function * freeFun = M.getFunction(free);
				if(freeFun) {
					std::string rfree = free;
					if(isLazy) {
						rfree = "lazyfree";
					} 
					freeFun->setName(tag+"_"+rfree);
				}
				for(int i = 0; i < NUMFUNCS; ++i) {
					Function * memfunc = M.getFunction(allocators[i]);
					if(memfunc != NULL){
						memfunc->setName(tag+"_"+allocators[i]);
					}
				}
				// 1-> preserve module
				// 0-> destroy module
				std::string * err = new std::string;

				bool failed = L->linkInModule(alloc_scheme,1,err);

				if(failed) {
					errs() << err << "\n";
					assert(0);
				}								
			}		
		}

	};	

}

using namespace legup;
char MemCast::ID = 0;
static RegisterPass<MemCast> X("legup-memCast", "Casting Dynamic Memory Calls");
