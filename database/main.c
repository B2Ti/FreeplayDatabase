// zlib may be installed as zlib.lib or just zlib, which need -lzlib and -lz respectively
// 1                ) clang src/*.c database.c -Iinclude -Llib -lzlib -O3 -o"main-10.exe" -march=native -mfma -mavx2 -DRUNNINGTEST=1 -fprofile-generate
// 2                ) ./main-10.exe
// 3 Powershell     ) rd -r  ./database-results
// 3 Linux Terminal ) rm -r ./database-results
// 4                ) llvm-profdata merge ./*.profraw -output="default.profdata"
// 5                ) clang src/*.c database.c -Iinclude -Llib -lzlib -O3 -o"main-10.exe" -march=native -mfma -mavx2 -fprofile-use
// 6                ) ./main-10.exe

#include <threading.h>
#include <compressedFile.h>
#include <seedSearching.h>
#include <defines.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test(void){
    seedSearchArg *args;
    args = makearg(0, FILE_NUM * FRAGMENT_NUM * FRAGMENT_SIZE,
                   FRAGMENT_SIZE, FRAGMENT_NUM, 
                   BEGIN_ROUND, END_ROUND,
                   PROGRESS_BAR_TYPE, PROGRESS_BAR_VALUE, false);
    if (args == NULL){
        perror("failed to allocate argument: ");
        return 1;
    }
    crossThreadReturnValue v = searchSeeds((void *)args);
    printf("\nthread exited with code: %" PRIuPTR "\n", (uintptr_t)v);
    return 0;
}


int main(int argc, char *argv[]){
    if (ensureDirectoryExists(DATABASE_DIR)){
        fprintf(stderr, "main: could not create database directory\n");
        return 1;
    }
    bool ver44 = false;
    if (argc > 1){
        if (argv[1][0] == 't'){
            return test();
        }
        if (strcmp(argv[1], "v44")){
            ver44 = true;
        }
    }
    seedSearchArg *args[NUM_THREADS];
    crossThread threads[NUM_THREADS];
    crossThreadReturnValue values[NUM_THREADS];
    for(uint32_t i = 0; i < NUM_THREADS; i++){
        args[i] = makearg(i * FILE_NUM * FRAGMENT_NUM * FRAGMENT_SIZE,
                          FILE_NUM * FRAGMENT_NUM * FRAGMENT_SIZE,
                          FRAGMENT_SIZE, FRAGMENT_NUM,
                          BEGIN_ROUND, END_ROUND,
                          PROGRESS_BAR_TYPE, PROGRESS_BAR_VALUE, ver44);
        if (!args[i]){
            perror("\nfailed to allocate argument: \n");
            return 1;
        }
        CreateCrossThread(&threads[i], searchSeeds, args[i]);
    }
    if (JoinCrossThreads(NUM_THREADS, threads, &values[0])){
        perror("\nthreading operation failed: \n");
        return 1;
    }
    printf("\n");
    for (uint32_t i = 0; i < NUM_THREADS; i++){
        free(args[i]);
        printf("seedSearching thread %d returned with exit code %zu\n", i, (size_t)values[i]);
    }
    return 0;
}
