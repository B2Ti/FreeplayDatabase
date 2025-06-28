#include <seedStatistics.h>
#include <seedSearching.h>
#include <freeplayGroups.h>
#include <bloonStats.h>
#include <threading.h>
#include <timing.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static FILE *fileopen(const char *filename, const char *mode){
    #ifdef _MSC_VER
    FILE *file = NULL;
    if (fopen_s(&file, filename, mode)){
        return file;
    }
    return file;
    #else
    return fopen(filename, mode);
    #endif
}

typedef struct U32Array {
    uint32_t *vals;
    size_t length, capacity;
} U32Array;

static int U32Array_Init(U32Array *arr, size_t size){
    uint32_t *vals = malloc(size * sizeof(uint32_t));
    if (!vals){
        perror("U32Array_Init");
        return 1;
    }
    arr->vals = vals;
    arr->length = 0;
    arr->capacity = size;
    return 0;
}

static void U32Array_Free(U32Array *arr){
    if (!arr) return;
    if (arr->vals){
        free(arr->vals);
        arr->vals = NULL;
    }
    arr->length = 0;
    arr->capacity = 0;
}

static int U32Array_Add(U32Array *arr, uint32_t idx, uint32_t val){
    if (arr->capacity <= idx){
        while (arr->capacity <= idx){
            arr->capacity <<= 1;
            if (!arr->capacity){
                arr->capacity = idx + 1;
            }
        }
        uint32_t *vals = realloc(arr->vals, arr->capacity * sizeof(uint32_t));
        if (!vals){
            return 1;
        }
        arr->vals = vals;
    }
    while (arr->length < idx){
        arr->vals[arr->length] = 0;
        arr->length++;
    }
    if (arr->length > idx){
        arr->vals[idx] += val;
        return 0;
    }
    arr->vals[arr->length] = val;
    arr->length++;
    return 0;
}

#define U32_LOAD_FACTOR 0.7

typedef struct U32Map {
    uint32_t *keys;
    uint32_t *values;
    size_t mapSize;
    size_t filledBuckets;
} U32Map;

uint32_t FNV1_U32(uint32_t num){
    uint32_t hash = 0x811c9dc5U;
    hash *= 0x01000193U;
    hash ^= num & 0xff;
    hash *= 0x01000193U;
    hash ^= (num >> 8) & 0xff;
    hash *= 0x01000193U;
    hash ^= (num >> 16) & 0xff;
    hash *= 0x01000193U;
    hash ^= (num >> 24) & 0xff;
    return hash;
}

static int U32Map_Init(U32Map *map, size_t size){
    uint32_t *keys = calloc(size, sizeof(uint32_t));
    if (!keys){
        perror("U32Map_Init");
        return 1;
    }
    uint32_t *vals = calloc(size, sizeof(uint32_t));
    if (!vals){
        free(keys);
        perror("U32Map_Init");
        return 1;
    }
    map->keys = keys;
    map->values = vals;
    map->mapSize = size;
    return 0;
}

static void U32Map_Free(U32Map *map){
    if (!map) return;
    if (map->keys) free(map->keys);
    if (map->values) free(map->values);
    map->keys = NULL;
    map->values = NULL;
    map->mapSize = 0;
}

static int U32Map_LinearInsert(U32Map *map, uint32_t key, uint32_t value){
    if (!key){
        fprintf(stderr, "U32Map_LinearInsert: cannot use 0 as a key\n");
        return 1;
    }
    if (map->mapSize == map->filledBuckets){
        fprintf(stderr, "U32Map_LinearInsert: insert into full map\n");
        return 1;
    }
    uint32_t idx = FNV1_U32(key) % map->mapSize;
    while (map->keys[idx]){
        idx = (idx + 1) % map->mapSize;
    }
    map->keys[idx] = key;
    map->values[idx] = value;
    map->filledBuckets++;
    return 0;
}

static int U32Map_DoubleSize(U32Map *map){
    size_t oldSize = map->mapSize;
    size_t newSize = map->mapSize << 1;
    if (!newSize){
        fprintf(stderr, "Resizing ended with zero size (possible overflow)\n");
        return 1;
    }
    uint32_t *keys = calloc(newSize, sizeof(uint32_t));
    if (!keys){
        perror("could not realloc map");
        return 1;
    }
    uint32_t *values = calloc(newSize, sizeof(uint32_t));
    if (!values){
        perror("could not realloc map");
        return 1;
    }
    map->mapSize = newSize;
    map->filledBuckets = 0;
    uint32_t *oldKeys = map->keys;
    map->keys = keys;
    uint32_t *oldValues = map->values;
    map->values = values;
    for (size_t i = 0; i < oldSize; i++){
        uint32_t key = oldKeys[i];
        if (!key) continue;
        if (U32Map_LinearInsert(map, key, oldValues[i])){
            fprintf(stderr, "U32Map_DoubleSize: LinearInsert called failed\n");
            free(oldKeys);
            free(oldValues);
            return 1;
        }
    }
    free(oldKeys);
    free(oldValues);
    return 0;
}

