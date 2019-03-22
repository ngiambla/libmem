/*  --> using an OR-tree to indicate availability, we can then 
 *      precisely an each level of the OR-tree to locate a free
 *      node for use.
 */

#include "legup/memutils.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>


/* Heap Size in bytes */
#define HEAPWIDTH_SZ 	4
#define MASK 			0x03

static uint32_t F_ARENA[2048] 			= {0};
static uint32_t FBMP[6]	 				= {0};
static uint32_t addr_map[63]			= {0};

const uint32_t MASKS[6] 	= {
								0xFFFFFFFF,
								0x0000FFFF,
								0x000000FF,
								0x0000000F,
								0x00000002,
								0x00000001  
							};


const uint32_t * OFFSETS[6][32] = {
								{
									F_ARENA, F_ARENA +64, F_ARENA +128, F_ARENA +192, F_ARENA +256, F_ARENA +320, F_ARENA +384, 
									F_ARENA +448, F_ARENA +512, F_ARENA +576, F_ARENA +640, F_ARENA +704, F_ARENA +768, F_ARENA +832, 
									F_ARENA +896, F_ARENA +960, F_ARENA +1024, F_ARENA +1088, F_ARENA +1152, F_ARENA +1216, F_ARENA +1280, 
									F_ARENA +1344, F_ARENA +1408, F_ARENA +1472, F_ARENA +1536, F_ARENA +1600, F_ARENA +1664, F_ARENA +1728, 
									F_ARENA +1792, F_ARENA +1856, F_ARENA +1920, F_ARENA +1984
								},
								{
									F_ARENA, F_ARENA +128, F_ARENA +256, F_ARENA +384, F_ARENA +512, F_ARENA +640, F_ARENA +768, 
									F_ARENA +896, F_ARENA +1024, F_ARENA +1152, F_ARENA +1280, F_ARENA +1408, F_ARENA +1536, F_ARENA +1664, 
									F_ARENA +1792, F_ARENA +1920	
								},
								{
									F_ARENA, F_ARENA +256, F_ARENA +512, F_ARENA +768, F_ARENA +1024, F_ARENA +1280, F_ARENA +1536, F_ARENA +1792
								},
								{
									F_ARENA, F_ARENA +512, F_ARENA +1024, F_ARENA + 1536
								},
								{
									F_ARENA, F_ARENA+1024
								},
								{
									F_ARENA
								}
					};




#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
static const char LogTable256[256] = 
{
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
    LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
};

