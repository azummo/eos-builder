eos-builder Event Builder
=========================
Introduction
------------
eos-builder is a fast, lightweight event builder originally developed for the
SNO+ experiment and adapted for Eos.

Installation
------------
Dependencies:

* C compiler (tested with GCC)
* jemalloc library (http://www.canonware.com/jemalloc/)
* pthreads library (-lpthread) (POSIX) 
* rt (time) library (-lrt) (POSIX)

To make:

    $ make

To make with debugging flags:

    $ FLAGS=-g make

