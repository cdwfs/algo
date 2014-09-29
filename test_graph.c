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
	kBen,

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

	/* Hard-code some graph data */
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

		ALGO_VALIDATE( algoGraphAddEdge(graph, people[kElaine].vertexId, people[kAlison].vertexId) );
	}
	
	/* Query the graph */
	{
		int32_t iPerson;
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

	/* BFS! */
	{
		void *bfsBuffer = NULL;
		size_t bfsBufferSize = 0;
		int32_t *parents = (int32_t*)malloc(kNumPeople*sizeof(int32_t));
		ALGO_VALIDATE( algoGraphBfsBufferSize(&bfsBufferSize, graph) );
		bfsBuffer = malloc(bfsBufferSize);
		const int32_t bfsRoot = kCort;
		printf("BFS search from %s...\n", people[bfsRoot].name);
		ALGO_VALIDATE( algoGraphBfs(graph, people[kCort].vertexId, parents, kNumPeople,
			processPersonEarly, processEdge, processPersonLate, bfsBuffer, bfsBufferSize) );
		for(int iPerson=0; iPerson<kNumPeople; iPerson += 1)
		{
			printf("%s's parent is %s\n", people[iPerson].name,
				parents[iPerson] >= 0 ? people[parents[iPerson]].name : "N/A");
		}
		free(parents);
		free(bfsBuffer);
	}

	free(graphBuffer);
}
