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
#define TOTAL_LEVELS 								(uint32_t)(BITS_TO_REPRESENT(NUM_OF_LEAVES)-1)



typedef struct uintx_t {
  uint32_t internal[NUM_OF_LEAVES_WORDS];
} uintx_t;

static uint32_t BUDDY_ARENA[MAX_BUDDY_CHUNK] 		= {0};
static uintx_t tree[TOTAL_LEVELS] = {0} ;




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



#ifdef __DO_NOT_INLINE__ 
   void * __attribute__ ((noinline)) bud_malloc(unsigned bytes)
#else
   void * bud_malloc(unsigned bytes)
#endif

{

	uint32_t intLogBytes;
	uint32_t level;
	uint32_t bitmap;

	if(bytes < MIN_REQ_SIZE) {
		bytes = MIN_REQ_SIZE;
	}

	intLogBytes = intlog2(bytes);

	if((bytes - (1<<intLogBytes)) > 0) {
		intLogBytes+=1;
	}
	
	level = intLogBytes - LOG2_MIN_REQ_SIZE;

	printf("Byte Request: %d\n", bytes);
	printf("TOTAL_LEVELS: %d\n", TOTAL_LEVELS);
	printf("level: %d\n", level);
	printf("NUM_OF_LEAVES_WORDS:  %d\n", NUM_OF_LEAVES_WORDS);
	printf("Starting Idx: %d\n", NUM_OF_LEAVES_WORDS - ((1 << (TOTAL_LEVELS - level))>>5) );

	uint32_t starting_idx = NUM_OF_LEAVES_WORDS - ((1<< (TOTAL_LEVELS - level))>>5);
	if(starting_idx == NUM_OF_LEAVES_WORDS) {
		uint32_t num_of_bits = (1<< (TOTAL_LEVELS - level));
		uint32_t bitmask = (1 << num_of_bits)-1;

		printf("how many bits: %d\n", num_of_bits);
		printf("bitmask: %08x\n", bitmask);

		bitmap = tree[level].internal[NUM_OF_LEAVES_WORDS-1];

	}

	for(int i = starting_idx; i < NUM_OF_LEAVES_WORDS; ++i) {
			bitmap = tree[level].internal[i];
			if(~bitmap == 0) {
				continue;
			}			
			uint32_t lvl_vec 		= ~bitmap & ~(~bitmap-1);

			// printf("  bitmap is %08x\n", bitmap);
			// printf("  level  is %08x\n", level);
			// printf("  lvlvec is %08x\n", lvl_vec);

			uint32_t addr_idx_buddy = intlog2(lvl_vec); 
			//printf("  addridx is %08x\n", addr_idx_buddy);
	}
	printf("\n");
	return NULL;
	
}

#ifdef __DO_NOT_INLINE__ 
   void __attribute__ ((noinline)) bud_free(void * p)
#else
   void bud_free(void * p)
#endif
{

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
		
		// uint32_t addr = (uint32_t)vp;
		// uint32_t level = addr_map[addr]+8;
		
		// uint32_t bound = ((1 << level) < newbytes) ? (1<<level) : newbytes;
		
		// bound >>= 2;

		cnewp   = newp;
		cvp     = vp;

		// for(idx = 0; idx < bound; ++ idx) {
		// 	cnewp[idx]=cvp[idx];
		// }
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

