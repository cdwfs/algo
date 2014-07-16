#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
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

AlgoError algoQueueCreate(AlgoQueue *outQueue, const int32_t queueCapacity);
AlgoError algoQueueDestroy(AlgoQueue queue);
AlgoError algoQueueInsert(AlgoQueue queue, const AlgoQueueData data);
AlgoError algoQueueRemove(AlgoQueue queue, AlgoQueueData *outData);
AlgoError algoQueueCapacity(const AlgoQueue queue, int32_t *outCapacity);
AlgoError algoQueueCurrentSize(const AlgoQueue queue, int32_t *outSize);


// Heap
typedef struct AlgoHeapImpl *AlgoHeap;
typedef int32_t AlgoHeapKey;
typedef union
{
	int32_t asInt;
	float asFloat;
	void *asPtr;
} AlgoHeapData;

AlgoError algoHeapCreate(AlgoHeap *heap, const int32_t heapCapacity);
AlgoError algoHeapDestroy(AlgoHeap heap);
AlgoError algoHeapCurrentSize(AlgoHeap heap, int32_t *outSize);
AlgoError algoHeapInsert(AlgoHeap heap, const AlgoHeapKey key, const AlgoHeapData data);
AlgoError algoHeapPeek(AlgoHeap heap, AlgoHeapKey *outTopKey, AlgoHeapData *outTopData);
AlgoError algoHeapPop(AlgoHeap heap);
AlgoError algoHeapCheck(AlgoHeap heap);
AlgoError algoHeapCapacity(AlgoHeap heap, int32_t *outCapacity);

#ifdef __cplusplus
}
#endif
