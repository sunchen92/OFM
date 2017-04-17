#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SDMBNDebug_H_
#define _SDMBNDebug_H_

#include <stdio.h>

//#define SDMBN_SERIALIZE
#define SDMBN_STATS
//#define SDMBN_DEBUG
#define SDMBN_INFO
#define SDMBN_ERROR

#ifdef SDMBN_SERIALIZE
    #define SERIALIZE_PRINT(...) printf(__VA_ARGS__); printf("\n");
#else
    #define SERIALIZE_PRINT(...)
#endif

#ifdef SDMBN_STATS
    #define STATS_PRINT(...) fprintf(stdout,__VA_ARGS__); fprintf(stdout,"\n");
#else
    #define STATS_PRINT(...)
#endif


#ifdef SDMBN_DEBUG
    #define DEBUG_PRINT(...) printf(__VA_ARGS__); printf("\n");
    #ifndef SDMBN_ERROR
        #define SDMBN_ERROR
    #endif
    #ifndef SDMBN_INFO
        #define SDMBN_INFO
    #endif
#else
    #define DEBUG_PRINT(...)
#endif

#ifdef SDMBN_INFO
    #define INFO_PRINT(...) printf(__VA_ARGS__); printf("\n");
    #ifndef SDMBN_ERROR
        #define SDMBN_ERROR
    #endif
#else
    #define INFO_PRINT(...)
#endif

#ifdef SDMBN_ERROR
    #define ERROR_PRINT(...) printf(__VA_ARGS__); printf("\n");
#else
    #define ERROR_PRINT(...)
#endif

#endif

#ifdef __cplusplus
}
#endif
