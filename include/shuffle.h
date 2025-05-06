#pragma once
#include "defines.h"
#include <bitsArray.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct SeededRandom {
    uint32_t seed;
} SeededRandom;

typedef struct ShuffleCacheEntry {
    int32_t seed;
    float budget_mult;
    uint16_t array[NUM_GROUPS];
    //The validity of groups is identical from r511-r1k so we cache this as well
    uint16_t r511Groups[MAX_COUNT];
} ShuffleCacheEntry;

typedef struct ShuffleCache {
    size_t size;
    ShuffleCacheEntry *cache;
} ShuffleCache;

float getNextSeed(SeededRandom* rand);
int64_t getNextSeedBounded(SeededRandom *rand, int64_t min, int64_t max);
/**
 * @param validityArray optional, creates the r511-r1k validity cache
 */
int initCache(ShuffleCache *cache, const size_t cacheSize, const Byte *validityArray);
void freeCache(ShuffleCache *cache);
/**
 * @param validityArray optional, creates/accesses the r511-r1k validity cache for the given seed
 */
ShuffleCacheEntry *requestFromCache(ShuffleCache *cache, const uint32_t seed, const Byte *validityArray);
