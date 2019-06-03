//===-- liblinmem.cpp -----------------------------------------*- C -*--------===//
//
//
//
// Written By: Nicholas V. Giamblanco
//===-------------------------------------------------------------------------===//
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "memutils.h"

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


void * __attribute__ ((noinline)) lin_malloc(unsigned nbytes) {
	uint32_t size = nbytes;
	uint32_t newsize=0;

	if(size == 0) {
		size = HEAPWIDTH_SZ;
	}
	
	if((size&MASK) != 0) {
		newsize=(size&(~0x03))+HEAPWIDTH_SZ;
	} else {
		newsize=size;
	}

	if(newsize >= (END-CURR_ADDR)) {
		printf(" Out of bounds...\n");
		return NULL;
	}
	PREV_ADDR = CURR_ADDR;
	CURR_ADDR += newsize;
	
	return GETADDR(PREV_ADDR);
}


void * __attribute__ ((noinline)) lin_realloc(void * p, unsigned newsize) {
	newsize = (newsize <= 0) ? HEAPWIDTH_SZ : newsize;
		
	if(!p) {
		return lin_malloc(newsize);
	}

	int size = ((newsize&MASK) == 0) ? newsize : ((newsize&(~MASK))+HEAPWIDTH_SZ);;
	
	if(p == PREV_ADDR && size < (END-PREV_ADDR)) {
		CURR_ADDR = PREV_ADDR + size;
		return p;
	}
	return NULL;
}


void * __attribute__ ((noinline)) lin_calloc(unsigned nelem, unsigned elsize) {
  void *vp;
  unsigned nbytes;
  unsigned char * cvp;
  unsigned int i = 0;

  nbytes = nelem * elsize;
  if ( (vp = lin_malloc(nbytes)) == NULL)
    return NULL;
  cvp = (unsigned char *)vp;
  for(i=0; i< nbytes; ++i) {
    cvp[i]='\0';
  }
  return vp;
}

void lin_free(void * p) {
}


void lin_freeall()  {
	CURR_ADDR = larena;
	PREV_ADDR = larena;
}