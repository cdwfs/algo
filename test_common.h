#ifndef ALGO_TEST_COMMON_H
#define ALGO_TEST_COMMON_H

#define ALGO_IMPLEMENTATION
#include "algo.h"
#include "platform.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(_MSC_VER)
#include <crtdbg.h>
#endif

#if 1
#	define ALGO_VALIDATE(expr) ZOMBO_RETVAL_CHECK(kAlgoErrorNone, expr)
#else
#	ifndef __has_feature
#		define __has_feature(x) 0 /* compatibility with non-clang compilers. */
#	endif
#	ifndef CLANG_ANALYZER_NORETURN
#		if __has_feature(attribute_analyzer_noreturn)
#		    define CLANG_ANALYZER_NORETURN __attribute__((analyzer_noreturn))
#		else
#		    define CLANG_ANALYZER_NORETURN
#		endif
#	endif
void algo_assert_handler(void) CLANG_ANALYZER_NORETURN {}
#	define ALGO_VALIDATE(expr) do {										\
		AlgoError error = ( expr );										\
		if (kAlgoErrorNone != error) {									\
			fprintf(stderr, "ERROR: %s returned %d\n", #expr, error);	\
			ALGO_DEBUGBREAK();											\
            algo_assert_handler();                                      \
		}																\
	} while(0,0)
#endif

#if !defined(max)
#	define max(x,y) ((x)>(y))?(x):(y)
#endif

#endif
