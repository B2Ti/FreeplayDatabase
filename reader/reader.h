#pragma once
typedef struct results {
    uint32_t seed;
    Byte fbads[1000-141];
    Byte bads[1000-141];
    double cash[1000-141];
} results;

int read(CompressedFile *file, results *result);
int openFile(CompressedFile *file, const char *filepath, const char *mode, size_t data_size);
results *allocateResults(void);
void closeFile(CompressedFile *file);
void freeptr(void *ptr);
