/* algo.h
 * Basic data structures & algorithms in C.
 * http://github.com/cdwfs/algo
 * no warranty implied; use at your own risk
 *
 * Do this:
 *    #define ALGO_IMPLEMENTATION
 * before you including file in *one* C/C++ file to create the implementation.
 *
 * To define all functions as static (to allow multiple inclusions without
 * link errors, do this:
 *    #define ALGO_STATIC
 *
 */

#ifndef ALGO_INCLUDE_H
#define ALGO_INCLUDE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ALGO_STATIC
#	define ALGODEF static
#else
#	define ALGODEF extern
#endif

#ifndef _MSC_VER
#	ifdef __cplusplus
#	define ALGO_INLINE inline
#else
#	define ALGO_INLINE
#	endif
#else
#	define ALGO_INLINE __forceinline
#endif

/* Redefine to replace standard library functions with custom implementations */
#ifndef ALGO_MEMSET
#	define ALGO_MEMSET memset
#endif
#ifndef ALGO_ASSERT
#	define ALGO_ASSERT assert
#endif


/** @brief Error code returned by algo functions. */
typedef enum AlgoError
{
	kAlgoErrorNone = 0,             /**< Function returned successfully; no errors occurred. */
	kAlgoErrorInvalidArgument = 1,  /**< One or more function arguments were invalid (e.g. NULL pointer, negative size, etc.). */
	kAlgoErrorOperationFailed = 2   /**< The requested operation could not be performed (e.g. popping from an empty stack). */
} AlgoError;

/**
 * @brief Poor man's polymorphism; allows containers to manipulate a variety of data types.
 *         Who needs type safety?
 */
typedef union AlgoData
{
	int32_t asInt; /**< The data value as a signed integer. */
	float asFloat; /**< The data value as a single-precision float. */
	void *asPtr;   /**< The data value as a void pointer. */
} AlgoData;
/** @brief Convenience function to wrap a signed integer as an AlgoData object. */
ALGODEF ALGO_INLINE AlgoData algoDataFromInt(int32_t i) { AlgoData d; d.asInt   = i; return d; }
/** @brief Convenience function to wrap a float as an AlgoData object. */
ALGODEF ALGO_INLINE AlgoData algoDataFromFloat(float f) { AlgoData d; d.asFloat = f; return d; }
/** @brief Convenience function to wrap a void pointer as an AlgoData object. */
ALGODEF ALGO_INLINE AlgoData algoDataFromPtr(void *p)   { AlgoData d; d.asPtr   = p; return d; }

/**
 * @brief Implements a pool-style memory allocator, with fixed-size blocks and O(1) alloc/free.
 */
typedef struct AlgoAllocPoolImpl *AlgoAllocPool;
/** @brief Computes the required buffer size for a pool allocator with the specified parameters. */
ALGODEF AlgoError algoAllocPoolBufferSize(size_t *outBufferSize, const int32_t elementSize, const int32_t elementCount);
/** @brief Initializes a pool allocator object.
	@param outAllocPool Pointer to the pool allocator to initialize.
	@param elementSize Size of each element in the pool. Must be at least 4 bytes.
	@param elementCount Number of elements in the pool. Must be greater than zero.
	@param buffer Memory buffer to use for this object. Use algoAllocPoolBufferSize() to compute the appropriate buffer size.
	@param bufferSize Size of the "buffer" parameter, in bytes. Use Use algoAllocPoolBufferSize() to compute the appropriate buffer size.
	*/
ALGODEF AlgoError algoAllocPoolCreate(AlgoAllocPool *outAllocPool, const int32_t elementSize, const int32_t elementCount,
	void *buffer, const size_t bufferSize);
/** @brief Allocates one element from the pool, and returns a pointer to it. */
ALGODEF AlgoError algoAllocPoolAlloc(AlgoAllocPool allocPool, void **outPtr);
/** @brief Frees an element previously allocated by algoAllocPoolAlloc(). */
ALGODEF AlgoError algoAllocPoolFree(AlgoAllocPool allocPool, void *p);
/** @brief Queries the element size of a pool allocator. */
ALGODEF AlgoError algoAllocPoolElementSize(AlgoAllocPool allocPool, int32_t *outElementSize);

/**
 * @brief Implements a stack (FILO/LIFO) data structure.
 * @code{.c}
 * int32_t stackCapacity = 1024; // Change to suit your needs
 * size_t stackBufferSize = 0;
 * void *stackBuffer = NULL;
 * AlgoStack stack;
 * AlgoError err; // Should be kAlgoErrorNone after every function call
 * AlgoData poppedData;
 *
 * err = algoStackBufferSize(&stackBufferSize, stackCapacity);
 * stackBuffer = malloc(stackBufferSize);
 * err = algoStackCreate(&stack, stackCapacity, stackBuffer, stackBufferSize);
 * err = algoStackPush(stack, algoDataFromInt(5)); // Push the number "5" to the stack
 * err = algoStackPop(stack, &poppedData); // Pop it off again into poppedData
 * free(stackBuffer); // No need to destroy the stack object itself; just free its buffer.
 * @endcode
 */
typedef struct AlgoStackImpl *AlgoStack;
/** @brief Computes the required buffer size for a stack with the specified capacity. */
ALGODEF AlgoError algoStackBufferSize(size_t *outBufferSize, int32_t stackCapacity);
/** @brief Initializes a stack object using the provided buffer. */
ALGODEF AlgoError algoStackCreate(AlgoStack *outStack, int32_t stackCapacity, void *buffer, size_t bufferSize);
/** @brief Pushes an element to the stack. */
ALGODEF AlgoError algoStackPush(AlgoStack stack, const AlgoData elem);
/** @brief Pops an element from the stack. */
ALGODEF AlgoError algoStackPop(AlgoStack stack, AlgoData *outElem);
/** @brief Retrieves the maximum number of elements that can be stored concurrently in the stack. */
ALGODEF AlgoError algoStackCapacity(const AlgoStack stack, int32_t *outCapacity);
/** @brief Retrieves the number of elements currently stored in the stack. */
ALGODEF AlgoError algoStackCurrentSize(const AlgoStack stack, int32_t *outSize);

/**
 * @brief Implements a queue (FIFO) data structure.
 * @code{.c}
 * int32_t queueCapacity = 1024; // Change to suit your needs
 * size_t queueBufferSize = 0;
 * void *queueBuffer = NULL;
 * AlgoQueue queue;
 * AlgoError err; // Should be kAlgoErrorNone after every function call
 * AlgoData removedData;
 *
 * err = algoQueueBufferSize(&queueBufferSize, queueCapacity);
 * queueBuffer = malloc(queueBufferSize);
 * err = algoQueueCreate(&queue, queueCapacity, queueBuffer, queueBufferSize);
 * err = algoQueueInsert(queue, algoDataFromInt(5)); // insert the number "5" into the queue.
 * err = algoQueueRemove(queue, &removedData); // Remove the head element from the queue and store it in removedData.
 * free(queueBuffer); // No need to destroy the queue object itself; just free its buffer.
 * @endcode
 */
