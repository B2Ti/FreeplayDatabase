#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <seedPrinting.h>
#include <shuffle.h>
#include <freeplayGroups.h>
#include <bitsArray.h>
#include <bloonStats.h>

static int printSingleRound(const uint32_t seed, const uint16_t round, bool ver44, ShuffleCache *cache, const Byte *groupsArray){
    const ShuffleCacheEntry *groupIdxs = requestFromCache(cache, seed + round, ver44, NULL);
    double cash = 0.0;
    uint16_t nbads = 0, nfbads = 0;
    float budget = (4000 * round - 225000) * groupIdxs->budget_mult;
    const uint32_t offset = (uint32_t)round * NUM_GROUPS;
    printf("+------------------------------------------------------+\n");
    printf("| ROUND %-46d |\n", round);
    printf("| BUDGET %-45.2f |\n", budget);
    printf("+------------------+-----------------+-----------------+\n");
    printf("|            Bloon |           Count |           Index |\n");
    printf("+------------------+-----------------+-----------------+\n");
    for (uint16_t i = 0; i < NUM_GROUPS; i++){
        if (!bitget(groupsArray, offset + groupIdxs->array[i])){
            continue;
        }
        
        const BoundlessGroup group = getBoundlessGroup(groupIdxs->array[i]);

        if (group.score > budget){
            continue;
        }

        int spaces, tmp;
        spaces = 18;
        tmp = printf("| %s", getBloonName(group.type));
        if (tmp < 0) return 1;
        spaces -= tmp;
        tmp = printf("%s", getBloonModifiers(group.type));
        if (tmp < 0) return 1;
        spaces -= tmp;
        if (spaces > 0){
            printf("%*c", spaces, ' ');
        }
        printf(" | %15d | %15d |\n", group.count, groupIdxs->array[i]);

        budget -= group.score;
        cash += getCash(group.type, round) * (double)group.count;

        if (group.type == FBAD_TYPE) nfbads += group.count;
        if (group.type == BAD_TYPE) nbads += group.count;
    }
    printf("+------------------+-----------------+-----------------+\n");
    printf("| Cash: %-46.2lf |\n", cash);
    printf("| BADs: %-46hu |\n", nbads);
    printf("| FBADs: %-45hu |\n", nfbads);
    printf("+------------------------------------------------------+\n");
    return 0;
}

int printSeed(uint32_t seed, uint16_t roundStart, int16_t roundEnd, bool ver44){
    ShuffleCache cache;
    /*print including the end round*/
    roundEnd++;
    Byte *groups = makeGroupsArray(roundEnd);
    if (!groups){
        return 1;
    }
    if (initCache(&cache, roundEnd, ver44, NULL)){
        free(groups);
        return 1;
    }
    for (uint16_t round = roundStart; round < roundEnd; round++){
        if (printSingleRound(seed, round, ver44, &cache, groups)){
            return 1;
        }
    }
    return 0;
}
