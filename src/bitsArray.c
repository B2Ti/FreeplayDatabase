#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <defines.h>
#include <freeplayGroups.h>
#include <bitsArray.h>

Byte bitget(const Byte *bits, const size_t idx){
    return (bits[idx >> 3] >> (idx & 0x7)) & 1;
}


void bitset(Byte *bits, size_t idx, bool value){
    if (value) bits[idx/8] |= (1 << (idx % 8));
    else bits[idx/8] &= ~(1 << (idx % 8));
}

Byte *makeGroupsArray(size_t maximumRound){
    size_t n_bits = NUM_GROUPS * maximumRound;
    size_t bytes = ((n_bits+7) / 8);
    Byte *bits = malloc(bytes);
    if (!bits){
        perror("makeGroupsArray could not allocate bits: ");
        return bits;
    }
    bool inbounds;
    double round_d;
    Group group;
    for (size_t g = 0; g < NUM_GROUPS; g++){
        group = getGroup(g);
        for (size_t round = 0; round < maximumRound; round++){
            round_d = (double)round;
            inbounds = false;
            for (int j = 0; j < NUM_BOUNDS; j++){
                double lbound = group.bounds[2*j];
                double ubound = group.bounds[1 + 2*j];
                if ((lbound <= round_d) && (round_d <= ubound)){
                    inbounds = true;
                    break;
                }
            }
            bitset(bits, round * NUM_GROUPS + g, inbounds);
        }
    }
    return bits;
}


