/* algo.h
 * Basic data structures & algorithms in C.
 * http://github.com/cdwfs/algo
 * no warranty implied; use at your own risk
 *
 * Do this:
 *    #define ALGO_IMPLEMENTATION
 * before including this file in *one* C/C++ file to create the implementation.
 *
 * To define all functions as static (to allow multiple inclusions without
 * link errors, do this:
 *    #define ALGO_STATIC
 *
 * GENERAL API GUIDE
 * -----------------
 * Error checking:
 * All functions return an AlgoError code, which will be kAlgoErrorNone (0) on success, or a non-zero value
 * on failure. Users are urged to define a macro function to check the return value of all algo.h function calls
 * and handle/report any errors in an application-specific fashion. See https://gist.github.com/cdwfs/070c6496fedf04b4873b
 * for an example of a suitable error-checking function. For clarity, Error checking will be omitted in the
 * remainder of this guide.
 *
 * Initializing an AlgoFoo object:
 * The idiom to create an object of type AlgoFoo is:
 *		size_t fooBufferSize = 0;
 *		algoFooComputeBufferSize(&fooBufferSize, [foo_args]);
 *		void *fooBuffer = malloc(fooBufferSize);
 *		AlgoFoo foo;
 *		algoFooCreate(&foo, [foo_args], fooBuffer, fooBufferSize);
 * To reset an existing AlgoFoo object to its default/empty state, just call algoFooCreate() again with the
 * same arguments.
 *
 * Deleting an AlgoFoo object:
 * Objects will never make any additional allocations, or use any memory beyond the buffer provided when
 * they were initialized. Therefore, deleting an object is as simple as releasing its memory buffer:
 *		free(fooBuffer);
 *
 * Copying or Relocating an AlgoFoo object:
 * The AlgoFoo object itself is just a typedef of a pointer to an opaque AlgoFooImpl structure, which is
 * always stored at the beginning of the object's memory buffer. The object may contain internal pointers
 * to other regions of the buffer; these pointers will need to be patched if the object's location changes.
 * This patching is handled by the algoFooRelocate() object, which patches all internal pointers based on
 * the object's current base pointer. The idiom to copy an existing AlgoFoo object to a new buffer is:
 *		size_t fooBuffer2Size = 0;
 *		void *fooBuffer2 = malloc(fooBuffer2Size);
 *		AlgoFoo fooCopy = fooBuffer2;
 *		memcpy(fooCopy, foo, fooBufferSize);
 *		algoFooRelocate(fooCopy, fooBuffer2Size);
 *
 * Serializing an AlgoFoo object:
 * Serialization is just a special case of copying. Any AlgoFoo object can be serialized by storing the contents
 * of its memory buffer, exactly as-is. To deserialize, load the contents into an appropriately-sized buffer,
 * assign an AlgoFoo object to the buffer's base address, and call algoFooRelocate() to update the internal
 * pointers accordingly.
 *
 * Resizing an AlgoFoo object:
 * AlgoFoo objects can be resized in-place, if their buffer contains sufficient space. The exact procecure is
 * different for growing and shrinking an object.
 *
 * Increasing the capacity of an AlgoFoo object usually requires more buffer space. The object's contents must
 * first be copied into the larger buffer, and the object reassigned to the new buffer's base address. See
 * the section on "Copying or Relocating an AlgoFoo object" for details. Once the object has been successfully
 * relocated, it can be resized in-place with algoFooResize(). The full idiom is:
 *		size_t fooBiggerBufferSize = 0;
 *		algoFooComputeBufferSize(&fooBiggerBufferSize, [foo_larger_args]);
 *		void *fooBiggerBuffer = malloc(fooBiggerBufferSize);
 *		AlgoFoo fooBigger = fooBiggerBuffer;
 *		memcpy(fooBigger, foo, fooBufferSize);
 *		free(foo);
 *		algoFooRelocate(fooBigger, fooBiggerBufferSize);
 *		algoFooResize(fooBigger, [foo_larger_args], fooBiggerBufferSize);
 *		foo = fooBigger;
 * Reducing the capacity of an AlgoFoo object involves resizing the object in-place, and then optionally copying
 * the object into a new (smaller) buffer and fixing up its internal pointers.
 *		size_t fooSmallerBufferSize = 0;
 *		algoFooComputeBufferSize(&fooSmallerBufferSize, [foo_smaller_args]);
 *		void *fooSmallerBuffer = malloc(fooSmallerBufferSize);
 *		AlgoFoo fooSmaller = fooSmallerBufer;
 *		algoFooResize(foo, [foo_smaller_args], fooSmallerBufferSize);
 *		memcpy(fooSmaller, foo, fooSmallerBufferSize);
 *		free(foo);
 *		algoFooRelocate(fooSmaller, fooSmallerBufferSize);
 *		foo = fooSmaller;
 * The TL;DR version:
 *		-	When growing an object, relocate before resizing
 *		-	When shrinking an object, resize before relocating.
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
#		define ALGO_INLINE inline
#	else
#		define ALGO_INLINE
#	endif
#else
#	define ALGO_INLINE __forceinline
#endif

/* Redefine to replace standard library functions with custom implementations */
#ifndef ALGO_MEMSET
#	define ALGO_MEMSET memset
#endif
#ifndef ALGO_MEMCPY
#	define ALGO_MEMCPY memcpy
#endif
#ifndef ALGO_ASSERT
#	define ALGO_ASSERT assert
#endif
#ifndef ALGO_MEMMOVE
#	define ALGO_MEMMOVE memmove
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
	int32_t asInt; /**< The data value as a signed 32-bit integer. */
	float asFloat; /**< The data value as a 32-bit single-precision IEEE float. */
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
ALGODEF AlgoError algoAllocPoolComputeBufferSize(size_t *outBufferSize, const int32_t elementSize, const int32_t elementCount);
/** @brief Initializes a pool allocator object.
	@param outAllocPool Pointer to the pool allocator to initialize.
	@param elementSize Size of each element in the pool. Must be at least 4 bytes.
	@param elementCount Number of elements in the pool. Must be greater than zero.
	@param buffer Memory buffer to use for this object. Use algoAllocPoolComputeBufferSize() to compute the appropriate buffer size.
	@param bufferSize Size of the "buffer" parameter, in bytes. Use Use algoAllocPoolComputeBufferSize() to compute the appropriate buffer size.
	*/
ALGODEF AlgoError algoAllocPoolCreate(AlgoAllocPool *outAllocPool, const int32_t elementSize, const int32_t elementCount,
	void *buffer, const size_t bufferSize);
/** @brief Retrieves the size of the buffer passed when an AlgoAllocPool was created. */
ALGODEF AlgoError algoAllocPoolGetBufferSize(const AlgoAllocPool allocPool, size_t *outBufferSize);
/** @brief Allocates one element from the pool, and returns a pointer to it. */
ALGODEF AlgoError algoAllocPoolAlloc(AlgoAllocPool allocPool, void **outPtr);
/** @brief Frees an element previously allocated by algoAllocPoolAlloc(). */
ALGODEF AlgoError algoAllocPoolFree(AlgoAllocPool allocPool, void *p);
/** @brief Queries the element size of a pool allocator. */
ALGODEF AlgoError algoAllocPoolGetElementSize(const AlgoAllocPool allocPool, int32_t *outElementSize);

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
 * err = algoStackComputeBufferSize(&stackBufferSize, stackCapacity);
 * stackBuffer = malloc(stackBufferSize);
 * err = algoStackCreate(&stack, stackCapacity, stackBuffer, stackBufferSize);
 * err = algoStackPush(stack, algoDataFromInt(5)); // Push the number "5" to the stack
 * err = algoStackPop(stack, &poppedData); // Pop it off again into poppedData
 * free(stackBuffer); // No need to destroy the stack object itself; just free its buffer.
 * @endcode
 */
typedef struct AlgoStackImpl *AlgoStack;
/** @brief Computes the required buffer size for a stack with the specified capacity. */
ALGODEF AlgoError algoStackComputeBufferSize(size_t *outBufferSize, int32_t stackCapacity);
/** @brief Initializes a stack object using the provided buffer. */
ALGODEF AlgoError algoStackCreate(AlgoStack *outStack, int32_t stackCapacity, void *buffer, size_t bufferSize);
/** @brief Retrieves the size of the buffer passed when an AlgoStack was created. */
ALGODEF AlgoError algoStackGetBufferSize(const AlgoStack stack, size_t *outBufferSize);
/** @brief Move a stack to a new location in memory, preserving its capacity and contents. */
ALGODEF AlgoError algoStackRelocate(AlgoStack *outNewStack, const AlgoStack oldStack, void *newBuffer, size_t newBufferSize);
/** @brief Change a stack's capacity in-place. */
ALGODEF AlgoError algoStackResize(AlgoStack stack, int32_t newStackCapacity, size_t bufferSize);
/** @brief Pushes an element to the stack. */
ALGODEF AlgoError algoStackPush(AlgoStack stack, const AlgoData elem);
/** @brief Pops an element from the stack. */
ALGODEF AlgoError algoStackPop(AlgoStack stack, AlgoData *outElem);
/** @brief Retrieves the maximum number of elements that can be stored concurrently in the stack. */
ALGODEF AlgoError algoStackGetCapacity(const AlgoStack stack, int32_t *outCapacity);
/** @brief Retrieves the number of elements currently stored in the stack. */
ALGODEF AlgoError algoStackGetCurrentSize(const AlgoStack stack, int32_t *outSize);

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
 * err = algoQueueComputeBufferSize(&queueBufferSize, queueCapacity);
 * queueBuffer = malloc(queueBufferSize);
 * err = algoQueueCreate(&queue, queueCapacity, queueBuffer, queueBufferSize);
 * err = algoQueueInsert(queue, algoDataFromInt(5)); // insert the number "5" into the queue.
 * err = algoQueueRemove(queue, &removedData); // Remove the head element from the queue and store it in removedData.
 * free(queueBuffer); // No need to destroy the queue object itself; just free its buffer.
 * @endcode
 */