typedef struct AlgoQueueImpl *AlgoQueue;
/** @brief Computes the required buffer size for a queue with the specified capacity. */
ALGODEF AlgoError algoQueueBufferSize(size_t *outBufferSize, int32_t queueCapacity);
/** @brief Initializes a queue object using the provided buffer. */
ALGODEF AlgoError algoQueueCreate(AlgoQueue *outQueue, int32_t queueCapacity, void *buffer, size_t bufferSize);
/** @brief Inserts an element into the queue. */
ALGODEF AlgoError algoQueueInsert(AlgoQueue queue, const AlgoData elem);
/** @brief Removes an element from the queue. */
ALGODEF AlgoError algoQueueRemove(AlgoQueue queue, AlgoData *outElem);
/** @brief Retrieves the maximum number of elements that can be stored concurrently in the queue. */
ALGODEF AlgoError algoQueueCapacity(const AlgoQueue queue, int32_t *outCapacity);
/** @brief Retrieves the number of elements currently stored in the queue. */
ALGODEF AlgoError algoQueueCurrentSize(const AlgoQueue queue, int32_t *outSize);


/**
 * @brief Implements a heap / priority queue data structure. Each element is inserted with a key, representing that
 *        element's priority. Popping removes the element with the lowest priority.
 * @code{.c}
 * int32_t heapCapacity = 1024; // Change to suit your needs
 * size_t heapBufferSize = 0;
 * void* heapBuffer = NULL;
 * AlgoHeap heap;
 * AlgoError err;
 * AlgoData poppedKey, poppedData;
 *
 * err = algoHeapBufferSize(&heapBufferSize, heapCapacity);
 * heapBuffer = malloc(heapBufferSize);
 * err = algoHeapCreate(&heap, heapCapacity, algoDataCompareIntAscending, heapBuffer, heapBufferSize);
 * err = algoHeapInsert(heap, key, value);
 * err = algoHeapPop(heap, &poppedKey, &poppedData);
 * @endcode
 */
typedef struct AlgoHeapImpl *AlgoHeap;

/**
 * @brief Allows users to define a custom ordering of keys within a heap.
 *        If this function returns < 0, keyL is higher priority than keyR.
 *        If this function returns > 0, keyR is higher priority than keyL.
 *        If this function returns   0, keyR has the same priority as keyL.
 */
typedef int (*AlgoDataCompareFunc)(const AlgoData keyL, const AlgoData keyR);
/** @brief Convenience function to sort heap keys as integers, in ascending order (lower value = higher priority) */
ALGODEF ALGO_INLINE int algoDataCompareIntAscending(const AlgoData keyL, const AlgoData keyR)
{
	if (keyL.asInt < keyR.asInt) return -1;
	if (keyL.asInt > keyR.asInt) return  1;
	return 0;
}
/** @brief Convenience function to sort heap keys as integers, in ascending order (higher value = higher priority) */
ALGODEF ALGO_INLINE int algoDataCompareIntDescending(const AlgoData keyL, const AlgoData keyR)
{
	if (keyL.asInt > keyR.asInt) return -1;
	if (keyL.asInt < keyR.asInt) return  1;
	return 0;
}
/** @brief Convenience function to sort heap keys as floats, in ascending order (lower value = higher priority) */
ALGODEF ALGO_INLINE int algoDataCompareFloatAscending(const AlgoData keyL, const AlgoData keyR)
{
	if (keyL.asFloat < keyR.asFloat) return -1;
	if (keyL.asFloat > keyR.asFloat) return  1;
	return 0;
}
/** @brief Convenience function to sort heap keys as floats, in ascending order (higher value = higher priority) */
ALGODEF ALGO_INLINE int algoDataCompareFloatDescending(const AlgoData keyL, const AlgoData keyR)
{
	if (keyL.asFloat > keyR.asFloat) return -1;
	if (keyL.asFloat < keyR.asFloat) return  1;
	return 0;
}
/** @brief Computes the required buffer size for a heap with the specified capacity. */
ALGODEF AlgoError algoHeapBufferSize(size_t *outBufferSize, int32_t heapCapacity);
/** @brief Initializes a heap object using the provided buffer. */
ALGODEF AlgoError algoHeapCreate(AlgoHeap *heap, int32_t heapCapacity, AlgoDataCompareFunc keyCompare,
	void *buffer, size_t bufferSize);
/** @brief Inserts an element into the heap, with the specified key. */
ALGODEF AlgoError algoHeapInsert(AlgoHeap heap, const AlgoData key, const AlgoData data);
/** @brief Inspects the "top" element, but does not remove it from the heap. */
ALGODEF AlgoError algoHeapPeek(AlgoHeap heap, AlgoData *outTopKey, AlgoData *outTopData);
/** @brief Removes the "top" element from the heap. */
ALGODEF AlgoError algoHeapPop(AlgoHeap heap, AlgoData *outTopKey, AlgoData *outTopData);
/** @brief Debugging function to validate heap consistency. */
ALGODEF AlgoError algoHeapCheck(AlgoHeap heap);
/** @brief Retrieves the maximum number of elements that can be stored concurrently in the heap. */
ALGODEF AlgoError algoHeapCapacity(AlgoHeap heap, int32_t *outCapacity);
/** @brief Retrieves the number of elements currently stored in the heap. */
ALGODEF AlgoError algoHeapCurrentSize(AlgoHeap heap, int32_t *outSize);

/**
 * Implements a generic graph structure.
 */
typedef struct AlgoGraphImpl *AlgoGraph;

typedef enum AlgoGraphEdgeMode
{
	kAlgoGraphEdgeUndirected = 0, /**< Graph edges are undirected; if v0->v1 exists, then so does v1->v0. */
	kAlgoGraphEdgeDirected   = 1, /**< Graph edges are directed; v0->v1 does not imply v1->v0. */
} AlgoGraphEdgeMode;

/** @brief Computes the required buffer size for a graph with the specified vertex and edge capacities. */
ALGODEF AlgoError algoGraphBufferSize(size_t *outBufferSize, int32_t vertexCapacity, int32_t edgeCapacity,
	const AlgoGraphEdgeMode edgeMode);
/** @brief Initializes a graph object using the provided buffer. */
ALGODEF AlgoError algoGraphCreate(AlgoGraph *outGraph, int32_t vertexCapacity, int32_t edgeCapacity,
	const AlgoGraphEdgeMode edgeMode, void *buffer, size_t bufferSize);
/** @brief Retrieves the current number of vertices in a graph. */
ALGODEF AlgoError algoGraphCurrentVertexCount(const AlgoGraph graph, int32_t *outCount);
/** @brief Retrieves the maximum number of vertices that can be stored in a graph. */
ALGODEF AlgoError algoGraphVertexCapacity(const AlgoGraph graph, int32_t *outCapacity);
/** @brief Retrieves the current number of edges in a graph. */
ALGODEF AlgoError algoGraphCurrentEdgeCount(const AlgoGraph graph, int32_t *outCount);
/** @brief Retrieves the maximum number of edges that can be stored in a graph. */
ALGODEF AlgoError algoGraphEdgeCapacity(const AlgoGraph graph, int32_t *outCapacity);
/** @brief Add a new vertex to a graph. Each vertex may optionally contain a piece of arbitrary user data. */
ALGODEF AlgoError algoGraphAddVertex(AlgoGraph graph, AlgoData vertexData, int32_t *outVertexId);
/** @brief Remove an existing vertex from a graph. This will implicitly remove any edges connecting this
           vertex to the rest of the graph. */
