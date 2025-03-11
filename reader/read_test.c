//cc read_test.c reader.c ..\src\compressedFile.c ..\src\timing.c -I ../include -L ../lib -lzlib -O3 -o"test.exe"

#include <compressedFile.h>
#include <stdlib.h>
#include <string.h>
#include <timing.h>
#include <zlib.h>
#include "reader.h"

typedef union bitbuffer64 {
    Byte bytes[8];
    uint64_t num;
} bitbuffer64;

int write(void){
    CompressedFile file;
    bitbuffer64 val1 = {.num = 0x12345};
    bitbuffer64 val2 = {.num = 0xabcdef};
    bitbuffer64 val3 = {.num = 0xDEADBEEF};
    if (createCompressedFile(&file, "lols.bin", "wb", 9+9+32)) return 1;
    if (writeToBuffer(&file, val1.bytes, 9)) return 1;
    if (writeToBuffer(&file, val2.bytes, 9)) return 1;
    if (writeToBuffer(&file, val3.bytes, 32)) return 1;
    if (dumpBufferToFile(&file, Z_BEST_COMPRESSION)) return 1;
    freeCompressedFile(&file);
    return 0;
}

int write56(void){
    CompressedFile file;
    bitbuffer64 val1 = {.num = 0x12345};
    bitbuffer64 val2 = {.num = 0xabcdef};
    bitbuffer64 val3 = {.num = 0xDEADBEEF};
    if (createCompressedFile(&file, "lols.bin", "wb", 9+9+32)) return 1;
    if (writeToBuffer56(&file, val1.num, 9)) return 1;
    if (writeToBuffer56(&file, val2.num, 9)) return 1;
    if (writeToBuffer56(&file, val3.num, 32)) return 1;
    if (dumpBufferToFile(&file, Z_BEST_COMPRESSION)) return 1;
    freeCompressedFile(&file);
    return 0;
}

int read_test56(void){
    CompressedFile file;
    bitbuffer64 val1 = {0};
    bitbuffer64 val2 = {0};
    bitbuffer64 val3 = {0};
    if (createCompressedFile(&file, "lols.bin", "rb", 9+9+32)) return 1;
    if (readBufferFromFile(&file)) return 1;
    if (readFromBufferedFile56(&file, &val1.num, 9)) return 1;
    if (readFromBufferedFile56(&file, &val2.num, 9)) return 1;
    if (readFromBufferedFile56(&file, &val3.num, 32)) return 1;
    printf("%llx, %llx, %llx\n", val1.num, val2.num, val3.num);
    freeCompressedFile(&file);
    return 0;
}
int read_test(void){
    CompressedFile file;
    bitbuffer64 val1 = {0};
    bitbuffer64 val2 = {0};
    bitbuffer64 val3 = {0};
    if (createCompressedFile(&file, "lols.bin", "rb", 9+9+32)) return 1;
    if (readBufferFromFile(&file)) return 1;
    if (readFromBufferedFile(&file, &val1.bytes[0], 9)) return 1;
    if (readFromBufferedFile(&file, &val2.bytes[0], 9)) return 1;
    if (readFromBufferedFile(&file, &val3.bytes[0], 32)) return 1;
    printf("%llx, %llx, %llx\n", val1.num, val2.num, val3.num);
    freeCompressedFile(&file);
    return 0;
}

int main(void){
    write56();
    read_test56();
    write();
    read_test56();
    uint64_t time_start = time_us();
    CompressedFile file;
    createCompressedFileSeeds(&file, "database/thread-0/seeds_0-999.bin", "rb", 1000-141, 100);
    results *r = malloc(100 * sizeof(results));
    read(&file, r);
    freeCompressedFile(&file);
    free(r);
    uint64_t time_end = time_us();
    printf("took %llu us\n", time_end - time_start);
}
