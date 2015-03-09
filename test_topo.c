#include "test_common.h"

/** TODO: test topological sort
	-	randomly generate a graph of N nodes
	-	randomly create edges between pairs of vertices. Keep a separate list of all edges,
		since the graph can't do it for you. Actually, hey, maybe do it with a BFS!
	-	run a topo sort
	-	for each edge v0->v1, make sure v0 appears earlier in the sorted vertices than v1.
		-	take one pass over the sorted vertices, and track each vertex's index in the sorted array.
		-	each edge test is then constant-time
		-	total time becomes O(V+E) instead of O(EV)
	*/
typedef struct GraphEdge
{
	int32_t v0, v1;
} GraphEdge;

int main(void)
{
	unsigned int randomSeed = 0x54F66659;//(unsigned int)time(NULL);
	printf("Random seed: 0x%08X\n", randomSeed);
	srand(randomSeed);

	for(;;)
	{
		int32_t kVertexCapacity = 16384;
		int32_t kEdgeCapacity = 65536;
		int32_t kAverageEdgesPerVertex = 4;
		int32_t vertexCount = (rand() % 8192) + 8192;
		int32_t *vertexIds = malloc(vertexCount*sizeof(int32_t));
		int32_t *sortedVertexIds = malloc(vertexCount*sizeof(int32_t));
		void *graphBuffer = NULL;
		size_t graphBufferSize = 0;
		AlgoGraph graph;

		ALGO_VALIDATE( algoGraphBufferSize(&graphBufferSize, kVertexCapacity, kEdgeCapacity, kAlgoGraphEdgeDirected) );
		graphBuffer = malloc(graphBufferSize);
		ALGO_VALIDATE( algoGraphCreate(&graph, kVertexCapacity, kEdgeCapacity, kAlgoGraphEdgeDirected, graphBuffer, graphBufferSize) );

		for(int iVert=0; iVert<vertexCount; ++iVert)
		{
			ALGO_VALIDATE( algoGraphAddVertex(graph, algoDataFromInt(0xABCD0000 + iVert), vertexIds+iVert) );
		}
		int32_t actualVertexCount = 0;
		ALGO_VALIDATE( algoGraphCurrentVertexCount(graph, &actualVertexCount) );
		ZOMBO_ASSERT(actualVertexCount == vertexCount, "adding N vertices didn't result in an N-vertex graph...?");

		GraphEdge *edges = malloc(kEdgeCapacity*sizeof(GraphEdge));
		size_t edgesEntryCount = 0;
		for(int iEdge=0; iEdge<vertexCount*kAverageEdgesPerVertex; ++iEdge)
		{
			int32_t srcVertex = iEdge % vertexCount;
			if (srcVertex == vertexCount-1)
				continue;
			int32_t dstVertex = srcVertex + 1 + (rand() % (vertexCount-srcVertex-1));
			ZOMBO_ASSERT(dstVertex < vertexCount, "I suck at math.");
			ALGO_VALIDATE( algoGraphAddEdge(graph, srcVertex, dstVertex) );
			edges[edgesEntryCount].v0 = srcVertex; /* TODO: a ton of redundant edges stored here, but there's no way to iterate over valid edges only */
			edges[edgesEntryCount].v1 = dstVertex;
			edgesEntryCount += 1;
		}
		int32_t actualEdgeCount = 0;
		ALGO_VALIDATE( algoGraphCurrentEdgeCount(graph, &actualEdgeCount) );
		printf("Testing graph (%5d vertices, %5d edges)\n", actualVertexCount, actualEdgeCount);
		printf("\tValidate...\n");
		ALGO_VALIDATE( algoGraphValidate(graph) );

		printf("\tTopoSort\n");
		size_t topoBufferSize = 0;
		ALGO_VALIDATE( algoGraphTopoSortBufferSize(&topoBufferSize, graph) );
		void *topoBuffer = malloc(topoBufferSize);
		ALGO_VALIDATE( algoGraphTopoSort(graph, sortedVertexIds, vertexCount, topoBuffer, topoBufferSize) );
		free(topoBuffer);

		printf("\tVerifying results\n");
		int isSortCorrect = 1;
		int32_t *vertexToSortedIndex = malloc(kVertexCapacity*sizeof(int32_t));
		for(int iSortedVert=0; iSortedVert<vertexCount; ++iSortedVert)
		{
			vertexToSortedIndex[ sortedVertexIds[iSortedVert] ] = iSortedVert;
		}
		for(int iEdge=0; iEdge<edgesEntryCount; ++iEdge)
		{
			int32_t sortedIndex0 = vertexToSortedIndex[edges[iEdge].v0];
			int32_t sortedIndex1 = vertexToSortedIndex[edges[iEdge].v1];
			if (sortedIndex0 >= sortedIndex1)
			{
				printf(     "\tERROR: Edge %d [%d->%d] is not properly sorted!\n", iEdge, edges[iEdge].v0, edges[iEdge].v1);
				ZOMBO_ERROR("\tERROR: Edge %d [%d->%d] is not properly sorted!\n", iEdge, edges[iEdge].v0, edges[iEdge].v1);
				isSortCorrect = 0;
			}
		}
		if (isSortCorrect)
			printf("\tTest complete (no errors!)\n");
		free(edges);
		free(vertexToSortedIndex);
		free(vertexIds);
		free(sortedVertexIds);
		free(graphBuffer);
	}
}
