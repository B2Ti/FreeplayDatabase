#include <shuffle.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <bitsArray.h>

float getNextSeed(SeededRandom *rand){
    rand->seed = (rand->seed * 0x41a7LL) % 0x7FFFFFFFLL;
    float value = ((float)rand->seed) / 2147483646.0f;
    return value;
}

int64_t _getNextSeedBounded(SeededRandom *rand, const int64_t min, const int64_t max){
    const int64_t inv_range = min - max;
    rand->seed = (((int64_t)(rand->seed)) * 0x41a7LL) % 0x7FFFFFFFLL;
    int64_t shift = rand->seed % inv_range;
    if (shift >= 0){
        shift += inv_range;
    }
    return max + shift;
}

int64_t getNextSeedBounded(SeededRandom *rand, int64_t min, int64_t max){
    if (min == max){
        return max;
    }
    if (min > max){
        const int64_t tmp = min;
        min = max;
        max = tmp;
    }
    return _getNextSeedBounded(rand, min, max);
}

void shuffleInPlace16(uint16_t array[], const size_t len, SeededRandom *rand){
    for (size_t i = 0; i < len; i++){
        int64_t index = getNextSeedBounded(rand, i, len);
        const uint16_t tmp = array[i];
        array[i] = array[index];
        array[index] = tmp;
    }
}


void setEntryValidity(ShuffleCacheEntry *entry, const Byte *validityArray){
    const uint32_t offset = 512 * NUM_GROUPS;
    uint16_t groups = 0;
    for(uint16_t i = 0; i < MAX_COUNT; i++){
        entry->r511Groups[i] = R511END;
    }
    for (uint16_t i = 0; i < NUM_GROUPS; i++){
        if (!bitget(validityArray, offset + entry->array[i])){
            continue;
        }
        assert(groups < MAX_COUNT);
        entry->r511Groups[groups] = entry->array[i];
        groups++;
    }
}

void setEntrySeed(ShuffleCacheEntry *entry, const uint32_t seed, const Byte *validityArray){
    for (uint16_t i = 0; i < NUM_GROUPS; i++){
        entry->array[i] = i;
    }
    entry->seed = seed;
    SeededRandom rand = {seed};
    shuffleInPlace16(entry->array, NUM_GROUPS, &rand);
    if (validityArray){
        setEntryValidity(entry, validityArray);
    }
    entry->budget_mult = 1.5 - getNextSeed(&rand);
}

int initCache(ShuffleCache *cache, const size_t cacheSize, const Byte *validityArray){
    cache->cache = malloc(cacheSize * sizeof(ShuffleCacheEntry));
    if (!cache->cache){
        return -1;
    }
    for (size_t i = 0; i < cacheSize; i++){
        setEntrySeed(&cache->cache[i], i, validityArray);
    }
    cache->size=cacheSize;
    return 0;
}

void freeCache(ShuffleCache *cache){
    free(cache->cache);
    cache->cache = NULL;
    cache->size = 0;
}

ShuffleCacheEntry *requestFromCache(ShuffleCache *cache, const uint32_t seed, const Byte *validityEntry){
    const uint32_t index = seed % cache->size;
    if ((uint32_t)cache->cache[index].seed != seed){
        setEntrySeed(&cache->cache[index], seed, validityEntry);
    }
    return &cache->cache[index];
}

/*
#include <string.h> // for memcpy for perf testing, as this is what the caching system replaces the need for
#include <stdio.h>
#include "timing.h"

#define loops_i 10000
#define loops_j 800
#define speedTest 1


int main(){
    uint64_t ts, te;
    ShuffleCache cache = createCache(loops_j);
    uint32_t i, j, arr[numGroups];
    uint32_t *result = malloc(numGroups * sizeof(uint32_t));
    for (i=0; i < numGroups; i++){
        arr[i] = i;
    }
    #if speedTest
    ts = time_us();
    for (i=0; i < loops_i; i++){
        for (j=0; j < loops_j; j++){
            memcpy(result, arr, numGroups * sizeof(uint32_t));
            shuffleSeeded(result, numGroups, i+j);
            //printf("%d, %d, %d\n", result[0], result[250], result[numGroups-1]);
        }
    }
    te = time_us();
    printf("time taken w/o caching: %llu us\n", te-ts);
    fflush(stdout);
    ts = time_us();
    for (i=0; i < loops_i * 50; i++){
        for (j=0; j < loops_j; j++){
            ShuffleCacheEntry *entry = requestFromCache(&cache, i+j);
            memcpy(result, entry->array, numGroups * sizeof(uint32_t));
            //printf("%d, %d, %d\n", result[0], result[250], result[numGroups-1]);
        }
    }
    te = time_us();
    printf("time taken w/ caching: %llu us\n", te-ts);
    #else
    uint32_t *resultB = malloc(numGroups * sizeof(uint32_t));
    bool failed = false;
    for (i=0; i < loops_i; i++){
        for (j=0; j < loops_j; j++){
            memcpy(result, arr, numGroups * sizeof(uint32_t));
            shuffleSeeded(result, numGroups, i+j);
            ShuffleCacheEntry *entry = requestFromCache(&cache, i+j);
            memcpy(resultB, entry->array, numGroups * sizeof(uint32_t));
            if (memcmp(result, resultB, numGroups * sizeof(uint32_t))){
                printf("failed at %d:%d\n", i, j);
                failed = true;
            }
        }
    }
    if (!failed){
        printf("all tests succeeded up to %d:%d\n", i,j);
    }
    #endif
}
*/
