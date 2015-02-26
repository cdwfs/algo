#include "test_common.h"

typedef struct Person
{
	const char *name;
	int32_t vertexId;
} Person;

enum {
	kCort = 0,
	kKen = 1,
	kNat = 2,
	kBryan = 3,
	kBrian = 4,
	kCharlie = 5,
	kTom = 6,
	kElaine = 7,
	kAlison = 8,
	kBen = 9,

	kNumPeople
};

static Person people[kNumPeople] = {
	{"cort", -1},
	{"kenneths", -1},
	{"npm", -1},
	{"babailey", -1},
	{"bolson", -1},
	{"cballowe", -1},
	{"tmaher", -1},
	{"eramundo", -1},
	{"alison", -1},
	{"biy", -1},
};

static void processPersonEarly(AlgoGraph graph, int32_t personId)
{
	(void)graph;
	assert(personId >= 0 && personId < kNumPeople);
	printf("begin processing %s:\n", people[personId].name);
}
static void processEdge(AlgoGraph graph, int32_t p0, int32_t p1)
{
	(void)graph;
	assert(p0 >= 0 && p0 < kNumPeople);
	assert(p1 >= 0 && p1 < kNumPeople);
	printf("\tedge to %s\n", people[p1].name);
}
static void processPersonLate(AlgoGraph graph, int32_t personId)
{
	(void)graph;
	assert(personId >= 0 && personId < kNumPeople);
	printf(" done processing %s\n", people[personId].name);
}

/* Represent a TicTacToe board as a uint32_t. An X in a cell corresponds to the following bits being set/clear:
 *    0|1|2
 *    -+-+-
 *    3|4|5
 *    -+-+-
 *    6|7|8
 * An O in a cell corresponds to the same indices plus 9.
 */
typedef uint32_t TicTacToeState;
static int tttIsValid(TicTacToeState state)
{
	return (state < (1<<18)) && ( ((state>>9) & 0x1FF) & ((state>>0) & 0x1FF) ) == 0;
}
static char tttGetNextPlayer(const TicTacToeState state)
{
	return (__popcnt(state) % 2) ? 'O' : 'X';
}
static char tttGetCell(const TicTacToeState state, int cellIndex)
{
	return ((state>> 0) & (1<<cellIndex)) ? 'X' : (((state>>9) & (1<<cellIndex)) ? 'O' : ' ');
}
static TicTacToeState tttSetCell(const TicTacToeState state, uint32_t cellIndex, char value)
{
	assert(value == 'X' || value == 'x' || value == 'O' || value == 'o');
	assert(tttIsValid(state));
	assert(cellIndex<9);
	assert(tttGetCell(state, cellIndex) == ' '); /* cell must be empty */
	cellIndex += ((value == 'X' || value == 'x') ? 0 : 9);
	return state | (1<<cellIndex);
}
static char tttGetWinner(const TicTacToeState state)
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
	assert(tttIsValid(state));
	int iMask;
	for(iMask=0; iMask<8; ++iMask)
	{
		if ( ((state>>0) & victoryMasks[iMask]) == victoryMasks[iMask] )
			return 'X'; /* winner is X */
		if ( ((state>>9) & victoryMasks[iMask]) == victoryMasks[iMask] )
			return 'O'; /* winner is O */
	}
	return ' '; /* No winner */
}
static void tttPrint(const TicTacToeState state)
{
	printf("%c|%c|%c\n-+-+-\n", tttGetCell(state, 0), tttGetCell(state, 1), tttGetCell(state, 2));
	printf("%c|%c|%c\n-+-+-\n", tttGetCell(state, 3), tttGetCell(state, 4), tttGetCell(state, 5));
	printf("%c|%c|%c\n\n",      tttGetCell(state, 6), tttGetCell(state, 7), tttGetCell(state, 8));
}

