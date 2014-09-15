#include "test_common.h"

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

int main(void)
{
	unsigned int randomSeed = (unsigned int)time(NULL);
	printf("Random seed: 0x%08X\n", randomSeed);
	srand(randomSeed);

	// Test AlgoQueue
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
