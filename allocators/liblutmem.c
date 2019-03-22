/* 
 * 
 */

#include "memutils.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

static uint32_t N_FAST_ARENA_0_32[ 256] 		= {0}; 
static uint32_t N_FAST_ARENA_1_32[ 256] 		= {0}; 
static uint32_t N_FAST_ARENA_2_32[ 256] 		= {0}; 
static uint32_t N_FAST_ARENA_3_32[ 256] 		= {0}; 
static uint32_t N_FAST_ARENA_4_32[ 256] 		= {0}; 


static uint32_t N_FAST_ARENA___64[ 512] 		= {0}; 
static uint32_t N_FAST_ARENA__128[1024] 		= {0}; 
static uint32_t N_FAST_ARENA__256[2048] 		= {0}; 
static uint32_t N_FAST_ARENA__512[4096] 		= {0}; 
static uint32_t N_FAST_ARENA_1024[8192] 		= {0}; 


static uint32_t N_FREE_ADDRESS___LT[10] = {
										0, 0, 0, 0, 0,
										0, 0, 0, 0, 0
									};

static const uint8_t  N_SHIFT_LEFTS__LT[10] = {
												5, 5, 5, 5, 5, 6, 7, 8, 9, 10
											};

static uint32_t * N_FAST_ADDRESS___LT[10][32] = {
												{
													N_FAST_ARENA_0_32 + 0, N_FAST_ARENA_0_32 + 8, N_FAST_ARENA_0_32 + 16, N_FAST_ARENA_0_32 + 24, 
													N_FAST_ARENA_0_32 + 32, N_FAST_ARENA_0_32 + 40, N_FAST_ARENA_0_32 + 48, N_FAST_ARENA_0_32 + 56, 
													N_FAST_ARENA_0_32 + 64, N_FAST_ARENA_0_32 + 72, N_FAST_ARENA_0_32 + 80, N_FAST_ARENA_0_32 + 88, 
													N_FAST_ARENA_0_32 + 96, N_FAST_ARENA_0_32 + 104, N_FAST_ARENA_0_32 + 112, N_FAST_ARENA_0_32 + 120, 
													N_FAST_ARENA_0_32 + 128, N_FAST_ARENA_0_32 + 136, N_FAST_ARENA_0_32 + 144, N_FAST_ARENA_0_32 + 152, 
													N_FAST_ARENA_0_32 + 160, N_FAST_ARENA_0_32 + 168, N_FAST_ARENA_0_32 + 176, N_FAST_ARENA_0_32 + 184, 
													N_FAST_ARENA_0_32 + 192, N_FAST_ARENA_0_32 + 200, N_FAST_ARENA_0_32 + 208, N_FAST_ARENA_0_32 + 216, 
													N_FAST_ARENA_0_32 + 224, N_FAST_ARENA_0_32 + 232, N_FAST_ARENA_0_32 + 240, N_FAST_ARENA_0_32 + 248 
												},
												{
													N_FAST_ARENA_1_32 + 0, N_FAST_ARENA_1_32 + 8, N_FAST_ARENA_1_32 + 16, N_FAST_ARENA_1_32 + 24, 
													N_FAST_ARENA_1_32 + 32, N_FAST_ARENA_1_32 + 40, N_FAST_ARENA_1_32 + 48, N_FAST_ARENA_1_32 + 56, 
													N_FAST_ARENA_1_32 + 64, N_FAST_ARENA_1_32 + 72, N_FAST_ARENA_1_32 + 80, N_FAST_ARENA_1_32 + 88, 
													N_FAST_ARENA_1_32 + 96, N_FAST_ARENA_1_32 + 104, N_FAST_ARENA_1_32 + 112, N_FAST_ARENA_1_32 + 120, 
													N_FAST_ARENA_1_32 + 128, N_FAST_ARENA_1_32 + 136, N_FAST_ARENA_1_32 + 144, N_FAST_ARENA_1_32 + 152, 
													N_FAST_ARENA_1_32 + 160, N_FAST_ARENA_1_32 + 168, N_FAST_ARENA_1_32 + 176, N_FAST_ARENA_1_32 + 184, 
													N_FAST_ARENA_1_32 + 192, N_FAST_ARENA_1_32 + 200, N_FAST_ARENA_1_32 + 208, N_FAST_ARENA_1_32 + 216, 
													N_FAST_ARENA_1_32 + 224, N_FAST_ARENA_1_32 + 232, N_FAST_ARENA_1_32 + 240, N_FAST_ARENA_1_32 + 248 
												},
												{
													N_FAST_ARENA_2_32 + 0, N_FAST_ARENA_2_32 + 8, N_FAST_ARENA_2_32 + 16, N_FAST_ARENA_2_32 + 24, 
													N_FAST_ARENA_2_32 + 32, N_FAST_ARENA_2_32 + 40, N_FAST_ARENA_2_32 + 48, N_FAST_ARENA_2_32 + 56, 
													N_FAST_ARENA_2_32 + 64, N_FAST_ARENA_2_32 + 72, N_FAST_ARENA_2_32 + 80, N_FAST_ARENA_2_32 + 88, 
													N_FAST_ARENA_2_32 + 96, N_FAST_ARENA_2_32 + 104, N_FAST_ARENA_2_32 + 112, N_FAST_ARENA_2_32 + 120, 
													N_FAST_ARENA_2_32 + 128, N_FAST_ARENA_2_32 + 136, N_FAST_ARENA_2_32 + 144, N_FAST_ARENA_2_32 + 152, 
													N_FAST_ARENA_2_32 + 160, N_FAST_ARENA_2_32 + 168, N_FAST_ARENA_2_32 + 176, N_FAST_ARENA_2_32 + 184, 
													N_FAST_ARENA_2_32 + 192, N_FAST_ARENA_2_32 + 200, N_FAST_ARENA_2_32 + 208, N_FAST_ARENA_2_32 + 216, 
													N_FAST_ARENA_2_32 + 224, N_FAST_ARENA_2_32 + 232, N_FAST_ARENA_2_32 + 240, N_FAST_ARENA_2_32 + 248 
												},
												{
													N_FAST_ARENA_3_32 + 0, N_FAST_ARENA_3_32 + 8, N_FAST_ARENA_3_32 + 16, N_FAST_ARENA_3_32 + 24, 
													N_FAST_ARENA_3_32 + 32, N_FAST_ARENA_3_32 + 40, N_FAST_ARENA_3_32 + 48, N_FAST_ARENA_3_32 + 56, 
													N_FAST_ARENA_3_32 + 64, N_FAST_ARENA_3_32 + 72, N_FAST_ARENA_3_32 + 80, N_FAST_ARENA_3_32 + 88, 
													N_FAST_ARENA_3_32 + 96, N_FAST_ARENA_3_32 + 104, N_FAST_ARENA_3_32 + 112, N_FAST_ARENA_3_32 + 120, 
													N_FAST_ARENA_3_32 + 128, N_FAST_ARENA_3_32 + 136, N_FAST_ARENA_3_32 + 144, N_FAST_ARENA_3_32 + 152, 
													N_FAST_ARENA_3_32 + 160, N_FAST_ARENA_3_32 + 168, N_FAST_ARENA_3_32 + 176, N_FAST_ARENA_3_32 + 184, 
													N_FAST_ARENA_3_32 + 192, N_FAST_ARENA_3_32 + 200, N_FAST_ARENA_3_32 + 208, N_FAST_ARENA_3_32 + 216, 
													N_FAST_ARENA_3_32 + 224, N_FAST_ARENA_3_32 + 232, N_FAST_ARENA_3_32 + 240, N_FAST_ARENA_3_32 + 248 
												},																																				
												{
													N_FAST_ARENA_4_32 + 0, N_FAST_ARENA_4_32 + 8, N_FAST_ARENA_4_32 + 16, N_FAST_ARENA_4_32 + 24, 
													N_FAST_ARENA_4_32 + 32, N_FAST_ARENA_4_32 + 40, N_FAST_ARENA_4_32 + 48, N_FAST_ARENA_4_32 + 56, 
													N_FAST_ARENA_4_32 + 64, N_FAST_ARENA_4_32 + 72, N_FAST_ARENA_4_32 + 80, N_FAST_ARENA_4_32 + 88, 
													N_FAST_ARENA_4_32 + 96, N_FAST_ARENA_4_32 + 104, N_FAST_ARENA_4_32 + 112, N_FAST_ARENA_4_32 + 120, 
													N_FAST_ARENA_4_32 + 128, N_FAST_ARENA_4_32 + 136, N_FAST_ARENA_4_32 + 144, N_FAST_ARENA_4_32 + 152, 
													N_FAST_ARENA_4_32 + 160, N_FAST_ARENA_4_32 + 168, N_FAST_ARENA_4_32 + 176, N_FAST_ARENA_4_32 + 184, 
													N_FAST_ARENA_4_32 + 192, N_FAST_ARENA_4_32 + 200, N_FAST_ARENA_4_32 + 208, N_FAST_ARENA_4_32 + 216, 
													N_FAST_ARENA_4_32 + 224, N_FAST_ARENA_4_32 + 232, N_FAST_ARENA_4_32 + 240, N_FAST_ARENA_4_32 + 248 
												},												
												{
													N_FAST_ARENA___64 + 0, N_FAST_ARENA___64 + 16, N_FAST_ARENA___64 + 32, N_FAST_ARENA___64 + 48, 
													N_FAST_ARENA___64 + 64, N_FAST_ARENA___64 + 80, N_FAST_ARENA___64 + 96, N_FAST_ARENA___64 + 112, 
													N_FAST_ARENA___64 + 128, N_FAST_ARENA___64 + 144, N_FAST_ARENA___64 + 160, N_FAST_ARENA___64 + 176, 
													N_FAST_ARENA___64 + 192, N_FAST_ARENA___64 + 208, N_FAST_ARENA___64 + 224, N_FAST_ARENA___64 + 240, 
													N_FAST_ARENA___64 + 256, N_FAST_ARENA___64 + 272, N_FAST_ARENA___64 + 288, N_FAST_ARENA___64 + 304, 
													N_FAST_ARENA___64 + 320, N_FAST_ARENA___64 + 336, N_FAST_ARENA___64 + 352, N_FAST_ARENA___64 + 368, 
													N_FAST_ARENA___64 + 384, N_FAST_ARENA___64 + 400, N_FAST_ARENA___64 + 416, N_FAST_ARENA___64 + 432, 
													N_FAST_ARENA___64 + 448, N_FAST_ARENA___64 + 464, N_FAST_ARENA___64 + 480, N_FAST_ARENA___64 + 496
												},
												{
													N_FAST_ARENA__128 + 0, N_FAST_ARENA__128 + 32, N_FAST_ARENA__128 + 64, N_FAST_ARENA__128 + 96, 
													N_FAST_ARENA__128 + 128, N_FAST_ARENA__128 + 160, N_FAST_ARENA__128 + 192, N_FAST_ARENA__128 + 224, 
													N_FAST_ARENA__128 + 256, N_FAST_ARENA__128 + 288, N_FAST_ARENA__128 + 320, N_FAST_ARENA__128 + 352, 
													N_FAST_ARENA__128 + 384, N_FAST_ARENA__128 + 416, N_FAST_ARENA__128 + 448, N_FAST_ARENA__128 + 480, 
													N_FAST_ARENA__128 + 512, N_FAST_ARENA__128 + 544, N_FAST_ARENA__128 + 576, N_FAST_ARENA__128 + 608, 
													N_FAST_ARENA__128 + 640, N_FAST_ARENA__128 + 672, N_FAST_ARENA__128 + 704, N_FAST_ARENA__128 + 736, 
													N_FAST_ARENA__128 + 768, N_FAST_ARENA__128 + 800, N_FAST_ARENA__128 + 832, N_FAST_ARENA__128 + 864, 
													N_FAST_ARENA__128 + 896, N_FAST_ARENA__128 + 928, N_FAST_ARENA__128 + 960, N_FAST_ARENA__128 + 992 
												},
												{
													N_FAST_ARENA__256 + 0, N_FAST_ARENA__256 + 64, N_FAST_ARENA__256 + 128, N_FAST_ARENA__256 + 192, 
													N_FAST_ARENA__256 + 256, N_FAST_ARENA__256 + 320, N_FAST_ARENA__256 + 384, N_FAST_ARENA__256 + 448, 
													N_FAST_ARENA__256 + 512, N_FAST_ARENA__256 + 576, N_FAST_ARENA__256 + 640, N_FAST_ARENA__256 + 704, 
													N_FAST_ARENA__256 + 768, N_FAST_ARENA__256 + 832, N_FAST_ARENA__256 + 896, N_FAST_ARENA__256 + 960, 
													N_FAST_ARENA__256 + 1024, N_FAST_ARENA__256 + 1088, N_FAST_ARENA__256 + 1152, N_FAST_ARENA__256 + 1216, 
													N_FAST_ARENA__256 + 1280, N_FAST_ARENA__256 + 1344, N_FAST_ARENA__256 + 1408, N_FAST_ARENA__256 + 1472, 
													N_FAST_ARENA__256 + 1536, N_FAST_ARENA__256 + 1600, N_FAST_ARENA__256 + 1664, N_FAST_ARENA__256 + 1728, 
													N_FAST_ARENA__256 + 1792, N_FAST_ARENA__256 + 1856, N_FAST_ARENA__256 + 1920, N_FAST_ARENA__256 + 1984 
												},
												{
													N_FAST_ARENA__512 + 0, N_FAST_ARENA__512 + 128, N_FAST_ARENA__512 + 256, N_FAST_ARENA__512 + 384, 
													N_FAST_ARENA__512 + 512, N_FAST_ARENA__512 + 640, N_FAST_ARENA__512 + 768, N_FAST_ARENA__512 + 896, 
													N_FAST_ARENA__512 + 1024, N_FAST_ARENA__512 + 1152, N_FAST_ARENA__512 + 1280, N_FAST_ARENA__512 + 1408, 
													N_FAST_ARENA__512 + 1536, N_FAST_ARENA__512 + 1664, N_FAST_ARENA__512 + 1792, N_FAST_ARENA__512 + 1920, 
													N_FAST_ARENA__512 + 2048, N_FAST_ARENA__512 + 2176, N_FAST_ARENA__512 + 2304, N_FAST_ARENA__512 + 2432, 
													N_FAST_ARENA__512 + 2560, N_FAST_ARENA__512 + 2688, N_FAST_ARENA__512 + 2816, N_FAST_ARENA__512 + 2944, 
													N_FAST_ARENA__512 + 3072, N_FAST_ARENA__512 + 3200, N_FAST_ARENA__512 + 3328, N_FAST_ARENA__512 + 3456, 
													N_FAST_ARENA__512 + 3584, N_FAST_ARENA__512 + 3712, N_FAST_ARENA__512 + 3840, N_FAST_ARENA__512 + 3968 
												},
												{
													N_FAST_ARENA_1024 + 0, N_FAST_ARENA_1024 + 256, N_FAST_ARENA_1024 + 512, N_FAST_ARENA_1024 + 768, 
													N_FAST_ARENA_1024 + 1024, N_FAST_ARENA_1024 + 1280, N_FAST_ARENA_1024 + 1536, N_FAST_ARENA_1024 + 1792, 
													N_FAST_ARENA_1024 + 2048, N_FAST_ARENA_1024 + 2304, N_FAST_ARENA_1024 + 2560, N_FAST_ARENA_1024 + 2816, 
													N_FAST_ARENA_1024 + 3072, N_FAST_ARENA_1024 + 3328, N_FAST_ARENA_1024 + 3584, N_FAST_ARENA_1024 + 3840, 
													N_FAST_ARENA_1024 + 4096, N_FAST_ARENA_1024 + 4352, N_FAST_ARENA_1024 + 4608, N_FAST_ARENA_1024 + 4864, 
													N_FAST_ARENA_1024 + 5120, N_FAST_ARENA_1024 + 5376, N_FAST_ARENA_1024 + 5632, N_FAST_ARENA_1024 + 5888, 
													N_FAST_ARENA_1024 + 6144, N_FAST_ARENA_1024 + 6400, N_FAST_ARENA_1024 + 6656, N_FAST_ARENA_1024 + 6912, 
													N_FAST_ARENA_1024 + 7168, N_FAST_ARENA_1024 + 7424, N_FAST_ARENA_1024 + 7680, N_FAST_ARENA_1024 + 7936
												}

											};