ALGODEF AlgoError algoGraphRemoveVertex(AlgoGraph graph, int32_t vertexId);
/** @brief Add a new edge to a graph, connecting srcVertexId to destVertexId.
           If the graph's edge mode is kAlgoGraphEdgeUndirected, a second edge will be added
		   from destVertexId to srcVertexId. */
ALGODEF AlgoError algoGraphAddEdge(AlgoGraph graph, int32_t srcVertexId, int32_t destVertexId);
/** @brief Remove an existing vertex from a graph.
           If the graph's edge mode is kAlgoGraphEdgeUndirected, the edge from destVertexId to srcVertexId
		   will also be removed. */
ALGODEF AlgoError algoGraphRemoveEdge(AlgoGraph graph, int32_t srcVertexId, int32_t destVertexId);
/** @brief Retrieve the degree (outgoing edge count) of a vertex in the graph. */
ALGODEF AlgoError algoGraphGetVertexDegree(const AlgoGraph graph, int32_t vertexId, int32_t *outDegree);
/** @brief Retrieve the vertices to which a given vertex is connected. */
ALGODEF AlgoError algoGraphGetVertexEdges(const AlgoGraph graph, int32_t srcVertexId, int32_t vertexDegree, int32_t outDestVertexIds[]);
/** @brief Retrieve a vertex's optional user data field. */
ALGODEF AlgoError algoGraphGetVertexData(const AlgoGraph graph, int32_t vertexId, AlgoData *outData);

typedef void (*AlgoGraphProcessVertexFunc)(AlgoGraph graph, int32_t vertexId);
typedef void (*AlgoGraphProcessEdgeFunc)(AlgoGraph graph, int32_t startVertexId, int32_t endVertexId);
/** @brief Compute the required buffer size to perform a breadth-first search on a graph. 
           This only includes the space required for temporary storage during the search, not the search results themselves. */
ALGODEF AlgoError algoGraphBfsBufferSize(size_t *outBufferSize, const AlgoGraph graph);
/** @brief Perform a breadth-first search on a graph.
	@param graph The graph to search.
	@param rootVertexId starting from the specified root vertex.
	@param outVertexParents If non-NULL, the parent of each vertex will be stored in this array, which must be large enough for the graph's
	                        maximum vertex capacity. The parent of the root vertex, vertices not connected to the root vertex,
							or invalid vertices will be -1.
	@param vertexParentCount The outVertexParents[] array must contain at least this many elements. This value should match the
	                         graph's maximum vertex capacity. If outVertexParents is NULL, this parameter is ignored.
	@param vertexFuncEarly If non-NULL, this function will be called on each vertex when it is first encountered during the traversal.
	@param edgeFunc If non-NULL, this function will be called on each edge when it is first encountered during the traversal. For undirected
	                graphs, the function will only be called once per pair of connected vertices.
	@param vertexFuncLate If non-NULL, this function will be called on each vertex after all of its edges have been explored.
	@param buffer Used for temporary storage during the search.
	@param bufferSize Size of the buffer[] array, in bytes. Given by algoGraphBfsBufferSize().
	*/
ALGODEF AlgoError algoGraphBfs(const AlgoGraph graph, int32_t rootVertexId, int32_t outVertexParents[], size_t vertexParentCount, 
	AlgoGraphProcessVertexFunc vertexFuncEarly, AlgoGraphProcessEdgeFunc edgeFunc, AlgoGraphProcessVertexFunc vertexFuncLate,
	void *buffer, size_t bufferSize);


#ifdef __cplusplus
}
#endif


/*------   end header file  -------------------------------------------- */
#endif /* ALGO_INCLUDE_H */

#ifdef ALGO_IMPLEMENTATION

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/******************************************
 * AlgoAllocPool
 ******************************************/

typedef struct AlgoAllocPoolImpl
{
	uint8_t *pool;
	int32_t elementSize; /* must be >= 4 */
	int32_t elementCount; /* must be > 0 */
	int32_t headIndex; /* if -1, pool is empty */
} AlgoAllocPoolImpl;

AlgoError algoAllocPoolBufferSize(size_t *outBufferSize, const int32_t elementSize, const int32_t elementCount)
{
	const size_t poolSize = elementCount*elementSize;
	if (NULL == outBufferSize ||
		elementSize < sizeof( int32_t) ||
		elementCount < 1)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outBufferSize = sizeof(AlgoAllocPoolImpl) + poolSize;
	return kAlgoErrorNone;
}

AlgoError algoAllocPoolCreate(AlgoAllocPool *outAllocPool, const int32_t elementSize, const int32_t elementCount, void *buffer, const size_t bufferSize)
{
	size_t minBufferSize = 0;
	AlgoError err;
	uint8_t *bufferNext = (uint8_t*)buffer;
	const size_t poolSize = elementCount*elementSize;
	if (NULL == outAllocPool ||
		elementSize < sizeof( (*outAllocPool)->headIndex) ||
		elementCount < 1)
	{
		return kAlgoErrorInvalidArgument;
	}
	err = algoAllocPoolBufferSize(&minBufferSize, elementSize, elementCount);
	if (err != kAlgoErrorNone)
	{
		return err;
	}
	if (NULL == buffer ||
		bufferSize < minBufferSize)
	{
		return kAlgoErrorInvalidArgument;
	}

	*outAllocPool = (AlgoAllocPoolImpl*)bufferNext;
	bufferNext += sizeof(AlgoAllocPoolImpl);

	(*outAllocPool)->pool = (uint8_t*)bufferNext;
	bufferNext += poolSize;

	ALGO_ASSERT( bufferNext-minBufferSize == buffer ); /* If this fails, algoAllocPoolBufferSize() is out of date */

	(*outAllocPool)->elementSize = elementSize;
	(*outAllocPool)->elementCount = elementCount;
	(*outAllocPool)->headIndex = 0;
	{
		uint8_t *elem = (*outAllocPool)->pool + 0;
		uint8_t *end  = (*outAllocPool)->pool + poolSize;
		int nextIndex;
		for(nextIndex = 1; nextIndex < elementCount; ++nextIndex)
		{
			*(int32_t*)elem = nextIndex;
			elem += elementSize;
		}
		*(int32_t*)elem = -1;
		ALGO_ASSERT(elem + elementSize == end);
		(void)end;
	}
	return kAlgoErrorNone;
}

AlgoError algoAllocPoolAlloc(AlgoAllocPool allocPool, void **outPtr)
{
	if (NULL == allocPool ||
		NULL == outPtr)
	{
		return kAlgoErrorInvalidArgument;
	}
	if (allocPool->headIndex == -1)
	{
		return kAlgoErrorOperationFailed;
	}

	*outPtr = (void*)(allocPool->pool + allocPool->headIndex * allocPool->elementSize);
	allocPool->headIndex = *(int32_t*)(*outPtr);
	return kAlgoErrorNone;
}

