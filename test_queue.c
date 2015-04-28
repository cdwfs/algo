#include "test_common.h"

static int testQueueInsert(AlgoQueue queue, const AlgoData elem)
{
	int32_t capacity = -1;
	int32_t beforeSize = -1, afterSize = -1;
	ALGO_VALIDATE( algoQueueGetCapacity(queue, &capacity) );

	ALGO_VALIDATE( algoQueueGetCurrentSize(queue, &beforeSize) );
	if (beforeSize == capacity)
	{
		return 0; /* queue is full */
	}

	ALGO_VALIDATE( algoQueueInsert(queue, elem) );

	ALGO_VALIDATE( algoQueueGetCurrentSize(queue, &afterSize) );
	ZOMBO_ASSERT(beforeSize+1 == afterSize, "queue grew by more than one element");
	return 1;
}

static int testQueueRemove(AlgoQueue queue, AlgoData *outElem)
{
	int32_t beforeSize = -1, afterSize = -1;
	ALGO_VALIDATE( algoQueueGetCurrentSize(queue, &beforeSize) );
	if (beforeSize == 0)
	{
		outElem->asInt = -1;
		return 0; /* queue is empty */
	}

	ALGO_VALIDATE( algoQueueRemove(queue, outElem) );

	ALGO_VALIDATE( algoQueueGetCurrentSize(queue, &afterSize) );
	ZOMBO_ASSERT(beforeSize-1 == afterSize, "queue shrunk by more than one element");

	return 1;
}

int main(void)
{
	unsigned int randomSeed = (unsigned int)time(NULL);
	int32_t kTestElemCount = 1024*1024;
	int32_t kQueueCapacity;
	void *queueBuffer = NULL;
	size_t queueBufferSize = 0;
	int32_t nextToAdd = 0, nextToCheck = 0;
	AlgoQueue queue;
	int32_t currentSize = -1;

	printf("Random seed: 0x%08X\n", randomSeed);
	srand(randomSeed);

	kQueueCapacity = 512 + (rand() % 1024);
	printf("Testing AlgoQueue (capacity: %d, test count: %d)\n", kQueueCapacity, kTestElemCount);
	ALGO_VALIDATE( algoQueueComputeBufferSize(&queueBufferSize, kQueueCapacity) );
	queueBuffer = malloc(queueBufferSize);
	ALGO_VALIDATE( algoQueueCreate(&queue, kQueueCapacity, queueBuffer, queueBufferSize) );

	ALGO_VALIDATE( algoQueueGetCurrentSize(queue, &currentSize) );
	ZOMBO_ASSERT(0 == currentSize, "newly created queue has size=%d", currentSize);

	/* In this test, we alternate between adding a chunk of values to
	   the end of the queue and removing a chunk from the front. */
	while (nextToCheck < kTestElemCount)
	{
		const int32_t numAdds = 1 + (rand() % kQueueCapacity);
		int32_t numRemoves;
		int iAdd, iRemove;
		ZOMBO_ASSERT(numAdds >= 0, "numAdds must not be zero");
		printf(" - Inserting at most %d elements...\n", numAdds);
		for(iAdd=0; iAdd<numAdds; ++iAdd)
		{
			nextToAdd += testQueueInsert(queue, algoDataFromInt(nextToAdd));
		}
		ALGO_VALIDATE( algoQueueGetCurrentSize(queue, &currentSize) );
		if (currentSize > kQueueCapacity)
		{
			fprintf(stderr, "ERROR: Queue size exceeds capacity\n");
			ZOMBO_ASSERT(currentSize <= kQueueCapacity, "currentSize (%d) exceeds queue capacity (%d)",
						 currentSize, kQueueCapacity);
		}

		/* Make sure we can't add elements to a full queue. */
		if (currentSize == kQueueCapacity)
		{
			AlgoError err = algoQueueInsert(queue, algoDataFromInt(0));
			ZOMBO_ASSERT(err == kAlgoErrorOperationFailed, "ERROR: algoQueueInsert() on a full queue returned %d (expected %d)\n",
				err, kAlgoErrorOperationFailed);
		}

		/* Make sure we can't remove elements from an empty queue. */
		if (currentSize == 0)
		{
			AlgoData elem;
			AlgoError err = algoQueueRemove(queue, &elem);
			ZOMBO_ASSERT(err == kAlgoErrorOperationFailed, "ERROR: algoQueueRemove() on an empty queue returned %d (expected %d)\n",
					err, kAlgoErrorOperationFailed);
		}

		numRemoves = 1 + (rand() % currentSize);
		printf(" - Removing at most %d elements...\n", numRemoves);
		for(iRemove=0; iRemove<numRemoves; ++iRemove)
		{
			AlgoData elem;
			int nextToCheckInc = testQueueRemove(queue, &elem);
			ZOMBO_ASSERT(elem.asInt == nextToCheck, "ERROR: Queue element mismatch\n");
			nextToCheck += nextToCheckInc;
		}
		ALGO_VALIDATE( algoQueueGetCurrentSize(queue, &currentSize) );
		ZOMBO_ASSERT(currentSize <= kQueueCapacity, "ERROR: Queue size exceeds capacity\n");

		printf(" - %d elements left to check\n\n", max(0, kTestElemCount - nextToCheck));
#if defined(_MSC_VER)
		_ASSERTE(_CrtCheckMemory());
#endif
	}

	free(queueBuffer);
}
