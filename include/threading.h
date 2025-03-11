#pragma once

#include <stdbool.h>
#include <stdint.h>

#define AbandonedMutex 2
#define CrossThreadingFail 1
#define CrossThreadingSuccess 0

#if defined _WIN32
#include <windows.h>
typedef HANDLE crossThread;
typedef DWORD crossThreadReturnValue;
#elif defined __unix
#include <pthread.h>
typedef pthread_t crossThread;
typedef void* crossThreadReturnValue;
#else
#error "OS not supported"
#endif
//so if you are used to the windows standard library then unfortunately you cannot
//seperate joining the thread and getting the return value with pthreads (the other threading library)
int CreateCrossThread(crossThread *thread, crossThreadReturnValue (func)(void *), void *args);
//returnValues = ptr to return value pointers, which will be set to the exit/return values of the threads
//if returnValues == NULL the exit/return values will be discarded
int JoinCrossThreads(int32_t numThreads, crossThread *theads, crossThreadReturnValue *returnValues);
