#include <threading.h>
#include <stdbool.h>
#include <stdint.h>

#if defined _WIN32
int CreateCrossThread(crossThread *thread, crossThreadReturnValue (func)(void *), void *args){
    DWORD threadID;
    *thread = CreateThread(NULL,
                            0,
                            (LPTHREAD_START_ROUTINE) func,
                            (LPVOID) args,
                            0,
                            &threadID);
    if (thread == NULL){
        return CrossThreadingFail;
    } else {
        return CrossThreadingSuccess;
    }
}
int JoinCrossThreads(int32_t numThreads, crossThread *threads, crossThreadReturnValue *returnValues){
    int32_t i;
    DWORD result = WAIT_TIMEOUT;
    //incase 49.7 days is not long enough
    while (result == WAIT_TIMEOUT){
        result = WaitForMultipleObjects(numThreads, threads, true, INFINITE);
    }
    if (result == ((DWORD)0xFFFFFFFF)){
        return CrossThreadingFail;
    }
    if (result >= WAIT_ABANDONED_0){
        return AbandonedMutex;
    }
    if (returnValues == NULL){
        return CrossThreadingSuccess;
    }
    for (i=0; i < numThreads; i++){
        result = (DWORD)GetExitCodeThread(threads[i], &returnValues[i]);
        if (result == 0){
            return CrossThreadingFail;
        }
    }
    return CrossThreadingSuccess;
}
#elif defined __unix || defined __APPLE__
int CreateCrossThread(crossThread *thread, crossThreadReturnValue (func)(void *), void *args){
    int resultCode = pthread_create(thread, NULL, func, args);
    return resultCode;
}
int JoinCrossThreads(int32_t numThreads, crossThread *threads, crossThreadReturnValue *returnValues){
    int32_t i;
    int32_t result;
    if (returnValues == NULL){
        for (i=0; i < numThreads; i++){
            result |= pthread_join(threads[i], NULL);
        }
        if (result){
            return CrossThreadingFail;
        }
    } else {
        for (i=0; i < numThreads; i++){
            result |= pthread_join(threads[i], &returnValues[i]);
        }
        if (result){
            return CrossThreadingFail;
        }
    }
    return 0;
}
#else
#error "OS not supported"
#endif
