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
	kVertexCapacity = 16*1024;
	kEdgeCapacity = 16*1024;
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

	free(graphBuffer);
}
