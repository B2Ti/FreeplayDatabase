#pragma once
#include <stdint.h>
//returns UINT64_MAX if it fails (Linux), windows is just more confident in not failing
uint64_t time_us (void);
void sleep_ms(int ms);