typedef struct AlgoQueueImpl *AlgoQueue;
/** @brief Computes the required buffer size for a queue with the specified capacity. */
ALGODEF AlgoError algoQueueComputeBufferSize(size_t *outBufferSize, int32_t queueCapacity);
/** @brief Initializes a queue object using the provided buffer. */
ALGODEF AlgoError algoQueueCreate(AlgoQueue *outQueue, int32_t queueCapacity, void *buffer, size_t bufferSize);
/** @brief Retrieves the size of the buffer passed when an AlgoQueue was created. */
ALGODEF AlgoError algoQueueGetBufferSize(const AlgoQueue queue, size_t *outBufferSize);
/** @brief Inserts an element into the queue. */
ALGODEF AlgoError algoQueueInsert(AlgoQueue queue, const AlgoData elem);
/** @brief Removes an element from the queue. */
ALGODEF AlgoError algoQueueRemove(AlgoQueue queue, AlgoData *outElem);
/** @brief Retrieves the maximum number of elements that can be stored concurrently in the queue. */
ALGODEF AlgoError algoQueueGetCapacity(const AlgoQueue queue, int32_t *outCapacity);
/** @brief Retrieves the number of elements currently stored in the queue. */
ALGODEF AlgoError algoQueueGetCurrentSize(const AlgoQueue queue, int32_t *outSize);


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
 * err = algoHeapComputeBufferSize(&heapBufferSize, heapCapacity);
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
ALGODEF AlgoError algoHeapComputeBufferSize(size_t *outBufferSize, int32_t heapCapacity);
/** @brief Initializes a heap object using the provided buffer. */
ALGODEF AlgoError algoHeapCreate(AlgoHeap *heap, int32_t heapCapacity, AlgoDataCompareFunc keyCompare,
	void *buffer, size_t bufferSize);
/** @brief Retrieves the size of the buffer passed when an AlgoHeap was created. */
ALGODEF AlgoError algoHeapGetBufferSize(const AlgoHeap heap, size_t *outBufferSize);
/** @brief Inserts an element into the heap, with the specified key. */
ALGODEF AlgoError algoHeapInsert(AlgoHeap heap, const AlgoData key, const AlgoData data);
/** @brief Inspects the "top" element, but does not remove it from the heap. */
ALGODEF AlgoError algoHeapPeek(const AlgoHeap heap, AlgoData *outTopKey, AlgoData *outTopData);
/** @brief Removes the "top" element from the heap. */
ALGODEF AlgoError algoHeapPop(AlgoHeap heap, AlgoData *outTopKey, AlgoData *outTopData);
/** @brief Debugging function to validate heap consistency. */
ALGODEF AlgoError algoHeapValidate(const AlgoHeap heap);
/** @brief Retrieves the maximum number of elements that can be stored concurrently in the heap. */
ALGODEF AlgoError algoHeapGetCapacity(const AlgoHeap heap, int32_t *outCapacity);
/** @brief Retrieves the number of elements currently stored in the heap. */
ALGODEF AlgoError algoHeapGetCurrentSize(const AlgoHeap heap, int32_t *outSize);

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
ALGODEF AlgoError algoGraphComputeBufferSize(size_t *outBufferSize, int32_t vertexCapacity, int32_t edgeCapacity,
	const AlgoGraphEdgeMode edgeMode);
/** @brief Initializes a graph object using the provided buffer. */
ALGODEF AlgoError algoGraphCreate(AlgoGraph *outGraph, int32_t vertexCapacity, int32_t edgeCapacity,
	const AlgoGraphEdgeMode edgeMode, void *buffer, size_t bufferSize);
/** @brief Retrieves the size of the buffer passed when an AlgoGraph was created. */
ALGODEF AlgoError algoGraphGetBufferSize(const AlgoGraph graph, size_t *outBufferSize);
/** @brief Debugging function to validate graph consistency. */
ALGODEF AlgoError algoGraphValidate(const AlgoGraph graph);
/** @brief Retrieves the current number of vertices in a graph. */
ALGODEF AlgoError algoGraphGetCurrentVertexCount(const AlgoGraph graph, int32_t *outCount);
/** @brief Retrieves the maximum number of vertices that can be stored in a graph. */
ALGODEF AlgoError algoGraphGetVertexCapacity(const AlgoGraph graph, int32_t *outCapacity);
/** @brief Retrieves the current number of edges in a graph. */
ALGODEF AlgoError algoGraphGetCurrentEdgeCount(const AlgoGraph graph, int32_t *outCount);
/** @brief Retrieves the maximum number of edges that can be stored in a graph. */
ALGODEF AlgoError algoGraphEdgeCapacity(const AlgoGraph graph, int32_t *outCapacity);
/** @brief Add a new vertex to a graph. Each vertex may optionally contain a piece of arbitrary user data. */
ALGODEF AlgoError algoGraphAddVertex(AlgoGraph graph, AlgoData vertexData, int32_t *outVertexId);
/** @brief Remove an existing vertex from a graph. This will implicitly remove any edges connecting this
           vertex to the rest of the graph.
	@note  For undirected graphs, this operations runs in O(1) time (expected), approaching O(Ecurrent) in pathological cases.
	       For directed graphs, this operation runs in O(Vcurrent+Ecurrent) time. */
ALGODEF AlgoError algoGraphRemoveVertex(AlgoGraph graph, int32_t vertexId);
/** @brief Add a new edge to a graph, connecting srcVertexId to destVertexId.
           If the graph's edge mode is kAlgoGraphEdgeUndirected, a second edge will automatically be added
		   from destVertexId to srcVertexId. */
ALGODEF AlgoError algoGraphAddEdge(AlgoGraph graph, int32_t srcVertexId, int32_t destVertexId);
/** @brief Remove an existing vertex from a graph.
           If the graph's edge mode is kAlgoGraphEdgeUndirected, the edge from destVertexId to srcVertexId
		   will also be removed.
	@note  This operation runs in O(1) time (expected), approaching O(Ecurrent) in pathological cases. */
ALGODEF AlgoError algoGraphRemoveEdge(AlgoGraph graph, int32_t srcVertexId, int32_t destVertexId);
/** @brief Retrieve the degree (outgoing edge count) of a vertex in the graph. */
ALGODEF AlgoError algoGraphGetVertexDegree(const AlgoGraph graph, int32_t vertexId, int32_t *outDegree);
/** @brief Retrieve the vertices to which a given vertex is connected. */
ALGODEF AlgoError algoGraphGetVertexEdges(const AlgoGraph graph, int32_t srcVertexId, int32_t vertexDegree, int32_t outDestVertexIds[]);
/** @brief Retrieve a vertex's optional user data field. */
ALGODEF AlgoError algoGraphGetVertexData(const AlgoGraph graph, int32_t vertexId, AlgoData *outValue);
/** @brief Retrieve a vertex's optional user data field. */
ALGODEF AlgoError algoGraphSetVertexData(AlgoGraph graph, int32_t vertexId, AlgoData value);

typedef struct AlgoGraphBfsStateImpl *AlgoGraphBfsState;
/** @brief Compute the required buffer size to perform a breadth-first search on a graph.
           This only includes the space required for temporary storage during the search, not the search results themselves. */
ALGODEF AlgoError algoGraphBfsStateComputeBufferSize(size_t *outBufferSize, const AlgoGraph graph);
ALGODEF AlgoError algoGraphBfsStateCreate(AlgoGraphBfsState *outState, const AlgoGraph graph, void *buffer, size_t bufferSize);
/** @brief Retrieves the size of the buffer passed when an AlgoGraphBfsState was created. */
ALGODEF AlgoError algoGraphBfsStateGetBufferSize(const AlgoGraphBfsState bfsState, size_t *outBufferSize);
ALGODEF AlgoError algoGraphBfsStateIsVertexDiscovered(const AlgoGraphBfsState bfsState, int32_t vertexId, int *outIsDiscovered);
ALGODEF AlgoError algoGraphBfsStateIsVertexProcessed(const AlgoGraphBfsState bfsState, int32_t vertexId, int *outIsProcessed);
ALGODEF AlgoError algoGraphBfsStateGetVertexParent(const AlgoGraphBfsState bfsState, int32_t vertexId, int32_t *outParentVertexId);

typedef void (*AlgoGraphBfsProcessVertexFunc)(AlgoGraph graph, AlgoGraphBfsState bfsState, int32_t vertexId, void *userData);
typedef void (*AlgoGraphBfsProcessEdgeFunc)(AlgoGraph graph, AlgoGraphBfsState bfsState, int32_t startVertexId, int32_t endVertexId,
	void *userData);
typedef struct AlgoGraphBfsCallbacks
{
	AlgoGraphBfsProcessVertexFunc vertexFuncEarly; /** If non-NULL, this function will be called on each vertex when it is first encountered during the traversal. */
	void *vertexFuncEarlyUserData; /** Passed as userData to vertexFuncEarly. */
	AlgoGraphBfsProcessEdgeFunc edgeFunc; /** If non-NULL, this function will be called on each edge when it is first encountered during the traversal. For undirected
	                                          graphs, the function will only be called once per pair of connected vertices. */
	void *edgeFuncUserData; /** Passed as userData to edgeFunc. */
	AlgoGraphBfsProcessVertexFunc vertexFuncLate; /**  If non-NULL, this function will be called on each vertex after all of its edges have been explored. */
	void *vertexFuncLateUserData; /** Passed as userData to vertexFuncLate. */
} AlgoGraphBfsCallbacks;
/** @brief Perform a breadth-first search on a graph.
	@param graph The graph to search.
	@param bfsState A container for all intermediate states (and results) of the search operation. Created with algoGraphBfsStateCreate().
	@param rootVertexId starting from the specified root vertex.
	@param callbacks Callback functions to invoke during various stages of the BFS.
	*/
ALGODEF AlgoError algoGraphBfs(const AlgoGraph graph, AlgoGraphBfsState bfsState, int32_t rootVertexId, AlgoGraphBfsCallbacks callbacks);

typedef struct AlgoGraphDfsStateImpl *AlgoGraphDfsState;
/** @brief Compute the required buffer size to perform a breadth-first search on a graph.
           This only includes the space required for temporary storage during the search, not the search results themselves. */
ALGODEF AlgoError algoGraphDfsStateComputeBufferSize(size_t *outBufferSize, const AlgoGraph graph);
ALGODEF AlgoError algoGraphDfsStateCreate(AlgoGraphDfsState *outState, const AlgoGraph graph, void *buffer, size_t bufferSize);
/** @brief Retrieves the size of the buffer passed when an AlgoGraphDfsState was created. */
ALGODEF AlgoError algoGraphDfsStateGetBufferSize(const AlgoGraphDfsState dfsState, size_t *outBufferSize);
ALGODEF AlgoError algoGraphDfsStateIsVertexDiscovered(const AlgoGraphDfsState dfsState, int32_t vertexId, int *outIsDiscovered);
ALGODEF AlgoError algoGraphDfsStateIsVertexProcessed(const AlgoGraphDfsState dfsState, int32_t vertexId, int *outIsProcessed);
ALGODEF AlgoError algoGraphDfsStateGetVertexParent(const AlgoGraphDfsState dfsState, int32_t vertexId, int32_t *outParentVertexId);
ALGODEF AlgoError algoGraphDfsStateGetVertexEntryTime(const AlgoGraphDfsState dfsState, int32_t vertexId, int32_t *outEntryTime);
ALGODEF AlgoError algoGraphDfsStateGetVertexExitTime(const AlgoGraphDfsState dfsState, int32_t vertexId, int32_t *outExitTime);

