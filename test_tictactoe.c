#include "test_common.h"
#include <math.h>

/* Represent a TicTacToe board as a uint32_t. An X in a cell corresponds to the following bits being set/clear:
 *    0|1|2
 *    -+-+-
 *    3|4|5
 *    -+-+-
 *    6|7|8
 * An O in a cell corresponds to the same indices plus 9.
 */
typedef uint32_t T3State;
static int t3IsValid(T3State state)
{
	return (state < (1<<18)) && ( ((state>>9) & 0x1FF) & ((state>>0) & 0x1FF) ) == 0;
}
static char t3GetNextPlayer(const T3State state)
{
	return (__popcnt(state) % 2) ? 'O' : 'X';
}
static char t3GetCell(const T3State state, int cellIndex)
{
	return ((state>> 0) & (1<<cellIndex)) ? 'X' : (((state>>9) & (1<<cellIndex)) ? 'O' : ' ');
}
static T3State t3SetCell(const T3State state, uint32_t cellIndex, char value)
{
	assert(value == 'X' || value == 'x' || value == 'O' || value == 'o');
	assert(t3IsValid(state));
	assert(cellIndex<9);
	assert(t3GetCell(state, cellIndex) == ' '); /* cell must be empty */
	cellIndex += ((value == 'X' || value == 'x') ? 0 : 9);
	return state | (1<<cellIndex);
}
typedef enum T3Score
{
	kT3ScoreO    = -1,
	kT3ScoreDraw =  0,
	kT3ScoreX    =  1
} T3Score;
static T3Score t3GetScore(const T3State state)
{
	const uint32_t victoryMasks[8] = {
		0x007, /* 0,1,2 */
		0x038, /* 3,4,5 */
		0x1C0, /* 6,7,8 */
		0x049, /* 0,3,6 */
		0x092, /* 1,4,7 */
		0x124, /* 2,5,8 */
		0x111, /* 0,4,8 */
		0x054, /* 2,4,6 */
	};
	assert(t3IsValid(state));
	int iMask;
	for(iMask=0; iMask<8; ++iMask)
	{
		if ( ((state>>0) & victoryMasks[iMask]) == victoryMasks[iMask] )
			return kT3ScoreX;
		if ( ((state>>9) & victoryMasks[iMask]) == victoryMasks[iMask] )
			return kT3ScoreO;
	}
	return kT3ScoreDraw;
}
static int t3IsGameOver(const T3State state)
{
	return t3GetScore(state) != kT3ScoreDraw ||
		(((state>>9) | state) & 0x1FF) == 0x1FF;
}
static void t3Print(const T3State state)
{
	printf("%c|%c|%c\n-+-+-\n", t3GetCell(state, 0), t3GetCell(state, 1), t3GetCell(state, 2));
	printf("%c|%c|%c\n-+-+-\n", t3GetCell(state, 3), t3GetCell(state, 4), t3GetCell(state, 5));
	printf("%c|%c|%c\n",        t3GetCell(state, 6), t3GetCell(state, 7), t3GetCell(state, 8));
}
typedef struct HashEntry_T3State
{
	T3State key;
	struct HashEntry_T3State *next;
	int32_t vertexId;
	T3Score score;
	T3State bestMove;
} HashEntry_T3State;
typedef struct HashTable_T3State
{
	int binCount;
	int entryCapacity;
	int freeEntryIndex;
	HashEntry_T3State **bins;
	HashEntry_T3State *entryPool;
} HashTable_T3State;
static size_t hashBufferSize_T3State(int binCount, int entryCapacity)
{
	return entryCapacity*sizeof(HashEntry_T3State) + binCount*sizeof(HashEntry_T3State*) + sizeof(HashTable_T3State);
}
static HashTable_T3State *hashCreate_T3State(void *buffer, size_t bufferSize, int binCount, int entryCapacity)
{
	size_t expectedBufferSize = hashBufferSize_T3State(binCount, entryCapacity);
	if (bufferSize < expectedBufferSize)
		return NULL;
	uint8_t *bufferNext = (uint8_t*)buffer;

	HashEntry_T3State *entryPool = (HashEntry_T3State*)bufferNext;
	bufferNext += entryCapacity*sizeof(HashEntry_T3State);

	HashEntry_T3State **bins = (HashEntry_T3State**)bufferNext;
	bufferNext += binCount*sizeof(HashEntry_T3State*);
	int iBin;
	for(iBin=0; iBin<binCount; iBin+=1)
		bins[iBin] = NULL;

	HashTable_T3State *outTable = (HashTable_T3State*)bufferNext;
	bufferNext += sizeof(HashTable_T3State);
	assert( (size_t)(bufferNext-(uint8_t*)buffer) == expectedBufferSize );
	outTable->binCount = binCount;
	outTable->entryCapacity = entryCapacity;
	outTable->freeEntryIndex = 0;
	outTable->bins = bins;
	outTable->entryPool = entryPool;
	return outTable;
}
static HashEntry_T3State **hashGetEntryRef_T3State(HashTable_T3State *table, T3State key, int binIndex)
{
	assert(binIndex >= 0 && binIndex < table->binCount);
	HashEntry_T3State **ppEntry;
	for(ppEntry = &(table->bins[binIndex]);
		NULL != *ppEntry;
		ppEntry = &(*ppEntry)->next)
	{
		if (key == (*ppEntry)->key)
			return ppEntry;
	}
	return NULL;
}
static HashEntry_T3State *hashGetEntry_T3State(HashTable_T3State *table, T3State key)
{
	int binIndex = (key ^ (key>>9)) % table->binCount;
	HashEntry_T3State **ppEntry = hashGetEntryRef_T3State(table, key, binIndex);
	return ppEntry ? *ppEntry : NULL;
}
static HashEntry_T3State *hashAddEntry_T3State(HashTable_T3State *table, T3State key)
{
	int binIndex = (key ^ (key>>9)) % table->binCount;
	HashEntry_T3State **ppEntry = hashGetEntryRef_T3State(table, key, binIndex);
	if (NULL != ppEntry)
		return *ppEntry;
	assert(table->freeEntryIndex < table->entryCapacity);
	HashEntry_T3State *newEntry = table->entryPool + table->freeEntryIndex;
	table->freeEntryIndex += 1;
	newEntry->key = key;
	newEntry->next = table->bins[binIndex];
	table->bins[binIndex] = newEntry;
	newEntry->vertexId = -1;
	newEntry->score = kT3ScoreDraw;
	newEntry->bestMove = 0;
	return newEntry;
}
static int isBetterScore(T3Score newScore, T3Score oldScore, char player)
{
	return (player == 'X') ? (newScore >= oldScore) : (oldScore >= newScore);
}
static void addMovesForState(AlgoGraph graph, HashTable_T3State *table, T3State state)
{
	HashEntry_T3State *entry = hashGetEntry_T3State(table, state);
	assert(entry);
	int32_t vertexId = entry->vertexId;
	char player = t3GetNextPlayer(state);
	if (t3IsGameOver(state))
	{
		entry->score = t3GetScore(state);
		entry->bestMove = 0;
		return;
	}
	entry->score = (player == 'X') ? kT3ScoreO : kT3ScoreX;
	int iCell = 0;
	for(iCell = 0; iCell<9; ++iCell)
	{
		if (t3GetCell(state, iCell) != ' ')
			continue;
		T3State nextState = t3SetCell(state, iCell, player);
		int nextStateIsNew = 0;
		HashEntry_T3State *nextEntry = hashGetEntry_T3State(table, nextState);
		if (NULL == nextEntry)
		{
			int32_t nextVertexId;
			ALGO_VALIDATE( algoGraphAddVertex(graph, algoDataFromInt(nextState), &nextVertexId) );
			nextEntry = hashAddEntry_T3State(table, nextState);
			assert(nextEntry);
			nextEntry->vertexId = nextVertexId;
			nextStateIsNew = 1;
		}
		ALGO_VALIDATE( algoGraphAddEdge(graph, vertexId, nextEntry->vertexId) );
		if (nextStateIsNew)
		{
			addMovesForState(graph, table, nextState);
		}
		if (isBetterScore(nextEntry->score, entry->score, player))
		{
			entry->score = nextEntry->score;
			entry->bestMove = nextState;
		}
	}
}

