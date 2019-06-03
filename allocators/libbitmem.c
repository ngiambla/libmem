//===-- libbitmem.cpp -----------------------------------------*- C -*--------===//
//
//
//
// Written By: Nicholas V. Giamblanco
//===-------------------------------------------------------------------------===//
#include "memutils.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>


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




#define ARENASIZE       ARENA_BYTES                   // Number of bytes available.
#define FACTOR          MIN_REQ_SIZE                  // Each bitmap translates to 16 bytes (Must be a power of two).
#define LT_FAC          FACTOR-1                      // Need to check for Less than Factors..
#define SHFTFACTOR      (uint32_t) (BITS_TO_REPRESENT(FACTOR)-1)    // Divisor translated into a shift amnt, to check req'd bits
#define BLOCKS          (ARENASIZE/FACTOR)            // Blocks which
#define BITMAP          32
#define MAPBLOCKS       BLOCKS/BITMAP

#define ROR(X, N)( (X >> N) | (X << (BITMAP-N)) )
#define ROL(X, N)( (X << N) | (X >> (BITMAP-N)) )


static uint32_t arena_b[ARENASIZE/4];
static uint32_t bitmap[MAPBLOCKS] = {0};
static uint16_t bitsize[BLOCKS] = {0};

/* For lazy free, store 32, 32-bit addresses */
static uint32_t lazyhold[32];
static uint8_t  lazyreserved = 0;


#ifdef __DO_NOT_INLINE__ 
   void * __attribute__ ((noinline)) bit_malloc(unsigned nbytes)
#else
   void * bit_malloc(unsigned nbytes)
#endif
   {
  
   uint32_t current_map;
   uint32_t * addr;
   uint32_t last_block = 0, cur_map_idx;

   // Force at least one byte req.
   nbytes = (nbytes <=MIN_REQ_SIZE) ? MIN_REQ_SIZE : nbytes;

   // Begin Searching the bitmap.
   // --------------------------------------
   // First, find req'd number of bits.
   uint16_t num_reqd_bits;

   // check if we need to add an extra bit for nbytes with values inbetween mod 16.
   // See if we wrap over div FACTOR... if so, add an extra bit (partially waste a block of 16 bytes)
   int mod_res = nbytes&(LT_FAC);
   // printf("mod_res = %08x, nbytes(%08x)&LT_FAC(%08x)SHFTFACTOR(%08x)\n", mod_res,nbytes,LT_FAC,SHFTFACTOR);

   // if we wrap, add the extra FACTOR bytes to the div of nbytes.
   num_reqd_bits = (mod_res==0) ? nbytes >> SHFTFACTOR : ((nbytes - mod_res)>>SHFTFACTOR) +1;
   //printbytes(1, 2, num_reqd_bits*MIN_REQ_SIZE);  

   uint16_t cur_bits_aqd      = 0; //Store bits acquired.
   uint16_t bitmap_start_bit  = 0; //Where to begin the mapping.
   uint16_t bitmap_end_bit    = 0;
   uint16_t real_bit_mod_res;
   uint16_t map_index;

   // Searching for free space within the bitmap.
   // printf("+=========================================+\n");
   // printf("bit_malloc %d bits needed for casted %d bytes from orig bytes %d\n",num_reqd_bits, num_reqd_bits*FACTOR, nbytes);   
   // printf("[S] free space search\n");
   for(cur_map_idx = 0; cur_map_idx < MAPBLOCKS; ++cur_map_idx) {
      // printf("Bitmap[%d] = 0x%08x\n",cur_map_idx, bitmap[cur_map_idx]);
      current_map = bitmap[cur_map_idx];
      for(int bit = 0; bit < 32; ++bit) {
         // Check each bit by ROL the bit map.
         cur_bits_aqd = !(ROL(0x01,bit)&(current_map)) ? cur_bits_aqd+1 : 0;
         // Maintain State of which bit we end on.
         ++bitmap_end_bit;

         if(cur_bits_aqd == num_reqd_bits) {
            bitmap_start_bit = bitmap_end_bit - num_reqd_bits;
            // printdbg("    [E] Bitmap Search\n");

            goto BLOCK_CLAIM;
         }
      }
   }

   printf("No available memory.\n");
   return NULL;

   BLOCK_CLAIM:

      // printf("+=========================================+\n");
      // printf("BLOCK_CLAIM\n");
      // printf("bitmap_end_start = %d\n", bitmap_start_bit);
      // printf("bitmap_end_bit   = %d\n", bitmap_end_bit);

      real_bit_mod_res = bitmap_start_bit&(0x1F);      
      map_index = (real_bit_mod_res==0) ? bitmap_start_bit>>5 : (bitmap_start_bit - real_bit_mod_res)>>5;

      addr = arena_b + (bitmap_start_bit << SHFTFACTOR);
      bitsize[bitmap_start_bit] = num_reqd_bits;
      
      cur_bits_aqd = 0;

      for(cur_map_idx = map_index; cur_map_idx < MAPBLOCKS; ++cur_map_idx) {
         current_map = bitmap[cur_map_idx];
         for(int bit = real_bit_mod_res; bit < 32; ++bit) {
            current_map |= 1<<bit;
            ++cur_bits_aqd;
            if(cur_bits_aqd == num_reqd_bits) {
               bitmap[cur_map_idx] = current_map;
               goto RET_ADDR;
            }
         }
         bitmap[cur_map_idx] = current_map;
         real_bit_mod_res = 0;
      }

   RET_ADDR:
      // printf("addr = %08x\n", (uint32_t)addr);
      // printf("+=========================================+\n\n");
      // printbytes(1, 0, (uint32_t)addr);  
      return (void *)addr;
}

