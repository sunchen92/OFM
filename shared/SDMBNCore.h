#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SDMBNCore_H_
#define _SDMBNCore_H_

#include "SDMBN.h"
#include "SDMBNConfig.h"

///// STRUCTS ////////////////////////////////////////////////////////////////
typedef struct {
    int gets_per_count;
    long gets_per_time;
    int puts_per_count;
    long puts_per_time;

    int gets_mul_count;
    long gets_mul_time;
    int puts_mul_count;
    long puts_mul_time;

    int pkts_received;
    int flows_active;
    int reprocess_raised;

    int pkts_nofilter_count;
    int pkts_drop_count;
    int pkts_buffer_count;
    int pkts_nobuffer_count;
    int pkts_release_count;
    int pkts_process_count;
    int events_count;
} SDMBNStatistics; 

///// GLOBALS ////////////////////////////////////////////////////////////////
extern SDMBNLocals *sdmbn_locals;
extern SDMBNStatistics sdmbn_stats;
extern SDMBNConfig sdmbn_config;
extern pthread_mutex_t sdmbn_lock_get;

#endif

#ifdef __cplusplus
}
#endif