typedef void (*AlgoGraphDfsProcessVertexFunc)(AlgoGraph graph, AlgoGraphDfsState dfsState, int32_t vertexId, void *userData);
typedef void (*AlgoGraphDfsProcessEdgeFunc)(AlgoGraph graph, AlgoGraphDfsState dfsState, int32_t startVertexId, int32_t endVertexId,
	void *userData);
typedef struct AlgoGraphDfsCallbacks
{
	AlgoGraphDfsProcessVertexFunc vertexFuncEarly; /** If non-NULL, this function will be called on each vertex when it is first encountered during the traversal. */
	void *vertexFuncEarlyUserData; /** Passed as userData to vertexFuncEarly. */
	AlgoGraphDfsProcessEdgeFunc edgeFunc; /** If non-NULL, this function will be called on each edge when it is first encountered during the traversal. For undirected
	                                          graphs, the function will only be called once per pair of connected vertices. */
	void *edgeFuncUserData; /** Passed as userData to edgeFunc. */
	AlgoGraphDfsProcessVertexFunc vertexFuncLate; /**  If non-NULL, this function will be called on each vertex after all of its edges have been explored. */
	void *vertexFuncLateUserData; /** Passed as userData to vertexFuncLate. */
} AlgoGraphDfsCallbacks;
/** @brief Perform a depth-first search on a graph.
	@param graph The graph to search.
	@param dfsState A container for all intermediate states (and results) of the search operation. Created with algoGraphDfsStateCreate().
	@param rootVertexId starting from the specified root vertex.
	@param callbacks Callback functions to invoke during various stages of the DFS.
	*/
ALGODEF AlgoError algoGraphDfs(const AlgoGraph graph, AlgoGraphDfsState dfsState, int32_t rootVertexId, AlgoGraphDfsCallbacks callbacks);

/** @brief Compute the required buffer size to perform a topological sort on a graph. 
           This only includes the space required for temporary storage during the sort, not the sort results themselves. */
ALGODEF AlgoError algoGraphTopoSortComputeBufferSize(size_t *outBufferSize, const AlgoGraph graph);
/** @brief Perform a topological sort on a graph.
	@param graph The graph to sort. It must be a directed acyclic graph (DAG) -- that is, its edgeMode must be kGraphEdgeModeDirected,
	             and it must not contain any cycles.
	@param outSortedVertices A list of sorted vertex IDs will be written to this array.
	@param sortedVertexCount The outSortedVertices[] array must contain at least this many elements. This value should match the
	                         graph's current vertex count.
	@param buffer Used for temporary storage during the sort.
	@param bufferSize Size of the buffer[] array, in bytes. Given by algoGraphTopoSortComputeBufferSize().
	*/
ALGODEF AlgoError algoGraphTopoSort(const AlgoGraph graph, int32_t outSortedVertices[], size_t sortedVertexCount, 
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

#define ALGO_UNUSED(x) (void)(x)
#define ALGO_INTERNAL static

#define ALGO_MIN(x,y) ((x)<(y) ? (x) : (y))
#define ALGO_MAX(x,y) ((x)>(y) ? (x) : (y))

/******************************************
 * AlgoAllocPool
 ******************************************/

typedef struct AlgoAllocPoolImpl
{
	const void *thisBuffer;
	size_t thisBufferSize;
	int32_t elementSize; /* must be >= 4 */
	int32_t elementCount; /* must be > 0 */
	int32_t headIndex; /* if -1, pool is empty */
	uint8_t *pool;
} AlgoAllocPoolImpl;

AlgoError algoAllocPoolComputeBufferSize(size_t *outBufferSize, const int32_t elementSize, const int32_t elementCount)
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
	err = algoAllocPoolComputeBufferSize(&minBufferSize, elementSize, elementCount);
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

	ALGO_ASSERT( bufferNext-minBufferSize == buffer ); /* If this fails, algoAllocPoolComputeBufferSize() is out of date */

	(*outAllocPool)->thisBuffer = buffer;
	(*outAllocPool)->thisBufferSize = bufferSize;
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
		ALGO_UNUSED(end);
	}
	return kAlgoErrorNone;
}

AlgoError algoAllocPoolGetBufferSize(const AlgoAllocPool allocPool, size_t *outBufferSize)
{
	if (NULL == allocPool ||
		NULL == outBufferSize)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outBufferSize = allocPool->thisBufferSize;
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
	allocPool->headIndex = (int32_t)( (elem - allocPool->pool) / allocPool->elementSize );
	return kAlgoErrorNone;
}

AlgoError algoAllocPoolGetElementSize(const AlgoAllocPool allocPool, int32_t *outElementSize)
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
	const void *thisBuffer;
	size_t thisBufferSize;
	int32_t capacity; /* Outside view of how many elements can be stored in the stack. Size of the nodes[] array. */
	int32_t top; /* index of next empty element in the stack. top=0 -> empty stack. top=capacity -> full */
	AlgoData *nodes;
} AlgoStackImpl;

ALGO_INTERNAL int iStackIsEmpty(const AlgoStack stack)
{
	ALGO_ASSERT(NULL != stack);
	return (stack->top == 0);
}
ALGO_INTERNAL int iStackIsFull(const AlgoStack stack)
{
	ALGO_ASSERT(NULL != stack);
	return stack->top == stack->capacity;
}

AlgoError algoStackComputeBufferSize(size_t *outBufferSize, int32_t stackCapacity)
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
	err = algoStackComputeBufferSize(&minBufferSize, stackCapacity);
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

	void *nodes = bufferNext;
	bufferNext += stackCapacity * sizeof(AlgoData);

	ALGO_ASSERT( bufferNext-minBufferSize == buffer ); /* If this fails, algoStackComputeBufferSize() is out of date. */
	(*outStack)->thisBuffer = buffer;
	(*outStack)->thisBufferSize = bufferSize;
	(*outStack)->capacity = stackCapacity;
	(*outStack)->top = 0;
	(*outStack)->nodes = nodes;

	return kAlgoErrorNone;
}

AlgoError algoStackGetBufferSize(const AlgoStack stack, size_t *outBufferSize)
{
	if (NULL == stack ||
		NULL == outBufferSize)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outBufferSize = stack->thisBufferSize;
	return kAlgoErrorNone;
}

AlgoError algoStackRelocate(AlgoStack *outNewStack, const AlgoStack oldStack, void *newBuffer, size_t newBufferSize)
{
	if (NULL == outNewStack ||
		NULL == oldStack ||
		NULL == newBuffer)
	{
		return kAlgoErrorInvalidArgument;
	}
	size_t minBufferSize = 0;
	AlgoError err;
	err = algoStackComputeBufferSize(&minBufferSize, oldStack->capacity);
	if (err != kAlgoErrorNone)
	{
		return err;
	}
	if (newBufferSize < minBufferSize)
	{
		return kAlgoErrorInvalidArgument;
	}

	if (newBuffer == oldStack->thisBuffer)
	{
		(*outNewStack) = oldStack;
		(*outNewStack)->thisBufferSize = newBufferSize;
		return kAlgoErrorNone;
	}

	const void *oldBuffer = oldStack->thisBuffer;
	const void *oldBufferEnd = (void*)( (intptr_t)oldBuffer + oldStack->thisBufferSize );
	const void *newBufferEnd = (void*)( (intptr_t)newBuffer + newBufferSize );
	if (oldBuffer <= newBufferEnd && newBuffer <= oldBufferEnd)
	{
		ALGO_MEMMOVE(newBuffer, oldBuffer, minBufferSize);
	}
	else
	{
		ALGO_MEMCPY(newBuffer, oldBuffer, minBufferSize);
	}
	intptr_t pointerOffset = (intptr_t)newBuffer - (intptr_t)oldBuffer;
	(*outNewStack) = newBuffer;
	(*outNewStack)->thisBuffer = newBuffer;
	(*outNewStack)->thisBufferSize = newBufferSize;
	(*outNewStack)->nodes = (AlgoData*)( (intptr_t)(*outNewStack)->nodes + pointerOffset );
	return kAlgoErrorNone;
}

AlgoError algoStackResize(AlgoStack stack, int32_t newStackCapacity, size_t bufferSize)
{
	if (NULL == stack ||
		newStackCapacity < 1)
	{
		return kAlgoErrorInvalidArgument;
	}
	size_t minBufferSize = 0;
	AlgoError err;
	err = algoStackComputeBufferSize(&minBufferSize, newStackCapacity);
	if (err != kAlgoErrorNone)
	{
		return err;
	}
	if (bufferSize < minBufferSize)
	{
		return kAlgoErrorInvalidArgument;
	}

	int32_t currentElemCount = 0;
	err = algoStackGetCurrentSize(stack, &currentElemCount);
	if (err != kAlgoErrorNone)
	{
		return err;
	}
	if (currentElemCount > newStackCapacity)
	{
		return kAlgoErrorOperationFailed;
	}

	stack->capacity = newStackCapacity;
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

AlgoError algoStackGetCapacity(const AlgoStack stack, int32_t *outCapacity)
{
	if (NULL == stack ||
		NULL == outCapacity)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outCapacity = stack->capacity;
	return kAlgoErrorNone;
}

AlgoError algoStackGetCurrentSize(const AlgoStack stack, int32_t *outSize)
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
	const void *thisBuffer;
	size_t thisBufferSize;
	int32_t nodeCount; /* Actual length of the nodes[] array. */
	int32_t capacity; /* Outside view of how many elements can be stored in the queue. */
	int32_t head; /* index of the next element to remove (if the queue isn't empty) */
	int32_t tail; /* index of the first empty element past the end of the queue. */
	AlgoData *nodes;
} AlgoQueueImpl;

/* We never let the nodes array fill up completely.
   if head == tail, that means the queue is empty.
   if head = (tail+1) % nodeCount, the queue is full. */

ALGO_INTERNAL int iQueueIsEmpty(const AlgoQueue queue)
{
	ALGO_ASSERT(NULL != queue);
	return (queue->head == queue->tail);
}
ALGO_INTERNAL int iQueueIsFull(const AlgoQueue queue)
{
	ALGO_ASSERT(NULL != queue);
	return queue->head == (queue->tail+1) % queue->nodeCount;
}

AlgoError algoQueueComputeBufferSize(size_t *outSize, int32_t queueCapacity)
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
	err = algoQueueComputeBufferSize(&minBufferSize, queueCapacity);
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

	(*outQueue)->thisBuffer = buffer;
	(*outQueue)->thisBufferSize = bufferSize;
	(*outQueue)->capacity = queueCapacity;
	(*outQueue)->nodeCount = queueCapacity+1; /* tail is always an empty node. */
	(*outQueue)->nodes = (AlgoData*)bufferNext;
	bufferNext += (*outQueue)->nodeCount * sizeof(AlgoData);
	(*outQueue)->head = 0;
	(*outQueue)->tail = 0;
	ALGO_ASSERT( bufferNext-minBufferSize == buffer ); /* If this fails, algoQueueComputeBufferSize() is out of date. */
	return kAlgoErrorNone;
}

