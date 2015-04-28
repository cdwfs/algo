#include "test_common.h"

static int testStackPush(AlgoStack stack, const AlgoData elem)
{
	int32_t capacity = -1;
	int32_t beforeSize = -1, afterSize = -1;
	ALGO_VALIDATE( algoStackGetCapacity(stack, &capacity) );

	ALGO_VALIDATE( algoStackGetCurrentSize(stack, &beforeSize) );
	if (beforeSize == capacity)
	{
		return 0; /* stack is full */
	}

	ALGO_VALIDATE( algoStackPush(stack, elem) );

	ALGO_VALIDATE( algoStackGetCurrentSize(stack, &afterSize) );
	ZOMBO_ASSERT(beforeSize+1 == afterSize, "stack grew by more than one element");
	return 1;
}

static int testStackPop(AlgoStack stack, AlgoData *outElem)
{
	int32_t beforeSize = -1, afterSize = -1;
	ALGO_VALIDATE( algoStackGetCurrentSize(stack, &beforeSize) );
	if (beforeSize == 0)
	{
		outElem->asInt = -1;
		return 0; /* stack is empty */
	}

	ALGO_VALIDATE( algoStackPop(stack, outElem) );

	ALGO_VALIDATE( algoStackGetCurrentSize(stack, &afterSize) );
	ZOMBO_ASSERT(beforeSize-1 == afterSize, "stack shrunk by more than one element");

	return 1;
}

int main(void)
{
	unsigned int randomSeed = (unsigned int)time(NULL);
	int32_t kStackCapacity = 0;
	void *stackBuffer = NULL;
	size_t stackBufferSize = 0;
	int32_t nextToAdd = 0, nextToCheck = 0;
	AlgoStack stack;
	int32_t currentSize = -1;

	printf("Random seed: 0x%08X\n", randomSeed);
	srand(randomSeed);
	for(;;)
	{
		nextToAdd = 0;
		nextToCheck = 0;
		kStackCapacity = 1 + (rand() % 1024);
		printf("Testing AlgoStack (capacity: %d)\n", kStackCapacity);

		nextToCheck = kStackCapacity-1;
		ALGO_VALIDATE( algoStackComputeBufferSize(&stackBufferSize, kStackCapacity) );
		stackBuffer = malloc(stackBufferSize);
		ALGO_VALIDATE( algoStackCreate(&stack, kStackCapacity, stackBuffer, stackBufferSize) );

		ALGO_VALIDATE( algoStackGetCurrentSize(stack, &currentSize) );
		ZOMBO_ASSERT(0 == currentSize, "newly created stack has size=%d", currentSize);

		/* Make sure we can't remove elements from an empty stack. */
		{
			AlgoError err;
			AlgoData elem;
			ALGO_VALIDATE( algoStackGetCurrentSize(stack, &currentSize) );
			err = algoStackPop(stack, &elem);
			ZOMBO_ASSERT(err == kAlgoErrorOperationFailed, "ERROR: algoStackPop() on an empty stack returned %d (expected %d)\n",
						 err, kAlgoErrorOperationFailed);
		}

		/* Fill the stack. */
		while (nextToAdd < kStackCapacity)
		{
			testStackPush(stack, algoDataFromInt(nextToAdd++));
		}

		/* Make sure we can't add elements to a full stack. */
		{
			AlgoError err;
			ALGO_VALIDATE( algoStackGetCurrentSize(stack, &currentSize) );
			ZOMBO_ASSERT(currentSize == kStackCapacity, "full stack has currentSize=%d, capacity=%d", currentSize, kStackCapacity);
			err = algoStackPush(stack, algoDataFromInt(0));
			ZOMBO_ASSERT(err == kAlgoErrorOperationFailed, "ERROR: algoStackPush() on a full stack returned %d (expected %d)\n",
						 err, kAlgoErrorOperationFailed);
		}

		/* Empty the stack. */
		while (nextToCheck >= 0)
		{
			AlgoData elem = algoDataFromInt(-1);
			testStackPop(stack, &elem);
			ZOMBO_ASSERT(elem.asInt == nextToCheck, "ERROR: algoStackPop() retrieved unexpected value %d (expected %d)\n",
						 elem.asInt, nextToCheck);
			nextToCheck -= 1;
		}

		/* Make sure we can't remove elements from an empty stack. */
		{
			AlgoError err;
			AlgoData elem;
			ALGO_VALIDATE( algoStackGetCurrentSize(stack, &currentSize) );
			ZOMBO_ASSERT(currentSize == 0, "empty stack has size=%d", currentSize);
			err = algoStackPop(stack, &elem);
			ZOMBO_ASSERT(err == kAlgoErrorOperationFailed, "ERROR: algoStackPop() on an empty stack returned %d (expected %d)\n",
						 err, kAlgoErrorOperationFailed);
		}

	#if defined(_MSC_VER)
		_ASSERTE(_CrtCheckMemory());
	#endif
		free(stackBuffer);
		printf("No errors detected\n\n");
	}
}