AlgoError algoAllocPoolFree(AlgoAllocPool allocPool, void *p)
{
	uint8_t *elem = (uint8_t*)p;
	if (elem == NULL)
		return kAlgoErrorNone;
	if (NULL == allocPool ||
		elem <  allocPool->pool ||
		elem >= allocPool->pool + (allocPool->elementCount * allocPool->elementSize) ||
		(elem - allocPool->pool) % allocPool->elementSize != 0)
	{
		return kAlgoErrorInvalidArgument;
	}
	*(int32_t*)elem = allocPool->headIndex;
	allocPool->headIndex = (elem - allocPool->pool) / allocPool->elementSize;
	return kAlgoErrorNone;
}

AlgoError algoAllocPoolElementSize(AlgoAllocPool allocPool, int32_t *outElementSize)
{
	if (NULL == allocPool ||
		NULL == outElementSize)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outElementSize = allocPool->elementSize;
	return kAlgoErrorNone;
}


/****************************************
 * AlgoStack
 ****************************************/

typedef struct AlgoStackImpl
{
	AlgoData *nodes;
	int32_t capacity; /* Outside view of how many elements can be stored in the stack. Size of the nodes[] array. */
	int32_t top; /* index of next empty element in the stack. top=0 -> empty stack. top=capacity -> full */
} AlgoStackImpl;

static int iStackIsEmpty(const AlgoStack stack)
{
	ALGO_ASSERT(NULL != stack);
	return (stack->top == 0);
}
static int iStackIsFull(const AlgoStack stack)
{
	ALGO_ASSERT(NULL != stack);
	return stack->top == stack->capacity;
}

AlgoError algoStackBufferSize(size_t *outBufferSize, int32_t stackCapacity)
{
	if (NULL == outBufferSize ||
		stackCapacity < 1)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outBufferSize = sizeof(AlgoStackImpl) + stackCapacity * sizeof(AlgoData);
	return kAlgoErrorNone;
}

AlgoError algoStackCreate(AlgoStack *outStack, int32_t stackCapacity, void *buffer, size_t bufferSize)
{
	size_t minBufferSize = 0;
	AlgoError err;
	uint8_t *bufferNext = (uint8_t*)buffer;
	if (NULL == outStack ||
		stackCapacity < 1)
	{
		return kAlgoErrorInvalidArgument;
	}
	err = algoStackBufferSize(&minBufferSize, stackCapacity);
	if (err != kAlgoErrorNone)
	{
		return err;
	}
	if (NULL == buffer ||
		bufferSize < minBufferSize)
	{
		return kAlgoErrorInvalidArgument;
	}

	*outStack = (AlgoStackImpl*)bufferNext;
	bufferNext += sizeof(AlgoStackImpl);

	(*outStack)->capacity = stackCapacity;
	(*outStack)->nodes = (AlgoData*)bufferNext;
	bufferNext += (*outStack)->capacity * sizeof(AlgoData);
	(*outStack)->top = 0;
	ALGO_ASSERT( bufferNext-minBufferSize == buffer ); /* If this fails, algoStackBufferSize() is out of date. */
	return kAlgoErrorNone;
}

AlgoError algoStackPush(AlgoStack stack, const AlgoData elem)
{
	if (NULL == stack)
	{
		return kAlgoErrorInvalidArgument;
	}
	if (iStackIsFull(stack))
	{
		return kAlgoErrorOperationFailed;
	}
	stack->nodes[(stack->top)++] = elem;
	return kAlgoErrorNone;
}
AlgoError algoStackPop(AlgoStack stack, AlgoData *outElem)
{
	if (NULL == stack ||
		NULL == outElem)
	{
		return kAlgoErrorInvalidArgument;
	}
	if (iStackIsEmpty(stack))
	{
		return kAlgoErrorOperationFailed;
	}
	*outElem = stack->nodes[--(stack->top)];
	return kAlgoErrorNone;
}

AlgoError algoStackCapacity(const AlgoStack stack, int32_t *outCapacity)
{
	if (NULL == stack ||
		NULL == outCapacity)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outCapacity = stack->capacity;
	return kAlgoErrorNone;
}

AlgoError algoStackCurrentSize(const AlgoStack stack, int32_t *outSize)
{
	if (NULL == stack ||
		NULL == outSize)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outSize = stack->top;
	return kAlgoErrorNone;
}

/*****************************************
 * AlgoQueue
 *****************************************/

typedef struct AlgoQueueImpl
{
	AlgoData *nodes;
	int32_t nodeCount; /* Actual length of the nodes[] array. */
	int32_t capacity; /* Outside view of how many elements can be stored in the queue. */
	int32_t head; /* index of the next element to remove (if the queue isn't empty) */
	int32_t tail; /* index of the first empty element past the end of the queue. */
} AlgoQueueImpl;

/* We never let the nodes array fill up completely.
   if head == tail, that means the queue is empty.
   if head = (tail+1) % nodeCount, the queue is full. */

static int iQueueIsEmpty(const AlgoQueue queue)
{
	ALGO_ASSERT(NULL != queue);
	return (queue->head == queue->tail);
}
static int iQueueIsFull(const AlgoQueue queue)
{
	ALGO_ASSERT(NULL != queue);
	return queue->head == (queue->tail+1) % queue->nodeCount;
}

AlgoError algoQueueBufferSize(size_t *outSize, int32_t queueCapacity)
{
	if (NULL == outSize ||
		queueCapacity < 1)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outSize = sizeof(AlgoQueueImpl) + (queueCapacity+1) * sizeof(AlgoData);
	return kAlgoErrorNone;
}

AlgoError algoQueueCreate(AlgoQueue *outQueue, int32_t queueCapacity, void *buffer, size_t bufferSize)
{
	size_t minBufferSize = 0;
	AlgoError err;
	uint8_t *bufferNext = (uint8_t*)buffer;
	if (NULL == outQueue ||
		queueCapacity < 1)
	{
		return kAlgoErrorInvalidArgument;
	}
	err = algoQueueBufferSize(&minBufferSize, queueCapacity);
	if (err != kAlgoErrorNone)
	{
		return err;
	}
	if (NULL == buffer ||
		bufferSize < minBufferSize)
	{
		return kAlgoErrorInvalidArgument;
	}
	
	*outQueue = (AlgoQueueImpl*)bufferNext;
	bufferNext += sizeof(AlgoQueueImpl);

	(*outQueue)->capacity = queueCapacity;
	(*outQueue)->nodeCount = queueCapacity+1; /* tail is always an empty node. */
	(*outQueue)->nodes = (AlgoData*)bufferNext;
	bufferNext += (*outQueue)->nodeCount * sizeof(AlgoData);
	(*outQueue)->head = 0;
	(*outQueue)->tail = 0;
	ALGO_ASSERT( bufferNext-minBufferSize == buffer ); /* If this fails, algoQueueBufferSize() is out of date. */
	return kAlgoErrorNone;
}