AlgoError algoQueueGetBufferSize(const AlgoQueue queue, size_t *outBufferSize)
{
	if (NULL == queue ||
		NULL == outBufferSize)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outBufferSize = queue->thisBufferSize;
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

AlgoError algoQueueGetCapacity(const AlgoQueue queue, int32_t *outCapacity)
{
	if (NULL == queue ||
		NULL == outCapacity)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outCapacity = queue->capacity;
	return kAlgoErrorNone;
}

AlgoError algoQueueGetCurrentSize(const AlgoQueue queue, int32_t *outSize)
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
	const void *thisBuffer;
	size_t thisBufferSize;
	AlgoDataCompareFunc keyCompare;
	int32_t capacity;
	int32_t nextEmpty; /* 1-based; N's kids = 2*N and 2*N+1; N's parent = N/2 */
	AlgoHeapNode *nodes;
} AlgoHeapImpl;

/* Internal utilities */

ALGO_INTERNAL const int32_t kAlgoHeapRootIndex = 1;

ALGO_INTERNAL int32_t iHeapCurrentSize(AlgoHeap heap)
{
	ALGO_ASSERT(NULL != heap);
	return heap->nextEmpty - kAlgoHeapRootIndex;
}

ALGO_INTERNAL int iHeapIsNodeValid(AlgoHeap heap, const int32_t nodeIndex)
{
	ALGO_ASSERT(NULL != heap);
	return
		nodeIndex >= kAlgoHeapRootIndex &&
		nodeIndex < heap->nextEmpty &&
		nodeIndex < heap->capacity + kAlgoHeapRootIndex;
}

ALGO_INTERNAL int32_t iHeapParentIndex(const int32_t childIndex)
{
	return childIndex/2;
}
ALGO_INTERNAL int32_t iHeapLeftChildIndex(const int32_t parentIndex)
{
	return parentIndex*2;
}
ALGO_INTERNAL int32_t iHeapRightChildIndex(const int32_t parentIndex)
{
	return parentIndex*2 + 1;
}

ALGO_INTERNAL void iHeapSwapNodes(AlgoHeap heap,
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

AlgoError algoHeapComputeBufferSize(size_t *outSize, int32_t heapCapacity)
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
	err = algoHeapComputeBufferSize(&minBufferSize, heapCapacity);
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
	(*outHeap)->thisBuffer = buffer;
	(*outHeap)->thisBufferSize = bufferSize;
	(*outHeap)->nodes = (AlgoHeapNode*)bufferNext;
	bufferNext += (heapCapacity+kAlgoHeapRootIndex) * sizeof(AlgoHeapNode);
	(*outHeap)->keyCompare = keyCompare;
	(*outHeap)->capacity = heapCapacity;
	(*outHeap)->nextEmpty = kAlgoHeapRootIndex;
	ALGO_ASSERT( bufferNext-minBufferSize == buffer ); /* If this fails, algoHeapComputeBufferSize() is out of date. */
	return kAlgoErrorNone;
}

AlgoError algoHeapGetBufferSize(const AlgoHeap heap, size_t *outBufferSize)
{
	if (NULL == heap ||
		NULL == outBufferSize)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outBufferSize = heap->thisBufferSize;
	return kAlgoErrorNone;
}

AlgoError algoHeapGetCurrentSize(const AlgoHeap heap, int32_t *outSize)
{
	if (NULL == heap ||
		NULL == outSize)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outSize = iHeapCurrentSize(heap);
	return kAlgoErrorNone;
}

AlgoError algoHeapGetCapacity(const AlgoHeap heap, int32_t *outCapacity)
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

AlgoError algoHeapPeek(const AlgoHeap heap, AlgoData *outTopKey, AlgoData *outTopData)
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

AlgoError algoHeapValidate(const AlgoHeap heap)
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
	ALGO_UNUSED(iHeapIsNodeValid(heap, kAlgoHeapRootIndex));

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
	const void *thisBuffer;
	size_t thisBufferSize;
	int32_t vertexCapacity;
	int32_t edgeCapacity;
	int32_t currentVertexCount; /* 0..vertexCapacity */
	int32_t currentEdgeCount; /* 0..edgeCapacity */
	int32_t nextFreeVertexId; /* Used to manage the unused vertex pool in vertexData[] */
	AlgoGraphEdgeMode edgeMode;
	int32_t *vertexDegrees; /* degree (outgoing edge count) per vertex. Unused/invalid vertices have a degree of -1. */
	AlgoData *vertexData; /* Arbitrary per-vertex data. One element per vertex. Unused elements used as a free list of unused vertices. */
	int32_t *validVertexIds; /* unsorted list of valid vertex IDs. The first currentVertexCount elements are valid; the rest are undefined. */
	int32_t *vertexIdToValidIndex; /* reverse-lookup from vertexId into validVertexIds[], to keep removal reasonably efficient. */
	AlgoGraphEdge **vertexEdges; /* One edge list per vertex. */
	AlgoAllocPool edgePool; /* pool from which edges are allocated. */
} AlgoGraphImpl;

ALGO_INTERNAL ALGO_INLINE int iGraphIsValidVertexId(const AlgoGraph graph, int32_t vertexId)
{
	ALGO_ASSERT(NULL != graph);
	return (
		vertexId >= 0 &&
		vertexId < graph->vertexCapacity &&
		graph->vertexDegrees[vertexId] >= 0) ? 1 : 0;
}

ALGO_INTERNAL int iGraphRemoveEdgeFromList(AlgoGraph graph, int32_t srcVertexId, int32_t destVertexId)
{
	/*  This function ONLY finds and removes a single edge, and decrements the source vertex's degree.
		The following are the responsibility of the caller, where applicable:
		- decrement the graph's edge count.
		- remove matching dest->src edge for undirected graphs.
		*/
	int found = 0;
	AlgoGraphEdge *edge = graph->vertexEdges[srcVertexId];
	if (edge == NULL)
		return 0; /* srcVertexId has no edges */
	else if (edge->destVertex == destVertexId)
	{
		graph->vertexEdges[srcVertexId] = edge->next;
		algoAllocPoolFree(graph->edgePool, edge);
		found = 1;
	}
	else
	{
		while(NULL != edge->next)
		{
			if (edge->next->destVertex == destVertexId)
			{
				AlgoGraphEdge *toFree = edge->next;
				edge->next = toFree->next;
				algoAllocPoolFree(graph->edgePool, toFree);
				found = 1;
				break;
			}
			edge = edge->next;
		}
	}
	if (!found)
		return 0;
	graph->vertexDegrees[srcVertexId] -= 1;
	return 1;
}

AlgoError algoGraphComputeBufferSize(size_t *outBufferSize, int32_t vertexCapacity, int32_t edgeCapacity,
	const AlgoGraphEdgeMode edgeMode)
{
	AlgoError err;
	const size_t vertexDegreesSize        = vertexCapacity * sizeof(int32_t);
	const size_t vertexDataSize           = vertexCapacity * sizeof(AlgoData);
	const size_t validVertexIdsSize       = vertexCapacity * sizeof(int32_t);
	const size_t vertexIdToValidIndexSize = vertexCapacity * sizeof(int32_t);
	const size_t vertexEdgesSize          = vertexCapacity * sizeof(AlgoGraphEdge*); /* one edge list per vertex */
	const int32_t nodesPerEdge = (edgeMode == kAlgoGraphEdgeDirected) ? 1 : 2; /* undirected edges store two nodes: x->y and y->x */
	size_t edgePoolSize = 0;
	if (NULL == outBufferSize ||
		vertexCapacity < 0 ||
		edgeCapacity < 0)
	{
		return kAlgoErrorInvalidArgument;
	}
	err = algoAllocPoolComputeBufferSize(&edgePoolSize, sizeof(AlgoGraphEdge), edgeCapacity*nodesPerEdge);
	if (err != kAlgoErrorNone)
	{
		return err;
	}
	*outBufferSize = sizeof(AlgoGraphImpl) + vertexDegreesSize + vertexDataSize + validVertexIdsSize
		+ vertexIdToValidIndexSize + vertexEdgesSize + edgePoolSize;
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
	err = algoGraphComputeBufferSize(&minBufferSize, vertexCapacity, edgeCapacity, edgeMode);
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


	const size_t vertexDegreeSize = vertexCapacity * sizeof(int32_t);
	(*outGraph)->vertexDegrees = (int32_t*)bufferNext;
	{
		int32_t iVert;
		for(iVert=0; iVert<vertexCapacity; iVert += 1)
		{
			(*outGraph)->vertexDegrees[iVert] = -1;
		}
	}
	bufferNext += vertexDegreeSize;

	const size_t vertexDataSize = vertexCapacity * sizeof(AlgoData);
	(*outGraph)->vertexData = (AlgoData*)bufferNext;
	{
		int32_t iVert;
		for(iVert=0; iVert<vertexCapacity-1; iVert += 1)
		{
			(*outGraph)->vertexData[iVert].asInt = iVert+1; /* store linked list of available vertices in the data fields */
		}
		(*outGraph)->vertexData[vertexCapacity-1].asInt = -1;
	}
	bufferNext += vertexDataSize;

	const size_t validVertexIdsSize = vertexCapacity * sizeof(int32_t);
	(*outGraph)->validVertexIds = (int32_t*)bufferNext;
	bufferNext += validVertexIdsSize;

	const size_t vertexIdToValidIndexSize = vertexCapacity * sizeof(int32_t);
	(*outGraph)->vertexIdToValidIndex = (int32_t*)bufferNext;
	bufferNext += vertexIdToValidIndexSize;

	const size_t vertexEdgesSize = vertexCapacity*sizeof(AlgoGraphEdge*); /* one linked list per vertex */
	(*outGraph)->vertexEdges = (AlgoGraphEdge**)bufferNext;
	{
		int32_t iVert;
		for(iVert=0; iVert<vertexCapacity; iVert += 1)
		{
			(*outGraph)->vertexEdges[iVert] = NULL;
		}
	}
	bufferNext += vertexEdgesSize;

	const int32_t nodesPerEdge = (edgeMode == kAlgoGraphEdgeDirected) ? 1 : 2; /* undirected edges store two nodes: x->y and y->x */
	size_t edgePoolSize = 0;
	err = algoAllocPoolComputeBufferSize(&edgePoolSize, sizeof(AlgoGraphEdge), edgeCapacity*nodesPerEdge);
	if (err != kAlgoErrorNone)
	{
		return err;
	}
	err = algoAllocPoolCreate(&((*outGraph)->edgePool), sizeof(AlgoGraphEdge), edgeCapacity*nodesPerEdge, bufferNext, edgePoolSize);
	bufferNext += edgePoolSize;


	ALGO_ASSERT( bufferNext-minBufferSize == buffer ); /* If this fails, algoGraphComputeBufferSize() is out of date. */

	(*outGraph)->thisBuffer = buffer;
	(*outGraph)->thisBufferSize = bufferSize;
	(*outGraph)->vertexCapacity = vertexCapacity;
	(*outGraph)->edgeCapacity   =   edgeCapacity;
	(*outGraph)->currentVertexCount = 0;
	(*outGraph)->currentEdgeCount   = 0;
	(*outGraph)->edgeMode    = edgeMode;
	(*outGraph)->nextFreeVertexId = 0;

	return kAlgoErrorNone;
}

