 /*
  * Trivial linear allocator implementation
 */
#include "memutils.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

typedef uint32_t LCHUNK;
typedef uint32_t HEAPWIDTH;

#define LARENA_CHUNKS 	ARENA_BYTES/4
#define MASK 			0x03

#define GETADDR(ADDR) ((void *)(ADDR))
#define HEAPWIDTH_SZ sizeof(HEAPWIDTH)

static HEAPWIDTH larena[LARENA_CHUNKS];
static LCHUNK * CURR_ADDR 	= larena;
static LCHUNK * PREV_ADDR 	= larena;
static LCHUNK * END 		= &larena[LARENA_CHUNKS-1];


void * __attribute__ ((noinline)) lmalloc(unsigned nbytes) {
	uint32_t size = nbytes;
	uint32_t newsize=0;
	printbytes(1, 1, size);

	printdbg("    [S] Size Quantization\n");

	if(size == 0) {
		size = HEAPWIDTH_SZ;
	}
	
	if((size&MASK) != 0) {
		newsize=(size&(~0x03))+HEAPWIDTH_SZ;
	} else {
		newsize=size;
	}

	printbytes(1, 2, newsize);

	printdbg("    [E] Size Quantization\n");

	printdbg("    [S] Pointer Increment\n");
	if(newsize >= (END-CURR_ADDR)) {
		printf(" Out of bounds...\n");
		return NULL;
	}

	PREV_ADDR = CURR_ADDR;
	CURR_ADDR += newsize;
	
	printbytes(1, 0, (uint32_t)GETADDR(PREV_ADDR));		

	printdbg("    [E] Pointer Increment\n");

	return GETADDR(PREV_ADDR);
}


void * __attribute__ ((noinline)) lrealloc(void * p, unsigned newsize) {
	newsize = (newsize <= 0) ? HEAPWIDTH_SZ : newsize;
		
	if(!p) {
		return lmalloc(newsize);
	}

	int size = ((newsize&MASK) == 0) ? newsize : ((newsize&(~MASK))+HEAPWIDTH_SZ);;
	
	if(p == PREV_ADDR && size < (END-PREV_ADDR)) {
		CURR_ADDR = PREV_ADDR + size;
		return p;
	}
	return NULL;
}


void * __attribute__ ((noinline)) lcalloc(unsigned nelem, unsigned elsize) {
  void *vp;
  unsigned nbytes;
  unsigned char * cvp;
  unsigned int i = 0;

  nbytes = nelem * elsize;
  if ( (vp = lmalloc(nbytes)) == NULL)
    return NULL;
  cvp = (unsigned char *)vp;
  for(i=0; i< nbytes; ++i) {
    cvp[i]='\0';
  }
  return vp;
}

void lfree(void * p) {
}


void lfreeall()  {
	CURR_ADDR = larena;
	PREV_ADDR = larena;
}