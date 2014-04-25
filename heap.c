#include "heap.h"

#include <assert.h>
#include <stdlib.h>

typedef struct HeapNode
{
	int32_t key;
	const void *data;
} HeapNode;

typedef struct Heap
{
	HeapNode *nodes;
	int32_t capacity;
	int32_t nextEmpty; // 1-based; N's kids = 2*N and 2*N+1; N's parent = N/2
} Heap;

///////////// Internal utilities

static const int32_t kHeapRootIndex = 1;

static inline int32_t iHeapCurrentSize(Heap *heap)
{
	assert(NULL != heap);
	return heap->nextEmpty - kHeapRootIndex;
}

static inline int32_t iHeapParentIndex(const int32_t childIndex)
{
	assert(childIndex > kHeapRootIndex);
	return childIndex/2;
}
static inline int32_t iHeapLeftChildIndex(const int32_t parentIndex)
{
	return parentIndex*2;
}
static inline int32_t iHeapRightChildIndex(const int32_t parentIndex)
{
	return parentIndex*2 + 1;
}

static inline void iHeapSwapNodes(Heap *heap,
	const int32_t index1, const int32_t index2)
{
	assert(NULL != heap);
	assert(index1 >= kHeapRootIndex && index1 < heap->nextEmpty);
	assert(index2 >= kHeapRootIndex && index2 < heap->nextEmpty);
	HeapNode tempNode = heap->nodes[index1];
	heap->nodes[index1] = heap->nodes[index2];
	heap->nodes[index2] = tempNode;
}

//////////// public API functions

int32_t heapCreate(Heap **heap, const int32_t heapCapacity)
{
	if (NULL == heap)
	{
		return -1;
	}
	*heap = malloc(sizeof(Heap));
	(*heap)->nodes = malloc((heapCapacity+kHeapRootIndex)*sizeof(HeapNode));
	(*heap)->capacity = heapCapacity;
	(*heap)->nextEmpty = kHeapRootIndex;
	return 0;
}

int32_t heapCurrentSize(Heap *heap, int32_t *outSize)
{
	if (NULL == heap)
	{
		return -1;
	}
	if (NULL != outSize)
	{
		*outSize = iHeapCurrentSize(heap);
	}
	return 0;
}

int32_t heapInsert(Heap *heap, const int32_t key, const void *data)
{
	if (NULL == heap)
	{
		return -1;
	}
	if (iHeapCurrentSize(heap) >= heap->capacity)
	{
		return -2; // Can't insert if it's full!
	}
	// Insert new node at the end
	int32_t childIndex = heap->nextEmpty;
	heap->nextEmpty += 1;
	heap->nodes[childIndex].key  = key;
	heap->nodes[childIndex].data = data;
	// Bubble up
	while(childIndex > kHeapRootIndex)
	{
		int32_t parentIndex = iHeapParentIndex(childIndex);
		if (heap->nodes[parentIndex].key <= heap->nodes[childIndex].key)
		{
			break;
		}
		iHeapSwapNodes(heap, parentIndex, childIndex);
		childIndex = parentIndex;
	}
	return 0;
}

int32_t heapPeek(Heap *heap, int32_t *outTopKey, const void **outTopData)
{
	if (NULL == heap)
	{
		return -1;
	}
	if (0 == iHeapCurrentSize(heap))
	{
		return -2; // Can't peek an empty heap
	}

	if (NULL != outTopKey)
	{
		*outTopKey = heap->nodes[kHeapRootIndex].key;
	}
	if (NULL != outTopData)
	{
		*outTopData = heap->nodes[kHeapRootIndex].data;
	}
	return 0;
}

int32_t heapPop(Heap *heap)
{
	if (NULL == heap)
	{
		return -1;
	}
	int32_t heapSize = iHeapCurrentSize(heap);
	if (0 == heapSize)
	{
		return -2; // Can't pop an empty heap
	}
	int32_t lastIndex = heapSize;
	heap->nodes[kHeapRootIndex] = heap->nodes[lastIndex];
	// Bubble down
	int32_t parentIndex = kHeapRootIndex;
	int32_t leftChildIndex = iHeapLeftChildIndex(parentIndex);
	while(leftChildIndex < heap->nextEmpty)
	{
		int32_t minKeyIndex = parentIndex;
		if (heap->nodes[leftChildIndex].key < heap->nodes[minKeyIndex].key)
		{
			minKeyIndex = leftChildIndex;
		}
		int32_t rightChildIndex = iHeapRightChildIndex(parentIndex);
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
	return 0;
}