AlgoError algoGraphGetBufferSize(const AlgoGraph graph, size_t *outBufferSize)
{
	if (NULL == graph ||
		NULL == outBufferSize)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outBufferSize = graph->thisBufferSize;
	return kAlgoErrorNone;
}

AlgoError algoGraphValidate(const AlgoGraph graph)
{
	int errorCode = 0;
	if (NULL == graph)
	{
		errorCode = 1; /* graph must not be NULL. */
		goto ALGO_GRAPH_VALIDATE_END;
	}
	if (graph->currentEdgeCount < 0 ||
		graph->edgeCapacity > graph->edgeCapacity)
	{
		errorCode = 2; /* edge count exceeds capacity. */
		goto ALGO_GRAPH_VALIDATE_END;
	}
	if (graph->currentVertexCount < 0 ||
		graph->currentVertexCount > graph->vertexCapacity)
	{
		errorCode = 3; /* vertex count exceeds capacity */
		goto ALGO_GRAPH_VALIDATE_END;
	}
	/* Check edge lists */
	{
		int validVertexCount = 0;
		int validEdgeNodeCount = 0;
		int iVertex;
		const int32_t nodesPerEdge = (graph->edgeMode == kAlgoGraphEdgeDirected) ? 1 : 2; /* undirected edges store two nodes: x->y and y->x */
		for(iVertex=0; iVertex<graph->vertexCapacity; iVertex += 1)
		{
			if (iGraphIsValidVertexId(graph, iVertex))
			{
				int edgeListLength = 0;
				const AlgoGraphEdge *edge = graph->vertexEdges[iVertex];
				while(edge)
				{
					if (!iGraphIsValidVertexId(graph, edge->destVertex))
					{
						errorCode = 4; /* edge's destination vertex is invalid. */
						goto ALGO_GRAPH_VALIDATE_END;
					}
					validEdgeNodeCount += 1;
					edgeListLength += 1;
					edge = edge->next;
				}
				if (edgeListLength != graph->vertexDegrees[iVertex])
				{
					errorCode = 5; /* vertex degree doesn't match edge list length. */
					goto ALGO_GRAPH_VALIDATE_END;
				}
				/* TODO: check for multiple edges to the same destination vertex. That's not currently supported. */
				/* TODO: for undirected graphs, make sure every v1->v2 edge has a matching v2->v1 edge. */
				validVertexCount += 1;
			}
			else
			{
				/* anything to check for invalid vertices? */
			}
		}
		if (validEdgeNodeCount != graph->currentEdgeCount*nodesPerEdge)
		{
			errorCode = 6; /* actual valid edge count doesn't match expected value. */
			goto ALGO_GRAPH_VALIDATE_END;
		}
		if (validVertexCount != graph->currentVertexCount)
		{
			errorCode = 7; /* actual valid vertex count doesn't match expected value. */
			goto ALGO_GRAPH_VALIDATE_END;
		}
	}
	/* Check free vertex list */
	{
		int32_t freeVertexCount = 0;
		int32_t nextFreeVertexId = graph->nextFreeVertexId;
		while(nextFreeVertexId >= 0)
		{
			if (iGraphIsValidVertexId(graph, nextFreeVertexId))
			{
				errorCode = 8; /* vertex in the free list thinks it's valid. */
				goto ALGO_GRAPH_VALIDATE_END;
			}
			freeVertexCount += 1;
			nextFreeVertexId = graph->vertexData[nextFreeVertexId].asInt;
		}
		if (graph->vertexCapacity - graph->currentVertexCount != freeVertexCount)
		{
			errorCode = 9; /* actual free vertex count doesn't match expected value. */
			goto ALGO_GRAPH_VALIDATE_END;
		}
	}
	/* Check valid vertex list */
	{
		for(int32_t iValidVert=0; iValidVert<graph->currentVertexCount; ++iValidVert)
		{
			int32_t validVertexId = graph->validVertexIds[iValidVert];
			if (!iGraphIsValidVertexId(graph, validVertexId))
			{
				errorCode = 10; /* vertex in the valid list is invalid */
				goto ALGO_GRAPH_VALIDATE_END;
			}
			if (graph->vertexIdToValidIndex[validVertexId] != iValidVert)
			{
				errorCode = 11; /* reverse lookup table mismatch */
				goto ALGO_GRAPH_VALIDATE_END;
			}
		}
	}

ALGO_GRAPH_VALIDATE_END:
	if (errorCode != 0)
		return kAlgoErrorInvalidArgument;
	return kAlgoErrorNone;
}

AlgoError algoGraphGetCurrentVertexCount(const AlgoGraph graph, int32_t *outCount)
{
	if (NULL == graph ||
		NULL == outCount)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outCount = graph->currentVertexCount;
	return kAlgoErrorNone;
}
AlgoError algoGraphGetVertexCapacity(const AlgoGraph graph, int32_t *outCapacity)
{
	if (NULL == graph ||
		NULL == outCapacity)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outCapacity = graph->vertexCapacity;
	return kAlgoErrorNone;
}
AlgoError algoGraphGetCurrentEdgeCount(const AlgoGraph graph, int32_t *outCount)
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
		0 == iGraphIsValidVertexId(graph, vertexId))
	{
		return kAlgoErrorInvalidArgument;
	}
	*outDegree = graph->vertexDegrees[vertexId];
	ALGO_ASSERT( *outDegree >= 0); /* testing against upper bound is more complicated... */
	return kAlgoErrorNone;
}
AlgoError algoGraphGetVertexEdges(const AlgoGraph graph, int32_t srcVertexId, int32_t vertexDegree, int32_t outDestVertexIds[])
{
	const AlgoGraphEdge *nextEdge;
	int32_t edgeCount, iEdge;
	if (NULL == graph ||
		NULL == outDestVertexIds ||
		0 == iGraphIsValidVertexId(graph, srcVertexId) ||
		graph->vertexDegrees[srcVertexId] != vertexDegree)
	{
		return kAlgoErrorInvalidArgument;
	}
	nextEdge = graph->vertexEdges[srcVertexId];
	edgeCount = graph->vertexDegrees[srcVertexId];
	for(iEdge=0; iEdge<edgeCount; iEdge += 1)
	{
		ALGO_ASSERT(NULL != nextEdge); /* edge list is shorter than expected! */
		outDestVertexIds[iEdge] = nextEdge->destVertex;
		nextEdge = nextEdge->next;
	}
	ALGO_ASSERT(NULL == nextEdge); /* edge list is longer than expected! */
	return kAlgoErrorNone;
}
AlgoError algoGraphGetVertexData(const AlgoGraph graph, int32_t vertexId, AlgoData *outValue)
{
	if (NULL == graph ||
		NULL == outValue ||
		0 == iGraphIsValidVertexId(graph, vertexId))
	{
		return kAlgoErrorInvalidArgument;
	}
	*outValue = graph->vertexData[vertexId];
	return kAlgoErrorNone;
}
AlgoError algoGraphSetVertexData(AlgoGraph graph, int32_t vertexId, AlgoData value)
{
	if (NULL == graph ||
		0 == iGraphIsValidVertexId(graph, vertexId))
	{
		return kAlgoErrorInvalidArgument;
	}
	graph->vertexData[vertexId] = value;
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
	ALGO_ASSERT(newVertexId >= 0 && newVertexId < graph->vertexCapacity); /* <0 means the vertex pool is empty */
	graph->nextFreeVertexId = graph->vertexData[newVertexId].asInt;
	if (NULL != outVertexId)
	{
		*outVertexId = newVertexId;
	}
	graph->vertexDegrees[newVertexId] = 0;
	graph->vertexEdges[newVertexId] = 0;
	graph->vertexData[newVertexId] = vertexData;
	graph->validVertexIds[graph->currentVertexCount] = newVertexId;
	graph->vertexIdToValidIndex[newVertexId] = graph->currentVertexCount;
	graph->currentVertexCount += 1;
	return kAlgoErrorNone;
}
AlgoError algoGraphRemoveVertex(AlgoGraph graph, int32_t vertexId)
{
	if (NULL == graph ||
		0 == iGraphIsValidVertexId(graph, vertexId))
	{
		return kAlgoErrorInvalidArgument;
	}
	if (kAlgoGraphEdgeUndirected == graph->edgeMode)
	{
		AlgoGraphEdge *outEdge = graph->vertexEdges[vertexId];
		while(outEdge != NULL)
		{
			int removed = 0;
			int32_t destVertexId = outEdge->destVertex;
			/* remove incoming edge */
			ALGO_ASSERT(graph->vertexEdges[destVertexId]); /* for undirected graphs, there must at least be an edge going back to vertexId! */
			removed = iGraphRemoveEdgeFromList(graph, destVertexId, vertexId);
			ALGO_ASSERT(removed); /* this removal MUST succeed, or else an invariant has failed somewhere. */
			/* Remove outgoing edge */
			AlgoGraphEdge *outToFree = outEdge;
			outEdge = outEdge->next;
			algoAllocPoolFree(graph->edgePool, outToFree);
			graph->currentEdgeCount -= 1;
		}
	}
	else /* kAlgoGraphEdgeDirected == graph->edgeMode */
	{
		/* Remove all outgoing edges */
		AlgoGraphEdge *outEdge = graph->vertexEdges[vertexId];
		int32_t iValidVert;
		while(outEdge != NULL)
		{
			AlgoGraphEdge *outToFree = outEdge;
			outEdge = outEdge->next;
			algoAllocPoolFree(graph->edgePool, outToFree);
			graph->currentEdgeCount -= 1;
		}
		/* Prevent this vert's edge list from being searched in the loop below */
		graph->vertexDegrees[vertexId] = 0;
		graph->vertexEdges[vertexId] = NULL;
		/*	Search all other vertices for incoming edges, and remove them. This *really* sucks, but to avoid it I'd need:
			-	a per-vertex list of vertices with edges linking to that vertex, which uses O(E) more memory but reduces running time to
				O(1) expected / O(E) worst-case, and adds a lot of extra bookkeeping.
			*/
		for(iValidVert=0; iValidVert<graph->currentVertexCount; iValidVert += 1)
		{
			int32_t srcVertexId = graph->validVertexIds[iValidVert];
			if (!iGraphIsValidVertexId(graph, srcVertexId))
				continue;
			int removed = iGraphRemoveEdgeFromList(graph, srcVertexId, vertexId);
			graph->currentEdgeCount -= (removed ? 1 : 0);
		}
	}
	/* Finally, remove the vertex. */
	graph->vertexData[vertexId].asInt = graph->nextFreeVertexId;
	graph->nextFreeVertexId = vertexId;
	{
		/* Update the valid vertex list and reverse-lookup table */
		int32_t destValidIndex = graph->vertexIdToValidIndex[vertexId];
		int32_t finalValidVertexId = graph->validVertexIds[graph->currentVertexCount-1];
		graph->validVertexIds[destValidIndex] = finalValidVertexId;
		graph->vertexIdToValidIndex[finalValidVertexId] = destValidIndex;
	}
	graph->currentVertexCount -= 1;
	graph->vertexDegrees[vertexId] = -1;
	graph->vertexEdges[vertexId] = NULL;
	return kAlgoErrorNone;
}
AlgoError algoGraphAddEdge(AlgoGraph graph, int32_t srcVertexId, int32_t destVertexId)
{
	if (NULL == graph ||
		0 == iGraphIsValidVertexId(graph, srcVertexId) ||
		0 == iGraphIsValidVertexId(graph, destVertexId) ||
		srcVertexId == destVertexId /* no self-connecting edges */
		)
	{
		return kAlgoErrorInvalidArgument;
	}

	{
		/* check that a src->dest edge doesn't already exist; return immediately if so. */
		for(AlgoGraphEdge *edge = graph->vertexEdges[srcVertexId];
			edge != NULL;
			edge = edge->next)
		{
			if (edge->destVertex == destVertexId)
				return kAlgoErrorNone;
		}

		AlgoGraphEdge *newEdge = NULL;
		AlgoError err = algoAllocPoolAlloc(graph->edgePool, (void**)&newEdge);
		if (err != kAlgoErrorNone)
		{
			return kAlgoErrorOperationFailed; /* exceeded edge capacity */
		}
		newEdge->weight = 0;
		newEdge->destVertex = destVertexId;
		newEdge->next = graph->vertexEdges[srcVertexId];
		graph->vertexEdges[srcVertexId] = newEdge;
		graph->vertexDegrees[srcVertexId] += 1;
	}

	if (graph->edgeMode == kAlgoGraphEdgeUndirected)
	{
		/* check that a dest->src edge doesn't already exist; return immediately if so. */
		for(AlgoGraphEdge *edge = graph->vertexEdges[destVertexId];
			edge != NULL;
			edge = edge->next)
		{
			if (edge->destVertex == srcVertexId)
				return kAlgoErrorNone;
		}

		/* Add a second edge in the opposite direction */
		AlgoGraphEdge *newEdge = NULL;
		AlgoError err = algoAllocPoolAlloc(graph->edgePool, (void**)&newEdge);
		if (err != kAlgoErrorNone)
		{
			/* TODO: free previous edge? or just assert that this can't happen? */
			return kAlgoErrorOperationFailed; /* exceeded edge capacity */
		}
		newEdge->weight = 0;
		newEdge->destVertex = srcVertexId;
		newEdge->next = graph->vertexEdges[destVertexId];
		graph->vertexEdges[destVertexId] = newEdge;
		graph->vertexDegrees[destVertexId] += 1;
	}

	/* As implemented, this counts "logical" edges, not actual AlgoGraphEdge objects. An undirected
		edge will allocate two AlgoGraphEdge objects, but only increment this value once. */
	graph->currentEdgeCount += 1;

	return kAlgoErrorNone;
}
AlgoError algoGraphRemoveEdge(AlgoGraph graph, int32_t srcVertexId, int32_t destVertexId)
{
	if (NULL == graph ||
		0 == iGraphIsValidVertexId(graph, srcVertexId) ||
		0 == iGraphIsValidVertexId(graph, destVertexId))
	{
		return kAlgoErrorInvalidArgument;
	}

	int removed = iGraphRemoveEdgeFromList(graph, srcVertexId, destVertexId);
	if (!removed)
		return kAlgoErrorOperationFailed;
	if (kAlgoGraphEdgeUndirected == graph->edgeMode)
	{
		removed = iGraphRemoveEdgeFromList(graph, destVertexId, srcVertexId);
		if (!removed)
			return kAlgoErrorOperationFailed;
	}
	graph->currentEdgeCount -= 1;
	return kAlgoErrorNone;
}