static void addMovesForState(AlgoGraph graph, int32_t stateVertexIds[], int32_t vertexId, char player)
{
	TicTacToeState state;
	ALGO_VALIDATE( algoGraphGetVertexData(graph, vertexId, (AlgoData*)&state) );
	int iCell = 0;
	for(iCell = 0; iCell<9; ++iCell)
	{
		/** If iCell is not empty:
				continue
			nextState = setCell(iCell, player)
			recurse = (nextState is not a winning state)
			If a vertex does not exist for nextState:
				nextVertexId = addVertex(nextState)
				recurse = true
			Add an edge from vertexId to nextVertexId
			if recurse:
				addMovesForState(graph, nextVertexId, !player)
			*/
		if (tttGetCell(state, iCell) != ' ')
			continue;
		TicTacToeState nextState = tttSetCell(state, iCell, player);
		char nextStateWinner = tttGetWinner(nextState);
		int nextStateIsNew = 0;
		int32_t nextVertexId = stateVertexIds[nextState];
		if (nextVertexId == -1)
		{
			ALGO_VALIDATE( algoGraphAddVertex(graph, algoDataFromInt(nextState), &nextVertexId) );
			stateVertexIds[nextState] = nextVertexId;
			nextStateIsNew = 1;
		}
		ALGO_VALIDATE( algoGraphAddEdge(graph, vertexId, nextVertexId) );
		if (nextStateIsNew && (nextStateWinner == ' '))
		{
			addMovesForState(graph, stateVertexIds, nextVertexId, (player == 'X') ? 'O' : 'X');
		}
	}
}

static void dfsValidateVertex(AlgoGraph graph, int32_t vertexId)
{
	TicTacToeState state = 0;
	ALGO_VALIDATE( algoGraphGetVertexData(graph, vertexId, (AlgoData*)&state) );
	assert( tttIsValid(state) );
	char winner = tttGetWinner(state);
	int32_t degree = -1;
	ALGO_VALIDATE( algoGraphGetVertexDegree(graph, vertexId, &degree) );
	assert(degree <= 9);
	assert(winner == ' ' || degree == 0);
	int32_t edges[9];
	ALGO_VALIDATE( algoGraphGetVertexEdges(graph, vertexId, degree, edges) );
	char nextPlayer = tttGetNextPlayer(state);
	for(int iEdge=0; iEdge<degree; ++iEdge)
	{
		TicTacToeState nextState = 0;
		ALGO_VALIDATE( algoGraphGetVertexData(graph, edges[iEdge], (AlgoData*)&nextState) );
		char nextNextPlayer = tttGetNextPlayer(nextState);
		assert(nextPlayer != nextNextPlayer);
		(void)nextPlayer;
		(void)nextNextPlayer;
	}
}
static void testTicTacToe(void)
{
	AlgoGraph graph;
	void *graphBuffer = NULL;
	size_t graphBufferSize = 0;
#if 0 /* conservative estimates */
	const int32_t kVertexCapacity = 3*3*3 * 3*3*3 * 3*3*3; /* 9 cells, 3 possible values per cell = 3^9 states */
	const int32_t kEdgeCapacity   = 9*8*7*6*5*4*3*2*1; // 9 choices for the first move, 8 for the next, etc. This is conservative, but reasonable. */
#else /* exact values -- smaller due to reusing state vertices & victory terminating the tree. */
	const int32_t kVertexCapacity = 5478;
	const int32_t kEdgeCapacity   = 16167;
#endif
	ALGO_VALIDATE( algoGraphBufferSize(&graphBufferSize, kVertexCapacity, kEdgeCapacity, kAlgoGraphEdgeDirected) );
	graphBuffer = malloc(graphBufferSize);
	ALGO_VALIDATE( algoGraphCreate(&graph, kVertexCapacity, kEdgeCapacity, kAlgoGraphEdgeDirected, graphBuffer, graphBufferSize) );
	ALGO_VALIDATE( algoGraphValidate(graph) );

	const size_t kNumStatesConservative = 1 << (9+9); /* all possible states, even illegal ones. */
	int32_t *stateToVertexId = (int32_t*)malloc(kNumStatesConservative*sizeof(int32_t));
	for(int iState=0; iState<kNumStatesConservative; ++iState)
	{
		stateToVertexId[iState] = -1;
	}
	
	TicTacToeState startState = 0;
	ALGO_VALIDATE( algoGraphAddVertex(graph, algoDataFromInt(startState), stateToVertexId+startState) );
	addMovesForState(graph, stateToVertexId, stateToVertexId[startState], 'X');

	size_t dfsBufferSize = 0;
	ALGO_VALIDATE( algoGraphDfsBufferSize(&dfsBufferSize, graph) );
	void *dfsBuffer = malloc(dfsBufferSize);
	int32_t *vertexParents = malloc(kVertexCapacity*sizeof(int32_t));
	/** TODO:
		Add algoGraphSetEdgeData(), algoGraphGetEdgeData()
		Add void parameters to AlgoGraphProcessVertexFunc and AlgoGraphProcessEdgeFunc
		lateFunc:
			winner = tttGetWinner();
			if winner != ' ':
				assert(degree == 0)
				mark vert as a winner
				return
			else:
				for each edge:
					if edge destination's state is a winner:
						set edge data as a winner

		*/		
	ALGO_VALIDATE( algoGraphDfs(graph, stateToVertexId[startState], vertexParents, kVertexCapacity,
		dfsValidateVertex, NULL, NULL, dfsBuffer, dfsBufferSize) );
	free(dfsBuffer);

	free(stateToVertexId);
	free(graphBuffer);
}

