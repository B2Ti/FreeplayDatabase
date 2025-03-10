#pragma once
#include <stdint.h>
#include <stddef.h>
#include "defines.h"


typedef struct Group {
    float score;
    uint16_t type;
    uint16_t count;
    int32_t bounds[10];
} Group;

typedef struct BoundlessGroup {
    float score;
    uint16_t type;
    uint16_t count;
} BoundlessGroup;

const Group *getGroupPtr(const size_t index);
Group getGroup(const size_t index);
BoundlessGroup getBoundlessGroup(const size_t index);
