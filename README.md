# `libmem` - A Dynamic Memory Allocation library for High-Level Synthesis designs.

## What is `libmem`

This C library has 5 different dynamic memory allocation schemes, suitable for software and HLS designers (even though the purpose of this library was intended to serve the latter case).
Our library is configurable on a minimum request size.

1. `libgnumem.c`: An allocator which inherits most of it's logic from Doug Lea's `malloc & free`.
2. `liblinmem.c`: A linear allocator which 'bumps' up a pointer by an incoming request size to the next available free region of memory.
3. `libbitmem.c`: A bitmap allocator, which configurable minimum request size.
4. `libbudmem.c`: A buddy allocator, which is configurable on minimum request size.
5. `liblutmem.c`: A LUT-like allocator, where a request size is mapped to a corresponding set of preallocated address, stored in a LUT.

We also include the LLVM-Transformation Pass which was outlined in Dynamic Memory Allocation Techniques for High-Level Synthesis. This pass is able to convert stack allocated arrays into dynamic memory calls in order to reduce BRAM pressure within an FPGA. This pass lives in `transformation`

## Why do we need `libmem`

For a beginner HLS developer wishing to port over designs using dynamic memory, it is not straightforward or easy to migrate designs to a static memory requirement.
With this library, we patch this concern.

## Is there an evaluation suite?

Yes, I have linked a separate repository which includes a suite of benchmarks (suitable for performance checking with HLS tools)
This link is [here](https://github.com/ngiambla/dmbenchhls)!

## Notes

This library doesn't use off-chip memory. It strings together a bunch of BRAMs and uses this as a heap.

This library is associated with this publication: Nicholas Giamblanco and Jason Anderson. A Dynamic Memory Allocation
Library for High-Level Synthesis. In 2019 29th International Conference on Field
Programmable Logic and Applications (FPL). IEEE, 2019



### Author

Nicholas V. Giamblanco, M.A.Sc. (2019)
