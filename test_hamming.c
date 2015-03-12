#include "test_common.h"

static int32_t computeWordId5(const char *word)
{
	int32_t id = (int32_t)(word[0]-'A');
	id = id*26 + (int32_t)(word[1]-'A');
	id = id*26 + (int32_t)(word[2]-'A');
	id = id*26 + (int32_t)(word[3]-'A');
	id = id*26 + (int32_t)(word[4]-'A');
	return id;
}

int main(void)
{
	unsigned int randomSeed = 0x54F66659;//(unsigned int)time(NULL);
	printf("Random seed: 0x%08X\n", randomSeed);
	srand(randomSeed);

	FILE *wordFile = zomboFopen("../upper5.txt", "r");
	if (!wordFile)
	{
		printf("ERROR: could not open word file");
		return -1;
	}
	fseek(wordFile, 0, SEEK_END);
	size_t wordFileSize = ftell(wordFile);
	fseek(wordFile, 0, SEEK_SET);
	char *wordData = malloc(wordFileSize);
	fread(wordData, wordFileSize, 1, wordFile);
	fclose(wordFile);

	// Count words; build wordId -> word string lookup table
	const char *wordDataEnd = wordData+wordFileSize;
	size_t wordIdCount = 26*26*26*26*26;
	size_t wordTableSize = wordIdCount * sizeof(char*);
	char **widToWord = malloc(wordTableSize);
	memset(widToWord, 0, wordTableSize);
	const size_t kWordLength = 5;
	int32_t wordCount = 0;
	for(char *nextWord = wordData;
		nextWord < wordDataEnd;
		nextWord += kWordLength+1)
	{
		ZOMBO_ASSERT(nextWord[kWordLength] == '\n', "word doesn't end in newline");
		nextWord[kWordLength] = 0;
		int32_t wordId = computeWordId5(nextWord);
		ZOMBO_ASSERT(wordId < wordIdCount, "wordId %d is out of range", wordId);
		assert(widToWord[wordId] == NULL);
		widToWord[wordId] = nextWord;
		wordCount += 1;
	}

	// Create graph; add word vertices; build wordId->vertexId lookup table
	size_t hamGraphBufferSize = 0;
	int32_t expectedEdgeCount = 7444; // worst-case: wordCount*(25*5)
	ALGO_VALIDATE( algoGraphBufferSize(&hamGraphBufferSize, wordCount, expectedEdgeCount, kAlgoGraphEdgeUndirected) );
	void *hamGraphBuffer = malloc(hamGraphBufferSize);
	AlgoGraph hamGraph;
	ALGO_VALIDATE( algoGraphCreate(&hamGraph, wordCount, expectedEdgeCount, kAlgoGraphEdgeUndirected, hamGraphBuffer, hamGraphBufferSize) );
	int32_t *widToVertexId = malloc(wordIdCount*sizeof(int32_t));
	memset(widToVertexId, 0xFF, wordIdCount*sizeof(int32_t));
	for(char *nextWord = wordData;
		nextWord < wordDataEnd;
		nextWord += kWordLength+1)
	{
		int32_t wordId = computeWordId5(nextWord);
		ALGO_VALIDATE( algoGraphAddVertex(hamGraph, algoDataFromInt(wordId), widToVertexId+wordId) );
	}

	// Create edges between words
	char *wordCopy = alloca(kWordLength+1);
	for(char *nextWord = wordData;
		nextWord < wordDataEnd;
		nextWord += kWordLength+1)
	{
		int32_t wordId = computeWordId5(nextWord);
		for(int iChar=0; iChar<kWordLength; ++iChar)
		{
			strncpy_s(wordCopy, kWordLength+1, nextWord, kWordLength+1);
			for(char iCandidate=nextWord[iChar]+1;
				iCandidate<='Z';
				iCandidate += 1)
			{
				wordCopy[iChar] = iCandidate;
				int32_t wordCopyId = computeWordId5(wordCopy);
				if (NULL != widToWord[wordCopyId])
				{
					ZOMBO_ASSERT(widToVertexId[wordId] >= 0, "%s (%d) has invalid vertex ID", nextWord, wordId);
					ZOMBO_ASSERT(widToVertexId[wordCopyId] >= 0, "%s (%d) has invalid vertex ID", wordCopy, wordCopyId);
					ALGO_VALIDATE( algoGraphAddEdge(hamGraph, widToVertexId[wordId], widToVertexId[wordCopyId]) );
				}
			}
		}
	}
	int32_t hamEdgeCount = -1;
	ALGO_VALIDATE( algoGraphCurrentEdgeCount(hamGraph, &hamEdgeCount) );
	ALGO_VALIDATE( algoGraphValidate(hamGraph) );

	// shortest-path
	size_t hamGraphBfsBufferSize = 0;
	ALGO_VALIDATE( algoGraphBfsStateBufferSize(&hamGraphBfsBufferSize, hamGraph) );
	void *hamGraphBfsBuffer = malloc(hamGraphBfsBufferSize);
	AlgoGraphBfsState hamBfs;
	ALGO_VALIDATE( algoGraphBfsStateCreate(&hamBfs, hamGraph, hamGraphBfsBuffer, hamGraphBfsBufferSize) );
	AlgoGraphBfsCallbacks hamBfsCallbacks = {
		NULL, NULL,
		NULL, NULL,
		NULL, NULL,
	};
	int32_t startWordId = computeWordId5("FORTY");
	ALGO_VALIDATE( algoGraphBfs(hamGraph, hamBfs, widToVertexId[startWordId], hamBfsCallbacks) );
	FILE *outFile = zomboFopen("forty.txt", "w");
	for(char *nextWord = wordData;
		nextWord < wordDataEnd;
		nextWord += kWordLength+1)
	{
		int32_t wordId = computeWordId5(nextWord);
		int32_t vertexId = widToVertexId[wordId];
		int32_t parentId = -1;
		ALGO_VALIDATE( algoGraphBfsStateGetVertexParent(hamBfs, vertexId, &parentId) );
		if (parentId == -1)
			continue;
		fprintf(outFile, "%s: ", nextWord);
		for(;;)
		{
			parentId = -1;
			ALGO_VALIDATE( algoGraphBfsStateGetVertexParent(hamBfs, vertexId, &parentId) );
			if (parentId == -1)
				break;
			AlgoData vertData;
			ALGO_VALIDATE( algoGraphGetVertexData(hamGraph, parentId, &vertData) );
			wordId = vertData.asInt;
			fprintf(outFile, "%s ", widToWord[wordId]);
			vertexId = parentId;
		}
		fprintf(outFile, "\n");
	}
	fclose(outFile);

	// Dump graph as JSON
	FILE *hamFile = zomboFopen("upper5-ham.json", "w");
	if (NULL == hamFile)
	{
		printf("ERROR: could not open output file\n");
		return -1;
	}
	fprintf(hamFile, "{\n");
	for(char *nextWord = wordData;
		nextWord < wordDataEnd;
		nextWord += kWordLength+1)
	{
		int32_t wordId = computeWordId5(nextWord);
		int32_t wordEdgeCount = 0;
		ALGO_VALIDATE( algoGraphGetVertexDegree(hamGraph, widToVertexId[wordId], &wordEdgeCount) );
		int32_t *wordEdges = alloca(wordEdgeCount*sizeof(int32_t));
		ALGO_VALIDATE( algoGraphGetVertexEdges(hamGraph, widToVertexId[wordId], wordEdgeCount, wordEdges) );
		fprintf(hamFile, "\t\"%s\": [", nextWord);
		for(int32_t iEdge=0;
			iEdge < wordEdgeCount;
			iEdge += 1)
		{
			AlgoData vertData;
			ALGO_VALIDATE( algoGraphGetVertexData(hamGraph, wordEdges[iEdge], &vertData) );
			fprintf(hamFile, "%s\"%s\"", (iEdge>0) ? ", " : "", widToWord[vertData.asInt]);
		}
		fprintf(hamFile, "]%s\n", (nextWord+kWordLength+1 < wordDataEnd) ? "," : "");
	}
	fprintf(hamFile, "}\n");
	fclose(hamFile);
	
	free(wordData);
	free(widToWord);
	free(widToVertexId);
	free(hamGraphBuffer);
	free(hamGraphBfsBuffer);
}
