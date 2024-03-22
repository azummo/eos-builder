eos-builder Event Builder
=========================
Introduction
------------
eos-builder is a fast, lightweight event builder originally developed for the
SNO+ experiment. It was developed primarily for support of monitoring tool
development, since those tools require built data. Designed from the ground up
for speed, it makes use of threading, the jemalloc thread-optimized memory
allocator, and optimized data structures to achieve very high throughput.

Installation
------------
Dependencies:

* C compiler. Only tested with GCC.
* jemalloc library (http://www.canonware.com/jemalloc/)
* pthreads library (-lpthread) (POSIX) 
* rt (time) library (-lrt) (POSIX)

To make:

    $ make

To make with debugging flags:

    $ FLAGS=-g make

