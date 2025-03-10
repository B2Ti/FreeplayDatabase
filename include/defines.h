#pragma once
#define NULL_PTR_FAIL -1
#define MALLOC_FAIL 1
#define FILE_FAIL 2
#define MISC_FAIL 3

#define NUM_GROUPS 529
#define NUM_BOUNDS 5

#define BAD_TYPE 136
#define FBAD_TYPE 140

#define ROLLING_SUM_SIZE 25

#define BEGIN_ROUND 141
#define END_ROUND 1000

#ifndef PROGRESS_BAR_VALUE
 #define PROGRESS_BAR_VALUE 50
#endif
#ifndef PROGRESS_BAR_TYPE
 #define PROGRESS_BAR_TYPE 2
#endif

#ifndef RUNNINGTEST
 #define RUNNINGTEST 0
#endif

#ifndef THREADINGTYPE
 #define THREADINGTYPE 10
#endif

#if RUNNINGTEST
 #define NUM_THREADS 1
 #define FRAGMENT_SIZE 100
 #define FRAGMENT_NUM 10
 #define FILE_NUM 5
#else
 #if THREADINGTYPE == 10
  #define NUM_THREADS 10
  #define FRAGMENT_SIZE 100
  #define FRAGMENT_NUM 10000
  #define FILE_NUM 100
 #elif THREADINGTYPE == 20
  #define NUM_THREADS 20
  #define FRAGMENT_SIZE 100
  #define FRAGMENT_NUM 10000
  #define FILE_NUM 50
 #else
  #error "THREADINGTYPE not supported"
 #endif
#endif
