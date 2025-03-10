#pragma once
#include <stdint.h>
#include "defines.h"

extern const double bloonCash[18][2];

double cashMultiplier(int32_t round);
double getCash(int32_t bloontype, int32_t round);
