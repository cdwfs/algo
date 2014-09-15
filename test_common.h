#ifndef ALGO_TEST_COMMON_H
#define ALGO_TEST_COMMON_H

#define ALGO_IMPLEMENTATION
#include "algo.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(_MSC_VER)
#include <crtdbg.h>
#endif

#define ALGO_VALIDATE(expr) do {							\
		AlgoError error = ( expr );							\
		if (kAlgoErrorNone != error) {							\
			fprintf(stderr, "ERROR: %s returned %d\n", #expr, error);\
			assert(kAlgoErrorNone == error);					\
		}													\
	} while(0,0)

#if !defined(max)
#define max(x,y) ((x)>(y))?(x):(y)
#endif

#endif // ALGO_TEST_COMMON_H
