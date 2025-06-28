#pragma once
#include <stddef.h>
#include <stdbool.h>

#ifndef ZCONF_H
 typedef unsigned char Byte;
#endif

Byte bitget(const Byte *bits, const size_t idx);
void bitset(Byte *bits, size_t idx, bool value);
Byte *makeGroupsArray(size_t maximumRound);

// bool boolget(const bool *bits, const size_t idx);
// void boolset(bool *bits, size_t idx, bool value);
// bool *makeBoolGroupsArray(size_t maximumRound);