uint32_t flog (uint32_t v) {  	// 32-bit word to find the log of
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

void update_fbmp() {
	FBMP[1] |= (((FBMP[0] & 0x80000000) | (FBMP[0] & 0x40000000)) << 15 | 
				((FBMP[0] & 0x20000000) | (FBMP[0] & 0x10000000)) << 14 | 

				((FBMP[0] & 0x08000000) | (FBMP[0] & 0x04000000)) << 13 |
				((FBMP[0] & 0x02000000) | (FBMP[0] & 0x01000000)) << 12 |

				((FBMP[0] & 0x00800000) | (FBMP[0] & 0x00400000)) << 11 |
				((FBMP[0] & 0x00200000) | (FBMP[0] & 0x00100000)) << 10 |

				((FBMP[0] & 0x00080000) | (FBMP[0] & 0x00040000)) <<  9 |
				((FBMP[0] & 0x00020000) | (FBMP[0] & 0x00010000)) <<  8 |

				((FBMP[0] & 0x00008000) | (FBMP[0] & 0x00004000)) <<  7 |
				((FBMP[0] & 0x00002000) | (FBMP[0] & 0x00001000)) <<  6 |

				((FBMP[0] & 0x00000800) | (FBMP[0] & 0x00000400)) <<  5 |
				((FBMP[0] & 0x00000200) | (FBMP[0] & 0x00000100)) <<  4 |

				((FBMP[0] & 0x00000080) | (FBMP[0] & 0x00000040)) <<  3 |
				((FBMP[0] & 0x00000020) | (FBMP[0] & 0x00000010)) <<  2 |

				((FBMP[0] & 0x00000008) | (FBMP[0] & 0x00000004)) <<  1 |
				((FBMP[0] & 0x00000002) | (FBMP[0] & 0x00000001)) );

	FBMP[2] |= (((FBMP[1] & 0x00008000) | (FBMP[1] & 0x00004000)) <<  7 |
				((FBMP[1] & 0x00002000) | (FBMP[1] & 0x00001000)) <<  6 |

				((FBMP[1] & 0x00000800) | (FBMP[1] & 0x00000400)) <<  5 |
				((FBMP[1] & 0x00000200) | (FBMP[1] & 0x00000100)) <<  4 |

				((FBMP[1] & 0x00000080) | (FBMP[1] & 0x00000040)) <<  3 |
				((FBMP[1] & 0x00000020) | (FBMP[1] & 0x00000010)) <<  2 |

				((FBMP[1] & 0x00000008) | (FBMP[1] & 0x00000004)) <<  1 |
				((FBMP[1] & 0x00000002) | (FBMP[1] & 0x00000001)) );

	FBMP[3] |= (((FBMP[2] & 0x00000080) | (FBMP[2] & 0x00000040)) <<  3 |
				((FBMP[2] & 0x00000020) | (FBMP[2] & 0x00000010)) <<  2 |
				((FBMP[2] & 0x00000008) | (FBMP[2] & 0x00000004)) <<  1 |
				((FBMP[2] & 0x00000002) | (FBMP[2] & 0x00000001)) );

	FBMP[4] |= (((FBMP[3] & 0x00000008) | (FBMP[3] & 0x00000004)) <<  1 |
				((FBMP[3] & 0x00000002) | (FBMP[3] & 0x00000001)) );

	FBMP[5] |= (((FBMP[4] & 0x00000002) | (FBMP[4] & 0x00000001)) );		
}


#ifdef __DO_NOT_INLINE__ 
   void * __attribute__ ((noinline)) fmalloc(unsigned bytes)
#else
   void * fmalloc(unsigned bytes)
#endif

{

	//printf("  bytes = %d\n", bytes);
	printbytes(1, 1, bytes);

	if(bytes < 256) {
		bytes = 256;
	}	
	//printf("  bytes = %d\n", bytes);

	uint32_t flogbytes = flog(bytes);

	//printf("  flogbytes = %d\n", flogbytes);

	if((bytes - (1<<flogbytes)) > 0) {
		flogbytes+=1;
	}
	
	printbytes(1, 2, 1<<flogbytes);


	//printf("  bytes = %d\n", bytes);

		uint8_t level = flogbytes-8;
		uint32_t bitmap = FBMP[level];
		
		if(bitmap != MASKS[level]) {
			uint32_t lvl_vec 		= ~bitmap & ~(~bitmap-1);
			//printf("  bitmap is %08x\n", bitmap);
			//printf("  level  is %08x\n", level);
			//printf("  lvlvec is %08x\n", lvl_vec);

			uint32_t addr_idx_buddy = flog(lvl_vec); 
			//printf("  addridx is %08x\n", addr_idx_buddy);

			FBMP[level] 				= bitmap | lvl_vec;
			//printf("  FBMP[level] is %08x\n", FBMP[level]);

			update_fbmp();


			addr_map[(uint32_t)OFFSETS[level][addr_idx_buddy]] = level;

			printbytes(1, 0, (uint32_t)OFFSETS[level][addr_idx_buddy]);

			return (void *)OFFSETS[level][addr_idx_buddy];
		}
	
	

	return NULL;
	
}

#ifdef __DO_NOT_INLINE__ 
   void __attribute__ ((noinline)) ffree(void * p)
#else
   void ffree(void * p)
#endif
	
 {
	printbytes(0, 1, (uint32_t)p);
	uint32_t addr = (uint32_t)p;
	uint32_t level = addr_map[addr];	

	//printf("%08x --> addr\n", addr);

	for(int j = (1<<(5-level)); j >= 0; --j) {
		if(addr == (uint32_t)OFFSETS[level][j]) {
			uint32_t msk = 1 << j;
			if((FBMP[level] & msk) == 1) {
				FBMP[level] &= ~(msk);
				update_fbmp();
				return;
			}
		}
	}
}


void * frealloc(void * vp, unsigned newbytes) {
	void *newp = NULL;
	uint32_t *cnewp, *cvp;

	int idx = 0;
	/* behavior on corner cases conforms to SUSv2 */
	if (vp == NULL)
		return fmalloc(newbytes);

	if (newbytes != 0) {
		if ( (newp = fmalloc(newbytes)) == NULL)
			return NULL;
		
		uint32_t addr = (uint32_t)vp;
		uint32_t level = addr_map[addr]+8;
		
		uint32_t bound = ((1 << level) < newbytes) ? (1<<level) : newbytes;
		
		bound >>= 2;

		cnewp   = newp;
		cvp     = vp;

		for(idx = 0; idx < bound; ++ idx) {
			cnewp[idx]=cvp[idx];
		}
	}

	ffree(vp);
	return newp;
} 

void * fcalloc(unsigned nelem, unsigned elsize) {
  void *vp;
  unsigned nbytes;
  uint32_t * cvp;
  unsigned int i = 0;

  nbytes = (nelem * elsize)>>2;
  if ( (vp = fmalloc(nbytes)) == NULL)
    return NULL;

  cvp = (uint32_t *)vp;
  for(i=0; i< nbytes; ++i) {
    cvp[i]='\0';
  }
  return vp;
}

