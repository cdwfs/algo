#include "test_common.h"

typedef struct Allocation
{
	int32_t id;
	size_t blockBytes;
	uint8_t byte;
	uint8_t *block;
} Allocation;

static void allocationInit(Allocation *outAlloc, int32_t id)
{
	outAlloc->blockBytes = 0;
	outAlloc->byte = rand() & 0xFF;
	outAlloc->block = NULL;
}
static void allocationSetBlock(Allocation *outAlloc, void *newBlock, size_t newBlockBytes)
{
	outAlloc->block = (uint8_t*)newBlock;
	outAlloc->blockBytes = newBlockBytes;
	if (NULL != newBlock)
	{
		memset(newBlock, outAlloc->byte, newBlockBytes);
	}
	else
	{
		assert(0 == newBlockBytes);
	}
}
static int isAllocationValid(const Allocation *alloc)
{
	int32_t iByte, iMax;
	if (NULL == alloc->block)
	{
		return 1; // NULL allocations are valid
	}
	for(iByte=0,iMax=alloc->blockBytes; iByte<iMax; ++iByte)
	{
		if (alloc->block[iByte] != alloc->byte)
			return 0;
	}
	return 1;
}

int main(void)
{
	unsigned int randomSeed = (unsigned int)time(NULL);
	printf("Random seed: 0x%08X\n", randomSeed);
	srand(randomSeed);

	// Test AlgoAllocPool
	for(;;)
	{
		AlgoAllocPool allocPool;
		const int32_t elemSize = (rand() % 124) + 4;
		const int32_t maxElemCount = (rand() % 1024) + 1;
		int32_t errorCount = 0;
		size_t poolBufferSize = 0;
		void *poolBuffer = NULL;
		Allocation *allocations = malloc(maxElemCount*sizeof(Allocation));
		int32_t iTest, iAlloc;
		ALGO_VALIDATE( algoAllocPoolBufferSize(&poolBufferSize, elemSize, maxElemCount) );
		poolBuffer = malloc(poolBufferSize);
		ALGO_VALIDATE( algoAllocPoolCreate(&allocPool, elemSize, maxElemCount, poolBuffer, poolBufferSize) );
		printf("AllocPool: Total capacity=%4d elements, elemSize=%3d\n", maxElemCount, elemSize);

		for(iAlloc=0; iAlloc<maxElemCount; ++iAlloc)
		{
			allocationInit(allocations+iAlloc, iAlloc);
		}
		for(iTest=0; iTest<1000; ++iTest)
		{
			// Allocate roughly half the available blocks
			for(iAlloc=0; iAlloc<maxElemCount; ++iAlloc)
			{
				if (rand() & 1)
				{
					void *block = algoAllocPoolAlloc(allocPool);
					if(NULL == block)
					{
						printf("\tERROR: shouldn't be returning NULL in the first round of allocations...\n");
						++errorCount;
						assert(0);
					}
					allocationSetBlock(allocations+iAlloc, block, elemSize);
				}
			}
			// Validate allocations
			for(iAlloc=0; iAlloc<maxElemCount; ++iAlloc)
			{
				if (!isAllocationValid(allocations+iAlloc))
				{
					printf("\tERROR: Corruption in alloc #%05d: ptr=%p\n",
						allocations[iAlloc].id, allocations[iAlloc].block);
					++errorCount;
					assert(0);
				}
			}
			// Free half the previous allocations
			for(iAlloc=0; iAlloc<maxElemCount; ++iAlloc)
			{
				if (NULL != allocations[iAlloc].block && (rand() & 1))
				{
					ALGO_VALIDATE( algoAllocPoolFree(allocPool, allocations[iAlloc].block) );
					allocationSetBlock(allocations+iAlloc, NULL, 0);
				}
			}
			// Validate allocations
			for(iAlloc=0; iAlloc<maxElemCount; ++iAlloc)
			{
				if (!isAllocationValid(allocations+iAlloc))
				{
					printf("\tERROR: Corruption in alloc #%05d: ptr=%p\n",
						allocations[iAlloc].id, allocations[iAlloc].block);
					++errorCount;
					assert(0);
				}
			}
			// Allocate all remaining blocks
			for(iAlloc=0; iAlloc<maxElemCount; ++iAlloc)
			{
				if (NULL == allocations[iAlloc].block)
				{
					void *block = algoAllocPoolAlloc(allocPool);
					if(NULL == block)
					{
						printf("\tERROR: shouldn't be returning NULL in the second round of allocations...\n");
						++errorCount;
						assert(0);
					}
					allocationSetBlock(allocations+iAlloc, block, elemSize);
				}
			}
			// Validate allocations
			for(iAlloc=0; iAlloc<maxElemCount; ++iAlloc)
			{
				if (!isAllocationValid(allocations+iAlloc))
				{
					printf("\tERROR: Corruption in alloc #%05d: ptr=%p\n",
						allocations[iAlloc].id, allocations[iAlloc].block);
					++errorCount;
					assert(0);
				}
			}
			// Attempt one more allocation, which SHOULD fail
			{
				void *shouldBeNull = algoAllocPoolAlloc(allocPool);
				if (NULL != shouldBeNull)
				{
					printf("\tERROR: Allocation succeeded from full pool! ptr=%p\n", shouldBeNull);
					++errorCount;
					assert(0);
				}
			}
			// Free all allocations
			for(iAlloc=0; iAlloc<maxElemCount; ++iAlloc)
			{
				ALGO_VALIDATE( algoAllocPoolFree(allocPool, allocations[iAlloc].block) );
				allocationSetBlock(allocations+iAlloc, NULL, 0);
			}
		}
		if (0 == errorCount)
		{
			printf("\tNo errors detected!\n");
		}
		free(allocations);
		free(poolBuffer);

#if defined(_MSC_VER)
		_ASSERTE(_CrtCheckMemory());
#endif
	}
}