static int U32Map_Add(U32Map *map, uint32_t key, uint32_t val){
    if (!key){
        fprintf(stderr, "U32Map_Add: Cannot use 0 as a key\n");
        return 1;
    }
    if ((double)map->filledBuckets / map->mapSize >= U32_LOAD_FACTOR){
        if (U32Map_DoubleSize(map)){
            fprintf(stderr, "U32Map_Add: DoubleSize call failed\n");
            return 1;
        }
    }
    uint32_t idx = FNV1_U32(key) % map->mapSize;
    uint32_t _key = map->keys[idx];
    while (_key && (_key != key)){
        idx = (idx + 1) % map->mapSize;
        _key = map->keys[idx];
    }
    if (!_key) {
        map->keys[idx] = key;
        map->filledBuckets++;
    }
    map->values[idx] += val;
    return 0;
}

static uint32_t displayCommandLineInfo(uint32_t seed, double lastTime, double timeNow, seedSearchArg *args){
    double totalTimeTaken = (timeNow - lastTime)*0.000001;

    uint32_t amountComplete = seed - args->seedStart;
    double portionDone = (double)amountComplete / (double)args->seedNum;
    double portionLeft = 1.0 - (double)amountComplete / (double)args->seedNum;
    uint64_t timeLeft = (uint64_t)(portionLeft * totalTimeTaken / portionDone);

    uint64_t h = (timeLeft/3600); 
    uint64_t m = (timeLeft -(3600*h))/60;
    uint64_t s = (timeLeft -(3600*h)-(m*60));
    uint32_t nextChar = seed;
    
    if (args->progBar.type == 1){
        printf("#");
        nextChar += args->seedNum / args->progBar.value;
    }
    if (args->progBar.type == 2){
        nextChar += args->progBar.value;
        printf("\r%u/%u; %1.9lf%%; %lf/s; going to take %2.0lf:%2.0lf:%2.0lf             ",
        amountComplete, args->seedNum,
        (double)(amountComplete * 100) / (double)(args->seedNum),
        (double)amountComplete / totalTimeTaken,
        (double)h, (double)m, (double)s);
    }
    return nextChar;
}

typedef struct RoundResult {
    U32Array bads, fbads;
    U32Map cash;
} RoundResult;

static int searchSingleSeed(const uint32_t seed, const uint16_t roundStart, const uint16_t roundEnd, bool ver44, const Byte *groupsArray, ShuffleCache *cache, RoundResult *res){
    for (uint16_t round_ = roundStart; round_ < roundEnd; round_++){
        const ShuffleCacheEntry *groupIdxs = requestFromCache(cache, seed + round_, ver44, groupsArray);
        double cash = 0;
        float budget = (round_ * 4000 - 225000) * groupIdxs->budget_mult;
        const uint32_t offset = ((uint32_t) round_) * NUM_GROUPS;
        uint16_t nfbads = 0, nbads = 0;
        if (round_ >= 511){
            for (uint16_t i = 0; i < MAX_COUNT; i++){
                if (groupIdxs->r511Groups[i] == R511END){
                    break;
                }
                const BoundlessGroup group = getBoundlessGroup(groupIdxs->r511Groups[i]);
                if (group.score > budget){
                    continue;
                }
                budget -= group.score;
                cash += getCash(group.type, round_) * (double)group.count;
                if (group.type == FBAD_TYPE){
                    nfbads+=group.count;
                }
                if (group.type == BAD_TYPE){
                    nbads+=group.count;
                }
            }
        } else {
            for (uint16_t i = 0; i < NUM_GROUPS; i++){
                if (!bitget(
                    groupsArray,
                    offset + groupIdxs->array[i]
                )){
                    continue;
                }
                const BoundlessGroup group = getBoundlessGroup(groupIdxs->array[i]);
                if (group.score > budget){
                    continue;
                }
                budget -= group.score;
                cash += getCash(group.type, round_) * (double)group.count;
                if (group.type == FBAD_TYPE){
                    nfbads+=group.count;
                }
                if (group.type == BAD_TYPE){
                    nbads+=group.count;
                }
            }
        }
        if (U32Array_Add(&res[round_ - roundStart].bads, (uint32_t)nbads, 1)) return 1;
        if (U32Array_Add(&res[round_ - roundStart].fbads, (uint32_t)nfbads, 1)) return 1;
        if (U32Map_Add(&res[round_ - roundStart].cash, (uint32_t)round(cash * 50), 1)) return 1;
    }
    return 0;
}

