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

static int64_t _getNextSeedBounded(SeededRandom *rand, const int64_t min, const int64_t max){
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

static void setEntryValidity(ShuffleCacheEntry *entry, const Byte *validityArray){
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


static void shuffleSeeded16V44(uint16_t array[], const size_t _len, SeededRandom *rand){
    const size_t len = _len - 1;
    int32_t i = len;
    while (true){
        float nextSeed = getNextSeed(rand);
        int32_t idx = (int32_t)(nextSeed * (float)len);
        const uint16_t tmp = array[i];
        array[i] = array[idx];
        array[idx] = tmp;
        if (i <= 0){
            return;
        }
        i--;
    }
}

static void setEntrySeedV44(ShuffleCacheEntry *entry, const uint32_t seed, const Byte *validityArray){
    for (uint16_t i = 0; i < NUM_GROUPS; i++){
        entry->array[i] = i;
    }
    entry->seed = seed;
    SeededRandom rand = {seed};
    shuffleSeeded16V44(entry->array, NUM_GROUPS, &rand);
    if (validityArray){
        setEntryValidity(entry, validityArray);
    }
    rand.seed = seed;
    entry->budget_mult = 1.5 - getNextSeed(&rand);
}

static void shuffleInPlace16(uint16_t array[], const size_t len, SeededRandom *rand){
    for (size_t i = 0; i < len; i++){
        int64_t index = getNextSeedBounded(rand, i, len);
        const uint16_t tmp = array[i];
        array[i] = array[index];
        array[index] = tmp;
    }
}

static void setEntrySeed(ShuffleCacheEntry *entry, const uint32_t seed, const Byte *validityArray){
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

int initCache(ShuffleCache *cache, const size_t cacheSize, bool ver44, const Byte *validityArray){
    cache->cache = malloc(cacheSize * sizeof(ShuffleCacheEntry));
    if (!cache->cache){
        return -1;
    }
    for (size_t i = 0; i < cacheSize; i++){
        if (ver44) {
            setEntrySeed(&cache->cache[i], i, validityArray);
        } else {
            setEntrySeedV44(&cache->cache[i], i, validityArray);
        }
    }
    cache->size=cacheSize;
    return 0;
}

void freeCache(ShuffleCache *cache){
    free(cache->cache);
    cache->cache = NULL;
    cache->size = 0;
}

ShuffleCacheEntry *requestFromCache(ShuffleCache *cache, const uint32_t seed, bool ver44, const Byte *validityEntry){
    const uint32_t index = seed % cache->size;
    if ((uint32_t)cache->cache[index].seed != seed){
        if (ver44) {
            setEntrySeed(&cache->cache[index], seed, validityEntry);
        } else {
            setEntrySeedV44(&cache->cache[index], seed, validityEntry);
        }
    }
    return &cache->cache[index];
}
