#define ALGO_IMPLEMENTATION
#define ALGO_STATIC
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

///////////////////////
// Test AlgoQueue
///////////////////////

static int testQueueInsert(AlgoQueue queue, const AlgoData elem)
{
	int32_t capacity = -1;
	int32_t beforeSize = -1, afterSize = -1;
	ALGO_VALIDATE( algoQueueCapacity(queue, &capacity) );

	ALGO_VALIDATE( algoQueueCurrentSize(queue, &beforeSize) );
	if (beforeSize == capacity)
	{
		return 0; // queue is full
	}

	ALGO_VALIDATE( algoQueueInsert(queue, elem) );

	ALGO_VALIDATE( algoQueueCurrentSize(queue, &afterSize) );
	assert(beforeSize+1 == afterSize);
	return 1;
}

static int testQueueRemove(AlgoQueue queue, AlgoData *outElem)
{
	int32_t beforeSize = -1, afterSize = -1;
	ALGO_VALIDATE( algoQueueCurrentSize(queue, &beforeSize) );
	if (beforeSize == 0)
	{
		outElem->asInt = -1;
		return 0; // queue is empty
	}

	ALGO_VALIDATE( algoQueueRemove(queue, outElem) );

	ALGO_VALIDATE( algoQueueCurrentSize(queue, &afterSize) );
	assert(beforeSize-1 == afterSize);

	return 1;
}


///////////////////////
// Test AlgoHeap
///////////////////////

static int testHeapInsert(AlgoHeap heap, int32_t heapContents[])
{
	int32_t capacity = -1;
	int32_t beforeSize = -1, afterSize = -1;
	AlgoData newKey = algoDataFromInt(rand() % capacity);
	AlgoData newData = newKey;
	ALGO_VALIDATE( algoHeapCapacity(heap, &capacity) );
	ALGO_VALIDATE( algoHeapCurrentSize(heap, &beforeSize) );
	if (beforeSize == capacity)
	{
		return 0; // heap is full
	}
	ALGO_VALIDATE( algoHeapInsert(heap, newKey, newData) );

	// Not a heap requirement; just making sure it's a valid index for
	// heapContents[].
	assert(newKey.asInt < capacity);
	                           
	heapContents[newKey.asInt] += 1;

	ALGO_VALIDATE( algoHeapCurrentSize(heap, &afterSize) );
	assert(beforeSize+1 == afterSize);
	ALGO_VALIDATE( algoHeapCheck(heap) );
	return 1;
}

static int testHeapPop(AlgoHeap heap, int32_t heapContents[])
{
	int32_t capacity = -1;
	int32_t beforeSize = -1, afterSize = -1;
	AlgoData minKey;
	AlgoData minData;
	int iVal;
	ALGO_VALIDATE( algoHeapCapacity(heap, &capacity) );
	ALGO_VALIDATE( algoHeapCurrentSize(heap, &beforeSize) );
	if (beforeSize == 0)
	{
		return 0; // heap is empty
	}
	ALGO_VALIDATE( algoHeapPeek(heap, &minKey, &minData) );
	// key and data must match (in this test environment)
	assert(minKey.asInt == minData.asInt);
	// Not a heap requirement; just making sure it's a valid index for
	// heapContents[].
	assert(minKey.asInt < capacity);
	// Make sure minKey is the smallest key in the heap (all counters below
	// it must be zero).
	for(iVal=0; iVal<minKey.asInt; ++iVal)
	{
		assert(heapContents[iVal] == 0);
	}
	// Make sure minKey is in the heap in the first place
	assert(heapContents[minKey.asInt] > 0);

	ALGO_VALIDATE( algoHeapPop(heap) );
	heapContents[minKey.asInt] -= 1;

	ALGO_VALIDATE( algoHeapCurrentSize(heap, &afterSize) );
	assert(beforeSize-1 == afterSize);
	ALGO_VALIDATE( algoHeapCheck(heap) );
	return 1;
}

// For this test, smaller keys have greater priority.
static int heapKeyCompare(const AlgoData keyL, const AlgoData keyR)
{
	if (keyL.asInt < keyR.asInt)
		return -1;
	else if (keyL.asInt > keyR.asInt)
		return 1;
	return 0;
}