#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
static const char LogTable256[256] = 
{
    -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
    LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
};

static const uint8_t  N_FAST_IDX_LOOKUP[20];


uint32_t log2 (uint32_t v) {  	// 32-bit word to find the log of
	uint32_t r;     // r will be lg(v)
	uint32_t t, tt; // temporaries
	tt = v >> 16;
	if (tt > 0) {
		t= tt >> 8;
		// 				24 +					16 +
	    return (t > 0) ? 0x18 | LogTable256[t] : 0x10 | LogTable256[tt];
	} else {
		t = v >> 8;
		//				08 +
	    return (t > 0) ? 0x08 | LogTable256[t] : LogTable256[v];
	}
}


#ifdef __NO_INLINE__ 
    void *  __attribute__ ((noinline)) nmalloc(unsigned bytes)
#else   
    void * nmalloc(unsigned bytes)
#endif

 {

	printbytes(1, 1, bytes);


	//printf("  [DEBUG] log2bytes\n");
	uint32_t log2bytes 			= log2(bytes);
	uint32_t roundbytes 		= 1 << log2bytes;
	//printf("  [DEBUG] log2bytes end\n");

	//printf("  [DEBUG] log2bytes = %08x\n", log2bytes);
	uint32_t logidx 			= (bytes > roundbytes ) ? log2bytes+1 : log2bytes;
	//printf("  [DEBUG] logidx = %08x\n", logidx);
	if(logidx <= 4)
		printbytes(1, 2, 32);
	else {
		printbytes(1, 2, 32 << (logidx-5));
	}
	if(logidx > 9) {
		printf("Out of space.\n");
		return NULL;
	}
	uint32_t FREE_ADDRESS 	= N_FREE_ADDRESS___LT[logidx];
	if(FREE_ADDRESS != 0xFFFFFFFF) {
		uint32_t z_vec 		= ~FREE_ADDRESS & ~(~FREE_ADDRESS-1);
		uint32_t addr_idx 	= log2(z_vec);
		N_FREE_ADDRESS___LT[logidx] |= z_vec;
		uint32_t * addr 	= N_FAST_ADDRESS___LT[logidx][addr_idx]; 
		
		//printf("  [DEBUG] calc addr end\n");		
		printbytes(1, 0, (uint32_t)addr);
		return (void *)addr;
	}

	// conduct linear search ^^ 
	for(int i = logidx+1; i < 10; ++ i) {
		//printf("  [DEBUG] calc addr\n");
		FREE_ADDRESS 	= N_FREE_ADDRESS___LT[i];
		if(FREE_ADDRESS != 0xFFFFFFFF) {
			uint32_t z_vec 		= ~FREE_ADDRESS & ~(~FREE_ADDRESS-1);
			uint32_t addr_idx 	= log2(z_vec);
			N_FREE_ADDRESS___LT[i] |= z_vec;
			uint32_t * addr 	= N_FAST_ADDRESS___LT[i][addr_idx]; 
			printbytes(1, 0, (uint32_t)addr);		
			//printf("  [DEBUG] calc addr end\n");		

			return (void *)addr;
		}

		//printf("  [DEBUG] calc addr end\n");		
	}
	printf("returning NULL.\n");
	return NULL;
	
}

