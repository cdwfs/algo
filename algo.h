#ifndef ALGO_INCLUDE_H
#define ALGO_INCLUDE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ALGO_STATIC
#define ALGODEF static
#else
#define ALGODEF extern
#endif

typedef enum AlgoError
{
	kAlgoErrorNone = 0,
	kAlgoErrorInvalidArgument = 1,
	kAlgoErrorOperationFailed = 2,
	kAlgoErrorAllocationFailed = 3,
} AlgoError;

// Queue
typedef struct AlgoQueueImpl *AlgoQueue;
typedef union
{
	int32_t asInt;
	float asFloat;
	void *asPtr;
} AlgoQueueData;

ALGODEF AlgoError algoQueueCreate(AlgoQueue *outQueue, int32_t queueCapacity);
ALGODEF AlgoError algoQueueDestroy(AlgoQueue queue);
ALGODEF AlgoError algoQueueInsert(AlgoQueue queue, const AlgoQueueData data);
ALGODEF AlgoError algoQueueRemove(AlgoQueue queue, AlgoQueueData *outData);
ALGODEF AlgoError algoQueueCapacity(const AlgoQueue queue, int32_t *outCapacity);
ALGODEF AlgoError algoQueueCurrentSize(const AlgoQueue queue, int32_t *outSize);


// Heap
typedef struct AlgoHeapImpl *AlgoHeap;
typedef int32_t AlgoHeapKey;
typedef union
{
	int32_t asInt;
	float asFloat;
	void *asPtr;
} AlgoHeapData;

ALGODEF AlgoError algoHeapCreate(AlgoHeap *heap, int32_t heapCapacity);
ALGODEF AlgoError algoHeapDestroy(AlgoHeap heap);
ALGODEF AlgoError algoHeapCurrentSize(AlgoHeap heap, int32_t *outSize);
ALGODEF AlgoError algoHeapInsert(AlgoHeap heap, const AlgoHeapKey key, const AlgoHeapData data);
ALGODEF AlgoError algoHeapPeek(AlgoHeap heap, AlgoHeapKey *outTopKey, AlgoHeapData *outTopData);
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
#if __STDC_VERSION__ >= 199901L || _MSC_VER >= 1800
// C99
#include <stdbool.h>
#else
#define bool int
#define false 0
#define true 1
#endif
#include <stdlib.h>
#include <string.h>

///////////////////////////////////////////////////////
// AlgoQueue
///////////////////////////////////////////////////////

typedef struct AlgoQueueImpl
{
	AlgoQueueData *nodes;
	int32_t nodeCount; // Actual length of the nodes[] array.
	int32_t capacity; // Outside view of how many elements can be stored in the queue.
	int32_t head; // index of the next element to remove (if the queue isn't empty)
	int32_t tail; // index of the first empty element past the end of the queue.
} AlgoQueueImpl;

// We never let the nodes array fill up completely.
// if head == tail, that means the queue is empty.
// if head = (tail+1) % nodeCount, the queue is full.

static bool iQueueIsEmpty(const AlgoQueue queue)
{
	assert(NULL != queue);
	return (queue->head == queue->tail);
}
static bool iQueueIsFull(const AlgoQueue queue)
{
	assert(NULL != queue);
	return queue->head == (queue->tail+1) % queue->nodeCount;
}

///////////////////////////////////////////////////////

AlgoError algoQueueCreate(AlgoQueue *outQueue, int32_t queueCapacity)
{
	if (NULL == outQueue ||
		queueCapacity < 1)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outQueue = malloc(sizeof(AlgoQueueImpl));
	if (NULL == *outQueue)
	{
		return kAlgoErrorAllocationFailed;
	}
	(*outQueue)->capacity = queueCapacity;
	(*outQueue)->nodeCount = queueCapacity+1; // tail is always an empty node
	(*outQueue)->nodes = malloc( (*outQueue)->nodeCount * sizeof(AlgoQueueData) );
	(*outQueue)->head = 0;
	(*outQueue)->tail = 0;
	if (NULL ==  (*outQueue)->nodes)
	{
		free(*outQueue);
		return kAlgoErrorAllocationFailed;
	}
	return kAlgoErrorNone;
}
AlgoError algoQueueDestroy(AlgoQueue queue)
{
	if (NULL == queue)
	{
		return kAlgoErrorNone;
	}
	if (NULL == queue->nodes)
	{
		return kAlgoErrorInvalidArgument;
	}
	free(queue->nodes);
	free(queue);
	return kAlgoErrorNone;
}

AlgoError algoQueueInsert(AlgoQueue queue, const AlgoQueueData data)
{
	if (NULL == queue)
	{
		return kAlgoErrorInvalidArgument;
	}
	if (iQueueIsFull(queue))
	{
		return kAlgoErrorOperationFailed;
	}
	queue->nodes[queue->tail] = data;
	queue->tail = (queue->tail + 1) % queue->nodeCount;
	return kAlgoErrorNone;
}
AlgoError algoQueueRemove(AlgoQueue queue, AlgoQueueData *outData)
{
	if (NULL == queue ||
		NULL == outData)
	{
		return kAlgoErrorInvalidArgument;
	}
	if (iQueueIsEmpty(queue))
	{
		return kAlgoErrorOperationFailed;
	}
	*outData = queue->nodes[queue->head];
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
	AlgoHeapKey key;
	AlgoHeapData data;
} AlgoHeapNode;

typedef struct AlgoHeapImpl
{
	AlgoHeapNode *nodes;
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

static bool iHeapIsNodeValid(AlgoHeap heap, const int32_t nodeIndex)
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

AlgoError algoHeapCreate(AlgoHeap *outHeap, int32_t heapCapacity)
{
	if (NULL == outHeap)
	{
		return kAlgoErrorInvalidArgument;
	}
	*outHeap = malloc(sizeof(AlgoHeapImpl));
	(*outHeap)->nodes = malloc((heapCapacity+kAlgoHeapRootIndex)*sizeof(AlgoHeapNode));
	(*outHeap)->capacity = heapCapacity;
	(*outHeap)->nextEmpty = kAlgoHeapRootIndex;
	return kAlgoErrorNone;
}
AlgoError algoHeapDestroy(AlgoHeap heap)
{
	if (NULL == heap)
	{
		return kAlgoErrorNone;
	}
	if (NULL == heap->nodes)
	{
		return kAlgoErrorInvalidArgument;
	}
	free(heap->nodes);
	free(heap);
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

AlgoError algoHeapInsert(AlgoHeap heap, const AlgoHeapKey key, const AlgoHeapData data)
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
		if (heap->nodes[parentIndex].key <= heap->nodes[childIndex].key)
		{
			break;
		}
		iHeapSwapNodes(heap, parentIndex, childIndex);
		childIndex = parentIndex;
	}
	return kAlgoErrorNone;
}

AlgoError algoHeapPeek(AlgoHeap heap, AlgoHeapKey *outTopKey, AlgoHeapData *outTopData)
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
		if (heap->nodes[leftChildIndex].key < heap->nodes[minKeyIndex].key)
		{
			minKeyIndex = leftChildIndex;
		}
		if (rightChildIndex < heap->nextEmpty &&
			heap->nodes[rightChildIndex].key < heap->nodes[minKeyIndex].key)
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
		if (heap->nodes[iNode].key < heap->nodes[parentIndex].key)
		{
			return kAlgoErrorInvalidArgument;
		}
	}
	return kAlgoErrorNone;
}

#endif // ALGO_IMPLEMENTATION
