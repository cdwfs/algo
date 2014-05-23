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

static void testQueueInsert(AlgoQueue queue, AlgoQueueData elem)
{
	int32_t capacity = -1;
	ALGO_VALIDATE( algoQueueCapacity(queue, &capacity) );

	int32_t beforeSize = -1, afterSize = -1;
	ALGO_VALIDATE( algoQueueCurrentSize(queue, &beforeSize) );
	if (beforeSize == capacity)
	{
		return; // queue is full
	}

	ALGO_VALIDATE( algoQueueInsert(queue, elem) );

	ALGO_VALIDATE( algoQueueCurrentSize(queue, &afterSize) );
	assert(beforeSize+1 == afterSize);
}

static AlgoQueueData testQueueRemove(AlgoQueue queue)
{
	int32_t beforeSize = -1, afterSize = -1;
	ALGO_VALIDATE( algoQueueCurrentSize(queue, &beforeSize) );
	if (beforeSize == 0)
	{
		return -1; // queue is empty
	}

	AlgoQueueData elem;
	ALGO_VALIDATE( algoQueueRemove(queue, &elem) );

	ALGO_VALIDATE( algoQueueCurrentSize(queue, &afterSize) );
	assert(beforeSize-1 == afterSize);

	return elem;
}


///////////////////////
// Test AlgoHeap
///////////////////////

static void testHeapInsert(AlgoHeap heap, int32_t heapContents[])
{
	int32_t capacity = -1;
	ALGO_VALIDATE( algoHeapCapacity(heap, &capacity) );
	int32_t beforeSize = -1, afterSize = -1;
	ALGO_VALIDATE( algoHeapCurrentSize(heap, &beforeSize) );
	if (beforeSize == capacity)
	{
		return; // heap is full
	}
	int32_t newKey = rand() % capacity;
	ALGO_VALIDATE( algoHeapInsert(heap, newKey, (const void*)(intptr_t)newKey) );

	// Not a heap requirement; just making sure it's a valid index for
	// heapContents[].
	assert(newKey < capacity); 
	                           
	heapContents[newKey] += 1;

	ALGO_VALIDATE( algoHeapCurrentSize(heap, &afterSize) );
	assert(beforeSize+1 == afterSize);
	ALGO_VALIDATE( algoHeapCheck(heap) );
}

static void testHeapPop(AlgoHeap heap, int32_t heapContents[])
{
	int32_t capacity = -1;
	ALGO_VALIDATE( algoHeapCapacity(heap, &capacity) );
	int32_t beforeSize = -1, afterSize = -1;
	ALGO_VALIDATE( algoHeapCurrentSize(heap, &beforeSize) );
	if (beforeSize == 0)
	{
		return; // heap is empty
	}
	int32_t minKey = -1;
	const void *minData = NULL;
	ALGO_VALIDATE( algoHeapPeek(heap, &minKey, &minData) );
	// key and data must match (in this test environment)
	assert((intptr_t)minKey == (intptr_t)minData);
	// Not a heap requirement; just making sure it's a valid index for
	// heapContents[].
	assert(minKey < capacity);
	// Make sure minKey is the smallest key in the heap (all counters below
	// it must be zero)
	for(int iVal=0; iVal<minKey; ++iVal)
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
}

