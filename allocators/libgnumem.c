//===-- libgnumem.cpp -----------------------------------------*- C -*--------===//
// This file implements a naive verion of Doug Lea's malloc() and free()
// with optimizations for HLS. This is commonly used within GNU frameworks.
// Written By: Nicholas V. Giamblanco
//===-------------------------------------------------------------------------===//

// Standard-Lib Includes
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

// Memory Utils Includes.
#include "memutils.h"

// CHUNK header for heap management. Each Chunk requires a minimum size of 16 bytes (according to this alignment scheme)
typedef struct CHUNK {
  uint8_t space_hold;
  struct CHUNK * l;
  struct CHUNK * r;
  uint8_t meta;
} CHUNK;

// We store the freebit -- 1 if the chunk is free, 0 if it is reserved --
#define SET_FREEBIT(chunk) ( (chunk)->meta = 0x01 )
#define CLR_FREEBIT(chunk) ( (chunk)->meta = 0x00 )
#define GET_FREEBIT(chunk) ( (chunk)->meta )

// chunk size is implicit from l-r
#define CHUNKSIZE(chunk) ( (char *)(chunk)->r - (char *)(chunk) )
#define TOCHUNK(vp) (-1 + (CHUNK *)(vp))
#define FROMCHUNK(chunk) ((void *)(1 + (chunk)))
#define ARENA_CHUNKS (ARENA_BYTES/sizeof(CHUNK))

static CHUNK arena[ARENA_CHUNKS];
static CHUNK *bot = NULL;       /* all free space, initially */
static CHUNK *top = NULL;       /* delimiter chunk for top of arena */

// For lazy free, store 32, 32-bit addresses
static uint32_t lazyhold[32];
static uint8_t  lazyreserved = 0;


// This initializes the doubly-linked list of CHUNKS to point to the beginning and end of the arena (indicating the availability of the entire arena)
static void init(void) {
  bot = &arena[0]; top = &arena[ARENA_CHUNKS-1];
  bot->l = NULL; 
  bot->r = top;
  bot->meta = 0x01;
  
  top->l = bot;  
  top->r = NULL;
  top->meta = 0x0;
}


// This search through the doubly-linked list of chunks to find enough free space for an incoming request (returns NULL if no space is available.)
#ifdef __DO_NOT_INLINE__ 
   void * __attribute__ ((noinline)) gnu_malloc(unsigned nbytes)
#else
    void * gnu_malloc(unsigned nbytes)
#endif
{
  CHUNK *p;
  uint64_t size, chunksz;
  uint8_t res1, res2;

  if (!bot){
    init();
  }

  nbytes = (nbytes <= 0) ? 1 : nbytes;

  /* Using Division to compute Upper Bound */
  size = sizeof(CHUNK) * ((nbytes+sizeof(CHUNK)-1)/sizeof(CHUNK) + 1); // Will be automatically converted into the corresponding shift operator defined in our paper 
  p = bot;
  while (p != NULL) {
    chunksz = CHUNKSIZE(p);

    res1 = chunksz > size;
    res2 = chunksz == size;
    if (GET_FREEBIT(p) && (res1 || res2)) {
        CLR_FREEBIT(p);

        /* create a remainder chunk */
        if (res1) { 
            CHUNK * q, * pr;

            q = (CHUNK *)(size + (char *)p );
            pr = p->r;

            q->l = p; 
            q->r = pr;
            
            p->r = q; 
            pr->l = q;

            SET_FREEBIT(q);
          }      
      break;
    }
    p = p->r;
  }
  return (!p) ? NULL : FROMCHUNK(p);

}

#ifdef __DO_NOT_INLINE__ 
    void __attribute__ ((noinline)) gnu_free(void * vp)
#else   
    void gnu_free(void * vp)
#endif
 {
  CHUNK *p, *q;
  if (!vp) {
    return;
  }

  p = TOCHUNK(vp);
  CLR_FREEBIT(p);
  q = p->l;
  if (q != NULL && GET_FREEBIT(q)) /* try to consolidate leftward */
    {

      CLR_FREEBIT(q);
      q->r        = p->r;
      ((p->r))->l = q;
      SET_FREEBIT(q);

      p = q;
    }

  q = p->r;

  if (q != NULL && GET_FREEBIT(q)) /* try to consolidate rightward */
    {
      CLR_FREEBIT(q);
      p->r      = q->r;
      (q->r)->l = p;
      SET_FREEBIT(q);
    }
  SET_FREEBIT(p);  
}

void * gnu_realloc(void * vp, unsigned newbytes) {
  void *newp = NULL;
  char *cnewp, *cvp;

  int idx = 0;
  /* behavior on corner cases conforms to SUSv2 */
  if (vp == NULL)
    return gnu_malloc(newbytes);

  if (newbytes != 0)
    {
      CHUNK *oldchunk;
      uint64_t bytes;

      if ( (newp = gnu_malloc(newbytes)) == NULL)
        return NULL;
      oldchunk = TOCHUNK(vp);
      bytes = CHUNKSIZE(oldchunk) - sizeof(CHUNK);
      if (bytes > newbytes)
        bytes = newbytes;

      cnewp   = (char *)newp;
      cvp     = (char *)vp;
      for(idx = 0; idx < bytes; ++ idx) {
        cnewp[idx]=cvp[idx];
      }
    }

  gnu_free(vp);
  return newp;
}

void * gnu_calloc(unsigned nelem, unsigned elsize) {
  void *vp;
  unsigned nbytes;
  unsigned char * cvp;
  unsigned int i = 0;

  nbytes = nelem * elsize;
  if ( (vp = gnu_malloc(nbytes)) == NULL)
    return NULL;

  cvp = (unsigned char *)vp;
  for(i=0; i< nbytes; ++i) {
    cvp[i]='\0';
  }
  return vp;
}



#ifdef __DO_NOT_INLINE__ 
    void __attribute__ ((noinline)) gnu_lazyfree(void * vp)
#else   
    void gnu_lazyfree(void * vp)
#endif
{

  if(lazyreserved < 32) {
    lazyhold[lazyreserved++] = (uint32_t)vp;
  } else {
    lazyreserved = 0;
    gnu_free(vp);
    for(int i = 0; i < 32; ++i) {
      gnu_free((void*)lazyhold[i]);
    }
  }


}