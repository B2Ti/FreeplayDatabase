#pragma once
#include "defines.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct SeededRandom {
    uint32_t seed;
} SeededRandom;

typedef struct ShuffleCacheEntry {
    int32_t seed;
    float budget_mult;
    uint16_t array[NUM_GROUPS];
} ShuffleCacheEntry;

typedef struct ShuffleCache {
    size_t size;
    ShuffleCacheEntry *cache;
} ShuffleCache;

float getNextSeed(SeededRandom* rand);
int initCache(ShuffleCache *cache, const size_t cacheSize);
void freeCache(ShuffleCache *cache);
ShuffleCacheEntry *requestFromCache(ShuffleCache *cache, const uint32_t seed);