AlgoError algoQueueInsert(AlgoQueue queue, const AlgoData elem)
{
	if (NULL == queue)
	{
		return kAlgoErrorInvalidArgument;
	}
	if (iQueueIsFull(queue))
	{
		return kAlgoErrorOperationFailed;
	}
	queue->nodes[queue->tail] = elem;
	queue->tail = (queue->tail + 1) % queue->nodeCount;
	return kAlgoErrorNone;
}
AlgoError algoQueueRemove(AlgoQueue queue, AlgoData *outElem)
{
	if (NULL == queue ||
		NULL == outElem)
	{
		return kAlgoErrorInvalidArgument;
	}
	if (iQueueIsEmpty(queue))
	{
		return kAlgoErrorOperationFailed;
	}
	*outElem = queue->nodes[queue->head];
	queue->head = (queue->head + 1) % queue->nodeCount;
	return kAlgoErrorNone;
}

AlgoError algoQueueCapacity(const AlgoQueue queue, int32_t *outCapacity)
{
	if (NULL == queue ||
		NULL == outCapacity)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outCapacity = queue->capacity;
	return kAlgoErrorNone;
}

AlgoError algoQueueCurrentSize(const AlgoQueue queue, int32_t *outSize)
{
	if (NULL == queue ||
		NULL == outSize)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outSize = (queue->tail + queue->nodeCount - queue->head) % queue->nodeCount;
	return kAlgoErrorNone;
}

/********************************************
 * AlgoHeap
 ********************************************/

typedef struct AlgoHeapNode
{
	AlgoData key;
	AlgoData data;
} AlgoHeapNode;

typedef struct AlgoHeapImpl
{
	AlgoHeapNode *nodes;
	AlgoDataCompareFunc keyCompare;
	int32_t capacity;
	int32_t nextEmpty; /* 1-based; N's kids = 2*N and 2*N+1; N's parent = N/2 */
} AlgoHeapImpl;

/* Internal utilities */

static const int32_t kAlgoHeapRootIndex = 1;

static int32_t iHeapCurrentSize(AlgoHeap heap)
{
	ALGO_ASSERT(NULL != heap);
	return heap->nextEmpty - kAlgoHeapRootIndex;
}

static int iHeapIsNodeValid(AlgoHeap heap, const int32_t nodeIndex)
{
	ALGO_ASSERT(NULL != heap);
	return
		nodeIndex >= kAlgoHeapRootIndex &&
		nodeIndex < heap->nextEmpty &&
		nodeIndex < heap->capacity + kAlgoHeapRootIndex;
}

static int32_t iHeapParentIndex(const int32_t childIndex)
{
	return childIndex/2;
}
static int32_t iHeapLeftChildIndex(const int32_t parentIndex)
{
	return parentIndex*2;
}
static int32_t iHeapRightChildIndex(const int32_t parentIndex)
{
	return parentIndex*2 + 1;
}

static void iHeapSwapNodes(AlgoHeap heap,
	const int32_t index1, const int32_t index2)
{
	AlgoHeapNode tempNode;
	ALGO_ASSERT(NULL != heap);
	ALGO_ASSERT(iHeapIsNodeValid(heap, index1));
	ALGO_ASSERT(iHeapIsNodeValid(heap, index2));
	tempNode = heap->nodes[index1];
	heap->nodes[index1] = heap->nodes[index2];
	heap->nodes[index2] = tempNode;
}

/* public API functions */

AlgoError algoHeapBufferSize(size_t *outSize, int32_t heapCapacity)
{
	if (NULL == outSize)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outSize = sizeof(AlgoHeapImpl) + (heapCapacity+kAlgoHeapRootIndex) * sizeof(AlgoHeapNode);
	return kAlgoErrorNone;
}

AlgoError algoHeapCreate(AlgoHeap *outHeap, int32_t heapCapacity, AlgoDataCompareFunc keyCompare,
	void *buffer, size_t bufferSize)
{
	size_t minBufferSize = 0;
	AlgoError err;
	uint8_t *bufferNext = (uint8_t*)buffer;
	if (NULL == outHeap ||
		NULL == keyCompare)
	{
		return kAlgoErrorInvalidArgument;
	}
	err = algoHeapBufferSize(&minBufferSize, heapCapacity);
	if (err != kAlgoErrorNone)
	{
		return err;
	}
	if (NULL == buffer ||
		bufferSize < minBufferSize)
	{
		return kAlgoErrorInvalidArgument;
	}

	*outHeap = (AlgoHeapImpl*)bufferNext;
	bufferNext += sizeof(AlgoHeapImpl);
	(*outHeap)->nodes = (AlgoHeapNode*)bufferNext;
	bufferNext += (heapCapacity+kAlgoHeapRootIndex) * sizeof(AlgoHeapNode);
	(*outHeap)->keyCompare = keyCompare;
	(*outHeap)->capacity = heapCapacity;
	(*outHeap)->nextEmpty = kAlgoHeapRootIndex;
	ALGO_ASSERT( bufferNext-minBufferSize == buffer ); /* If this fails, algoHeapBufferSize() is out of date. */
	return kAlgoErrorNone;
}

AlgoError algoHeapCurrentSize(AlgoHeap heap, int32_t *outSize)
{
	if (NULL == heap ||
		NULL == outSize)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outSize = iHeapCurrentSize(heap);
	return kAlgoErrorNone;
}

AlgoError algoHeapCapacity(AlgoHeap heap, int32_t *outCapacity)
{
	if (NULL == heap ||
		NULL == outCapacity)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outCapacity = heap->capacity;
	return kAlgoErrorNone;
}

AlgoError algoHeapInsert(AlgoHeap heap, const AlgoData key, const AlgoData data)
{
	int32_t childIndex;
	if (NULL == heap)
	{
		return kAlgoErrorInvalidArgument;
	}
	if (iHeapCurrentSize(heap) >= heap->capacity)
	{
		return kAlgoErrorOperationFailed; /* Can't insert if it's full! */
	}
	/* Insert new node at the end. */
	childIndex = heap->nextEmpty;
	heap->nextEmpty += 1;
	heap->nodes[childIndex].key  = key;
	heap->nodes[childIndex].data = data;
	/* Bubble up. */
	while(childIndex > kAlgoHeapRootIndex)
	{
		int32_t parentIndex = iHeapParentIndex(childIndex);
		if (heap->keyCompare(heap->nodes[parentIndex].key, heap->nodes[childIndex].key) <= 0)
		{
			break;
		}
		iHeapSwapNodes(heap, parentIndex, childIndex);
		childIndex = parentIndex;
	}
	return kAlgoErrorNone;
}

AlgoError algoHeapPeek(AlgoHeap heap, AlgoData *outTopKey, AlgoData *outTopData)
{
	if (NULL == heap)
	{
		return kAlgoErrorInvalidArgument;
	}
	if (0 == iHeapCurrentSize(heap))
	{
		return kAlgoErrorOperationFailed; /* Can't peek an empty heap. */
	}

	if (NULL != outTopKey)
	{
		*outTopKey = heap->nodes[kAlgoHeapRootIndex].key;
	}
	if (NULL != outTopData)
	{
		*outTopData = heap->nodes[kAlgoHeapRootIndex].data;
	}
	return kAlgoErrorNone;
}

