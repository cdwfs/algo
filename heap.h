#pragma once

#include <stdint.h>

typedef struct HeapImpl *Heap;
typedef int32_t HeapKey;
typedef const void* HeapData;

int32_t heapCreate(Heap *heap, const int32_t heapCapacity);
int32_t heapCurrentSize(Heap heap, int32_t *outSize);
int32_t heapInsert(Heap heap, const HeapKey key, const HeapData data);
int32_t heapPeek(Heap heap, HeapKey *outTopKey, HeapData *outTopData);
int32_t heapPop(Heap heap);
int32_t heapCheck(Heap heap);
int32_t heapCapacity(Heap heap, int32_t *outCapacity);
