#pragma once
typedef struct Results {
    uint32_t seed;
    Byte fbads[1000-141];
    Byte bads[1000-141];
    double cash[1000-141];
} Results;

int read(CompressedFile *file, Results *result);
Results *readSeeds(uint32_t low, uint32_t high);
int openFile(CompressedFile *file, const char *filepath, const char *mode, size_t data_size);
Results *allocateResults(void);
void closeFile(CompressedFile *file);
void freeptr(void *ptr);