ALGO_INTERNAL ALGO_INLINE void iSetBit(int32_t *bits, size_t capacity, int32_t index)
{
	ALGO_ASSERT(index >= 0 && (size_t)index < capacity);
	ALGO_UNUSED(capacity);
	bits[index/32] |= 1<<(index%32);
}
ALGO_INTERNAL ALGO_INLINE void iClearBit(int32_t *bits, size_t capacity, int32_t index)
{
	ALGO_ASSERT(index >= 0 && (size_t)index < capacity);
	ALGO_UNUSED(capacity);
	bits[index/32] &= ~(1<<(index%32));
}
ALGO_INTERNAL ALGO_INLINE void iFlipBit(int32_t *bits, size_t capacity, int32_t index)
{
	ALGO_ASSERT(index >= 0 && (size_t)index < capacity);
	ALGO_UNUSED(capacity);
	bits[index/32] ^= 1<<(index%32);
}
ALGO_INTERNAL ALGO_INLINE int32_t iTestBit(const int32_t *bits, size_t capacity, int32_t index)
{
	ALGO_ASSERT(index >= 0 && (size_t)index < capacity);
	ALGO_UNUSED(capacity);
	return ( bits[index/32] & (1<<(index%32)) ) ? 1 : 0;
}

typedef struct AlgoGraphBfsStateImpl
{
	const void *thisBuffer;
	size_t thisBufferSize;
	AlgoGraphImpl *graph;
	int32_t *isVertexDiscovered;
	int32_t *isVertexProcessed;
	int32_t *vertexParents;
	AlgoQueue vertexQueue;
} AlgoGraphBfsStateImpl;
AlgoError algoGraphBfsStateComputeBufferSize(size_t *outBufferSize, const AlgoGraph graph)
{
	if (NULL == outBufferSize ||
		NULL == graph)
	{
		return kAlgoErrorInvalidArgument;
	}
	int32_t vertexCapacityRounded = (graph->vertexCapacity+31) & ~31;
	size_t discoveredSize         = vertexCapacityRounded * sizeof(int32_t)  / 32;
	size_t processedSize          = vertexCapacityRounded * sizeof(int32_t)  / 32;
	size_t parentsSize            = graph->vertexCapacity * sizeof(int32_t);
	size_t queueSize              = 0;
	AlgoError err;
	err = algoQueueComputeBufferSize(&queueSize, graph->vertexCapacity);
	if (kAlgoErrorNone != err)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outBufferSize = sizeof(AlgoGraphBfsStateImpl) + discoveredSize + processedSize + parentsSize + queueSize;
	return kAlgoErrorNone;
}
AlgoError algoGraphBfsStateCreate(AlgoGraphBfsState *outState, const AlgoGraph graph, void *buffer, size_t bufferSize)
{
	if (NULL == outState ||
		NULL == graph ||
		NULL == buffer)
	{
		return kAlgoErrorInvalidArgument;
	}
	size_t minBufferSize = 0;
	AlgoError err;
	uint8_t *bufferNext = (uint8_t*)buffer;
	err = algoGraphBfsStateComputeBufferSize(&minBufferSize, graph);
	if (bufferSize < minBufferSize ||
		kAlgoErrorNone != err)
	{
		return kAlgoErrorInvalidArgument;
	}

	int32_t vertexCapacityRounded = (graph->vertexCapacity+31) & ~31;
	size_t discoveredSize         = vertexCapacityRounded * sizeof(int32_t) / 32;
	size_t processedSize          = vertexCapacityRounded * sizeof(int32_t)  / 32;
	size_t parentsSize            = graph->vertexCapacity * sizeof(int32_t);
	size_t queueSize              = 0;

	(*outState) = (AlgoGraphBfsStateImpl *)bufferNext;
	bufferNext += sizeof(AlgoGraphBfsStateImpl);

	int32_t *discovered = (int32_t*)bufferNext;
	bufferNext += discoveredSize;

	int32_t *processed = (int32_t*)bufferNext;
	bufferNext += processedSize;

	int32_t *parents = (int32_t*)bufferNext;
	bufferNext += parentsSize;

	err = algoQueueComputeBufferSize(&queueSize, graph->vertexCapacity);
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

	ALGO_ASSERT( bufferNext-minBufferSize == buffer ); /* If this fails, algoGraphBfsStateComputeBufferSize() is out of date */
	(*outState)->thisBuffer         = buffer;
(	 *outState)->thisBufferSize     = bufferSize;
	(*outState)->graph              = graph;
	(*outState)->isVertexDiscovered = discovered;
	(*outState)->isVertexProcessed  = processed;
	(*outState)->vertexParents      = parents;
	(*outState)->vertexQueue        = vertexQueue;

	ALGO_MEMSET(discovered, 0, discoveredSize);
	ALGO_MEMSET(processed,  0, processedSize);
	for(int iParent=0; iParent<graph->vertexCapacity; ++iParent)
	{
		parents[iParent] = -1;
	}
	return kAlgoErrorNone;
}

AlgoError algoGraphBfsStateGetBufferSize(const AlgoGraphBfsState bfsState, size_t *outBufferSize)
{
	if (NULL == bfsState ||
		NULL == outBufferSize)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outBufferSize = bfsState->thisBufferSize;
	return kAlgoErrorNone;
}

AlgoError algoGraphBfsStateIsVertexDiscovered(const AlgoGraphBfsState bfsState, int32_t vertexId, int *outIsDiscovered)
{
	if (NULL == bfsState ||
		NULL == outIsDiscovered ||
		vertexId < 0 || vertexId >= bfsState->graph->vertexCapacity)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outIsDiscovered = iTestBit(bfsState->isVertexDiscovered, bfsState->graph->vertexCapacity, vertexId);
	return kAlgoErrorNone;
}
AlgoError algoGraphBfsStateIsVertexProcessed(const AlgoGraphBfsState bfsState, int32_t vertexId, int *outIsProcessed)
{
	if (NULL == bfsState ||
		NULL == outIsProcessed ||
		vertexId < 0 || vertexId >= bfsState->graph->vertexCapacity)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outIsProcessed = iTestBit(bfsState->isVertexProcessed, bfsState->graph->vertexCapacity, vertexId);
	return kAlgoErrorNone;
}
AlgoError algoGraphBfsStateGetVertexParent(const AlgoGraphBfsState bfsState, int32_t vertexId, int32_t *outParentVertexId)
{
	if (NULL == bfsState ||
		NULL == outParentVertexId ||
		!iGraphIsValidVertexId(bfsState->graph, vertexId))
	{
		return kAlgoErrorInvalidArgument;
	}
	*outParentVertexId = bfsState->vertexParents[vertexId];
	return kAlgoErrorNone;
}


