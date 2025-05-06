#pragma once
#include <stdint.h>
#include "defines.h"

const char *getBloonName(int32_t bloontype);
const char *getBloonModifiers(int32_t bloontype);
double cashMultiplier(int32_t round);
double getCash(int32_t bloontype, int32_t round);