AlgoError algoHeapPop(AlgoHeap heap, AlgoData *outTopKey, AlgoData *outTopData)
{
	int32_t lastIndex, parentIndex, leftChildIndex;
	if (NULL == heap)
	{
		return kAlgoErrorInvalidArgument;
	}
	if (0 == iHeapCurrentSize(heap))
	{
		return kAlgoErrorOperationFailed; /* Can't pop an empty heap. */
	}
	/* Store top element in output arguments */
	if (NULL != outTopKey)
	{
		*outTopKey = heap->nodes[kAlgoHeapRootIndex].key;
	}
	if (NULL != outTopData)
	{
		*outTopData = heap->nodes[kAlgoHeapRootIndex].data;
	}
	/* Overwrite top element. */
	lastIndex = heap->nextEmpty-1;
	heap->nodes[kAlgoHeapRootIndex] = heap->nodes[lastIndex];
	/* Bubble down. */
	parentIndex = kAlgoHeapRootIndex;
	leftChildIndex = iHeapLeftChildIndex(parentIndex);
	while(leftChildIndex < heap->nextEmpty)
	{
		int32_t minKeyIndex = parentIndex;
		int32_t rightChildIndex = iHeapRightChildIndex(parentIndex);
		if (heap->keyCompare(heap->nodes[leftChildIndex].key, heap->nodes[minKeyIndex].key) < 0)
		{
			minKeyIndex = leftChildIndex;
		}
		if (rightChildIndex < heap->nextEmpty &&
			heap->keyCompare(heap->nodes[rightChildIndex].key, heap->nodes[minKeyIndex].key) < 0)
		{
			minKeyIndex = rightChildIndex;
		}
		if (parentIndex == minKeyIndex)
		{
			break; /* early out */
		}
		iHeapSwapNodes(heap, parentIndex, minKeyIndex);
		parentIndex = minKeyIndex;
		leftChildIndex = 2*parentIndex;
	}
	heap->nextEmpty -= 1;
	return kAlgoErrorNone;
}

AlgoError algoHeapCheck(AlgoHeap heap)
{
	int32_t iNode;
	/* Basic tests */
	if (NULL == heap ||
		NULL == heap->nodes)
	{
		return kAlgoErrorInvalidArgument; /* AlgoHeap pointer(s) are NULL. */
	}
	if (heap->nextEmpty < kAlgoHeapRootIndex ||
		heap->capacity < 0 ||
		iHeapCurrentSize(heap) > heap->capacity)
	{
		return kAlgoErrorInvalidArgument; /* AlgoHeap size/capacity are invalid. */
	}
	if (iHeapCurrentSize(heap) == 0)
	{
		return kAlgoErrorNone; /* Empty heaps are valid. */
	}
	/* This is mainly here to prevent warnings about an unused function. */
	(void)iHeapIsNodeValid(heap, kAlgoHeapRootIndex);

	/* Recursively test all nodes to verify the heap condition holds. */
	for(iNode=kAlgoHeapRootIndex+1; iNode<heap->nextEmpty; ++iNode)
	{
		int32_t parentIndex = iHeapParentIndex(iNode);
		ALGO_ASSERT(iHeapIsNodeValid(heap, parentIndex));
		if (heap->keyCompare(heap->nodes[iNode].key, heap->nodes[parentIndex].key) < 0)
		{
			return kAlgoErrorInvalidArgument;
		}
	}
	return kAlgoErrorNone;
}

/************************************************
 * AlgoGraph
 ************************************************/

typedef struct AlgoGraphEdge
{
	int32_t destVertex; /* Adjacency info */
	int32_t weight;
	struct AlgoGraphEdge *next;
} AlgoGraphEdge;

typedef struct AlgoGraphImpl
{
	int32_t *degree; /* degree (outgoing edge count) per vertex. Unused/invalid vertices have a degree of -1. */
	AlgoData *vertexData; /* Arbitrary per-vertex data. One element per vertex. Unused elements used as a free list of unused vertices. */
	AlgoGraphEdge **edges; /* One edge list per vertex. */
	AlgoAllocPool edgePool; /* pool from which edges are allocated. */
	int32_t vertexCapacity;
	int32_t edgeCapacity;
	int32_t currentVertexCount; /* 0..vertexCapacity */
	int32_t currentEdgeCount; /* 0..edgeCapacity */
	int32_t nextFreeVertexId; /* Used to manage the unused vertex pool in vertexData[] */
	AlgoGraphEdgeMode edgeMode;
} AlgoGraphImpl;

AlgoError algoGraphBufferSize(size_t *outBufferSize, int32_t vertexCapacity, int32_t edgeCapacity,
	const AlgoGraphEdgeMode edgeMode)
{
	AlgoError err;
	const size_t degreeSize     = vertexCapacity * sizeof(int32_t);
	const size_t vertexDataSize = vertexCapacity * sizeof(AlgoData);
	const size_t edgesSize      = vertexCapacity * sizeof(AlgoGraphEdge*); /* one edge list per vertex */
	const size_t nodesPerEdge = (edgeMode == kAlgoGraphEdgeDirected) ? 1 : 2; /* undirected edges store two nodes: x->y and y->x */
	size_t edgePoolSize = 0;
	if (NULL == outBufferSize ||
		vertexCapacity < 0 ||
		edgeCapacity < 0)
	{
		return kAlgoErrorInvalidArgument;
	}
	err = algoAllocPoolBufferSize(&edgePoolSize, sizeof(AlgoGraphEdge), edgeCapacity*nodesPerEdge);
	if (err != kAlgoErrorNone)
	{
		return err;
	}
	*outBufferSize = sizeof(AlgoGraphImpl) + degreeSize + vertexDataSize + edgesSize + edgePoolSize;
	return kAlgoErrorNone;
}

AlgoError algoGraphCreate(AlgoGraph *outGraph, int32_t vertexCapacity, int32_t edgeCapacity,
	const AlgoGraphEdgeMode edgeMode, void *buffer, size_t bufferSize)
{
	size_t minBufferSize = 0;
	AlgoError err;
	if (NULL == outGraph ||
		vertexCapacity <= 0 ||
		edgeCapacity <= 0)
	{
		return kAlgoErrorInvalidArgument;
	}
	err = algoGraphBufferSize(&minBufferSize, vertexCapacity, edgeCapacity, edgeMode);
	if (err != kAlgoErrorNone)
	{
		return err;
	}
	if (NULL == buffer ||
		bufferSize < minBufferSize)
	{
		return kAlgoErrorInvalidArgument;
	}

	uint8_t *bufferNext = (uint8_t*)buffer;
	*outGraph = (AlgoGraphImpl*)bufferNext;
	bufferNext += sizeof(AlgoGraphImpl);


	const size_t degreeSize = vertexCapacity * sizeof(int32_t);
	(*outGraph)->degree = (int32_t*)bufferNext;
	bufferNext += degreeSize;

	const size_t vertexDataSize = vertexCapacity * sizeof(AlgoData);
	(*outGraph)->vertexData = (AlgoData*)bufferNext;
	{
		int32_t iVert;
		for(iVert=0; iVert<vertexCapacity-1; iVert += 1)
		{
			(*outGraph)->vertexData[iVert].asInt = iVert+1;
		}
		(*outGraph)->vertexData[vertexCapacity-1].asInt = -1;
	}
	bufferNext += vertexDataSize;


	const size_t edgesSize = vertexCapacity*sizeof(AlgoGraphEdge*); /* one linked list per vertex */
	(*outGraph)->edges = (AlgoGraphEdge**)bufferNext;
	bufferNext += edgesSize;

	const size_t nodesPerEdge = (edgeMode == kAlgoGraphEdgeDirected) ? 1 : 2; /* undirected edges store two nodes: x->y and y->x */
	size_t edgePoolSize = 0;
	err = algoAllocPoolBufferSize(&edgePoolSize, sizeof(AlgoGraphEdge), edgeCapacity*nodesPerEdge);
	if (err != kAlgoErrorNone)
	{
		return err;
	}
	err = algoAllocPoolCreate(&((*outGraph)->edgePool), sizeof(AlgoGraphEdge), edgeCapacity*nodesPerEdge, bufferNext, edgePoolSize);
	bufferNext += edgePoolSize;


	assert( bufferNext-minBufferSize == buffer ); /* If this fails, algoGraphBufferSize() is out of date. */

	ALGO_MEMSET( (*outGraph)->edges, 0, edgesSize ); /* initialize all vertex edge lists to empty */
	ALGO_MEMSET( (*outGraph)->degree, 0xFF, degreeSize ); /* initialize all vertex degrees to -1 (patently invalid */

	(*outGraph)->vertexCapacity = vertexCapacity;
	(*outGraph)->edgeCapacity   =   edgeCapacity;
	(*outGraph)->currentVertexCount = 0;
	(*outGraph)->currentEdgeCount   = 0;
	(*outGraph)->edgeMode    = edgeMode;
	(*outGraph)->nextFreeVertexId = 0;

	return kAlgoErrorNone;
}

