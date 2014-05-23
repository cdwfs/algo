#pragma once

#include <stdint.h>

typedef enum AlgoError
{
	kAlgoErrorNone = 0,
	kAlgoErrorInvalidArgument = 1,
	kAlgoErrorOperationFailed = 2,
	kAlgoErrorAllocationFailed = 3,
} AlgoError;

// Queue
typedef struct AlgoQueueImpl *AlgoQueue;
typedef int32_t AlgoQueueData;

AlgoError algoQueueCreate(AlgoQueue *outQueue, const int32_t queueCapacity);
AlgoError algoQueueInsert(AlgoQueue queue, const AlgoQueueData data);
AlgoError algoQueueRemove(AlgoQueue queue, AlgoQueueData *outData);
AlgoError algoQueueCapacity(const AlgoQueue queue, int32_t *outCapacity);
AlgoError algoQueueCurrentSize(const AlgoQueue queue, int32_t *outSize);


// Heap
typedef struct AlgoHeapImpl *AlgoHeap;
typedef int32_t AlgoHeapKey;
typedef const void* AlgoHeapData;

AlgoError algoHeapCreate(AlgoHeap *heap, const int32_t heapCapacity);
AlgoError algoHeapCurrentSize(AlgoHeap heap, int32_t *outSize);
AlgoError algoHeapInsert(AlgoHeap heap, const AlgoHeapKey key, const AlgoHeapData data);
AlgoError algoHeapPeek(AlgoHeap heap, AlgoHeapKey *outTopKey, AlgoHeapData *outTopData);
AlgoError algoHeapPop(AlgoHeap heap);
AlgoError algoHeapCheck(AlgoHeap heap);
AlgoError algoHeapCapacity(AlgoHeap heap, int32_t *outCapacity);
