#pragma once
#include <stdint.h>
#include "threading.h"
#include "compressedFile.h"
#include "shuffle.h"
#include "bitsArray.h"
#include "defines.h"

typedef struct progressBar {
    uint32_t type, value;
} progressBar;

typedef struct seedSearchArg {
    progressBar progBar;
    uint32_t seedStart, seedNum;
    uint32_t seedsPerFragment, fragmentsPerFile;
    uint16_t roundStart, roundEnd;
    bool ver44;
} seedSearchArg;

seedSearchArg *makearg(uint32_t seedStart, uint32_t seedNum,
                       uint32_t seedsPerFragment, uint32_t fragmentsPerFile,
                       uint16_t roundStart, uint16_t roundEnd,
                       uint32_t progBarType, uint32_t progBarValue,
                       bool ver44);

void freearg(seedSearchArg *arg);

crossThreadReturnValue searchSeeds(void *arg);
