//cc printing.c -o"printing" src/*.c -Iinclude -lz -lm -Wall -Wextra -std=c11 -pedantic

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <seedPrinting.h>

typedef enum helpCodes {
    extended = 1,
    error = 2,
} helpCodes;

void printHelp(helpCodes code){
    printf("%s\n", "usage: printing [-h] seed start end");
    if (code & error){
        printf("%s\n", "error: the following arguments are required: seed, start, end");
    }
    if (code & extended){
        printf("\n%s\n%s\n%s\n%s\n\n%s\n%s\n",
            "positional arguments:",
            "  seed - which seed to print from",
            "  start - which round to start printing from (inclusive)",
            "  end - which round to stop printing on (exclusive)",
            "options:",
            "  -h, -?, --help - show this help message and exit"
        );
    }
}

int main(int argc, char *argv[]){
    for (int i = 0; i < argc; i++){
        const char *arg = argv[i];
        if (
            strcmp(arg, "-h") == 0 ||
            strcmp(arg, "-?") == 0 || 
            strcmp(arg, "--help") == 0
        ){
            printHelp(extended);
            return 0;
        }
    }
    if (argc != 4){
        printHelp(error);
        return 1;
    }
    char *endptr;
    unsigned long seed = strtoul(argv[1], &endptr, 10);
    if (*endptr != '\0'){
        printf("error: seed argument encountered non numeric character\n");
        return 1;
    }
    if (seed > UINT32_MAX){
        printf("error: seed argument provided is larger than u32 max\n");
        return 1;
    }
    unsigned long start = strtoul(argv[2], &endptr, 10);
    if (*endptr != '\0'){
        printf("error: start argument encountered non numeric character\n");
        return 1;
    }
    if (start > UINT16_MAX){
        printf("error: start argument provided is larger than u16 max\n");
        return 1;
    }
    unsigned long end = strtoul(argv[3], &endptr, 10);
    if (*endptr != '\0'){
        printf("error: end argument encountered non numeric character\n");
        return 1;
    }
    if (end > UINT16_MAX){
        printf("error: end argument provided is larger than u16 max\n");
        return 1;
    }
    return printSeed(seed, start, end);
}
