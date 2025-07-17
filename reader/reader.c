//cc reader\reader.c src\compressedFile.c src\timing.c -I include -L lib -lzlib -O3 -shared -o"reader.dll"

#include <compressedFile.h>
#include <seedSearching.h>
#include <defines.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <timing.h>
#include <zlib.h>
#include "reader.h"

int read(CompressedFile *file, Results *result){
    if (CompressedFile_LoadNextBuffer(file)) return 1;
    for (uint16_t j = 0; j < 100; j++){
        memset(result[j].fbads, 0, sizeof(Byte) * (1000-141));
        memset(result[j].bads, 0, sizeof(Byte) * (1000-141));
        memset(result[j].cash, 0, sizeof(double) * (1000-141));
        uint64_t seed = 0;
        if (CompressedFile_Read56(file, &seed, 32)) return 1;
        result[j].seed = seed;
        for (uint16_t i = 0; i < 1000 - 141; i++){
            uint64_t fbads = 0;
            uint64_t bads = 0;
            uint64_t cash = 0;
            if (CompressedFile_Read56(file, &fbads, 6)) return 1;
            if (CompressedFile_Read56(file, &bads, 6)) return 1;
            if (CompressedFile_Read56(file, &cash, 21)) return 1;
            result[j].fbads[i] = fbads;
            result[j].bads[i] = bads;
            result[j].cash[i] = (double)cash / 50.0;
        }
    }
    return 0;
}

int setPath(char *path, int buffer_size, int file){
    memset(path, 0, buffer_size);
    int thread = file / FILE_NUM;
    int seeds_per_file = FRAGMENT_NUM * FRAGMENT_SIZE;
    int err = snprintf(path, buffer_size,
        DATABASE_THREAD_DIR "seeds_%d-%d.bin",
        thread,
        file * seeds_per_file,
        -1 + (file + 1) * seeds_per_file
    );
    if (err >= buffer_size){
        return 1;
    }
    if (err < 0){
        return 1;
    }
    return 0;
}

int readFromFileNum(uint32_t filenum, Results *results, uint32_t low, uint32_t high){
    char path[128] = {0};
    CompressedFile file;
    uint32_t buffer_offset = low % FRAGMENT_SIZE;
    uint32_t result_size = 32 + 859 * 33;
    if (setPath(path, 128, filenum)){
        fprintf(stderr, "readFromFileNum: Failed to set filepath\n");
        return 1;
    }
    if (CompressedFile_InitSeeds(&file, path, "rb", END_ROUND - BEGIN_ROUND, FRAGMENT_SIZE)){
        fprintf(stderr, "readFromFileNum: Failed to open file\n");
        return 1;
    }
    for (uint32_t i = 0; i < (low / FRAGMENT_SIZE); i++){
        if (CompressedFile_SkipNextBuffer(&file)){
            fprintf(stderr, "readFromFileNum: Failed to read from file\n");
            CompressedFile_Free(&file);
            return 1;
        }
    }
    if (CompressedFile_LoadNextBuffer(&file)){
        fprintf(stderr, "readFromFileNum: Failed to read from file\n");
        CompressedFile_Free(&file);
        return 1;
    }
    if (CompressedFile_ShiftBufferIndex(&file, buffer_offset * result_size)){
        fprintf(stderr, "readFromFileNum: Failed to move buffer index\n");
        CompressedFile_Free(&file);
        return 1;
    }
    int64_t rem_seeds = FRAGMENT_SIZE - buffer_offset;
    for (uint32_t seed = 0; seed <= high - low; seed++){
        if (rem_seeds == 0){
            if (CompressedFile_LoadNextBuffer(&file)){
                fprintf(stderr, "readFromFileNum: Failed to read from file\n");
                CompressedFile_Free(&file);
                return 1;
            }
            rem_seeds = FRAGMENT_SIZE;
        }
        uint64_t data = 0;
        if (CompressedFile_Read56(&file, &data, 32)){
            fprintf(stderr, "readFromFileNum: Failed to read from buffer\n");
            CompressedFile_Free(&file);
            return 1;
        }
        //printf("Writing from %p to %p\n", (void*)&results[seed], (void*)&results[seed].cash[858]);
        results[seed].seed = data;
        for (uint16_t i = 0; i < END_ROUND - BEGIN_ROUND; i++){
            data = 0;
            if (CompressedFile_Read56(&file, &data, 6)){
                fprintf(stderr, "readFromFileNum: Failed to read from buffer\n");
                CompressedFile_Free(&file);
                return 1;
            }
            results[seed].fbads[i] = data;
            data = 0;
            if (CompressedFile_Read56(&file, &data, 6)){
                fprintf(stderr, "readFromFileNum: Failed to read from buffer\n");
                CompressedFile_Free(&file);
                return 1;
            }
            results[seed].bads[i] = data;
            data = 0;
            if (CompressedFile_Read56(&file, &data, 21)){
                fprintf(stderr, "readFromFileNum: Failed to read from buffer\n");
                CompressedFile_Free(&file);
                return 1;
            }
            results[seed].cash[i] = (double)data / 50.0;
        }
        rem_seeds--;
    }
    CompressedFile_Free(&file);
    return 0;
}

Results *readSeeds(uint32_t low, uint32_t high){
    uint32_t seeds_per_file = FRAGMENT_NUM * FRAGMENT_SIZE;
    Results *results = malloc((high - low + 1) * sizeof(Results));
    if (!results){
        perror("readSeeds: could not allocate results: ");
        return NULL;
    }
    //printf("Results are located at %p\n", (void *)results);
    //printf("End of result buffer = %p\n", (void *)(&results[high - low]));
    uint32_t first_file = low / seeds_per_file;
    uint32_t last_file = high / seeds_per_file;
    uint32_t inital_offset = low % seeds_per_file;
    uint32_t final_offset = high % seeds_per_file;
    size_t results_offset = 0;
    if (first_file == last_file){
        /* read from inital_offset -> final_offset */
        if (readFromFileNum(first_file, results, inital_offset, final_offset)){
            free(results);
            return NULL;
        }
        return results;
    }
    /* read from inital_offset -> seeds_per_file in first file */
    if (readFromFileNum(first_file, results, inital_offset, seeds_per_file - 1)){
        free(results);
        return NULL;
    }
    results_offset += seeds_per_file - inital_offset;
    /* read all in the files from first_file + 1 to last_file - 1 */
    for (uint32_t file_index = first_file + 1; file_index < last_file; file_index++){
        if (readFromFileNum(file_index, results + results_offset, 0, seeds_per_file - 1)){
            free(results);
            return NULL;
        }
        results_offset += seeds_per_file;
    }
    /* read from 0 -> inital_offset in last file */
    if (readFromFileNum(last_file, results + results_offset, 0, final_offset)){
        free(results);
        return NULL;
    }
    return results;
}

int openFile(CompressedFile *file, const char *filepath, const char *mode, size_t data_size){
    return CompressedFile_Init(file, filepath, mode, data_size);
}

Results *allocateResults(void){
    return malloc(100 * sizeof(Results));
}

void closeFile(CompressedFile *file){
    CompressedFile_Free(file);
}

void freeResults(Results *ptr){
    //printf("Freeing results at %p\n", (void *)ptr);
    free((void *)ptr);
}

void freeptr(void *ptr){
    free(ptr);
}
