#include <threading.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#if defined _WIN32
int CreateCrossThread(crossThread *thread, crossThreadReturnValue (func)(void *), void *args){
    DWORD threadID;
    *thread = CreateThread(NULL,
                            0,
                            (LPTHREAD_START_ROUTINE) func,
                            (LPVOID) args,
                            0,
                            &threadID);
    if (!thread){
        fprintf(stderr, "CreateCrossThread: CreateThread returned error code %lu\n", GetLastError());
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
        fprintf(stderr, "JoinCrossThreads: WaitForMultipleObjects returned error code %lu\n", GetLastError());
        return CrossThreadingFail;
    }
    if (result >= WAIT_ABANDONED_0){
        fprintf(stderr, "JoinCrossThreads: WaitForMultipleObjects returned WAIT_ABANDONED\n");
        return AbandonedMutex;
    }
    if (!returnValues){
        return CrossThreadingSuccess;
    }
    for (i=0; i < numThreads; i++){
        result = (DWORD)GetExitCodeThread(threads[i], &returnValues[i]);
        if (result == 0){
            fprintf(stderr, "JoinCrossThreads: GetExitCodeThread returned error code %lu\n", GetLastError());
            return CrossThreadingFail;
        }
    }
    return CrossThreadingSuccess;
}
#elif defined __unix || defined __APPLE__
int CreateCrossThread(crossThread *thread, crossThreadReturnValue (func)(void *), void *args){
    int resultCode = pthread_create(thread, NULL, func, args);
    if (resultCode){
        perror("CreateCrossThread unable to create new thread: ");
    }
    return resultCode;
}
int JoinCrossThreads(int32_t numThreads, crossThread *threads, crossThreadReturnValue *returnValues){
    int32_t i = 0;
    int32_t result = 0;
    if (!returnValues){
        for (i=0; i < numThreads; i++){
            result |= pthread_join(threads[i], NULL);
        }
        if (result){
            perror("JoinCrossThreads pthread_join failed: ");
            return CrossThreadingFail;
        }
    } else {
        for (i=0; i < numThreads; i++){
            result |= pthread_join(threads[i], &returnValues[i]);
        }
        if (result){
            perror("JoinCrossThreads pthread_join failed: ");
            return CrossThreadingFail;
        }
    }
    return 0;
}
#else
#error "OS not supported"
#endif
