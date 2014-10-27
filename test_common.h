#ifndef ALGO_TEST_COMMON_H
#define ALGO_TEST_COMMON_H

#define ALGO_IMPLEMENTATION
#ifdef _MSC_VER
#	define ALGO_ASSERT(cond) if (!(cond)) __debugbreak()
#endif
#include "algo.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(_MSC_VER)
#include <crtdbg.h>
#endif

#ifdef _MSC_VER
#	define ALGO_DEBUGBREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#	define ALGO_DEBUGBREAK() asm("int $3")
#else
#   error Unsupported compiler
#endif

#define ALGO_VALIDATE(expr) do {										\
		AlgoError error = ( expr );										\
		if (kAlgoErrorNone != error) {									\
			fprintf(stderr, "ERROR: %s returned %d\n", #expr, error);	\
			ALGO_DEBUGBREAK();											\
		}																\
	} while(0,0)

#if !defined(max)
#define max(x,y) ((x)>(y))?(x):(y)
#endif

#endif
