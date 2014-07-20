#ifndef ALGO_INCLUDE_H
#define ALGO_INCLUDE_H

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


typedef enum AlgoError
{
	kAlgoErrorNone = 0,
	kAlgoErrorInvalidArgument = 1,
	kAlgoErrorOperationFailed = 2,
	kAlgoErrorAllocationFailed = 3,
} AlgoError;

typedef union
{
	int32_t asInt;
	float asFloat;
	void *asPtr;
} AlgoData;
ALGODEF ALGO_INLINE AlgoData algoDataFromInt(int32_t i) { AlgoData d; d.asInt   = i; return d; }
ALGODEF ALGO_INLINE AlgoData algoDataFromFloat(float f) { AlgoData d; d.asFloat = f; return d; }
ALGODEF ALGO_INLINE AlgoData algoDataFromPtr(void *p)   { AlgoData d; d.asPtr   = p; return d; }

// Queue
typedef struct AlgoQueueImpl *AlgoQueue;
ALGODEF AlgoError algoQueueBufferSize(size_t *outBufferSize, int32_t queueCapacity);
ALGODEF AlgoError algoQueueCreate(AlgoQueue *outQueue, int32_t queueCapacity, void *buffer, size_t bufferSize);
ALGODEF AlgoError algoQueueInsert(AlgoQueue queue, const AlgoData elem);
ALGODEF AlgoError algoQueueRemove(AlgoQueue queue, AlgoData *outElem);
ALGODEF AlgoError algoQueueCapacity(const AlgoQueue queue, int32_t *outCapacity);
ALGODEF AlgoError algoQueueCurrentSize(const AlgoQueue queue, int32_t *outSize);


// Heap
typedef struct AlgoHeapImpl *AlgoHeap;
// Defines the ordering of keys within the heap.
// If this function returns < 0, keyL is higher priority than keyR.
// If this function returns > 0, keyR is higher priority than keyL.
// If this function returns   0, keyR has the same priority as keyL.
typedef int (*AlgoHeapKeyCompareFunc)(const AlgoData keyL, const AlgoData keyR);

ALGODEF AlgoError algoHeapBufferSize(size_t *outBufferSize, int32_t heapCapacity);
ALGODEF AlgoError algoHeapCreate(AlgoHeap *heap, int32_t heapCapacity, AlgoHeapKeyCompareFunc keyCompare,
	void *buffer, size_t bufferSize);
ALGODEF AlgoError algoHeapCurrentSize(AlgoHeap heap, int32_t *outSize);
ALGODEF AlgoError algoHeapInsert(AlgoHeap heap, const AlgoData key, const AlgoData data);
ALGODEF AlgoError algoHeapPeek(AlgoHeap heap, AlgoData *outTopKey, AlgoData *outTopData);
ALGODEF AlgoError algoHeapPop(AlgoHeap heap);
ALGODEF AlgoError algoHeapCheck(AlgoHeap heap);
ALGODEF AlgoError algoHeapCapacity(AlgoHeap heap, int32_t *outCapacity);

#ifdef __cplusplus
}
#endif
//
//
////   end header file   /////////////////////////////////////////////////////
#endif // ALGO_INCLUDE_H

#ifdef ALGO_IMPLEMENTATION

#include <assert.h>
#include <stdlib.h>

///////////////////////////////////////////////////////
// AlgoQueue
///////////////////////////////////////////////////////

typedef struct AlgoQueueImpl
{
	AlgoData *nodes;
	int32_t nodeCount; // Actual length of the nodes[] array.
	int32_t capacity; // Outside view of how many elements can be stored in the queue.
	int32_t head; // index of the next element to remove (if the queue isn't empty)
	int32_t tail; // index of the first empty element past the end of the queue.
} AlgoQueueImpl;

// We never let the nodes array fill up completely.
// if head == tail, that means the queue is empty.
// if head = (tail+1) % nodeCount, the queue is full.

static int iQueueIsEmpty(const AlgoQueue queue)
{
	assert(NULL != queue);
	return (queue->head == queue->tail);
}
static int iQueueIsFull(const AlgoQueue queue)
{
	assert(NULL != queue);
	return queue->head == (queue->tail+1) % queue->nodeCount;
}

///////////////////////////////////////////////////////

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
	uint8_t *bufferNext = buffer;
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
	(*outQueue)->nodeCount = queueCapacity+1; // tail is always an empty node
	(*outQueue)->nodes = (AlgoData*)bufferNext;
	bufferNext += (*outQueue)->nodeCount * sizeof(AlgoData);
	(*outQueue)->head = 0;
	(*outQueue)->tail = 0;
	assert( (uintptr_t)bufferNext - (uintptr_t)buffer == minBufferSize ); // If this fails, algoQueueBufferSize() is out of date
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

