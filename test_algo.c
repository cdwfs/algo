#define ALGO_IMPLEMENTATION
#define ALGO_STATIC
#include "algo.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

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

static int testQueueInsert(AlgoQueue queue, AlgoQueueData elem)
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

static int testQueueRemove(AlgoQueue queue, AlgoQueueData *outElem)
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
	int32_t newKey;
	AlgoHeapData heapData;
	ALGO_VALIDATE( algoHeapCapacity(heap, &capacity) );
	ALGO_VALIDATE( algoHeapCurrentSize(heap, &beforeSize) );
	if (beforeSize == capacity)
	{
		return 0; // heap is full
	}
	newKey = rand() % capacity;
	heapData.asInt = newKey;
	ALGO_VALIDATE( algoHeapInsert(heap, newKey, heapData) );

	// Not a heap requirement; just making sure it's a valid index for
	// heapContents[].
	assert(newKey < capacity); 
	                           
	heapContents[newKey] += 1;

	ALGO_VALIDATE( algoHeapCurrentSize(heap, &afterSize) );
	assert(beforeSize+1 == afterSize);
	ALGO_VALIDATE( algoHeapCheck(heap) );
	return 1;
}

static int testHeapPop(AlgoHeap heap, int32_t heapContents[])
{
	int32_t capacity = -1;
	int32_t beforeSize = -1, afterSize = -1;
	int32_t minKey = -1;
	AlgoHeapData minData;
	int iVal;
	ALGO_VALIDATE( algoHeapCapacity(heap, &capacity) );
	ALGO_VALIDATE( algoHeapCurrentSize(heap, &beforeSize) );
	if (beforeSize == 0)
	{
		return 0; // heap is empty
	}
	ALGO_VALIDATE( algoHeapPeek(heap, &minKey, &minData) );
	// key and data must match (in this test environment)
	assert(minKey == minData.asInt);
	// Not a heap requirement; just making sure it's a valid index for
	// heapContents[].
	assert(minKey < capacity);
	// Make sure minKey is the smallest key in the heap (all counters below
	// it must be zero)
	for(iVal=0; iVal<minKey; ++iVal)
	{
		assert(heapContents[iVal] == 0);
	}
	// Make sure minKey is in the heap in the first place
	assert(heapContents[minKey] > 0);

	ALGO_VALIDATE( algoHeapPop(heap) );
	heapContents[minKey] -= 1;

	ALGO_VALIDATE( algoHeapCurrentSize(heap, &afterSize) );
	assert(beforeSize-1 == afterSize);
	ALGO_VALIDATE( algoHeapCheck(heap) );
	return 1;
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
		AlgoQueueData *testElements = malloc(kTestElemCount*sizeof(AlgoQueueData));
		int32_t nextToAdd = 0, nextToCheck = 0;
		AlgoQueue queue;
		int32_t currentSize = -1;
		int iElem;
		printf("Testing AlgoQueue (capacity: %d, test count: %d)\n", kQueueCapacity, kTestElemCount);
		for(iElem=0; iElem<kTestElemCount; ++iElem)
		{
			testElements[iElem].asInt = rand();
		}

		ALGO_VALIDATE( algoQueueCreate(&queue, kQueueCapacity) );

		ALGO_VALIDATE( algoQueueCurrentSize(queue, &currentSize) );
		assert(0 == currentSize);

		// In this test, we alternate between adding a chunk of values to the end of the queue and removing a chunk from the front.
		while (nextToCheck < kTestElemCount)
		{
			const int32_t numAdds = min(kQueueCapacity, kTestElemCount-nextToAdd);
			int32_t numRemoves;
			int iAdd, iRemove;
			assert(numAdds >= 0);
			printf(" - Inserting %d elements...\n", numAdds);
			for(iAdd=0; iAdd<numAdds; ++iAdd)
			{
				nextToAdd += testQueueInsert(queue, testElements[nextToAdd]);
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
				AlgoQueueData elem = {0};
				AlgoError err = algoQueueInsert(queue, elem);
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
				AlgoQueueData elem;
				AlgoError err = algoQueueRemove(queue, &elem);
				if (err != kAlgoErrorOperationFailed)
				{
					fprintf(stderr, "ERROR: algoQueueRemove() on an empty queue returned %d (expected %d)\n",
						err, kAlgoErrorOperationFailed);
					assert(err == kAlgoErrorOperationFailed);
				}
			}

			numRemoves = currentSize;
			printf(" - Removing %d elements...\n", numRemoves);
			for(iRemove=0; iRemove<numRemoves; ++iRemove)
			{
				AlgoQueueData elem;
				int nextToCheckInc = testQueueRemove(queue, &elem);
				if (elem.asInt != testElements[nextToCheck].asInt)
				{
					fprintf(stderr, "ERROR: Queue element mismatch\n");
					assert(elem.asInt == testElements[nextToCheck].asInt);
				}
				nextToCheck += nextToCheckInc;
			}
			ALGO_VALIDATE( algoQueueCurrentSize(queue, &currentSize) );
			if (currentSize > kQueueCapacity)
			{
				fprintf(stderr, "ERROR: Queue size exceeds capacity\n");
				assert(currentSize <= kQueueCapacity);
			}

			printf(" - %d elements left to check\n\n", kTestElemCount - nextToCheck);
		}

		ALGO_VALIDATE( algoQueueDestroy(queue) );
		free(testElements);
	}

	// Test AlgoHeap
	{
		const int32_t kHeapCapacity = 16*1024;
		const int32_t kTestCount = 100;
		int32_t *heapContents = heapContents = malloc(kHeapCapacity*sizeof(int32_t));
		AlgoHeap heap;
		int currentSize = 0;
		int iHeapTest;
		printf("Testing AlgoHeap (capacity: %d, test count: %d)\n", kHeapCapacity, kTestCount);
		memset(heapContents, 0, kHeapCapacity*sizeof(int32_t));

		ALGO_VALIDATE( algoHeapCreate(&heap, kHeapCapacity) );
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
		}

		ALGO_VALIDATE( algoHeapDestroy(heap) );
		free(heapContents);
	}
}
