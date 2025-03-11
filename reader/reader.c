//cc reader.c ..\src\compressedFile.c ..\src\timing.c -I ../include -L ../lib -lzlib -O3 -shared -o"reader.dll"

#include <compressedFile.h>
#include <stdlib.h>
#include <string.h>
#include <timing.h>
#include <zlib.h>
#include "reader.h"

int read(CompressedFile *file, results *result){
    if (readBufferFromFile(file)) return 1;
    for (uint16_t j = 0; j < 100; j++){
        memset(result[j].fbads, 0, sizeof(Byte) * (1000-141));
        memset(result[j].bads, 0, sizeof(Byte) * (1000-141));
        memset(result[j].cash, 0, sizeof(double) * (1000-141));
        uint64_t seed = 0;
        if (readFromBufferedFile56(file, &seed, 32)) return 1;
        result[j].seed = seed;
        for (uint16_t i = 0; i < 1000 - 141; i++){
            uint64_t fbads = 0;
            uint64_t bads = 0;
            uint64_t cash = 0;
            if (readFromBufferedFile56(file, &fbads, 6)) return 1;
            if (readFromBufferedFile56(file, &bads, 6)) return 1;
            if (readFromBufferedFile56(file, &cash, 21)) return 1;
            result[j].fbads[i] = fbads;
            result[j].bads[i] = bads;
            result[j].cash[i] = (double)cash / 50.0;
        }
    }
    return 0;
}

int openFile(CompressedFile *file, const char *filepath, const char *mode, size_t data_size){
    return createCompressedFile(file, filepath, mode, data_size);
}

results *allocateResults(void){
    return malloc(100 * sizeof(results));
}

void closeFile(CompressedFile *file){
    freeCompressedFile(file);
}

void freeptr(void *ptr){
    free(ptr);
}