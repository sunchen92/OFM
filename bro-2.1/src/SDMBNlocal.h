#ifndef _SDMBNLocal_H_
#define _SDMBNLocal_H_

#include <stdio.h>
#include "Conn.h"
#include "AnalyzerTags.h"
#include <SDMBN.h>

// Enable the define below to output to a device when using the -w option and // a device name (instead of a filename)
#define WRITEIFACE

///// DEBUGGING MACROS ///////////////////////////////////////////////////////
//#define SDMBN_SERIALIZE

#define SDMBN_INFO
//#define SDMBN_ERROR

#ifdef SDMBN_SERIALIZE
    #define SERIALIZE_PRINT(...) printf(__VA_ARGS__); printf("\n");
#else
    #define SERIALIZE_PRINT(...)
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

///// TYPEDEFS ///////////////////////////////////////////////////////////////
int sdmbn_can_serialize(AnalyzerTag::Tag check);

///// FUNCTION PROTOTYPES ////////////////////////////////////////////////////
int local_get_perflow(PerflowKey *key, int id, int raiseEvents, SDMBNExt *unused);
int local_put_perflow(int hashkey, PerflowKey *key, char *state);
int local_get_shared(int id);
int local_put_shared(int hashkey, char *state);

#endif
