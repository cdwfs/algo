#include "heap.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define HEAP_VALIDATE(f) {									\
		int32_t error = ( f );								\
		if (0 != error) {									\
			printf("ERROR: %s returned %d\n", #f, error);	\
			assert(0 == error);								\
		}													\
	}

int main(void)
{
	srand((unsigned int)time(NULL));

	struct Heap *heap = NULL;
	int32_t heapCap = 1024*1024;
	int currentSize = 0;
	HEAP_VALIDATE( heapCreate(&heap, heapCap) );
	HEAP_VALIDATE( heapCurrentSize(heap, &currentSize) );
	assert(0 == currentSize);
	for(;;)
	{
		HEAP_VALIDATE( heapInsert(heap, 3, (void*)3) );
		HEAP_VALIDATE( heapInsert(heap, 1, (void*)1) );
		HEAP_VALIDATE( heapInsert(heap, 4, (void*)4) );
		HEAP_VALIDATE( heapInsert(heap, 1, (void*)1) );
		HEAP_VALIDATE( heapInsert(heap, 5, (void*)5) );
		HEAP_VALIDATE( heapInsert(heap, 9, (void*)9) );
		
		HEAP_VALIDATE( heapCurrentSize(heap, &currentSize) );
		assert(6 == currentSize);

		for(int32_t iElem=0; iElem<currentSize; ++iElem)
		{
			int32_t key = -1;
			HEAP_VALIDATE( heapPeek(heap, &key, NULL) );
			HEAP_VALIDATE( heapPop(heap) );
			printf("%d ", key);
		}
		printf("\n");
		
		HEAP_VALIDATE( heapCurrentSize(heap, &currentSize) );
		assert(0 == currentSize);

		break;
	}
}
