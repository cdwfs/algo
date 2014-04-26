#include "heap.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define HEAP_VALIDATE(f) {									\
		int32_t error = ( f );								\
		if (0 != error) {									\
			printf("ERROR: %s returned %d\n", #f, error);	\
			assert(0 == error);								\
		}													\
	}

static void testHeapInsert(Heap heap, int32_t heapContents[])
{
	int32_t capacity = -1;
	HEAP_VALIDATE( heapCapacity(heap, &capacity) );
	int32_t beforeSize = -1, afterSize = -1;
	HEAP_VALIDATE( heapCurrentSize(heap, &beforeSize) );
	if (beforeSize == capacity)
	{
		return; // heap is full
	}
	int32_t newKey = rand() % capacity;
	HEAP_VALIDATE( heapInsert(heap, newKey, (const void*)(intptr_t)newKey) );

	// Not a heap requirement; just making sure it's a valid index for
	// heapContents[].
	assert(newKey < capacity); 
	                           
	heapContents[newKey] += 1;

	HEAP_VALIDATE( heapCurrentSize(heap, &afterSize) );
	assert(beforeSize+1 == afterSize);
	HEAP_VALIDATE( heapCheck(heap) );
}

static void testHeapPop(Heap heap, int32_t heapContents[])
{
	int32_t capacity = -1;
	HEAP_VALIDATE( heapCapacity(heap, &capacity) );
	int32_t beforeSize = -1, afterSize = -1;
	HEAP_VALIDATE( heapCurrentSize(heap, &beforeSize) );
	if (beforeSize == 0)
	{
		return; // heap is empty
	}
	int32_t minKey = -1;
	const void *minData = NULL;
	HEAP_VALIDATE( heapPeek(heap, &minKey, &minData) );
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

	HEAP_VALIDATE( heapPop(heap) );
	heapContents[minKey] -= 1;

	HEAP_VALIDATE( heapCurrentSize(heap, &afterSize) );
	assert(beforeSize-1 == afterSize);
	HEAP_VALIDATE( heapCheck(heap) );
}

int main(void)
{
	srand((unsigned int)time(NULL));

	const int32_t kHeapCapacity = 16*1024;
	int32_t *heapContents = heapContents = malloc(kHeapCapacity*sizeof(int32_t));
	memset(heapContents, 0, kHeapCapacity*sizeof(int32_t));

	Heap heap;
	int currentSize = 0;
	HEAP_VALIDATE( heapCreate(&heap, kHeapCapacity) );
	HEAP_VALIDATE( heapCurrentSize(heap, &currentSize) );
	assert(0 == currentSize);
	for(;;)
	{
		int32_t numAdds = rand() % (kHeapCapacity-currentSize);
		printf("Adding %d elements...\n", numAdds);
		for(int32_t iAdd=0; iAdd<numAdds; ++iAdd)
		{
			testHeapInsert(heap, heapContents);
		}

		HEAP_VALIDATE( heapCurrentSize(heap, &currentSize) );
		int32_t numPops = 1 + rand() % currentSize;
		printf("Popping %d elements...\n", numPops);
		for(int32_t iPop=0; iPop<numPops; ++iPop)
		{
			testHeapPop(heap, heapContents);
		}

		HEAP_VALIDATE( heapCurrentSize(heap, &currentSize) );
		int32_t elemCount = 0;
		for(int32_t iVal=0; iVal<kHeapCapacity; ++iVal)
		{
			elemCount += heapContents[iVal];
		}
		assert(elemCount == currentSize);
		printf("%d elements left!\n\n", currentSize);
	}

	free(heapContents);
}