#ifdef __DO_NOT_INLINE__ 
   void __attribute__ ((noinline)) bit_free(void * p) 
#else
   void bit_free(void * p) 
#endif
{

   uint16_t bitmap_start_bit = (uint16_t)(((uint32_t *)p - arena_b) >> SHFTFACTOR);
   uint16_t real_bit_mod_res = bitmap_start_bit&(0x1F);      
   uint16_t map_index = (real_bit_mod_res==0) ? bitmap_start_bit>>5 : (bitmap_start_bit - real_bit_mod_res)>>5;

   uint16_t bits_to_free = bitsize[bitmap_start_bit];   
   for(int cur_map_idx = map_index; cur_map_idx < MAPBLOCKS; ++cur_map_idx) {
      uint32_t current_map = bitmap[cur_map_idx];
      for(int bit = real_bit_mod_res; bit < 32; ++bit) {
         current_map &= ROL(0xFFFFFFFE, (bit));
         --bits_to_free;
         if(bits_to_free == 0) {
            bitmap[cur_map_idx] = current_map;
            return;
         }
      }
      bitmap[cur_map_idx] = current_map;
   }
}

void * bit_calloc(unsigned nelem, unsigned elsize) {
   unsigned nbytes;
   unsigned char * cvp;
   unsigned int i = 0;

   nbytes = nelem * elsize;
   cvp = (unsigned char *)bit_malloc(nbytes);
   if (!cvp){
      return NULL;
   }

   for(i=0; i< nbytes; ++i) {
      cvp[i]='\0';
   }
   return (void *)cvp;
}

void * bit_realloc(void * vp, unsigned newbytes) {
   char *cnewp, *cvp;
   uint16_t starting_bit_num = ((uint16_t)((uint32_t *)vp - arena_b) >> SHFTFACTOR);
   uint16_t bits_to_free = bitsize[starting_bit_num];   

   int idx = 0;
   /* behavior on corner cases conforms to SUSv2 */
   cvp = (char *)vp;

   if (!cvp) {
      return bit_malloc(newbytes);
   }

   if (newbytes != 0) {
      uint64_t bytes;

      cnewp = (char *)bit_malloc(newbytes);
      if(!cnewp) {
         return NULL;
      }

      bytes = bits_to_free << SHFTFACTOR;
      
      if (bytes > newbytes){
         bytes = newbytes;
      }

      for(idx = 0; idx < bytes; ++ idx) {
         cnewp[idx]=cvp[idx];
      }
   
   }

   bit_free(cvp);
   return (void *)cnewp;
}


#ifdef __DO_NOT_INLINE__ 
    void __attribute__ ((noinline)) bit_lazyfree(void * vp)
#else   
    void bit_lazyfree(void * vp)
#endif
{

  if(lazyreserved < 32) {
    lazyhold[lazyreserved++] = (uint32_t)vp;
  } else {
    lazyreserved = 0;
    bit_free(vp);
    for(int i = 0; i < 32; ++i) {
      bit_free((void*)lazyhold[i]);
    }
  }


}