AlgoError algoGraphCurrentVertexCount(const AlgoGraph graph, int32_t *outCount)
{
	if (NULL == graph ||
		NULL == outCount)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outCount = graph->currentVertexCount;
	return kAlgoErrorNone;
}
AlgoError algoGraphVertexCapacity(const AlgoGraph graph, int32_t *outCapacity)
{
	if (NULL == graph ||
		NULL == outCapacity)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outCapacity = graph->vertexCapacity;
	return kAlgoErrorNone;
}
AlgoError algoGraphCurrentEdgeCount(const AlgoGraph graph, int32_t *outCount)
{
	if (NULL == graph ||
		NULL == outCount)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outCount = graph->currentEdgeCount;
	return kAlgoErrorNone;
}
AlgoError algoGraphEdgeCapacity(const AlgoGraph graph, int32_t *outCapacity)
{
	if (NULL == graph ||
		NULL == outCapacity)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outCapacity = graph->edgeCapacity;
	return kAlgoErrorNone;
}


AlgoError algoGraphGetVertexDegree(const AlgoGraph graph, int32_t vertexId, int32_t *outDegree)
{
	if (NULL == graph ||
		NULL == outDegree ||
		vertexId < 0 ||
		vertexId >= graph->vertexCapacity ||
		graph->degree[vertexId] == -1)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outDegree = graph->degree[vertexId];
	assert( *outDegree >= 0); /* testing against upper bound is more complicated... */
	return kAlgoErrorNone;
}
AlgoError algoGraphGetVertexEdges(const AlgoGraph graph, int32_t srcVertexId, int32_t vertexDegree, int32_t outDestVertexIds[])
{
	const AlgoGraphEdge *nextEdge;
	int32_t edgeCount, iEdge;
	if (NULL == graph ||
		NULL == outDestVertexIds ||
		srcVertexId < 0 ||
		srcVertexId >= graph->vertexCapacity ||
		vertexDegree != graph->degree[srcVertexId])
	{
		return kAlgoErrorInvalidArgument;
	}
	nextEdge = graph->edges[srcVertexId];
	edgeCount = graph->degree[srcVertexId];
	for(iEdge=0; iEdge<edgeCount; iEdge += 1)
	{
		assert(NULL != nextEdge); /* edge list is shorter than expected! */
		outDestVertexIds[iEdge] = nextEdge->destVertex;
		nextEdge = nextEdge->next;
	}
	assert(NULL == nextEdge); /* edge list is longer than expected! */
	return kAlgoErrorNone;
}
AlgoError algoGraphGetVertexData(const AlgoGraph graph, int32_t vertexId, AlgoData *outData)
{
	if (NULL == graph ||
		NULL == outData ||
		vertexId < 0 ||
		vertexId >= graph->vertexCapacity ||
		graph->degree[vertexId] == -1)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outData = graph->vertexData[vertexId];
	return kAlgoErrorNone;
}
AlgoError algoGraphAddVertex(AlgoGraph graph, AlgoData vertexData, int32_t *outVertexId)
{
	int32_t newVertexId;
	if (NULL == graph)
	{
		return kAlgoErrorInvalidArgument;
	}
	if (graph->currentVertexCount >= graph->vertexCapacity)
	{
		return kAlgoErrorOperationFailed;
	}
	newVertexId = graph->nextFreeVertexId;
	assert(newVertexId >= 0 && newVertexId < graph->vertexCapacity); /* <0 means the vertex pool is empty */
	graph->nextFreeVertexId = graph->vertexData[newVertexId].asInt;
	if (NULL != outVertexId)
	{
		*outVertexId = newVertexId;
	}
	graph->degree[newVertexId] = 0;
	graph->edges[newVertexId] = 0;
	graph->vertexData[newVertexId] = vertexData;
	graph->currentVertexCount += 1;
	return kAlgoErrorNone;
}
AlgoError algoGraphRemoveVertex(AlgoGraph graph, int32_t vertexId)
{
	(void)graph;
	(void)vertexId;
	return kAlgoErrorOperationFailed; /* Currently unsupported */
}
AlgoError algoGraphAddEdge(AlgoGraph graph, int32_t srcVertexId, int32_t destVertexId)
{
	if (NULL == graph ||
		srcVertexId < 0  || srcVertexId  >= graph->currentVertexCount || graph->degree[srcVertexId]  == -1 ||
		destVertexId < 0 || destVertexId >= graph->currentVertexCount || graph->degree[destVertexId] == -1 ||
		srcVertexId == destVertexId /* no self-connecting edges */
		)
	{
		return kAlgoErrorInvalidArgument;
	}
	{
		AlgoGraphEdge *newEdge = NULL;
		AlgoError err = algoAllocPoolAlloc(graph->edgePool, &newEdge);
		if (err != kAlgoErrorNone)
		{
			return kAlgoErrorOperationFailed; /* exceeded edge capacity */
		}
		newEdge->weight = 0;
		newEdge->destVertex = destVertexId;
		newEdge->next = graph->edges[srcVertexId];
		graph->edges[srcVertexId] = newEdge;
		graph->degree[srcVertexId] += 1;
	}

	if (graph->edgeMode == kAlgoGraphEdgeUndirected)
	{
		/* Add a second edge in the opposite direction */
		AlgoGraphEdge *newEdge = NULL;
		AlgoError err = algoAllocPoolAlloc(graph->edgePool, &newEdge);
		if (err != kAlgoErrorNone)
		{
			/* TODO: free previous edge? or just assert that this can't happen? */
			return kAlgoErrorOperationFailed; /* exceeded edge capacity */
		}
		newEdge->weight = 0;
		newEdge->destVertex = srcVertexId;
		newEdge->next = graph->edges[destVertexId];
		graph->edges[destVertexId] = newEdge;
		graph->degree[destVertexId] += 1;
	}

	/* As implemented, this counts "logical" edges, not actual AlgoGraphEdge objects. An undirected
		edge will allocate two AlgoGraphEdge objects, but only increment this value once. */
	graph->currentEdgeCount += 1;

	return kAlgoErrorNone;
}
AlgoError algoGraphRemoveEdge(AlgoGraph graph, int32_t srcVertexId, int32_t destVertexId)
{
	(void)graph;
	(void)srcVertexId;
	(void)destVertexId;
	return kAlgoErrorOperationFailed; /* Currently unsupported */
}

