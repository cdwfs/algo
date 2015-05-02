#include "test_common.h"
#include <math.h>

static const int kMaxWordLength = 13;

static uint64_t packWordAsInt(const char *word)
{
	int len = (int)strlen(word);
	if (len > kMaxWordLength)
	{
		ZOMBO_ERROR("length of word (%s) must be <= %d letters", word, kMaxWordLength);
		return 0;
	}
	uint64_t id = 0;
	for(int iChar = len-1;
		iChar >= 0;
		iChar -= 1)
	{
		id = id*(26+1) + toupper(word[iChar]) + 1 - 'A';
	}
	return id;
}

static int32_t wangHash32(int32_t key) /* see https://gist.github.com/badboy/6267743 */
{
  key = ~key + (key << 15); /* key = (key << 15) - key - 1; */
  key =  key ^ (key >> 12);
  key =  key + (key <<  2);
  key =  key ^ (key >>  4);
  key =  key * 2057;        /* key = (key + (key << 3)) + (key << 11); */
  key =  key ^ (key >> 16);
  return key;
}
static uint64_t wangHash64(uint64_t key)
{
  key = (~key) + (key << 21);               // key = (key << 21) - key - 1;
  key =   key  ^ (key >> 24);
  key =  (key  + (key <<  3)) + (key << 8); // key * 265
  key =   key  ^ (key >> 14);
  key =  (key  + (key <<  2)) + (key << 4); // key * 21
  key =   key  ^ (key >> 28);
  key =   key  + (key << 31);
  return key;
}

typedef struct HashEntry_Word
{
	const char *key;
	struct HashEntry_Word *next;
	/* begin custom fields */
	int32_t vertexId;
	/* end custom fields */
} HashEntry_Word;
typedef struct HashTable_Word
{
	int binCount;
	int entryCapacity;
	int freeEntryIndex;
	HashEntry_Word **bins;
	HashEntry_Word *entryPool;
	HashEntry_Word *recycledEntryList;
} HashTable_Word;
static size_t hashBufferSize_Word(int binCount, int entryCapacity)
{
	return entryCapacity*sizeof(HashEntry_Word) + binCount*sizeof(HashEntry_Word*) + sizeof(HashTable_Word);
}
static HashTable_Word *hashCreate_Word(void *buffer, size_t bufferSize, int binCount, int entryCapacity)
{
	size_t expectedBufferSize = hashBufferSize_Word(binCount, entryCapacity);
	if (bufferSize < expectedBufferSize)
		return NULL;
	uint8_t *bufferNext = (uint8_t*)buffer;

	HashEntry_Word *entryPool = (HashEntry_Word*)bufferNext;
	bufferNext += entryCapacity*sizeof(HashEntry_Word);

	HashEntry_Word **bins = (HashEntry_Word**)bufferNext;
	bufferNext += binCount*sizeof(HashEntry_Word*);
	int iBin;
	for(iBin=0; iBin<binCount; iBin+=1)
		bins[iBin] = NULL;

	HashTable_Word *outTable = (HashTable_Word*)bufferNext;
	bufferNext += sizeof(HashTable_Word);
	ZOMBO_ASSERT( (size_t)(bufferNext-(uint8_t*)buffer) == expectedBufferSize, "buffer usage did not match expected buffer size" );
	outTable->binCount = binCount;
	outTable->entryCapacity = entryCapacity;
	outTable->freeEntryIndex = 0;
	outTable->bins = bins;
	outTable->entryPool = entryPool;
	outTable->recycledEntryList = NULL;
	return outTable;
}
static HashEntry_Word **hashGetEntryRef_Word(HashTable_Word *table, const char *key, int binIndex)
{
	ZOMBO_ASSERT(table, "table must not be NULL");
	ZOMBO_ASSERT(binIndex >= 0 && binIndex < table->binCount, "binIndex (%d) must be in the range [0..%d)", binIndex, table->binCount);
	HashEntry_Word **ppEntry;
	for(ppEntry = &(table->bins[binIndex]);
		NULL != *ppEntry;
		ppEntry = &(*ppEntry)->next)
	{
		if (_stricmp(key, (*ppEntry)->key) == 0)
			return ppEntry;
	}
	return NULL;
}
static HashEntry_Word *hashGetEntry_Word(HashTable_Word *table, const char *key)
{
	ZOMBO_ASSERT(table, "table must not be NULL");
	/* hash function: binIndex = f(key) % table->binCount */
	int binIndex = wangHash64( packWordAsInt(key) ) % table->binCount;
	/* end hash function */
	HashEntry_Word **ppEntry = hashGetEntryRef_Word(table, key, binIndex);
	return ppEntry ? *ppEntry : NULL;
}
static HashEntry_Word *hashAddEntry_Word(HashTable_Word *table, const char *key)
{
	ZOMBO_ASSERT(table, "table must not be NULL");
	/* hash function: binIndex = f(key) % table->binCount */
	int binIndex = wangHash64( packWordAsInt(key) ) % table->binCount;
	/* end hash function */
	HashEntry_Word **ppEntry = hashGetEntryRef_Word(table, key, binIndex);
	if (NULL != ppEntry)
		return *ppEntry;
	ZOMBO_ASSERT(table->freeEntryIndex < table->entryCapacity, "freeEntryIndex (%d) must be in the range [0..%d)", table->freeEntryIndex, table->entryCapacity);
	HashEntry_Word *newEntry = table->recycledEntryList;
	if (newEntry == NULL)
	{
		newEntry = table->entryPool + table->freeEntryIndex;
		table->freeEntryIndex += 1;
	}
	else
	{
		table->recycledEntryList = newEntry->next;
	}
	newEntry->key = key;
	newEntry->next = table->bins[binIndex];
	table->bins[binIndex] = newEntry;
	/* begin custom fields */
	newEntry->vertexId = -1;
	/* end custom fields */
	return newEntry;
}
static void hashRemoveEntry_Word(HashTable_Word *table, const char *key)
{
	ZOMBO_ASSERT(table, "table must not be NULL");
	/* hash function: binIndex = f(key) % table->binCount */
	int binIndex = wangHash64( packWordAsInt(key) ) % table->binCount;
	/* end hash function */
	HashEntry_Word **ppEntry = hashGetEntryRef_Word(table, key, binIndex);
	if (NULL == ppEntry)
		return;
	HashEntry_Word *doomedEntry = *ppEntry;
	*ppEntry = doomedEntry->next;
	doomedEntry->next = table->recycledEntryList;
	table->recycledEntryList = doomedEntry;
}