int main(void)
{
	srand((unsigned int)time(NULL));

	// Test AlgoQueue
	{
		const int32_t kTestElemCount = 1024*1024;
		const int32_t kQueueCapacity = 16*1024;
		printf("Testing AlgoQueue (capacity: %d, test count: %d)\n", kQueueCapacity, kTestElemCount);
		AlgoQueueData *testElements = malloc(kTestElemCount*sizeof(AlgoQueueData));
		for(int iElem=0; iElem<kTestElemCount; ++iElem)
		{
			testElements[iElem] = (AlgoQueueData)rand();
		}
		int32_t nextToAdd = 0, nextToCheck = 0;

		AlgoQueue queue;
		ALGO_VALIDATE( algoQueueCreate(&queue, kQueueCapacity) );

		int32_t currentSize = -1;
		ALGO_VALIDATE( algoQueueCurrentSize(queue, &currentSize) );
		assert(0 == currentSize);

		while (nextToCheck < kTestElemCount)
		{
			const int32_t numAdds = min( 1 + (rand() % (kQueueCapacity-currentSize)), kTestElemCount-nextToAdd );
			assert(numAdds >= 0);
			printf(" - Inserting %d elements...\n", numAdds);
			for(int32_t iAdd=0; iAdd<numAdds; ++iAdd)
			{
				testQueueInsert(queue, testElements[nextToAdd]);
				nextToAdd += 1;
			}
			ALGO_VALIDATE( algoQueueCurrentSize(queue, &currentSize) );
			if (currentSize > kQueueCapacity)
			{
				fprintf(stderr, "ERROR: Queue size exceeds capacity\n");
				assert(currentSize <= kQueueCapacity);
			}


			const int32_t numRemoves = 1 + (rand() % currentSize);
			assert(numRemoves > 0);
			printf(" - Removing %d elements...\n", numRemoves);
			for(int32_t iRemove=0; iRemove<numRemoves; ++iRemove)
			{
				AlgoQueueData elem = testQueueRemove(queue);
				if (elem != testElements[nextToCheck])
				{
					fprintf(stderr, "ERROR: Queue element mismatch\n");
					assert(elem == testElements[nextToCheck]);
				}
				nextToCheck += 1;
			}
			ALGO_VALIDATE( algoQueueCurrentSize(queue, &currentSize) );
			if (currentSize > kQueueCapacity)
			{
				fprintf(stderr, "ERROR: Queue size exceeds capacity\n");
				assert(currentSize <= kQueueCapacity);
			}

			printf(" - %d elements left to check\n\n", kTestElemCount - nextToCheck);
		}

		// TODO: algoQueueDestroy()
		free(testElements);
	}

	// Test AlgoHeap
	{
		const int32_t kHeapCapacity = 16*1024;
		const int32_t kTestCount = 100;
		printf("Testing AlgoHeap (capacity: %d, test count: %d)\n", kHeapCapacity, kTestCount);
		int32_t *heapContents = heapContents = malloc(kHeapCapacity*sizeof(int32_t));
		memset(heapContents, 0, kHeapCapacity*sizeof(int32_t));

		AlgoHeap heap;
		int currentSize = 0;
		ALGO_VALIDATE( algoHeapCreate(&heap, kHeapCapacity) );
		ALGO_VALIDATE( algoHeapCurrentSize(heap, &currentSize) );
		assert(0 == currentSize);
		for(int iHeapTest=0; iHeapTest<kTestCount; ++iHeapTest)
		{
			int32_t numAdds = rand() % (kHeapCapacity-currentSize);
			printf(" - Adding %d elements...\n", numAdds);
			for(int32_t iAdd=0; iAdd<numAdds; ++iAdd)
			{
				testHeapInsert(heap, heapContents);
			}

			ALGO_VALIDATE( algoHeapCurrentSize(heap, &currentSize) );
			int32_t numPops = 1 + (rand() % currentSize);
			printf(" - Popping %d elements...\n", numPops);
			for(int32_t iPop=0; iPop<numPops; ++iPop)
			{
				testHeapPop(heap, heapContents);
			}

			ALGO_VALIDATE( algoHeapCurrentSize(heap, &currentSize) );
			int32_t elemCount = 0;
			for(int32_t iVal=0; iVal<kHeapCapacity; ++iVal)
			{
				elemCount += heapContents[iVal];
			}
			assert(elemCount == currentSize);
			printf(" - %d elements left!\n\n", currentSize);
		}

		// TODO: algoHeapDestroy(heap)
		free(heapContents);
	}
}