int output(int threadNum, uint16_t round, uint16_t roundStart, const RoundResult *r){
    char fname[128] = {0};
    if (snprintf(
        fname,
        128,
        STATISTICS_THREAD_DIR "round-%hu_bads.txt",
        threadNum,
        round
    ) > 128 ){
        return 1;
    }
    FILE *file = fileopen(fname, "w");
    if (!file) return 1;
    for (size_t i = 0; i < r[round - roundStart].bads.length; i++){
        if (fprintf(file, "%u:%zu\n", r[round - roundStart].bads.vals[i], i) < 0) return 1;
    }
    if (fclose(file)) return 1;
    memset(fname, 0, 128);
    if (snprintf(
        fname,
        128,
        STATISTICS_THREAD_DIR "round-%hu_fbads.txt",
        threadNum,
        round
    ) > 128 ){
        return 1;
    }
    file = fileopen(fname, "w");
    if (!file) return 1;
    for (size_t i = 0; i < r[round - roundStart].fbads.length; i++){
        if (fprintf(file, "%u:%zu\n", r[round - roundStart].fbads.vals[i], i) < 0) return 1;
    }
    if (fclose(file)) return 1;
    memset(fname, 0, 128);
    if (snprintf(
        fname,
        128,
        STATISTICS_THREAD_DIR "round-%hu_cash.bin",
        threadNum,
        round
    ) > 128 ){
        return 1;
    }
    file = fileopen(fname, "wb");
    if (!file) return 1;
    for (size_t j = 0; j < r[round - roundStart].cash.mapSize; j++){
        uint32_t key = r[round - roundStart].cash.keys[j];
        if (!key) continue;
        uint32_t value = r[round - roundStart].cash.values[j];
        if (fwrite(&value, sizeof(uint32_t), 1, file) < 1){
            fclose(file);
            printf("errors and stuff\n");
            return 1;
        }
        if (fwrite(&key, sizeof(uint32_t), 1, file) < 1){
            fclose(file);
            printf("errors and stuff\n");
            return 1;
        }
    }
    if (fclose(file)) return 1;
    return 0;
}

crossThreadReturnValue seedStatistics(void *arg){
    seedSearchArg args = *(seedSearchArg*)arg;
    Byte *groups = makeGroupsArray(args.roundEnd);
    if (!groups){
        return (crossThreadReturnValue)1;
    }
    ShuffleCache cache;
    if (initCache(&cache, args.roundEnd - args.roundStart + 1, args.ver44, groups)){
        return (crossThreadReturnValue)1;
    }
    RoundResult *r = calloc(args.roundEnd - args.roundStart + 1, sizeof(RoundResult));
    if (!r) return (crossThreadReturnValue) 1;
    for (uint16_t round = 0; round < args.roundEnd - args.roundStart; round++){
        if (U32Array_Init(&r[round].bads, 2)) return (crossThreadReturnValue) 1;
        if (U32Array_Init(&r[round].fbads, 2)) return (crossThreadReturnValue) 1;
        if (U32Map_Init(&r[round].cash, 0x2000)) return (crossThreadReturnValue) 1;
    }
    const double startTime = (double)time_us();
    uint32_t nextPrint;
    const int threadNum = args.seedStart / args.seedNum;
    if ((threadNum % NUM_THREADS) == 0){
        nextPrint = 0;
    } else {
        nextPrint = UINT32_MAX;
    }
    {
        char path[128] = {0};
        if (snprintf(path, 128, STATISTICS_THREAD_DIR, threadNum) > 128){
            return (crossThreadReturnValue) 1;
        }
        if (ensureDirectoryExists(path)){
            return (crossThreadReturnValue) 1;
        }
    }
    for (uint32_t seed = args.seedStart; seed < args.seedStart + args.seedNum; seed++){
        if (searchSingleSeed(
            seed, args.roundStart, args.roundEnd, args.ver44, groups, &cache, r
        )){
            return (crossThreadReturnValue)1;
        }
        if (seed > nextPrint){
            double currentTime = (double)time_us();
            if (currentTime == (double)UINT64_MAX){
                perror("time_us failed: ");
                return (crossThreadReturnValue) 1;
            }
            nextPrint = displayCommandLineInfo(seed, startTime, currentTime, &args);
        }
    }
    for (uint16_t round = args.roundStart; round < args.roundEnd; round++){
        if (output(threadNum, round, args.roundStart, r)) return (crossThreadReturnValue) 1;
    }
    freeCache(&cache);
    free(groups);
    for (uint16_t round = 0; round < args.roundEnd - args.roundStart; round++){
        U32Array_Free(&r[round].bads);
        U32Array_Free(&r[round].fbads);
        U32Map_Free(&r[round].cash);
    }
    free(r);
    return (crossThreadReturnValue)0;
}
