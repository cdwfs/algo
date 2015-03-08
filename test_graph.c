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

static void bfsProcessPersonEarly(AlgoGraph graph, AlgoGraphBfsState bfsState, int32_t personId, void *userData)
{
	(void)graph;
	(void)bfsState;
	(void)userData;
	ZOMBO_ASSERT(personId >= 0 && personId < kNumPeople, "invalid personId %d", personId);
	printf("begin processing %s:\n", people[personId].name);
}
static void bfsProcessEdge(AlgoGraph graph, AlgoGraphBfsState bfsState, int32_t p0, int32_t p1, void *userData)
{
	(void)graph;
	(void)bfsState;
	(void)userData;
	ZOMBO_ASSERT(p0 >= 0 && p0 < kNumPeople, "invalid personId %d", p0);
	ZOMBO_ASSERT(p1 >= 0 && p1 < kNumPeople, "invalid personId %d", p1);
	printf("\tedge to %s\n", people[p1].name);
}
static void bfsProcessPersonLate(AlgoGraph graph, AlgoGraphBfsState bfsState, int32_t personId, void *userData)
{
	(void)graph;
	(void)bfsState;
	(void)userData;
	ZOMBO_ASSERT(personId >= 0 && personId < kNumPeople, "invalid personId %d", personId);
	printf(" done processing %s\n", people[personId].name);
}

static void dfsProcessPersonEarly(AlgoGraph graph, AlgoGraphDfsState dfsState, int32_t personId, void *userData)
{
	(void)graph;
	(void)dfsState;
	(void)userData;
	ZOMBO_ASSERT(personId >= 0 && personId < kNumPeople, "invalid personId %d", personId);
	printf("begin processing %s:\n", people[personId].name);
}
static void dfsProcessEdge(AlgoGraph graph, AlgoGraphDfsState dfsState, int32_t p0, int32_t p1, void *userData)
{
	(void)graph;
	(void)dfsState;
	(void)userData;
	ZOMBO_ASSERT(p0 >= 0 && p0 < kNumPeople, "invalid personId %d", p0);
	ZOMBO_ASSERT(p1 >= 0 && p1 < kNumPeople, "invalid personId %d", p1);
	printf("\tedge to %s\n", people[p1].name);
}
static void dfsProcessPersonLate(AlgoGraph graph, AlgoGraphDfsState dfsState, int32_t personId, void *userData)
{
	(void)graph;
	(void)dfsState;
	(void)userData;
	ZOMBO_ASSERT(personId >= 0 && personId < kNumPeople, "invalid personId %d", personId);
	printf(" done processing %s\n", people[personId].name);
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
				AlgoData vertData;
				ALGO_VALIDATE( algoGraphGetVertexData(graph, roommateIds[iRoommate], &vertData) );
				roommate = vertData.asPtr;
				printf("\t%s\n", roommate->name);
			}
		}
	}
	ALGO_VALIDATE( algoGraphValidate(graph) );

	/* BFS */
	{
		void *bfsStateBuffer = NULL;
		size_t bfsStateBufferSize = 0;
		AlgoGraphBfsState bfsState;
		ALGO_VALIDATE( algoGraphBfsStateBufferSize(&bfsStateBufferSize, graph) );
		bfsStateBuffer = malloc(bfsStateBufferSize);
		ALGO_VALIDATE( algoGraphBfsStateCreate(&bfsState, graph, bfsStateBuffer, bfsStateBufferSize) );
		const int32_t bfsRoot = kCort;
		printf("\n\nBFS search from %s...\n", people[bfsRoot].name);
		AlgoGraphBfsCallbacks bfsCallbacks = {
			bfsProcessPersonEarly, NULL,
			bfsProcessEdge, NULL,
			bfsProcessPersonLate, NULL
		};
		ALGO_VALIDATE( algoGraphBfs(graph, bfsState, people[kCort].vertexId, bfsCallbacks) );
		int32_t *parents = (int32_t*)malloc(kNumPeople*sizeof(int32_t));
		int iPerson;
		for(iPerson=0; iPerson<kNumPeople; iPerson += 1)
		{
			int32_t parent = -1;
			ALGO_VALIDATE( algoGraphBfsStateGetVertexParent(bfsState, people[iPerson].vertexId, &parent) );
			printf("%s's parent is %s\n", people[iPerson].name,
				parent >= 0 ? people[parent].name : "N/A");
		}
		free(parents);
		free(bfsStateBuffer);
	}
	ALGO_VALIDATE( algoGraphValidate(graph) );

	/* DFS */
	{
		void *dfsStateBuffer = NULL;
		size_t dfsStateBufferSize = 0;
		AlgoGraphDfsState dfsState;
		ALGO_VALIDATE( algoGraphDfsStateBufferSize(&dfsStateBufferSize, graph) );
		dfsStateBuffer = malloc(dfsStateBufferSize);
		ALGO_VALIDATE( algoGraphDfsStateCreate(&dfsState, graph, dfsStateBuffer, dfsStateBufferSize) );
		int iPerson;
		const int32_t dfsRoot = kCort;
		printf("\n\nDFS search from %s...\n", people[dfsRoot].name);
		AlgoGraphDfsCallbacks dfsCallbacks = {
			dfsProcessPersonEarly, NULL,
			dfsProcessEdge, NULL,
			dfsProcessPersonLate, NULL
		};
		ALGO_VALIDATE( algoGraphDfs(graph, dfsState, people[kCort].vertexId, dfsCallbacks) );
		for(iPerson=0; iPerson<kNumPeople; iPerson += 1)
		{
			int32_t parent = -1;
			ALGO_VALIDATE( algoGraphDfsStateGetVertexParent(dfsState, people[iPerson].vertexId, &parent) );
			printf("%s's parent is %s\n", people[iPerson].name,
				parent >= 0 ? people[parent].name : "N/A");
		}
		free(dfsStateBuffer);
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
				AlgoData vertData;
				ALGO_VALIDATE( algoGraphGetVertexData(graph, roommateIds[iRoommate], &vertData) );
				roommate = vertData.asPtr;
				printf("\t%s\n", roommate->name);
			}
		}
	}
	ALGO_VALIDATE( algoGraphValidate(graph) );

	free(graphBuffer);
}
