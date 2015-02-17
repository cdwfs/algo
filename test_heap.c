#include "test_common.h"

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
		return 0; /* heap is full */
	}
	ALGO_VALIDATE( algoHeapInsert(heap, newKey, newData) );

	/* Not a heap requirement; just making sure it's a valid index for heapContents[]. */
	assert(newKey.asInt < capacity);
	                           
	heapContents[newKey.asInt] += 1;

	ALGO_VALIDATE( algoHeapCurrentSize(heap, &afterSize) );
	assert(beforeSize+1 == afterSize);
	ALGO_VALIDATE( algoHeapValidate(heap) );
	return 1;
}

static int testHeapPop(AlgoHeap heap, int32_t heapContents[])
{
	int32_t capacity = -1;
	int32_t beforeSize = -1, afterSize = -1;
	AlgoData minKey, minKeyPeek;
	AlgoData minData, minDataPeek;
	int iVal;
	ALGO_VALIDATE( algoHeapCapacity(heap, &capacity) );
	ALGO_VALIDATE( algoHeapCurrentSize(heap, &beforeSize) );
	if (beforeSize == 0)
	{
		return 0; /* heap is empty */
	}
	ALGO_VALIDATE( algoHeapPeek(heap, &minKeyPeek, &minDataPeek) );
	ALGO_VALIDATE( algoHeapPop(heap, &minKey, &minData) );
	/* Peeked data should match popped data */
	assert(minKeyPeek.asInt == minKey.asInt);
	assert(minDataPeek.asInt == minData.asInt);
	/* key and data must match (in this test environment) */
	assert(minKey.asInt == minData.asInt);
	/* Not a heap requirement; just making sure it's a valid index for heapContents[]. */
	assert(minKey.asInt < capacity);
	/* Make sure minKey is the smallest key in the heap (all counters below it must be zero). */
	for(iVal=0; iVal<minKey.asInt; ++iVal)
	{
		assert(heapContents[iVal] == 0);
	}
	/* Make sure minKey is in the heap in the first place */
	assert(heapContents[minKey.asInt] > 0);

	heapContents[minKey.asInt] -= 1;

	ALGO_VALIDATE( algoHeapCurrentSize(heap, &afterSize) );
	assert(beforeSize-1 == afterSize);
	ALGO_VALIDATE( algoHeapValidate(heap) );
	return 1;
}

int main(void)
{
	unsigned int randomSeed = (unsigned int)time(NULL);
	const int32_t kHeapCapacity = 16*1024;
	const int32_t kTestCount = 100;
	int32_t *heapContents = NULL;
	void *heapBuffer = NULL;
	size_t heapBufferSize = 0;
	AlgoHeap heap;
	int currentSize = 0;
	int iHeapTest;
	printf("Random seed: 0x%08X\n", randomSeed);
	srand(randomSeed);
	printf("Testing AlgoHeap (capacity: %d, test count: %d)\n", kHeapCapacity, kTestCount);

	heapContents = malloc(kHeapCapacity*sizeof(int32_t));
	memset(heapContents, 0, kHeapCapacity*sizeof(int32_t));

	ALGO_VALIDATE( algoHeapBufferSize(&heapBufferSize, kHeapCapacity) );
	heapBuffer = malloc(heapBufferSize);
	ALGO_VALIDATE( algoHeapCreate(&heap, kHeapCapacity, algoDataCompareIntAscending, heapBuffer, heapBufferSize) );
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