/////////////////////////////////////////////
// AlgoHeap
/////////////////////////////////////////////

typedef struct AlgoHeapNode
{
	AlgoData key;
	AlgoData data;
} AlgoHeapNode;

typedef struct AlgoHeapImpl
{
	AlgoHeapNode *nodes;
	AlgoHeapKeyCompareFunc keyCompare;
	int32_t capacity;
	int32_t nextEmpty; // 1-based; N's kids = 2*N and 2*N+1; N's parent = N/2
} AlgoHeapImpl;

///////////// Internal utilities

static const int32_t kAlgoHeapRootIndex = 1;

static int32_t iHeapCurrentSize(AlgoHeap heap)
{
	assert(NULL != heap);
	return heap->nextEmpty - kAlgoHeapRootIndex;
}

static int iHeapIsNodeValid(AlgoHeap heap, const int32_t nodeIndex)
{
	assert(NULL != heap);
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
	assert(NULL != heap);
	assert(iHeapIsNodeValid(heap, index1));
	assert(iHeapIsNodeValid(heap, index2));
	tempNode = heap->nodes[index1];
	heap->nodes[index1] = heap->nodes[index2];
	heap->nodes[index2] = tempNode;
}

//////////// public API functions

AlgoError algoHeapBufferSize(size_t *outSize, int32_t heapCapacity)
{
	if (NULL == outSize)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outSize = sizeof(AlgoHeapImpl) + (heapCapacity+kAlgoHeapRootIndex) * sizeof(AlgoHeapNode);
	return kAlgoErrorNone;
}

AlgoError algoHeapCreate(AlgoHeap *outHeap, int32_t heapCapacity, AlgoHeapKeyCompareFunc keyCompare,
	void *buffer, size_t bufferSize)
{
	size_t minBufferSize = 0;
	AlgoError err;
	uint8_t *bufferNext = buffer;
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
	assert( (uintptr_t)bufferNext - (uintptr_t)buffer == minBufferSize ); // If this fails, algoHeapBufferSize() is out of date
	return kAlgoErrorNone;
}

int32_t algoHeapCurrentSize(AlgoHeap heap, int32_t *outSize)
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
		return kAlgoErrorOperationFailed; // Can't insert if it's full!
	}
	// Insert new node at the end
	childIndex = heap->nextEmpty;
	heap->nextEmpty += 1;
	heap->nodes[childIndex].key  = key;
	heap->nodes[childIndex].data = data;
	// Bubble up
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
		return kAlgoErrorOperationFailed; // Can't peek an empty heap
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

AlgoError algoHeapPop(AlgoHeap heap)
{
	int32_t lastIndex, parentIndex, leftChildIndex;
	if (NULL == heap)
	{
		return kAlgoErrorInvalidArgument;
	}
	if (0 == iHeapCurrentSize(heap))
	{
		return kAlgoErrorOperationFailed; // Can't pop an empty heap
	}
	lastIndex = heap->nextEmpty-1;
	heap->nodes[kAlgoHeapRootIndex] = heap->nodes[lastIndex];
	// Bubble down
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
			break; // early out
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
	// Basic tests
	if (NULL == heap ||
		NULL == heap->nodes)
	{
		return kAlgoErrorInvalidArgument; // AlgoHeap pointer(s) are NULL
	}
	if (heap->nextEmpty < kAlgoHeapRootIndex ||
		heap->capacity < 0 ||
		iHeapCurrentSize(heap) > heap->capacity)
	{
		return kAlgoErrorInvalidArgument; // AlgoHeap size/capacity are invalid
	}
	
	if (iHeapCurrentSize(heap) == 0)
	{
		return kAlgoErrorNone; // Empty heaps are valid
	}

	// Recursively test all nodes to verify the heap condition holds.
	for(iNode=kAlgoHeapRootIndex+1; iNode<heap->nextEmpty; ++iNode)
	{
		int32_t parentIndex = iHeapParentIndex(iNode);
		assert(iHeapIsNodeValid(heap, parentIndex));
		if (heap->keyCompare(heap->nodes[iNode].key, heap->nodes[parentIndex].key) < 0)
		{
			return kAlgoErrorInvalidArgument;
		}
	}
	return kAlgoErrorNone;
}

#endif // ALGO_IMPLEMENTATION