static void dfsValidateVertex(AlgoGraph graph, int32_t vertexId)
{
	T3State state = 0;
	ALGO_VALIDATE( algoGraphGetVertexData(graph, vertexId, (AlgoData*)&state) );
	assert( t3IsValid(state) );
	T3Score score = t3GetScore(state);
	int32_t degree = -1;
	ALGO_VALIDATE( algoGraphGetVertexDegree(graph, vertexId, &degree) );
	assert(degree <= 9);
	assert(score == kT3ScoreDraw || degree == 0);
	/* TODO: count Os and Xs; make sure Xs == Os or Xs == Os+1 */
	int32_t edges[9];
	ALGO_VALIDATE( algoGraphGetVertexEdges(graph, vertexId, degree, edges) );
	char nextPlayer = t3GetNextPlayer(state);
	for(int iEdge=0; iEdge<degree; ++iEdge)
	{
		T3State nextState = 0;
		ALGO_VALIDATE( algoGraphGetVertexData(graph, edges[iEdge], (AlgoData*)&nextState) );
		char nextNextPlayer = t3GetNextPlayer(nextState);
		assert(nextPlayer != nextNextPlayer);
		(void)nextPlayer;
		(void)nextNextPlayer;
	}
}

int main(void)
{
	unsigned int randomSeed = (unsigned int)time(NULL);
	printf("Random seed: 0x%08X\n", randomSeed);
	srand(randomSeed);

	AlgoGraph graph;
	void *graphBuffer = NULL;
	size_t graphBufferSize = 0;
#if 0 /* conservative estimates */
	const int32_t kVertexCapacity = 3*3*3 * 3*3*3 * 3*3*3; /* 9 cells, 3 possible values per cell = 3^9 states */
	const int32_t kEdgeCapacity   = 9*8*7*6*5*4*3*2*1; // 9 choices for the first move, 8 for the next, etc. This is conservative, but reasonable. */
#else /* exact values -- smaller due to illegal states, reusing state vertices, and victory terminating the tree. */
	const int32_t kVertexCapacity = 5478;
	const int32_t kEdgeCapacity   = 16167;
#endif
	ALGO_VALIDATE( algoGraphBufferSize(&graphBufferSize, kVertexCapacity, kEdgeCapacity, kAlgoGraphEdgeDirected) );
	graphBuffer = malloc(graphBufferSize);
	ALGO_VALIDATE( algoGraphCreate(&graph, kVertexCapacity, kEdgeCapacity, kAlgoGraphEdgeDirected, graphBuffer, graphBufferSize) );
	ALGO_VALIDATE( algoGraphValidate(graph) );

	size_t hashTableBufferSize = hashBufferSize_T3State(4096, kVertexCapacity);
	void *hashTableBuffer = malloc(hashTableBufferSize);
	HashTable_T3State *table = hashCreate_T3State(hashTableBuffer, hashTableBufferSize, 4096, kVertexCapacity);
	assert(table);

	T3State startState = 0;
	int32_t startVertexId = 0;
	ALGO_VALIDATE( algoGraphAddVertex(graph, algoDataFromInt(startState), &startVertexId) );
	HashEntry_T3State *startEntry = hashAddEntry_T3State(table, startState);
	startEntry->vertexId = startVertexId;
	addMovesForState(graph, table, startState);

	size_t dfsBufferSize = 0;
	ALGO_VALIDATE( algoGraphDfsBufferSize(&dfsBufferSize, graph) );
	void *dfsBuffer = malloc(dfsBufferSize);
	int32_t *vertexParents = malloc(kVertexCapacity*sizeof(int32_t));
	ALGO_VALIDATE( algoGraphDfs(graph, startVertexId, vertexParents, kVertexCapacity,
		dfsValidateVertex, NULL, NULL, dfsBuffer, dfsBufferSize) );
	free(dfsBuffer);

#if 0 /* just some quicky hash table analysis */
	int iBin=0;
	int maxLength = 0;
	float meanLength = (float)table->entryCapacity / (float)table->binCount;
	float stdDeviation = 0.0f;
	for(iBin=0; iBin<table->binCount; ++iBin)
	{
		HashEntry_T3State *entry = table->bins[iBin];
		int length = 0;
		while(entry)
		{
			length += 1;
			entry = entry->next;
		}
		float variance = (float)length - meanLength;
		stdDeviation += variance*variance;
		if (length > maxLength)
			maxLength = length;
	}
	stdDeviation = sqrtf(stdDeviation / (float)table->binCount);
#endif

	T3State currentState = 0;
	for(;;)
	{
		HashEntry_T3State *entry = hashGetEntry_T3State(table, currentState);
		assert(entry);
		printf("%c's turn:\n", t3GetNextPlayer(currentState));
		t3Print(currentState);
		printf("score: %2d   bestMove: %d\n\n", entry->score, entry->bestMove);
		if (t3IsGameOver(currentState))
			break;
		if (t3GetNextPlayer(currentState) == 'X')
		{
			printf("move [0..8]: ");
			int moveCell = -1;
			do
			{
				scanf_s("%d", &moveCell);
			}
			while(moveCell < 0 || moveCell > 8);
			currentState = t3SetCell(currentState, moveCell, 'X');
		}
		else
		{
			currentState = entry->bestMove;
		}
	}

	free(hashTableBuffer);
	free(graphBuffer);
}