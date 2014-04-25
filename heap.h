#pragma once

#include <stdint.h>

struct Heap;

int32_t heapCreate(struct Heap **heap, const int32_t heapCapacity);
int32_t heapCurrentSize(struct Heap *heap, int32_t *outSize);
int32_t heapInsert(struct Heap *heap, const int32_t key, const void *data);
int32_t heapPeek(struct Heap *heap, int32_t *outTopKey, const void **outTopData);
int32_t heapPop(struct Heap *heap);