AlgoError algoGraphBfs(const AlgoGraph graph, AlgoGraphBfsState bfsState, int32_t rootVertexId, AlgoGraphBfsCallbacks callbacks)
{
	if (NULL == graph ||
		NULL == bfsState ||
		graph != bfsState->graph ||
		0 == iGraphIsValidVertexId(graph, rootVertexId))
	{
		return kAlgoErrorInvalidArgument;
	}
	AlgoError err;
	int32_t vertexCapacityRounded = (graph->vertexCapacity+31) & ~31;
	err = algoQueueInsert(bfsState->vertexQueue, algoDataFromInt(rootVertexId));
	iSetBit(bfsState->isVertexDiscovered, vertexCapacityRounded, rootVertexId);
	int32_t currentQueueSize = 1;
	do
	{
		AlgoData queueElem;
		int32_t v0 = -1;
		int32_t edgeCount = 0;
		/* Pop the next vertex and process it. */
		const AlgoGraphEdge *e = NULL;
		err = algoQueueRemove(bfsState->vertexQueue, &queueElem);
		v0 = queueElem.asInt;
		ALGO_ASSERT( 1 == iGraphIsValidVertexId(graph, v0) );
		if (NULL != callbacks.vertexFuncEarly)
			callbacks.vertexFuncEarly(graph, bfsState, v0, callbacks.vertexFuncEarlyUserData);
		ALGO_ASSERT(0 == iTestBit(bfsState->isVertexProcessed, vertexCapacityRounded, v0));
		iSetBit(bfsState->isVertexProcessed, vertexCapacityRounded, v0); /* must be set here to prevent undirected edges from looping infinitely. */
		/* Explore v0's edges. */
		e = graph->vertexEdges[v0];
		while(NULL != e)
		{
			edgeCount += 1;
			ALGO_ASSERT(edgeCount <= graph->vertexDegrees[v0]);
			int32_t v1 = e->destVertex;
			ALGO_ASSERT( 1 == iGraphIsValidVertexId(graph, v1) );
			/* Run the edge function, if this is the first time we've seen it. */
			if (0 == iTestBit(bfsState->isVertexProcessed, vertexCapacityRounded, v1) ||
				graph->edgeMode == kAlgoGraphEdgeDirected)
			{
				if (NULL != callbacks.edgeFunc)
					callbacks.edgeFunc(graph, bfsState, v0, v1, callbacks.edgeFuncUserData);
			}
			/* Enqueue v1, if we haven't seen it before. */
			if (0 == iTestBit(bfsState->isVertexDiscovered, vertexCapacityRounded, v1))
			{
				ALGO_ASSERT(0 == iTestBit(bfsState->isVertexProcessed, vertexCapacityRounded, v1));
				iSetBit(bfsState->isVertexDiscovered, vertexCapacityRounded, v1);
				err = algoQueueInsert(bfsState->vertexQueue, algoDataFromInt(v1));
				bfsState->vertexParents[v1] = v0;
			}
			e = e->next;
		}
		ALGO_ASSERT(edgeCount == graph->vertexDegrees[v0]);
		/* Run the late vertex function after all edges are processed. */
		if (NULL != callbacks.vertexFuncLate)
			callbacks.vertexFuncLate(graph, bfsState, v0, callbacks.vertexFuncLateUserData);
		err = algoQueueGetCurrentSize(bfsState->vertexQueue, &currentQueueSize);
	}
	while (currentQueueSize > 0);
	return kAlgoErrorNone;
}

typedef struct AlgoGraphDfsStateImpl
{
	const void *thisBuffer;
	size_t thisBufferSize;
	int32_t currentTime;
	AlgoGraphImpl *graph;
	int32_t *isVertexDiscovered;
	int32_t *isVertexProcessed;
	int32_t *vertexParents;
	int32_t *vertexEntryTime;
	int32_t *vertexExitTime;
	AlgoGraphEdge **vertexNextEdge;
	AlgoStack vertexStack;
} AlgoGraphDfsStateImpl;
AlgoError algoGraphDfsStateComputeBufferSize(size_t *outBufferSize, const AlgoGraph graph)
{
	if (NULL == outBufferSize ||
		NULL == graph)
	{
		return kAlgoErrorInvalidArgument;
	}
	int32_t vertexCapacityRounded = (graph->vertexCapacity+31) & ~31;
	size_t discoveredSize         = vertexCapacityRounded * sizeof(int32_t)  / 32;
	size_t processedSize          = vertexCapacityRounded * sizeof(int32_t)  / 32;
	size_t parentsSize            = graph->vertexCapacity * sizeof(int32_t);
	size_t entryTimeSize          = graph->vertexCapacity * sizeof(int32_t);
	size_t exitTimeSize           = graph->vertexCapacity * sizeof(int32_t);
	size_t nextEdgeSize           = graph->vertexCapacity * sizeof(AlgoGraphEdge*);
	size_t stackSize = 0;
	algoStackComputeBufferSize(&stackSize, graph->vertexCapacity);
	*outBufferSize = sizeof(AlgoGraphDfsStateImpl) + discoveredSize + processedSize + parentsSize
		+ entryTimeSize + exitTimeSize + nextEdgeSize + stackSize;
	return kAlgoErrorNone;
}
AlgoError algoGraphDfsStateCreate(AlgoGraphDfsState *outState, const AlgoGraph graph, void *buffer, size_t bufferSize)
{
	size_t minBufferSize = 0;
	AlgoError err;
	uint8_t *bufferNext = (uint8_t*)buffer;
	if (NULL == outState ||
		NULL == graph ||
		NULL == buffer)
	{
		return kAlgoErrorInvalidArgument;
	}
	err = algoGraphDfsStateComputeBufferSize(&minBufferSize, graph);
	if (bufferSize < minBufferSize ||
		kAlgoErrorNone != err)
	{
		return kAlgoErrorInvalidArgument;
	}

	int32_t vertexCapacityRounded = (graph->vertexCapacity+31) & ~31;
	size_t discoveredSize         = vertexCapacityRounded * sizeof(int32_t)  / 32;
	size_t processedSize          = vertexCapacityRounded * sizeof(int32_t)  / 32;
	size_t parentsSize            = graph->vertexCapacity * sizeof(int32_t);
	size_t entryTimeSize          = graph->vertexCapacity * sizeof(int32_t);
	size_t exitTimeSize           = graph->vertexCapacity * sizeof(int32_t);
	size_t nextEdgeSize           = graph->vertexCapacity * sizeof(AlgoGraphEdge*);
	size_t stackSize              = 0;

	*outState = (AlgoGraphDfsStateImpl *)bufferNext;
	bufferNext += sizeof(AlgoGraphDfsStateImpl);

	int32_t *discovered = (int32_t*)bufferNext;
	bufferNext += discoveredSize;

	int32_t *processed = (int32_t*)bufferNext;
	bufferNext += processedSize;

	int32_t *vertexParents = (int32_t*)bufferNext;
	bufferNext += parentsSize;

	int32_t *entryTime = (int32_t*)bufferNext;
	bufferNext += entryTimeSize;

	int32_t *exitTime = (int32_t*)bufferNext;
	bufferNext += exitTimeSize;

	AlgoGraphEdge **vertexNextEdge = (AlgoGraphEdge**)bufferNext;
	bufferNext += nextEdgeSize;

	err = algoStackComputeBufferSize(&stackSize, graph->vertexCapacity);
	AlgoStack vertexStack;
	err = algoStackCreate(&vertexStack, graph->vertexCapacity, bufferNext, stackSize);
	bufferNext += stackSize;

	ALGO_ASSERT( bufferNext-minBufferSize == buffer ); /* If this fails, algoGraphDfsStateComputeBufferSize() is out of date */
	(*outState)->thisBuffer         = buffer;
	(*outState)->thisBufferSize     = bufferSize;
	(*outState)->graph              = graph;
	(*outState)->isVertexDiscovered = discovered;
	(*outState)->isVertexProcessed  = processed;
	(*outState)->vertexParents      = vertexParents;
	(*outState)->vertexEntryTime    = entryTime;
	(*outState)->vertexExitTime     = exitTime;
	(*outState)->vertexNextEdge     = vertexNextEdge;
	(*outState)->vertexStack        = vertexStack;
	(*outState)->currentTime        = 0;

	ALGO_MEMSET(discovered, 0, discoveredSize);
	ALGO_MEMSET(processed, 0, processedSize);
	ALGO_MEMSET(entryTime, 0, entryTimeSize); /* TODO: lazily initialize just-in-time? Better for sparse graphs. */
	ALGO_MEMSET(exitTime,  0, exitTimeSize); /* TODO: lazily initialize just-in-time? Better for sparse graphs. */
	for(int iVertex=0; iVertex<graph->vertexCapacity; ++iVertex)
	{
		vertexParents[iVertex] = -1;
	}
	ALGO_MEMCPY(vertexNextEdge, graph->vertexEdges, nextEdgeSize); /* TODO: lazily initialize just-in-time? Better for sparse graphs. */
	return kAlgoErrorNone;
}

AlgoError algoGraphDfsStateGetBufferSize(const AlgoGraphDfsState dfsState, size_t *outBufferSize)
{
	if (NULL == dfsState ||
		NULL == outBufferSize)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outBufferSize = dfsState->thisBufferSize;
	return kAlgoErrorNone;
}

