//===-- memutils.h --------------------------------------------*- C -*--------===//
//
// This file is a header for all allocators...
//
// Written By: Nicholas V. Giamblanco
//===-------------------------------------------------------------------------===//
#ifndef __MEMUTILS_H__
#define __MEMUTILS_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Defines
#define __DO_NOT_INLINE__ 				/* Can enforce each allocator to be separate functions */
// #define __DEBUG__

/* This makes each allocator's arena use 65536 bytes or 64 kB (Needs to be power of two) */
#define ARENA_BYTES		65536
/* Defines the minimum requestable size (only applies to buddy, bit and lut) (Needs to be power of two, and cannot be larger )*/
#define MIN_REQ_SIZE 	16
#define BUDDY_SAFETY_CHECK 		ARENA_BYTES/MIN_REQ_SIZE
#if BUDDY_SAFETY_CHECK < 32
#error "ARENA_BYTES/MIN_REQ_SIZE must be at most 32."
#endif

//==------------------------------------------==//
// [GNU - Based Allocation: Single Heap]
void * 	gnu_malloc	(unsigned nbytes);
void * 	gnu_realloc	(void * vp, unsigned newbytes);
void * 	gnu_calloc	(unsigned nelem, unsigned elsize);
void 	gnu_free 	(void * vp);
//==-----------------------------------------
//
// [Linear Based Allocation: Single Heap]
void * 	lin_malloc	(unsigned size);
void * 	lin_realloc	(void * p, unsigned newbytes);
void * 	lin_calloc	(unsigned nelem, unsigned elsize);
void 	lin_free	(void * p);
//==-----------------------------------------
//
// [Bitmap Based Allocation: Single Heap]
void * 	bit_malloc	(unsigned size);
void * 	bit_realloc	(void * p, unsigned newbytes);
void * 	bit_calloc	(unsigned nelem, unsigned elsize);
void 	bit_free	(void * p);
//------------------------------------------
//
// [Buddy Based Allocation: Single Heap]
void * 	bud_malloc	(unsigned size);
void * 	bud_realloc	(void * p, unsigned newbytes);
void * 	bud_calloc	(unsigned nelem, unsigned elsize);
void 	bud_free	(void * p);
//------------------------------------------
//
// [Buddy Based Allocation: Single Heap]
void * 	lut_malloc	(unsigned size);
void * 	lut_realloc	(void * p, unsigned newbytes);
void * 	lut_calloc	(unsigned nelem, unsigned elsize);
void 	lut_free	(void * p);
//------------------------------------------

#endif
