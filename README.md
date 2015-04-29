Algo
====

You don't want to use this code. 

I've been reading Steven Skiena's [The Algorithm Design Manual](http://www.algorist.com) lately,
to refresh my long-dormant knowledge of fundamental data structures and algorithms. I decided to start
implementing a few of the things discussed in the book as a C90 single-header-file library (cribbing
heavily from the gospel of [stblib](https://github.com/nothings/stb)). I'm sure every aspiring programmer
in the world has already written this code; many of them probably did a better job than I have.
If you're looking for quality code, I recommend using theirs instead.

Currently includes:
- stack (last in, first out)
- queue (first in, first out)
- heap / priority queue (log-N insertion, log-N removal of highest-priority element)
- pool allocator (dynamic memory allocation of fixed-size elements).
- graph (vertex and edge management, plus flexible breadth- and depth-first searches and topological sorting)

On deck:
- hash table

Key Features / Design Goals
---------------------------
- **No dynamic memory allocations**: all data structures let the user place a strict upper bound on their capacity/usage,
  and then work entirely within a user-provided buffer of the appropriate size.
- **No (mandatory) external dependencies**: only standard C library functions are used, and even these can be overridden with
  custom implementations through #defines if desired.
- **Robust API**: all function inputs are validated. A mechanism to disable this validation is planned, but not yet implemented.
  All functions should return without side effects if an error occurs (not 100% guaranteed yet, but that's the goal).

