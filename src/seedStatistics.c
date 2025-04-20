#include <seedStatistics.h>
#include <seedSearching.h>
#include <freeplayGroups.h>
#include <bloonStats.h>
#include <threading.h>
#include <timing.h>

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
    fprintf(stderr, "[INFO] U32Array_Init: allocated %zu bytes to %p\n", size * sizeof(uint32_t), (void *)vals);
    arr->vals = vals;
    arr->length = 0;
    arr->capacity = size;
    return 0;
}

static void U32Array_Free(U32Array *arr){
    if (!arr) return;
    if (arr->vals){
        fprintf(stderr, "[INFO] U32Array_Free: freed memory at %p\n", (void *)arr->vals);
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
        fprintf(stderr, "[INFO] U32Array_Add: reallocated %zu bytes to %p\n", arr->capacity * sizeof(uint32_t), (void *)vals);
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

typedef struct U32Map {
    U32Array *keys;
    U32Array *values;
    size_t mapSize;
} U32Map;

uint32_t FVN1_U32(uint32_t num){
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
    U32Array *keys = calloc(size, sizeof(U32Array));
    if (!keys){
        perror("U32Map_Init");
        return 1;
    }
    U32Array *vals = calloc(size, sizeof(U32Array));
    if (!vals){
        free(keys);
        perror("U32Map_Init");
        return 1;
    }
    fprintf(stderr, "[INFO] U32Map_Init: allocated %zu bytes to %p\n", size * sizeof(U32Array), (void *)keys);
    fprintf(stderr, "[INFO] U32Map_Init: allocated %zu bytes to %p\n", size * sizeof(U32Array), (void *)vals);
    map->keys = keys;
    map->values = vals;
    map->mapSize = size;
    return 0;
}

static void U32Map_Free(U32Map *map){
    if (!map) return;
    for (uint32_t i = 0; i < map->mapSize; i++){
        if (map->keys[i].length){
            U32Array_Free(&map->keys[i]);
        }
        if (map->values[i].length){
            U32Array_Free(&map->values[i]);
        }
    }
    fprintf(stderr, "[INFO] U32Map_Free: freeing memory at %p\n", (void *)map->keys);
    fprintf(stderr, "[INFO] U32Map_Free: freeing memory at %p\n", (void *)map->values);
    free(map->keys);
    map->keys = NULL;
    free(map->values);
    map->values = NULL;
    map->mapSize = 0;
}

static int U32Map_Add(U32Map *map, uint32_t key, uint32_t val){
    uint32_t idx = FVN1_U32(key) % map->mapSize;
    U32Array *keys = &map->keys[idx];
    if (keys->capacity == 0){
        if (U32Array_Init(keys, 2)){
            return 1;
        }
    }
    for (size_t i = 0; i < keys->length; i++){
        if (keys->vals[i] == key){
            map->values[idx].vals[i] += val;
            return 0;
        }
    }
    if (U32Array_Add(keys, keys->length, key)){
        return 1;
    }
    if (U32Array_Add(&map->values[idx], map->values[idx].length, val)){
        return 1;
    }
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
        //printf("\r                                                                                   ");
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

static int searchSingleSeed(const uint32_t seed, const uint16_t roundStart, const uint16_t roundEnd, const Byte *groupsArray, ShuffleCache *cache, RoundResult *res){
    for (uint16_t round = roundStart; round < roundEnd; round++){
        const ShuffleCacheEntry *groupIdxs = requestFromCache(cache, seed + round, groupsArray);
        double cash = 0;
        float budget = (round * 4000 - 225000) * groupIdxs->budget_mult;
        const uint32_t offset = ((uint32_t) round) * NUM_GROUPS;
        uint16_t nfbads = 0, nbads = 0;
        for (uint16_t i = 0; i < NUM_GROUPS; i++){
            if (!bitget(groupsArray, offset + groupIdxs->array[i])){
                continue;
            }
            const BoundlessGroup group = getBoundlessGroup(groupIdxs->array[i]);
            if (group.score > budget){
                continue;
            }
            budget -= group.score;
            cash += getCash(group.type, round) * (double)group.count;
            if (group.type == FBAD_TYPE){
                nfbads += group.count;
            }
            if (group.type == BAD_TYPE){
                nbads += group.count;
            }
        }
        if (U32Array_Add(&res[round - roundStart].bads, (uint32_t)nbads, 1)) return 1;
        if (U32Array_Add(&res[round - roundStart].fbads, (uint32_t)nfbads, 1)) return 1;
        if (U32Map_Add(&res[round - roundStart].cash, (uint32_t)(cash * 50), 1)) return 1;
    }
    return 0;
}

int output(int threadNum, uint16_t round, uint16_t roundStart, const RoundResult *r){
    char fname[128] = {0};
    if (snprintf(
        fname,
        128,
        "statistics/thread-%d/round-%hu_bads.txt",
        threadNum,
        round
    ) > 128 ){
        return 1;
    }
    //printf("here1\n");
    FILE *file = fileopen(fname, "w");
    if (!file) return 1;
    //printf("here2\n");
    for (size_t i = 0; i < r[round - roundStart].bads.length; i++){
        if (fprintf(file, "%u:%zu\n", r[round - roundStart].bads.vals[i], i) < 0) return 1;
    }
    //printf("here3\n");
    if (fclose(file)) return 1;
    //printf("here4\n");
    memset(fname, 0, 128);
    if (snprintf(
        fname,
        128,
        "statistics/thread-%d/round-%hu_fbads.txt",
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
        "statistics/thread-%d/round-%hu_cash.bin",
        threadNum,
        round
    ) > 128 ){
        return 1;
    }
    file = fileopen(fname, "wb");
    if (!file) return 1;
    for (size_t j = 0; j < r[round - roundStart].cash.mapSize; j++){
        U32Array keys = r[round - roundStart].cash.keys[j];
        U32Array values = r[round - roundStart].cash.values[j];
        for (size_t i = 0; i < keys.length; i++){
            //if (fprintf(file, "%u:%f\n",
            //    values.vals[i], (double)keys.vals[i] / 50.0
            //) < 0) return 1;
            if (fwrite(&values.vals[i], sizeof(uint32_t), 1, file) < 1){
                printf("errors and stuff\n");
                return 1;
            }
            if (fwrite(&keys.vals[i], sizeof(uint32_t), 1, file) < 1){
                printf("errors and stuff\n");
                return 1;
            }
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
    if (initCache(&cache, args.roundEnd - args.roundStart + 1, groups)){
        return (crossThreadReturnValue)1;
    }
    RoundResult *r = calloc(args.roundEnd - args.roundStart + 1, sizeof(RoundResult));
    if (!r) return (crossThreadReturnValue) 1;
    fprintf(stderr, "[INFO] seedStatistics: allocated %zu bytes to %p\n", (args.roundEnd - args.roundStart + 1) * sizeof(RoundResult), (void *)r);
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
        if (snprintf(path, 128, "statistics/thread-%d", threadNum) > 128){
            return (crossThreadReturnValue) 1;
        }
        if (ensureDirectoryExists(path)){
            return (crossThreadReturnValue) 1;
        }
    }
    for (uint32_t seed = args.seedStart; seed < args.seedStart + args.seedNum; seed++){
        if (searchSingleSeed(
            seed, args.roundStart, args.roundEnd, groups, &cache, r
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
    fprintf(stderr, "[INFO] seedStatistics: freed memory at %p\n", (void *)r);
    free(r);
    return (crossThreadReturnValue)0;
}
