/* 
 * Parameterizable buddy allocator.
 */

#include "memutils.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>

#define IS_REPRESENTIBLE_IN_D_BITS(D, N)                \
  (((unsigned long) N >= (1UL << (D - 1)) && (unsigned long) N < (1UL << D)) ? D : -1)

#define BITS_TO_REPRESENT(N)                            \
  (N == 0 ? 1 : (31                                     \
                 + IS_REPRESENTIBLE_IN_D_BITS( 1, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS( 2, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS( 3, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS( 4, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS( 5, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS( 6, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS( 7, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS( 8, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS( 9, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(10, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(11, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(12, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(13, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(14, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(15, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(16, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(17, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(18, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(19, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(20, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(21, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(22, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(23, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(24, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(25, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(26, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(27, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(28, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(29, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(30, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(31, N)    \
                 + IS_REPRESENTIBLE_IN_D_BITS(32, N)    \
                 )                                      \
   )



#define MAX_BUDDY_CHUNK 							(uint32_t)(ARENA_BYTES >> 2)				// dividing by 4 to represent 32-bit words.
#define NUM_OF_LEAVES 								(uint32_t)(ARENA_BYTES/MIN_REQ_SIZE) 		
#define NUM_OF_LEAVES_WORDS 						(uint32_t)(NUM_OF_LEAVES>>5)
#define LOG2_MIN_REQ_SIZE 							(uint32_t)(BITS_TO_REPRESENT(MIN_REQ_SIZE)-1)
#define TOTAL_LEVELS 								(uint32_t)(BITS_TO_REPRESENT(NUM_OF_LEAVES))


typedef struct uintx_t {
	uint32_t internal[NUM_OF_LEAVES_WORDS];
} uintx_t;


static uint32_t 	BUDDY_ARENA[MAX_BUDDY_CHUNK] 		= {0};
static uint8_t 		ADDR_LUT[ARENA_BYTES] 				= {0};
static uintx_t 		tree[TOTAL_LEVELS] 					= {0};




#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
static const char LogTable256[256] = 
{
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
    LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
};

uint32_t intlog2 (uint32_t v) {  	// 32-bit word to find the log of
	uint32_t r;     // r will be lg(v)
	uint32_t t, tt; // temporaries
	tt = v >> 16;
	if (tt > 0) {
		t= tt >> 8;
		// 				24 +					16 +
	    r = (t > 0) ? 0x18 | LogTable256[t] : 0x10 | LogTable256[tt];
	} else {
		t = v >> 8;
		//				08 +
	    r = (t > 0) ? 0x08 | LogTable256[t] : LogTable256[v];
	}
	return r;
}



void __attribute__ ((always_inline)) update_tree(int level) {
	uint32_t starting_idx, next_idx;
	uint32_t bitmap;
	// Mark Up
	// printf("\n++++++++++++++++ BITMAP UPDATE ++++++++++++++++\n");

	// printf("Marking Up\n");
	for(int i = level; i < TOTAL_LEVELS ; i++) {
		starting_idx = NUM_OF_LEAVES_WORDS - ((1<< (TOTAL_LEVELS - 1 - i)) >> 5);
		// printf("level: %d\n", i);
		// printf("starting_idx: %d\n", starting_idx);

		if(starting_idx >= NUM_OF_LEAVES_WORDS) {
			uint32_t tmp = 0;
			for(int k = 0; k < 32; k += 2 ) {
				uint32_t res1 = (tree[i-1].internal[NUM_OF_LEAVES_WORDS-1] >> k)&0x1;
				uint32_t res2 = (tree[i-1].internal[NUM_OF_LEAVES_WORDS-1] >> (k+1))&0x1;
				tmp |= (res1 | res2) << (k>>1);
			}
			tree[i].internal[NUM_OF_LEAVES_WORDS-1] = tmp;	
				
		} else {
			next_idx = NUM_OF_LEAVES_WORDS - ((1<< (TOTAL_LEVELS - 2 - i)) >> 5);
			// printf("next_idx:     %d\n", next_idx);
		}


		for(int j = starting_idx; j < NUM_OF_LEAVES_WORDS; j+=2) {
			uint32_t tmp = 0;
			for(int k = 0; k < 32; k += 2 ) {
				uint32_t res1 = (tree[i].internal[j+1] >> k)&0x1;
				uint32_t res2 = (tree[i].internal[j+1] >> (k+1))&0x1;
				tmp |= (res1 | res2) << (k>>1);
			}
			
			tmp <<= 16;

			for(int k = 0; k < 32; k += 2 ) {
				uint32_t res1 = (tree[i].internal[j] >> k)&0x1;
				uint32_t res2 = (tree[i].internal[j] >> (k+1))&0x1;
				tmp |= (res1 | res2) << (k>>1);
			}			
			
			tree[i+1].internal[next_idx] = tmp;	
			++next_idx;		
		}
	}

	// Mark Down
	// printf("------------------\n");
	for(int i = level; i > 0; i--) {
		// printf("marking down.\n");

		// starting_idx = NUM_OF_LEAVES_WORDS - ((1<< (TOTAL_LEVELS - 1 - i))>>5);
		starting_idx = NUM_OF_LEAVES_WORDS - ((1<< (TOTAL_LEVELS - 1 - i))>>5);
		// printf("level: %d\n", i);
		// printf("starting_idx: %d\n", starting_idx);		
		if(starting_idx >= NUM_OF_LEAVES_WORDS) {
			for(int k = 0; k < 16; k+=1) {
				uint32_t res = (tree[i].internal[NUM_OF_LEAVES_WORDS-1] >> k)&0x1;
				uint32_t bitidx1 = k << 1;
				uint32_t bitidx2 = 1+(k << 1);
				tree[i-1].internal[NUM_OF_LEAVES_WORDS-1] = (tree[i-1].internal[NUM_OF_LEAVES_WORDS-1] & ~(res << bitidx1)) | (res << bitidx1);
				tree[i-1].internal[NUM_OF_LEAVES_WORDS-1] = (tree[i-1].internal[NUM_OF_LEAVES_WORDS-1] & ~(res << bitidx2)) | (res << bitidx2);
			}				
		} else {
			next_idx = NUM_OF_LEAVES_WORDS - ((1<< (TOTAL_LEVELS - i)) >> 5);
			// printf("Next IDX = %d\n", next_idx);
		}
		for(int j = starting_idx; j < NUM_OF_LEAVES_WORDS; ++j) {
			// printf("I am here at level %d\n", i);
			uint32_t tmp1 = ~0;
			for(int k = 0; k < 16; ++k) {
				uint32_t res = (tree[i].internal[j] >> k)&0x1;
				uint32_t bitidx1 = k << 1;
				uint32_t bitidx2 = 1+(k << 1);
				tree[i-1].internal[next_idx] = (tree[i-1].internal[next_idx] & ~(res << bitidx1)) | (res << bitidx1);
				tree[i-1].internal[next_idx] = (tree[i-1].internal[next_idx] & ~(res << bitidx2)) | (res << bitidx2);
			}
			for(int k = 16; k < 32; ++k) {
				uint32_t res = (tree[i].internal[j] >> k)&0x1;
				uint32_t bitidx1 = k << 1;
				uint32_t bitidx2 = 1+(k << 1);
				tree[i-1].internal[next_idx+1] = (tree[i-1].internal[next_idx+1] & ~(res << bitidx1)) | (res << bitidx1);
				tree[i-1].internal[next_idx+1] = (tree[i-1].internal[next_idx+1] & ~(res << bitidx2)) | (res << bitidx2);
			}
			next_idx+=2;
		}
	}
}

#ifdef __DO_NOT_INLINE__ 
   void * __attribute__ ((noinline)) bud_malloc(unsigned bytes)
#else
   void * bud_malloc(unsigned bytes)
#endif

{

	uint32_t intLogBytes;
	uint32_t level;
	uint32_t bitmap;

	if(bytes > ARENA_BYTES) {
		return NULL;
	}

	if(bytes < MIN_REQ_SIZE) {
		bytes = MIN_REQ_SIZE;
	}

	intLogBytes = intlog2(bytes);

	if((bytes - (1<<intLogBytes)) > 0) {
		intLogBytes+=1;
	}
	
	level = intLogBytes - LOG2_MIN_REQ_SIZE;
	uint32_t starting_idx = NUM_OF_LEAVES_WORDS - ((1<< (TOTAL_LEVELS-1 - level))>>5);

	if(starting_idx == NUM_OF_LEAVES_WORDS) {
		uint32_t num_of_bits = (1<< (TOTAL_LEVELS - 1 - level));
		uint32_t bitmask = (1 << num_of_bits);

		bitmap = tree[level].internal[NUM_OF_LEAVES_WORDS-1];
		if(bitmap < bitmask) {
			uint32_t lvl_vec 		= ~bitmap & ~(~bitmap-1);
			uint32_t addr_idx 		= intlog2(lvl_vec);
			tree[level].internal[NUM_OF_LEAVES_WORDS-1] = bitmap | lvl_vec;
			update_tree(level);
			uint32_t ADDR_TAG = ((uint32_t)BUDDY_ARENA + (addr_idx << (intLogBytes)) );
			ADDR_TAG ^= (uint32_t)BUDDY_ARENA;
			ADDR_LUT[ADDR_TAG>>LOG2_MIN_REQ_SIZE] = level;
			return (void *) (BUDDY_ARENA + (addr_idx << (intLogBytes)) );
		}
	}


	for(int i = starting_idx; i < NUM_OF_LEAVES_WORDS; ++i) {
			bitmap = tree[level].internal[i];
			if(~bitmap == 0) {
				continue;
			}			
			uint32_t lvl_vec 		= ~bitmap & ~(~bitmap-1);
			uint32_t addr_idx 		= intlog2(lvl_vec);

			tree[level].internal[i] = bitmap | lvl_vec;
			
			update_tree(level);
			uint32_t ADDR_TAG = ((uint32_t)BUDDY_ARENA + ((((i-starting_idx) << 5)+addr_idx) << (intLogBytes)));
			ADDR_TAG ^= (uint32_t)BUDDY_ARENA;			
			ADDR_LUT[ADDR_TAG>>LOG2_MIN_REQ_SIZE] = level;
			return (void *) (BUDDY_ARENA + ((((i-starting_idx) << 5)+addr_idx) << (intLogBytes)));
	}
	return NULL;
	
}



#ifdef __DO_NOT_INLINE__ 
   void __attribute__ ((noinline)) bud_free(void * p)
#else
   void bud_free(void * p)
#endif
{

	uint32_t addr_map = ((uint32_t)p ^ (uint32_t)BUDDY_ARENA) >> LOG2_MIN_REQ_SIZE;
	uint32_t level = ADDR_LUT[addr_map];
	uint32_t intLogBytes = level + LOG2_MIN_REQ_SIZE;
	uint32_t starting_idx = NUM_OF_LEAVES_WORDS - ((1<< (TOTAL_LEVELS-1 - level))>>5);
	uint32_t res = ((uint32_t)p - ((uint32_t)BUDDY_ARENA + (((starting_idx) << 5) << intLogBytes))) >> intLogBytes;
	uint32_t res2 = (res >> 5)<<5;
	tree[level].internal[starting_idx+(res-res2)] &= ~(1<<res2);
	update_tree(level);
}


void * bud_realloc(void * vp, unsigned newbytes) {
	void *newp = NULL;
	uint32_t *cnewp, *cvp;

	int idx = 0;
	/* behavior on corner cases conforms to SUSv2 */
	if (vp == NULL)
		return bud_malloc(newbytes);

	if (newbytes != 0) {
		if ( (newp = bud_malloc(newbytes)) == NULL)
			return NULL;

		uint32_t addr_map = ((uint32_t)vp ^ (uint32_t)BUDDY_ARENA) >> LOG2_MIN_REQ_SIZE;
		uint32_t level = ADDR_LUT[addr_map];
		uint32_t intLogBytes = level + LOG2_MIN_REQ_SIZE;
		uint32_t w0rds = (1 << intLogBytes)>>2; 

		cnewp   = newp;
		cvp     = vp;

		for(idx = 0; idx < w0rds; ++ idx) {
			cnewp[idx]=cvp[idx];
		}
	}

	bud_free(vp);
	return newp;
} 

void * bud_calloc(unsigned nelem, unsigned elsize) {
  void *vp;
  unsigned nbytes;
  uint32_t * cvp;
  unsigned int i = 0;

  nbytes = (nelem * elsize)>>2;
  if ( (vp = bud_malloc(nbytes)) == NULL)
    return NULL;

  cvp = (uint32_t *)vp;
  for(i=0; i< nbytes; ++i) {
    cvp[i]='\0';
  }
  return vp;
}

