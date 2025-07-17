#include <timing.h>
#include "reader.h"
#include <inttypes.h>
#include <stdlib.h>

int main(void){
    uint32_t start = 99;
    uint32_t end = 1098;
    uint64_t begin_time = time_us();
    Results *results = readSeeds(start, end);
    if (!results) return 1;
    uint64_t end_time = time_us();
    printf("Took %" PRIu64 " us to read %" PRIu32 " seeds\n", end_time - begin_time, end - start);
    free(results);
    return 0;
}
