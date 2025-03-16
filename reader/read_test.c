//cc reader/read_test.c reader/reader.c src\compressedFile.c src\timing.c -I include -L lib -lzlib -O3 -o"read-test.exe"

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
    if (CompressedFile_Init(&file, "lols.bin", "wb", 9+9+32)) return 1;
    if (CompressedFile_Write(&file, val1.bytes, 9)) return 1;
    if (CompressedFile_Write(&file, val2.bytes, 9)) return 1;
    if (CompressedFile_Write(&file, val3.bytes, 32)) return 1;
    if (CompressedFile_Flush(&file, Z_BEST_COMPRESSION)) return 1;
    return CompressedFile_Free(&file);
}

int write56(void){
    CompressedFile file;
    bitbuffer64 val1 = {.num = 0x12345};
    bitbuffer64 val2 = {.num = 0xabcdef};
    bitbuffer64 val3 = {.num = 0xDEADBEEF};
    if (CompressedFile_Init(&file, "lols.bin", "wb", 9+9+32)) return 1;
    if (CompressedFile_Write56(&file, val1.num, 9)) return 1;
    if (CompressedFile_Write56(&file, val2.num, 9)) return 1;
    if (CompressedFile_Write56(&file, val3.num, 32)) return 1;
    if (CompressedFile_Flush(&file, Z_BEST_COMPRESSION)) return 1;
    return CompressedFile_Free(&file);
}

int read_test56(void){
    CompressedFile file;
    bitbuffer64 val1 = {0};
    bitbuffer64 val2 = {0};
    bitbuffer64 val3 = {0};
    if (CompressedFile_Init(&file, "lols.bin", "rb", 9+9+32)) return 1;
    if (CompressedFile_LoadNextBuffer(&file)) return 1;
    if (CompressedFile_Read56(&file, &val1.num, 9)) return 1;
    if (CompressedFile_Read56(&file, &val2.num, 9)) return 1;
    if (CompressedFile_Read56(&file, &val3.num, 32)) return 1;
    printf("%llx, %llx, %llx\n", val1.num, val2.num, val3.num);
    return CompressedFile_Free(&file);
}
int read_test(void){
    CompressedFile file;
    bitbuffer64 val1 = {0};
    bitbuffer64 val2 = {0};
    bitbuffer64 val3 = {0};
    if (CompressedFile_Init(&file, "lols.bin", "rb", 9+9+32)) return 1;
    if (CompressedFile_LoadNextBuffer(&file)) return 1;
    if (CompressedFile_Read(&file, &val1.bytes[0], 9)) return 1;
    if (CompressedFile_Read(&file, &val2.bytes[0], 9)) return 1;
    if (CompressedFile_Read(&file, &val3.bytes[0], 32)) return 1;
    printf("%llx, %llx, %llx\n", val1.num, val2.num, val3.num);
    return CompressedFile_Free(&file);
}

int main(void){
    if (write56()){
        fprintf(stderr, "write operation failed\n");
        return 1;
    }
    if (read_test56()){
        fprintf(stderr, "write operation failed\n");
        return 1;
    }
    if (write()){
        fprintf(stderr, "write operation failed\n");
        return 1;
    }
    if (read_test56()){
        fprintf(stderr, "write operation failed\n");
        return 1;
    }
    uint64_t time_start = time_us();
    CompressedFile file;
    if (CompressedFile_InitSeeds(&file, "database/thread-0/seeds_0-99999.bin", "rb", 1000-141, 100)){
        fprintf(stderr, "failed to open file\n");
        return 1;
    }
    Results *r = malloc(100 * sizeof(Results));
    if (!r) {
        fprintf(stderr, "failed to allocate results\n");
        return 1;
    }
    read(&file, r);
    CompressedFile_Free(&file);
    free(r);
    uint64_t time_end = time_us();
    printf("reading 100 seeds took %llu us\n", time_end - time_start);
    time_start = time_us();
    r = readSeeds(110, 120);
    if (!r){
        fprintf(stderr, "readSeeds returned NULL\n");
        return 1;
    } 
    free(r);
    time_end = time_us();
    printf("reading %d seeds took %llu us\n", 202-99, time_end - time_start);
    return 0;
}
