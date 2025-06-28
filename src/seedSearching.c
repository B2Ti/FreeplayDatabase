#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <seedSearching.h>
#include <compressedFile.h>
#include <threading.h>
#include <shuffle.h>
#include <bitsArray.h>
#include <freeplayGroups.h>
#include <bloonStats.h>
#include <timing.h>
#include <zlib.h>
#include <stdatomic.h>

seedSearchArg *makearg(uint32_t seedStart, uint32_t seedNum,
                       uint32_t seedsPerFragment, uint32_t fragmentsPerFile,
                       uint16_t roundStart, uint16_t roundEnd,
                       uint32_t progBarType, uint32_t progBarValue,
                       bool ver44){
    seedSearchArg *arg = malloc(sizeof(seedSearchArg));
    if (!arg) {
        return arg;
    }
    arg->roundStart = roundStart;
    arg->roundEnd = roundEnd;
    arg->seedsPerFragment = seedsPerFragment;
    arg->fragmentsPerFile = fragmentsPerFile;
    arg->seedStart = seedStart;
    arg->seedNum = seedNum;
    arg->progBar.type = progBarType;
    arg->progBar.value = progBarValue;
    arg->ver44 = ver44;
    return arg;
}

void freearg(seedSearchArg *arg){
    free(arg);
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
        seed, args->seedNum,
        (double)(seed * 100) / (double)(args->seedStart + args->seedNum),
        (double)amountComplete / totalTimeTaken,
        (double)h, (double)m, (double)s);
    }
    return nextChar;
}

static int searchSingleSeed(const uint32_t seed, const uint16_t roundStart, const uint16_t roundEnd, bool ver44, CompressedFile *file, ShuffleCache *cache, const Byte *groupsArray){
    if (CompressedFile_Write56(file, seed, 32)){
        return 1;
    }
    for (uint16_t round = roundStart; round < roundEnd; round++){
        const ShuffleCacheEntry *groupIdxs = requestFromCache(cache, seed + round, ver44, groupsArray);
        double cash = 0;
        float budget = (round * 4000 - 225000) * groupIdxs->budget_mult;
        const uint32_t offset = ((uint32_t) round) * NUM_GROUPS;
        uint16_t nfbads = 0, nbads = 0;
        if (round >= 511){
            for (uint16_t i = 0; i < MAX_COUNT; i++){
                if (groupIdxs->r511Groups[i] == R511END){
                    break;
                }
                const BoundlessGroup group = getBoundlessGroup(groupIdxs->r511Groups[i]);
                if (group.score > budget){
                    continue;
                }
                budget -= group.score;
                cash += getCash(group.type, round) * (double)group.count;
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
                cash += getCash(group.type, round) * (double)group.count;
                if (group.type == FBAD_TYPE){
                    nfbads+=group.count;
                }
                if (group.type == BAD_TYPE){
                    nbads+=group.count;
                }
            }
        }
        if (CompressedFile_Write56(file, nfbads, 6)) {
            return 1;
        }
        if (CompressedFile_Write56(file, nbads, 6)) {
            return 1;
        }
        if (CompressedFile_Write56(file, (uint64_t)(cash * 50), 21)){
            return 1;
        }
    }
    return 0;
}

atomic_bool threadsPause;

crossThreadReturnValue searchSeeds(void *arg){
    seedSearchArg args = *(seedSearchArg*)arg;
    ShuffleCache cache;
    CompressedFile file;
    Byte *groups = makeGroupsArray(args.roundEnd);
    if (!groups){
        perror("malloc failed: ");
        return (crossThreadReturnValue) 1;
    }
    if (initCache(&cache, args.roundEnd, args.ver44, groups)){
        perror("malloc failed: ");
        return (crossThreadReturnValue) 1;
    }
    //probably could be smaller (better cpu cache locality perhaps?)
    uint32_t nextPrint;
    const double startTime = (double)time_us();
    if (startTime == (double)UINT64_MAX){
        perror("time_us failed: ");
        return (crossThreadReturnValue) 1;
    }
    if (CompressedFile_InitSeeds(&file, NULL, NULL, args.roundEnd-args.roundStart, FRAGMENT_SIZE)){
        perror("allocating compressfile failed: ");
        return (crossThreadReturnValue) 1;
    }
    const int threadNum = args.seedStart / args.seedNum;
    char path[100];
    memset(path, 0, 100);
    if (snprintf(path, 100, DATABASE_THREAD_DIR, threadNum) > 100){
        fprintf(stderr, "searchSeeds did not have a large enough buffer to store the filename\n");
        return (crossThreadReturnValue) 1;
    }
    if (ensureDirectoryExists(path)){
        fprintf(stderr, "searchSeeds could not create a folder for thread %d\n", threadNum);
        return (crossThreadReturnValue) 1;
    }
    if (threadNum == 0){
        nextPrint = 0;
    } else {
        nextPrint = UINT32_MAX;
    }
    uint32_t seed = args.seedStart;
    uint32_t seedEnd = args.seedNum + args.seedStart;
    while (seed < seedEnd){
        memset(path, 0, 100);
        if (snprintf(
                path, 100, DATABASE_THREAD_DIR "seeds_%d-%d.bin", threadNum,
                seed, seed + args.fragmentsPerFile * args.seedsPerFragment - 1
            ) >= 100
        ){
            fprintf(stderr, "searchSeeds did not have a large enough buffer to store the filename\n");     
            return (crossThreadReturnValue) 1;
        }
        if (CompressedFile_SetFilename(&file, &path[0], "ab")){
            perror("could not set filepath: ");
            return (crossThreadReturnValue) 1;
        }
        for (uint32_t i = 0; i < args.fragmentsPerFile; i++){
            for (uint32_t j = 0; j < args.seedsPerFragment; j++){
                if (searchSingleSeed(seed, args.roundStart, args.roundEnd, args.ver44, &file, &cache, groups)){
                    return (crossThreadReturnValue) 1;
                }
                seed++;
            }
            if (seed > nextPrint){
                double currentTime = (double)time_us();
                if (currentTime == (double)UINT64_MAX){
                    perror("time_us failed: ");
                    return (crossThreadReturnValue) 1;
                }
                nextPrint = displayCommandLineInfo(seed, startTime, currentTime, &args);
            }
            if (CompressedFile_Flush(&file, Z_BEST_COMPRESSION)){
                fprintf(stderr, "CompressedFile_Flush failed\n");
                return (crossThreadReturnValue) 1;
            }
            bool doPause = atomic_load(&threadsPause);
            if (threadNum == 0) {
                if (getInput() == 'q') {
                    atomic_store(&threadsPause, true);
                    sleep_ms(100);
                    printf("\npaused until 'c': ");
                    while (1) {
                        sleep_ms(100);
                        if (getInput() == 'c') {
                            threadsPause = false;
                            break;
                        }
                    }
                }
            } else if (doPause) {
                printf("\nthread %d pausing\n", threadNum);
                while (doPause) {
                    sleep_ms(100);
                    doPause = atomic_load(&threadsPause);
                }
                printf("\nthread %d continuing\n", threadNum);
            }
        }
    }
    free(groups);
    CompressedFile_Free(&file);
    freeCache(&cache);
    return (crossThreadReturnValue) 0;
}
