// zlib may be installed as zlib.lib or just zlib, which need -lzlib and -lz respectively
// 1                ) clang src/*.c -Iinclude -Llib -lzlib -O3 -o"main-10.exe" -march=native -mfma -mavx2 -DRUNNINGTEST=1 -fprofile-generate
// 2                ) ./main-10.exe
// 3 Powershell     ) rd -r  ./database
// 3 Linux Terminal ) rm -r ./database
// 4                ) llvm-profdata merge ./*.profraw -output="default.profdata"
// 5                ) clang src/*.c -Iinclude -Llib -lzlib -O3 -o"main-10.exe" -march=native -mfma -mavx2 -fprofile-use
// 6                ) ./main-10.exe

#include <threading.h>
#include <compressedFile.h>
#include <seedSearching.h>
#include <defines.h>
#include <stdio.h>

/*
void test(void){
    seedSearchArg *args;
    args = makearg(0, FILE_NUM * FRAGMENT_NUM * FRAGMENT_SIZE,
                   FRAGMENT_SIZE, FRAGMENT_NUM, 
                   BEGIN_ROUND, END_ROUND,
                   PROGRESS_BAR_TYPE, PROGRESS_BAR_VALUE);
    if (args == NULL){
        perror("failed to allocate argument: ");
        exit(1);
    }
    crossThreadReturnValue v = searchSeeds((void *)args);
    printf("\nthread exited with code: %d\n", (int)v);
    return;
}
*/

int main(void){
    if (ensureDirectoryExists("database/")){
        fprintf(stderr, "cannot create database directory");
        return 1;
    }
    seedSearchArg *args[NUM_THREADS];
    crossThread threads[NUM_THREADS];
    crossThreadReturnValue values[NUM_THREADS];
    for(uint32_t i = 0; i < NUM_THREADS; i++){
        args[i] = makearg(i * FILE_NUM * FRAGMENT_NUM * FRAGMENT_SIZE,
                          FILE_NUM * FRAGMENT_NUM * FRAGMENT_SIZE,
                          FRAGMENT_SIZE, FRAGMENT_NUM,
                          BEGIN_ROUND, END_ROUND,
                          PROGRESS_BAR_TYPE, PROGRESS_BAR_VALUE);
        if (args[i] == NULL){
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
        fprintf(stderr, "seedSearching thread %d returned with exit code %zu\n", i, (size_t)values[i]);
    }
    return 0;
}
