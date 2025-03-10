#include <stdint.h>
#include <timing.h>


#ifdef _WIN32
#include <windows.h>
#include <minwinbase.h>

/**
 * the windows thing but as a ull int
 */
static uint64_t time_100ns (void) {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    uint64_t timestamp = 0;
    timestamp |= ft.dwHighDateTime;
    timestamp <<= 32;
    timestamp |= ft.dwLowDateTime;
    return timestamp;
}
uint64_t time_us (void) {
    uint64_t t = time_100ns();
    return (t / 10) + ((t % 10) > 5); //adjust and round 100ns to us
}

void sleep_ms(int ms){
    Sleep(ms);
}
#elif defined __unix
#include <time.h>
uint64_t time_us (void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0){
        return UINT64_MAX;
    }
    uint64_t time_in_micros = 1000000 * tv.tv_sec + tv.tv_nsec / 1000;
    return time_in_micros;
}

void sleep_ms(int ms){
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, &ts);
}
#else
#error OS not supported
#endif