int main(void)
{
	unsigned int randomSeed = (unsigned int)time(NULL);
	int32_t kVertexCapacity;
	int32_t kEdgeCapacity;
	void *graphBuffer = NULL;
	size_t graphBufferSize = 0;
	const int32_t kTestCount = 100;
	AlgoGraph graph;
	AlgoGraphEdgeMode edgeMode;

	printf("Random seed: 0x%08X\n", randomSeed);
	srand(randomSeed);

	testTicTacToe();

	kVertexCapacity = kNumPeople;
	kEdgeCapacity = 8;
	edgeMode = kAlgoGraphEdgeUndirected; /*(rand() % 2) ? kAlgoGraphEdgeDirected : kAlgoGraphEdgeUndirected; */
	printf("Testing AlgoGraph (vertexCapacity: %d, edgeCapacity: %d, test count: %d)\n", kVertexCapacity, kEdgeCapacity, kTestCount);

	ALGO_VALIDATE( algoGraphBufferSize(&graphBufferSize, kVertexCapacity, kEdgeCapacity, edgeMode) );
	graphBuffer = malloc(graphBufferSize);
	ALGO_VALIDATE( algoGraphCreate(&graph, kVertexCapacity, kEdgeCapacity, edgeMode, graphBuffer, graphBufferSize) );
	ALGO_VALIDATE( algoGraphValidate(graph) );

	/* Hard-code some graph data */
	/* TODO: test directed graphs as well */
	{
		int32_t iPerson;
		for(iPerson=0; iPerson<kNumPeople; iPerson += 1)
		{
			ALGO_VALIDATE( algoGraphAddVertex(graph, algoDataFromPtr(people+iPerson), &(people[iPerson].vertexId)) );
		}

		ALGO_VALIDATE( algoGraphAddEdge(graph, people[kCort].vertexId, people[kKen].vertexId) );
		ALGO_VALIDATE( algoGraphAddEdge(graph, people[kCort].vertexId, people[kCharlie].vertexId) );
		ALGO_VALIDATE( algoGraphAddEdge(graph, people[kCort].vertexId, people[kBen].vertexId) );

		ALGO_VALIDATE( algoGraphAddEdge(graph, people[kNat].vertexId, people[kBrian].vertexId) );
		ALGO_VALIDATE( algoGraphAddEdge(graph, people[kNat].vertexId, people[kBryan].vertexId) );
		ALGO_VALIDATE( algoGraphAddEdge(graph, people[kNat].vertexId, people[kKen].vertexId) );

		ALGO_VALIDATE( algoGraphAddEdge(graph, people[kKen].vertexId, people[kBrian].vertexId) );

		ALGO_VALIDATE( algoGraphAddEdge(graph,    people[kCort].vertexId, people[kElaine].vertexId) );
		ALGO_VALIDATE( algoGraphRemoveEdge(graph, people[kElaine].vertexId, people[kCort].vertexId) );

		ALGO_VALIDATE( algoGraphAddEdge(graph, people[kElaine].vertexId, people[kAlison].vertexId) );
	}
	ALGO_VALIDATE( algoGraphValidate(graph) );

	/* Query the graph */
	{
		int32_t iPerson;
		printf("Manual graph queries:\n");
		for(iPerson=0; iPerson<kNumPeople; iPerson += 1)
		{
			int32_t degree, iRoommate;
			int32_t roommateIds[kNumPeople];
			ALGO_VALIDATE( algoGraphGetVertexDegree(graph, people[iPerson].vertexId, &degree) );
			printf("%s had %d roommates:\n", people[iPerson].name, degree);
			ALGO_VALIDATE( algoGraphGetVertexEdges(graph, people[iPerson].vertexId, degree, roommateIds) );
			for(iRoommate=0; iRoommate<degree; ++iRoommate)
			{
				Person *roommate = NULL;
				ALGO_VALIDATE( algoGraphGetVertexData(graph, roommateIds[iRoommate], (AlgoData*)&roommate) );
				printf("\t%s\n", roommate->name);
			}
		}
	}
	ALGO_VALIDATE( algoGraphValidate(graph) );

	/* BFS */
	{
		void *bfsBuffer = NULL;
		size_t bfsBufferSize = 0;
		int32_t *parents = (int32_t*)malloc(kNumPeople*sizeof(int32_t));
		int iPerson;
		ALGO_VALIDATE( algoGraphBfsBufferSize(&bfsBufferSize, graph) );
		bfsBuffer = malloc(bfsBufferSize);
		const int32_t bfsRoot = kCort;
		printf("\n\nBFS search from %s...\n", people[bfsRoot].name);
		ALGO_VALIDATE( algoGraphBfs(graph, people[kCort].vertexId, parents, kNumPeople,
			processPersonEarly, processEdge, processPersonLate, bfsBuffer, bfsBufferSize) );
		for(iPerson=0; iPerson<kNumPeople; iPerson += 1)
		{
			printf("%s's parent is %s\n", people[iPerson].name,
				parents[iPerson] >= 0 ? people[parents[iPerson]].name : "N/A");
		}
		free(parents);
		free(bfsBuffer);
	}
	ALGO_VALIDATE( algoGraphValidate(graph) );

	/* DFS */
	{
		void *dfsBuffer = NULL;
		size_t dfsBufferSize = 0;
		int32_t *parents = (int32_t*)malloc(kNumPeople*sizeof(int32_t));
		int iPerson;
		ALGO_VALIDATE( algoGraphDfsBufferSize(&dfsBufferSize, graph) );
		dfsBuffer = malloc(dfsBufferSize);
		const int32_t bfsRoot = kCort;
		printf("\n\nDFS search from %s...\n", people[bfsRoot].name);
		ALGO_VALIDATE( algoGraphDfs(graph, people[kCort].vertexId, parents, kNumPeople,
			processPersonEarly, processEdge, processPersonLate, dfsBuffer, dfsBufferSize) );
		for(iPerson=0; iPerson<kNumPeople; iPerson += 1)
		{
			printf("%s's parent is %s\n", people[iPerson].name,
				parents[iPerson] >= 0 ? people[parents[iPerson]].name : "N/A");
		}
		free(parents);
		free(dfsBuffer);
	}
	ALGO_VALIDATE( algoGraphValidate(graph) );

	{
		int32_t iPerson;
		printf("\n\nRemoving cort...\n");
		ALGO_VALIDATE( algoGraphRemoveVertex(graph, people[kCort].vertexId) );
		printf("Manual graph queries:\n");
		for(iPerson=0; iPerson<kNumPeople; iPerson += 1)
		{
			int32_t degree, iRoommate;
			int32_t roommateIds[kNumPeople];
			if (iPerson == kCort)
				continue; /* gone now! */
			ALGO_VALIDATE( algoGraphGetVertexDegree(graph, people[iPerson].vertexId, &degree) );
			printf("%s had %d roommates:\n", people[iPerson].name, degree);
			ALGO_VALIDATE( algoGraphGetVertexEdges(graph, people[iPerson].vertexId, degree, roommateIds) );
			for(iRoommate=0; iRoommate<degree; ++iRoommate)
			{
				Person *roommate = NULL;
				ALGO_VALIDATE( algoGraphGetVertexData(graph, roommateIds[iRoommate], (AlgoData*)&roommate) );
				printf("\t%s\n", roommate->name);
			}
		}
	}
	ALGO_VALIDATE( algoGraphValidate(graph) );

	free(graphBuffer);
}