int main(void)
{
	unsigned int randomSeed = (unsigned int)time(NULL);
	printf("Random seed: 0x%08X\n", randomSeed);
	srand(randomSeed);

	// Test AlgoQueue
	{
		const int32_t kTestElemCount = 1024*1024;
		const int32_t kQueueCapacity = 512 + (rand() % 1024);
		void *queueBuffer = NULL;
		size_t queueBufferSize = 0;
		int32_t nextToAdd = 0, nextToCheck = 0;
		AlgoQueue queue;
		int32_t currentSize = -1;
		printf("Testing AlgoQueue (capacity: %d, test count: %d)\n", kQueueCapacity, kTestElemCount);

		ALGO_VALIDATE( algoQueueBufferSize(&queueBufferSize, kQueueCapacity) );
		queueBuffer = malloc(queueBufferSize);
		ALGO_VALIDATE( algoQueueCreate(&queue, kQueueCapacity, queueBuffer, queueBufferSize) );

		ALGO_VALIDATE( algoQueueCurrentSize(queue, &currentSize) );
		assert(0 == currentSize);

		// In this test, we alternate between adding a chunk of values to the end of the queue and removing a chunk from the front.
		while (nextToCheck < kTestElemCount)
		{
			const int32_t numAdds = 1 + (rand() % kQueueCapacity);
			int32_t numRemoves;
			int iAdd, iRemove;
			assert(numAdds >= 0);
			printf(" - Inserting at most %d elements...\n", numAdds);
			for(iAdd=0; iAdd<numAdds; ++iAdd)
			{
				nextToAdd += testQueueInsert(queue, algoDataFromInt(nextToAdd));
			}
			ALGO_VALIDATE( algoQueueCurrentSize(queue, &currentSize) );
			if (currentSize > kQueueCapacity)
			{
				fprintf(stderr, "ERROR: Queue size exceeds capacity\n");
				assert(currentSize <= kQueueCapacity);
			}

			// Make sure we can't add elements to a full queue
			if (currentSize == kQueueCapacity)
			{
				AlgoError err = algoQueueInsert(queue, algoDataFromInt(0));
				if (err != kAlgoErrorOperationFailed)
				{
					fprintf(stderr, "ERROR: algoQueueInsert() on a full queue returned %d (expected %d)\n",
						err, kAlgoErrorOperationFailed);
					assert(err == kAlgoErrorOperationFailed);
				}
			}

			// Make sure we can't remove elements from an empty queue
			if (currentSize == 0)
			{
				AlgoData elem;
				AlgoError err = algoQueueRemove(queue, &elem);
				if (err != kAlgoErrorOperationFailed)
				{
					fprintf(stderr, "ERROR: algoQueueRemove() on an empty queue returned %d (expected %d)\n",
						err, kAlgoErrorOperationFailed);
					assert(err == kAlgoErrorOperationFailed);
				}
			}

			numRemoves = 1 + (rand() % currentSize);
			printf(" - Removing at most %d elements...\n", numRemoves);
			for(iRemove=0; iRemove<numRemoves; ++iRemove)
			{
				AlgoData elem;
				int nextToCheckInc = testQueueRemove(queue, &elem);
				if (elem.asInt != nextToCheck)
				{
					fprintf(stderr, "ERROR: Queue element mismatch\n");
					assert(elem.asInt == nextToCheck);
				}
				nextToCheck += nextToCheckInc;
			}
			ALGO_VALIDATE( algoQueueCurrentSize(queue, &currentSize) );
			if (currentSize > kQueueCapacity)
			{
				fprintf(stderr, "ERROR: Queue size exceeds capacity\n");
				assert(currentSize <= kQueueCapacity);
			}

			printf(" - %d elements left to check\n\n", max(0, kTestElemCount - nextToCheck));
#if defined(_MSC_VER)
			_ASSERTE(_CrtCheckMemory());
#endif
		}

		free(queueBuffer);
	}

	// Test AlgoHeap
	{
		const int32_t kHeapCapacity = 16*1024;
		const int32_t kTestCount = 100;
		int32_t *heapContents = heapContents = malloc(kHeapCapacity*sizeof(int32_t));
		void *heapBuffer = NULL;
		size_t heapBufferSize = 0;
		AlgoHeap heap;
		int currentSize = 0;
		int iHeapTest;
		printf("Testing AlgoHeap (capacity: %d, test count: %d)\n", kHeapCapacity, kTestCount);
		memset(heapContents, 0, kHeapCapacity*sizeof(int32_t));

		ALGO_VALIDATE( algoHeapBufferSize(&heapBufferSize, kHeapCapacity) );
		heapBuffer = malloc(heapBufferSize);
		ALGO_VALIDATE( algoHeapCreate(&heap, kHeapCapacity, heapKeyCompare, heapBuffer, heapBufferSize) );
		ALGO_VALIDATE( algoHeapCurrentSize(heap, &currentSize) );
		assert(0 == currentSize);
		for(iHeapTest=0; iHeapTest<kTestCount; ++iHeapTest)
		{
			int32_t numAdds = rand() % (kHeapCapacity-currentSize);
			int32_t iAdd, iPop, iVal;
			int32_t numPops;
			int32_t elemCount = 0;
			printf(" - Adding %d elements...\n", numAdds);
			for(iAdd=0; iAdd<numAdds; ++iAdd)
			{
				testHeapInsert(heap, heapContents);
			}

			ALGO_VALIDATE( algoHeapCurrentSize(heap, &currentSize) );
			numPops = 1 + (rand() % currentSize);
			printf(" - Popping %d elements...\n", numPops);
			for(iPop=0; iPop<numPops; ++iPop)
			{
				testHeapPop(heap, heapContents);
			}

			ALGO_VALIDATE( algoHeapCurrentSize(heap, &currentSize) );
			for(iVal=0; iVal<kHeapCapacity; ++iVal)
			{
				elemCount += heapContents[iVal];
			}
			assert(elemCount == currentSize);
			printf(" - %d elements left!\n\n", currentSize);
#if defined(_MSC_VER)
			_ASSERTE(_CrtCheckMemory());
#endif
		}

		free(heapBuffer);
		free(heapContents);
	}
}