int main(void)
{
	unsigned int randomSeed = 0x54F66659;//(unsigned int)time(NULL);
	printf("Random seed: 0x%08X\n", randomSeed);
	srand(randomSeed);

	FILE *wordFile = zomboFopen("../upper.txt", "r");
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
	const char *wordDataEnd = wordData+wordFileSize;

	const int kHashBinCount = 128*1024;
	const int kHashEntryCapacity = 64*1024;
	size_t hashSizeInBytes = hashBufferSize_Word(kHashBinCount, kHashEntryCapacity);
	void *hashBuffer = malloc(hashSizeInBytes);
	HashTable_Word *wordHash = hashCreate_Word(hashBuffer, hashSizeInBytes, kHashBinCount, kHashEntryCapacity);

	// Count words; build word hash table
	int32_t wordCount = 0;
	for(char *nextWord = wordData;
		nextWord < wordDataEnd;
		nextWord += strlen(nextWord)+1)
	{
		char *newline = strchr(nextWord, '\n');
		ZOMBO_ASSERT(NULL != newline, "word doesn't end in newline");
		size_t wordLength = newline - nextWord;
		nextWord[wordLength] = 0;
		if (wordLength > kMaxWordLength)
			continue;
		hashAddEntry_Word(wordHash, nextWord);
		wordCount += 1;
	}
#if 1 /* just some quicky hash table analysis */
	float stdDeviation = 0.0f;
	int maxLength = 0;
	float meanLength = (float)wordHash->entryCapacity / (float)wordHash->binCount;
	{
		int iBin=0;
		for(iBin=0; iBin<wordHash->binCount; ++iBin)
		{
			HashEntry_Word *entry = wordHash->bins[iBin];
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
	}
	stdDeviation = sqrtf(stdDeviation / (float)wordHash->binCount);
#endif

	// Create graph; add word vertices; build wordId->vertexId lookup table
	size_t hamGraphBufferSize = 0;
	int32_t expectedEdgeCount = 33383;//wordCount*(26-1)*kMaxWordLength;
	ALGO_VALIDATE( algoGraphComputeBufferSize(&hamGraphBufferSize, wordCount, expectedEdgeCount, kAlgoGraphEdgeUndirected) );
	void *hamGraphBuffer = malloc(hamGraphBufferSize);
	AlgoGraph hamGraph;
	ALGO_VALIDATE( algoGraphCreate(&hamGraph, wordCount, expectedEdgeCount, kAlgoGraphEdgeUndirected, hamGraphBuffer, hamGraphBufferSize) );
	for(int iBin=0;
		iBin<wordHash->binCount;
		++iBin)
	{
		for(HashEntry_Word *wordEntry = wordHash->bins[iBin];
			wordEntry;
			wordEntry = wordEntry->next)
		{
			ALGO_VALIDATE( algoGraphAddVertex(hamGraph, algoDataFromPtr(wordEntry), &(wordEntry->vertexId)) );
		}
	}

	// Create edges between words. Keep track of maximum vertex degree.
	int32_t maxWordEdgeCount = 0;
	char *wordCopy = alloca(kMaxWordLength+1);
	wordCopy[kMaxWordLength] = 0;
	for(int iBin=0;
		iBin<wordHash->binCount;
		++iBin)
	{
		for(HashEntry_Word *wordEntry = wordHash->bins[iBin];
			wordEntry;
			wordEntry = wordEntry->next)
		{
			const char *word = wordEntry->key;
			size_t wordLength = strlen(word);
			for(int iChar=0;
				iChar<wordLength;
				++iChar)
			{
				strncpy_s(wordCopy, kMaxWordLength+1, word, wordLength+1);
				/* Each word only looks at candidates that appear lexically later */
				for(char iCandidate = word[iChar]+1;
					iCandidate<='Z';
					iCandidate += 1)
				{
					wordCopy[iChar] = iCandidate;
					const HashEntry_Word *copyEntry = hashGetEntry_Word(wordHash, wordCopy);
					if (NULL != copyEntry)
					{
						ZOMBO_ASSERT(wordEntry->vertexId >= 0, "%s (%d) has invalid vertex ID", word,     wordEntry->vertexId);
						ZOMBO_ASSERT(copyEntry->vertexId >= 0, "%s (%d) has invalid vertex ID", wordCopy, copyEntry->vertexId);
						ALGO_VALIDATE( algoGraphAddEdge(hamGraph, wordEntry->vertexId, copyEntry->vertexId) );
						int32_t srcEdgeCount = 0, dstEdgeCount = 0;
						ALGO_VALIDATE( algoGraphGetVertexDegree(hamGraph, wordEntry->vertexId, &srcEdgeCount) );
						ALGO_VALIDATE( algoGraphGetVertexDegree(hamGraph, copyEntry->vertexId, &dstEdgeCount) );
						if (srcEdgeCount > maxWordEdgeCount)
						{
							maxWordEdgeCount = srcEdgeCount;
						}
						if (dstEdgeCount > maxWordEdgeCount)
						{
							maxWordEdgeCount = dstEdgeCount;
						}
					}
				}
			}
		}
	}
	int32_t hamEdgeCount = -1;
	ALGO_VALIDATE( algoGraphGetCurrentEdgeCount(hamGraph, &hamEdgeCount) );
	ALGO_VALIDATE( algoGraphValidate(hamGraph) );

	{
		HashEntry_Word *doomedEntry = hashGetEntry_Word(wordHash, "SORER");
		ALGO_VALIDATE( algoGraphRemoveVertex(hamGraph, doomedEntry->vertexId) );
		ALGO_VALIDATE( algoGraphValidate(hamGraph) );
		hashRemoveEntry_Word(wordHash, "SORER");
		wordCount -=1 ;
	}

	// Dump graph as JSON
	FILE *hamFile = zomboFopen("upper-ham.json", "w");
	if (NULL == hamFile)
	{
		printf("ERROR: could not open output file\n");
		return -1;
	}
	fprintf(hamFile, "{\n");
	int32_t *wordEdges = alloca(maxWordEdgeCount*sizeof(int32_t));
	for(int iBin=0;
		iBin<wordHash->binCount;
		++iBin)
	{
		for(HashEntry_Word *wordEntry = wordHash->bins[iBin];
			wordEntry;
			wordEntry = wordEntry->next)
		{
			const char *word = wordEntry->key;
			int32_t wordEdgeCount = 0;
			ALGO_VALIDATE( algoGraphGetVertexDegree(hamGraph, wordEntry->vertexId, &wordEdgeCount) );
			ZOMBO_ASSERT(wordEdgeCount <= maxWordEdgeCount, "word edge count (%d) exceeds expected maximum (%d)", wordEdgeCount, maxWordEdgeCount);
			ALGO_VALIDATE( algoGraphGetVertexEdges(hamGraph, wordEntry->vertexId, wordEdgeCount, wordEdges) );
			fprintf(hamFile, "\t\"%s\": [", word);
			for(int32_t iEdge=0;
				iEdge < wordEdgeCount;
				iEdge += 1)
			{
				AlgoData vertData;
				ALGO_VALIDATE( algoGraphGetVertexData(hamGraph, wordEdges[iEdge], &vertData) );
				HashEntry_Word *edgeEntry = vertData.asPtr;
				fprintf(hamFile, "%s\"%s\"", (iEdge>0) ? ", " : "", edgeEntry->key);
			}
			/*	TODO: this logic breaks if we remove the last word from the graph/hash table.
				It should skip the leading comma on the first word, and add it to all the others.
				*/
			fprintf(hamFile, "]%s\n", (word+strlen(word)+1 < wordDataEnd) ? "," : "");
		}
	}
	fprintf(hamFile, "}\n");
	fclose(hamFile);
	
	// shortest-path
	size_t hamGraphBfsBufferSize = 0;
	ALGO_VALIDATE( algoGraphBfsStateComputeBufferSize(&hamGraphBfsBufferSize, hamGraph) );
	void *hamGraphBfsBuffer = malloc(hamGraphBfsBufferSize);
	AlgoGraphBfsState hamBfs;
	printf("Ctrl-D + Enter to exit\n\n");
	for(;;)
	{
		printf("start: ");
		char startWord[32] = {0};
		scanf_s("%31s", startWord, 32);
		if (strlen(startWord) > kMaxWordLength)
		{
			printf("ERROR: Too long! max word length is %d\n\n", kMaxWordLength);
			int c;
			while((c = getchar()) != '\n' && c != EOF)
			{
				/* discard remainder of input */
			}
			continue;
		}
		else if (startWord[0] == 4)
			break;

		printf(" goal: ");
		char goalWord[32] = {0};
		scanf_s("%31s", goalWord, 32);
		if (strlen(goalWord) > kMaxWordLength)
		{
			printf("ERROR: Too long! max word length is %d\n\n", kMaxWordLength);
			int c;
			while((c = getchar()) != '\n' && c != EOF)
			{
				/* discard remainder of input */
			}
			continue;
		}
		else if (goalWord[0] == 4)
			break;

		/*	Results are printed in reverse order (from goal to start), so we'll actually search from goal to start to
			get it to come out correctly.
			*/
		HashEntry_Word *startEntry = hashGetEntry_Word(wordHash, goalWord);
		if (!startEntry)
		{
			printf("ERROR: '%s' not found in dictionary\n\n", goalWord);
			continue;
		}
		const HashEntry_Word *goalEntry = hashGetEntry_Word(wordHash, startWord);
		if (!goalEntry)
		{
			printf("ERROR: '%s' not found in dictionary\n\n", startWord);
			continue;
		}

		ALGO_VALIDATE( algoGraphBfsStateCreate(&hamBfs, hamGraph, hamGraphBfsBuffer, hamGraphBfsBufferSize) );
		AlgoGraphBfsCallbacks hamBfsCallbacks = {
			NULL, NULL,
			NULL, NULL,
			NULL, NULL,
		};
		ALGO_VALIDATE( algoGraphBfs(hamGraph, hamBfs, startEntry->vertexId, hamBfsCallbacks) );
		int32_t parentId = -1;
		ALGO_VALIDATE( algoGraphBfsStateGetVertexParent(hamBfs, goalEntry->vertexId, &parentId) );
		if (parentId == -1)
		{
			printf("ERROR: no valid hamming path found from '%s' to '%s'\n\n", startWord, goalWord);
			continue;
		}
		printf("%s ", goalEntry->key);
		for(;;)
		{
			parentId = -1;
			ALGO_VALIDATE( algoGraphBfsStateGetVertexParent(hamBfs, goalEntry->vertexId, &parentId) );
			if (parentId == -1)
				break;
			AlgoData vertData;
			ALGO_VALIDATE( algoGraphGetVertexData(hamGraph, parentId, &vertData) );
			goalEntry = vertData.asPtr;
			printf("%s ", goalEntry->key);
		}
		printf("\n\n");
	}

	free(hamGraphBfsBuffer);
	free(hamGraphBuffer);
	free(hashBuffer);
	free(wordData);
}