static ALGO_INLINE void iSetBit(int32_t *bits, size_t capacity, int32_t index)
{
	assert(index >= 0 && (size_t)index < capacity);
	(void)capacity;
	bits[index/32] |= 1<<(index%32);
}
static ALGO_INLINE void iClearBit(int32_t *bits, size_t capacity, int32_t index)
{
	assert(index >= 0 && (size_t)index < capacity);
	(void)capacity;
	bits[index/32] &= ~(1<<(index%32));
}
static ALGO_INLINE void iFlipBit(int32_t *bits, size_t capacity, int32_t index)
{
	assert(index >= 0 && (size_t)index < capacity);
	(void)capacity;
	bits[index/32] ^= 1<<(index%32);
}
static ALGO_INLINE int32_t iTestBit(const int32_t *bits, size_t capacity, int32_t index)
{
	assert(index >= 0 && (size_t)index < capacity);
	(void)capacity;
	return ( bits[index/32] & (1<<(index%32)) ) ? 1 : 0;
}

AlgoError algoGraphBfsBufferSize(size_t *outBufferSize, const AlgoGraph graph)
{
	if (NULL == outBufferSize ||
		NULL == graph)
	{
		return kAlgoErrorInvalidArgument;
	}
	int32_t vertexCapacityRounded = (graph->vertexCapacity+31) & ~31;
	size_t discoveredSize         = vertexCapacityRounded / 32;
	size_t processedSize          = vertexCapacityRounded / 32;
	size_t queueSize              = 0;
	AlgoError err;
	err = algoQueueBufferSize(&queueSize, graph->vertexCapacity);
	if (kAlgoErrorNone != err)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outBufferSize = discoveredSize + processedSize + queueSize;
	return kAlgoErrorNone;
}

AlgoError algoGraphBfs(const AlgoGraph graph, int32_t rootVertexId, int32_t outVertexParents[], size_t vertexParentCount, 
	AlgoGraphProcessVertexFunc vertexFuncEarly, AlgoGraphProcessEdgeFunc edgeFunc, AlgoGraphProcessVertexFunc vertexFuncLate,
	void *buffer, size_t bufferSize)
{
	size_t minBufferSize = 0;
	AlgoError err;
	uint8_t *bufferNext = (uint8_t*)buffer;
	if (NULL == graph ||
		(NULL != outVertexParents && vertexParentCount < (size_t)graph->vertexCapacity) ||
		rootVertexId < 0 ||
		rootVertexId >= graph->vertexCapacity ||
		graph->degree[rootVertexId] == -1)
	{
		return kAlgoErrorInvalidArgument;
	}
	err = algoGraphBfsBufferSize(&minBufferSize, graph);
	if (NULL == buffer ||
		bufferSize < minBufferSize ||
		kAlgoErrorNone != err)
	{
		return kAlgoErrorInvalidArgument;
	}

	int32_t vertexCapacityRounded = (graph->vertexCapacity+31) & ~31;
	size_t discoveredSize         = vertexCapacityRounded / 32;
	size_t processedSize          = vertexCapacityRounded / 32;
	size_t queueSize              = 0;

	int32_t *discovered = (int32_t*)bufferNext;
	bufferNext += discoveredSize;

	int32_t *processed = (int32_t*)bufferNext;
	bufferNext += processedSize;

	err = algoQueueBufferSize(&queueSize, graph->vertexCapacity);
	if (kAlgoErrorNone != err)
	{
		return kAlgoErrorInvalidArgument;
	}
	AlgoQueue vertexQueue;
	err = algoQueueCreate(&vertexQueue, graph->vertexCapacity, bufferNext, queueSize);
	if (kAlgoErrorNone != err)
	{
		return kAlgoErrorInvalidArgument;
	}
	bufferNext += queueSize;

	assert( bufferNext-minBufferSize == buffer ); /* If this fails, algoGraphBfsBufferSize() is out of date */

	ALGO_MEMSET(discovered, 0, discoveredSize);
	ALGO_MEMSET(processed, 0, processedSize);
	if (NULL != outVertexParents)
	{
		ALGO_MEMSET(outVertexParents, 0xFF, vertexParentCount*sizeof(int32_t));
	}
	err = algoQueueInsert(vertexQueue, algoDataFromInt(rootVertexId));
	iSetBit(discovered, vertexCapacityRounded, rootVertexId);
	int32_t currentQueueSize = 1;
	do
	{
		AlgoData queueElem;
		int32_t v0 = -1;
		int32_t edgeCount = 0;
		/* Pop the next vertex and process it. */
		const AlgoGraphEdge *e = NULL;
		err = algoQueueRemove(vertexQueue, &queueElem);
		v0 = queueElem.asInt;
		assert(v0 >= 0 && v0 < graph->vertexCapacity && graph->degree[v0] != -1);
		if (NULL != vertexFuncEarly)
			vertexFuncEarly(graph, v0);
		iSetBit(processed, vertexCapacityRounded, v0); /* must be set here to prevent undirected edges from looping infinitely. */
		/* Explore v0's edges. */
		e = graph->edges[v0];
		while(NULL != e)
		{
			edgeCount += 1;
			assert(edgeCount <= graph->degree[v0]);
			int32_t v1 = e->destVertex;
			assert(v0 >= 0 && v0 < graph->vertexCapacity && graph->degree[v0] != -1);
			/* Run the edge function, if this is the first time we've seen it. */
			if (0 == iTestBit(processed, vertexCapacityRounded, v1) ||
				graph->edgeMode == kAlgoGraphEdgeDirected)
			{
				if (NULL != edgeFunc)
					edgeFunc(graph, v0, v1);
			}
			/* Enqueue v1, if we haven't seen it before. */
			if (0 == iTestBit(discovered, vertexCapacityRounded, v1))
			{
				iSetBit(discovered, vertexCapacityRounded, v1);
				err = algoQueueInsert(vertexQueue, algoDataFromInt(v1));
				if (NULL != outVertexParents)
				{
					outVertexParents[v1] = v0;
				}
			}
			e = e->next;
		}
		assert(edgeCount == graph->degree[v0]);
		/* Run the late vertex function after all edges are processed. */
		if (NULL != vertexFuncLate)
			vertexFuncLate(graph, v0);
		err = algoQueueCurrentSize(vertexQueue, &currentQueueSize);
	}
	while (currentQueueSize > 0);
	return kAlgoErrorNone;
}

#endif /* ALGO_IMPLEMENTATION */