AlgoError algoGraphDfsStateIsVertexDiscovered(const AlgoGraphDfsState dfsState, int32_t vertexId, int *outIsDiscovered)
{
	if (NULL == dfsState ||
		NULL == outIsDiscovered ||
		vertexId < 0 || vertexId >= dfsState->graph->vertexCapacity)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outIsDiscovered = iTestBit(dfsState->isVertexDiscovered, dfsState->graph->vertexCapacity, vertexId);
	return kAlgoErrorNone;
}
AlgoError algoGraphDfsStateIsVertexProcessed(const AlgoGraphDfsState dfsState, int32_t vertexId, int *outIsProcessed)
{
	if (NULL == dfsState ||
		NULL == outIsProcessed ||
		vertexId < 0 || vertexId >= dfsState->graph->vertexCapacity)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outIsProcessed = iTestBit(dfsState->isVertexProcessed, dfsState->graph->vertexCapacity, vertexId);
	return kAlgoErrorNone;
}
AlgoError algoGraphDfsStateGetVertexParent(const AlgoGraphDfsState dfsState, int32_t vertexId, int32_t *outVertexParentId)
{
	if (NULL == dfsState ||
		NULL == outVertexParentId ||
		!iGraphIsValidVertexId(dfsState->graph, vertexId))
	{
		return kAlgoErrorInvalidArgument;
	}
	*outVertexParentId = dfsState->vertexParents[vertexId];
	return kAlgoErrorNone;
}
AlgoError algoGraphDfsStateGetVertexEntryTime(const AlgoGraphDfsState dfsState, int32_t vertexId, int32_t *outEntryTime)
{
	if (NULL == dfsState ||
		NULL == outEntryTime ||
		vertexId < 0 || vertexId >= dfsState->graph->vertexCapacity)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outEntryTime = dfsState->vertexEntryTime[vertexId];
	return kAlgoErrorNone;
}
AlgoError algoGraphDfsStateGetVertexExitTime(const AlgoGraphDfsState dfsState, int32_t vertexId, int32_t *outExitTime)
{
	if (NULL == dfsState ||
		NULL == outExitTime ||
		vertexId < 0 || vertexId >= dfsState->graph->vertexCapacity)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outExitTime = dfsState->vertexExitTime[vertexId];
	return kAlgoErrorNone;
}

AlgoError algoGraphDfs(const AlgoGraph graph, AlgoGraphDfsState dfsState, int32_t rootVertexId, AlgoGraphDfsCallbacks callbacks)
{
	if (NULL == graph ||
		NULL == dfsState ||
		0 == iGraphIsValidVertexId(graph, rootVertexId))
	{
		return kAlgoErrorInvalidArgument;
	}

	algoStackPush(dfsState->vertexStack, algoDataFromInt(rootVertexId));
	int32_t currentStackSize = -1;
	algoStackGetCurrentSize(dfsState->vertexStack, &currentStackSize);
	while(currentStackSize > 0)
	{
		AlgoData stackElem;
		algoStackPop(dfsState->vertexStack, &stackElem);
		int32_t v0 = stackElem.asInt;
		ALGO_ASSERT( iGraphIsValidVertexId(graph, v0) );
		if ( 0 == iTestBit(dfsState->isVertexDiscovered, graph->vertexCapacity, v0) )
		{
			/* discovered! */
			iSetBit(dfsState->isVertexDiscovered, graph->vertexCapacity, v0);
			dfsState->currentTime += 1;
			dfsState->vertexEntryTime[v0] = dfsState->currentTime;
			if (NULL != callbacks.vertexFuncEarly)
				callbacks.vertexFuncEarly(graph, dfsState, v0, callbacks.vertexFuncEarlyUserData);
		}
		if (NULL != dfsState->vertexNextEdge[v0])
		{
			AlgoGraphEdge *edge = dfsState->vertexNextEdge[v0];
			dfsState->vertexNextEdge[v0] = edge->next;
			algoStackPush(dfsState->vertexStack, algoDataFromInt(v0));
			if (0 == iTestBit(dfsState->isVertexDiscovered, graph->vertexCapacity, edge->destVertex))
			{
				assert(dfsState->vertexParents[edge->destVertex] < 0);
				dfsState->vertexParents[edge->destVertex] = v0;
				if (NULL != callbacks.edgeFunc)
					callbacks.edgeFunc(graph, dfsState, v0, edge->destVertex, callbacks.edgeFuncUserData);
				algoStackPush(dfsState->vertexStack, algoDataFromInt(edge->destVertex));
			}
			else if ((0 == iTestBit(dfsState->isVertexProcessed, graph->vertexCapacity, edge->destVertex) && dfsState->vertexParents[v0] != edge->destVertex) ||
					 kAlgoGraphEdgeDirected == graph->edgeMode)
			{
				if (NULL != callbacks.edgeFunc)
					callbacks.edgeFunc(graph, dfsState, v0, edge->destVertex, callbacks.edgeFuncUserData);
			}
		}
		else
		{
			/* v0 has no more edges to visit; it is now fully processed. */
			if (NULL != callbacks.vertexFuncLate)
				callbacks.vertexFuncLate(graph, dfsState, v0, callbacks.vertexFuncLateUserData);
			ALGO_ASSERT( 0 == iTestBit(dfsState->isVertexProcessed, graph->vertexCapacity, v0) );
			dfsState->currentTime += 1;
			dfsState->vertexExitTime[v0] = dfsState->currentTime;
			iSetBit(dfsState->isVertexProcessed, graph->vertexCapacity, v0);
		}
		algoStackGetCurrentSize(dfsState->vertexStack, &currentStackSize);
	}

	return kAlgoErrorNone;
}

typedef enum IGraphEdgeType
{
	kGraphEdgeTypeTree    = 0,
	kGraphEdgeTypeBack    = 1,
	kGraphEdgeTypeForward = 2,
	kGraphEdgeTypeCross   = 3,
} IGraphEdgeType;
ALGO_INTERNAL void iGraphTopoSortEdge(AlgoGraph graph, AlgoGraphDfsState dfsState, int32_t v0, int32_t v1, void *userData)
{
	ALGO_UNUSED(v0);
	ALGO_UNUSED(userData);
	/** All we're doing here is making sure there are no BACK edges. I'll leave in the full edge classification
		in case I need it later.
		*/
	IGraphEdgeType edgeType = kGraphEdgeTypeTree;
#if 0
	if ( iTestBit(dfsState->isVertexDiscovered, graph->vertexCapacity, v1) && 
		!iTestBit(dfsState->isVertexProcessed,  graph->vertexCapacity, v1))
	{
		edgeType = kGraphEdgeTypeBack;
	}
#else
	if (dfsState->vertexParents[v1] == v0)
	{
		edgeType = kGraphEdgeTypeTree;
	}
	else if ( iTestBit(dfsState->isVertexDiscovered, graph->vertexCapacity, v1) && 
			 !iTestBit(dfsState->isVertexProcessed,  graph->vertexCapacity, v1))
	{
		edgeType = kGraphEdgeTypeBack;
	}
	else if (iTestBit(dfsState->isVertexProcessed, graph->vertexCapacity, v1) &&
			 dfsState->vertexEntryTime[v1] > dfsState->vertexEntryTime[v0])
	{
		edgeType = kGraphEdgeTypeForward;
	}
	else if (iTestBit(dfsState->isVertexProcessed, graph->vertexCapacity, v1) &&
			 dfsState->vertexEntryTime[v1] < dfsState->vertexEntryTime[v0])
	{
		edgeType = kGraphEdgeTypeCross;
	}
	else
	{
		ALGO_ASSERT(0); /* unclassified edge from v0 to v1 */
	}
#endif
	ALGO_ASSERT(edgeType != kGraphEdgeTypeBack);
}
typedef struct IGraphTopoSortResults
{
	int32_t *sortedVertices;
	int32_t nextFreeIndex; /* Next free index in the sortedVertices array. */
} IGraphTopoSortResults;
ALGO_INTERNAL void iGraphTopoSortVertexLate(AlgoGraph graph, AlgoGraphDfsState dfsState, int32_t vertexId, void *userData)
{
	ALGO_UNUSED(graph);
	ALGO_UNUSED(dfsState);
	IGraphTopoSortResults *results = (IGraphTopoSortResults*)userData;
	ALGO_ASSERT(results->nextFreeIndex >= 0 && results->nextFreeIndex < graph->currentVertexCount);
	results->sortedVertices[results->nextFreeIndex] = vertexId;
	results->nextFreeIndex -= 1;
}
AlgoError algoGraphTopoSortComputeBufferSize(size_t *outBufferSize, const AlgoGraph graph)
{
	AlgoError err;
	if (NULL == graph ||
		NULL == outBufferSize)
	{
		return kAlgoErrorInvalidArgument;
	}
	size_t dfsStateBufferSize = 0;
	err = algoGraphDfsStateComputeBufferSize(&dfsStateBufferSize, graph);
	if (kAlgoErrorNone != err)
		return err;
	*outBufferSize = dfsStateBufferSize;
	return kAlgoErrorNone;
}
AlgoError algoGraphTopoSort(const AlgoGraph graph, int32_t outSortedVertices[], size_t sortedVertexCount, 
	void *buffer, size_t bufferSize)
{
	if (NULL == graph ||
		NULL == outSortedVertices ||
		sortedVertexCount < graph->currentVertexCount)
	{
		return kAlgoErrorInvalidArgument;
	}
	if (kAlgoGraphEdgeUndirected == graph->edgeMode)
	{
		return kAlgoErrorOperationFailed;
	}
	size_t minBufferSize = 0;
	AlgoError err;
	uint8_t *bufferNext = (uint8_t*)buffer;
	err = algoGraphTopoSortComputeBufferSize(&minBufferSize, graph);
	if (NULL == buffer ||
		bufferSize < minBufferSize ||
		kAlgoErrorNone != err)
	{
		return kAlgoErrorInvalidArgument;
	}

	size_t dfsStateBufferSize = 0;
	err = algoGraphDfsStateComputeBufferSize(&dfsStateBufferSize, graph);
	void *dfsStateBuffer = bufferNext;
	bufferNext += dfsStateBufferSize;

	ALGO_ASSERT( bufferNext-minBufferSize == buffer ); /* If this fails, algoGraphTopoSortComputeBufferSize() is out of date */
	AlgoGraphDfsState dfsState;
	err = algoGraphDfsStateCreate(&dfsState, graph, dfsStateBuffer, dfsStateBufferSize);
	if (kAlgoErrorNone != err)
		return err;

	IGraphTopoSortResults sortResults;
	sortResults.sortedVertices = outSortedVertices;
	sortResults.nextFreeIndex = graph->currentVertexCount-1;
	AlgoGraphDfsCallbacks dfsCallbacks = {
		NULL, NULL,
		iGraphTopoSortEdge, NULL,
		iGraphTopoSortVertexLate, NULL
	};
	dfsCallbacks.vertexFuncLateUserData = &sortResults;
	for(int iValidVert=0; iValidVert<graph->currentVertexCount; ++iValidVert)
	{
		int32_t vertexId = graph->validVertexIds[iValidVert];
		int isVertexSorted = 0;
		err = algoGraphDfsStateIsVertexProcessed(dfsState, vertexId, &isVertexSorted);
		if (iGraphIsValidVertexId(graph, vertexId) &&
			!isVertexSorted)
		{
			err = algoGraphDfs(graph, dfsState, vertexId, dfsCallbacks);
			if (kAlgoErrorNone != err)
				return err;
			if (sortResults.nextFreeIndex < 0)
				break;
		}
	}

	return kAlgoErrorNone;
}

#endif /* ALGO_IMPLEMENTATION */