#ifdef __NO_INLINE__ 
    void __attribute__ ((noinline)) nfree(void * p)
#else   
    void nfree(void * p)
#endif
{
	printbytes(0, 0, (uint32_t)p);
	uint32_t addr = (uint32_t)p;
	uint32_t idx = addr >> 23;

	idx -= (uint32_t)N_FAST_ADDRESS___LT[0][0] >> 23;
	
	if(idx < 10) {
		//printf("  [DEBUG] pre N_FREE_ADDRESS___LT %08x\n", N_FREE_ADDRESS___LT[idx]);
		//printf("  [DEBUG] idx %d\n", idx);
		//printf("  [DEBUG] addr %08x\n", addr);
		N_FREE_ADDRESS___LT[idx] ^= (1 << ( (addr >> N_SHIFT_LEFTS__LT[idx]) & 0x0000001F ));
		//printf("  [DEBUG] pst N_FREE_ADDRESS___LT %08x\n", N_FREE_ADDRESS___LT[idx]);

	} else {
		// ---> using an alternate method for freeing
		// no longer look-up based;
	}
}

void * nrealloc(void * vp, unsigned newbytes) {
	void *newp = NULL;
	uint32_t *cnewp, *cvp;

	int idx = 0;
	/* behavior on corner cases conforms to SUSv2 */
	if (vp == NULL)
		return nmalloc(newbytes);

	if (newbytes != 0) {
		if ( (newp = nmalloc(newbytes)) == NULL)
			return NULL;
		
		uint32_t addr = (uint32_t)vp;
		//printf("  [DEBUG] --> %08x\n", addr);
		uint32_t idx = addr >> 23;
		idx -= (uint32_t)N_FAST_ADDRESS___LT[0][0] >> 23;
		//printf("  [DEBUG] --> %08x\n", idx);

		for(int i = 0; i < 10; ++i) {
			//printf("  [DEBUG] Addresses %08x\n", N_FAST_ADDRESS___LT[i][0]);
		}

		uint32_t bound = (1 << N_SHIFT_LEFTS__LT[idx] < newbytes) ? N_FREE_ADDRESS___LT[idx] : newbytes;
		bound >>= 2;

		//printf("  [DEBUG] bound is %08x\n", bound);

		cnewp   = (uint32_t *)newp;
		cvp     = (uint32_t *)vp;

		for(int i = 0; i < bound; ++i) {
			cnewp[i]=cvp[i];
		}

		//printf("  [DEBUG] finished copying.\n"); 
	}

	nfree(vp);
	return newp;
}


void * ncalloc(unsigned nelem, unsigned elsize) {
  void *vp;
  unsigned nbytes;
  uint32_t * cvp;
  unsigned int i = 0;

  nbytes = (nelem * elsize)>>2;
  if ( (vp = nmalloc(nbytes)) == NULL)
    return NULL;

  cvp = (uint32_t *)vp;
  for(i=0; i< nbytes; ++i) {
    cvp[i]='\0';
  }
  return vp;
}
