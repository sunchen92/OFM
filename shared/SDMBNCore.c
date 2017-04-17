#include "SDMBN.h"
#include "SDMBNCore.h"
#include "SDMBNConn.h"
#include "SDMBNDebug.h"
#include "SDMBNJson.h"
#include "SDMBNConfig.h"
#include "event.h"
#include "state.h"
#include "discovery.h"

#include <json/json.h>
#include <pthread.h>
#include <assert.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>

SDMBNConfig sdmbn_config;

SDMBNLocals *sdmbn_locals = NULL;

// Global statistics variables
SDMBNStatistics sdmbn_stats;

// Hack for benchmarking
pthread_mutex_t sdmbn_lock_get;

// Thread for sending discovery packets
static pthread_t discovery_thread;

int sdmbn_init(SDMBNLocals *locals)
{
    // Check arguments
    if (NULL == locals)
    { return -1; }

    // Initialize statistics
    bzero(&sdmbn_stats, sizeof(sdmbn_stats));

    // Parse configuration
    sdmbn_parse_config();

    // Store list of MB-specific functions
    sdmbn_locals = locals;

    // Initialize connection lock
    assert(0 == pthread_mutex_init(&sdmbn_lock_conn, NULL));

    // Initialize state
    if (state_init() < 0)
    { 
        ERROR_PRINT("Failed to initialize state"); 
        return -1;
    }

    // Initialize events
    if (events_init() < 0)
    { 
        ERROR_PRINT("Failed to initialize events"); 
        return -1;
    }

    // Create get lock
    assert(0 == pthread_mutex_init(&sdmbn_lock_get, NULL));
    if (sdmbn_config.release_get_pkts > 0 
            || sdmbn_config.release_get_flows > 0) 
    { pthread_mutex_lock(&sdmbn_lock_get); }

    // Create discovery packet sending thread
    if (sdmbn_locals->device != NULL)
    { pthread_create(&discovery_thread, NULL, discovery_send, NULL); }
    else
    { ERROR_PRINT("No device provided"); }

    INFO_PRINT("Initialized");

    return 1;
}

int sdmbn_cleanup()
{
    // Destroy get lock
    pthread_mutex_destroy(&sdmbn_lock_get);

    // Clean-up state
    if (state_cleanup() < 0)
    { ERROR_PRINT("Failed to cleanup state"); }

    // Clean-up events
    if (events_cleanup() < 0)
    { ERROR_PRINT("Failed to cleanup events"); }

    // Call local cleanup function
    if (sdmbn_locals->cleanup != NULL)
    { sdmbn_locals->cleanup(); }

    // Remove list of MB-specific functions
    sdmbn_locals = NULL;
    
    INFO_PRINT("Cleaned-up");

    return 1;
}

void sdmbn_notify_packet_received(char *type, struct timeval *recv_time)
{
    sdmbn_stats.pkts_received++;
    
    // Output count statistics 
    if (0 == (sdmbn_stats.pkts_received % 10))
    { INFO_PRINT("pkts_received=%d, flows_active=%d",
            sdmbn_stats.pkts_received, sdmbn_stats.flows_active); }

    // Unlock get lock
    if (sdmbn_config.release_get_pkts > 0 
            || sdmbn_config.release_get_flows > 0) 
    {
        if (sdmbn_config.release_get_pkts == sdmbn_stats.pkts_received)
        {
            INFO_PRINT("Reached packet threshold => Get lock released");
            pthread_mutex_unlock(&sdmbn_lock_get);
        }
        else if (sdmbn_config.release_get_flows == sdmbn_stats.flows_active)
        {
            INFO_PRINT("Reached active flows threshold => Get lock released");
            pthread_mutex_unlock(&sdmbn_lock_get);
        }
    }
}

void sdmbn_notify_flow_created()
{
    sdmbn_stats.flows_active++;
}

void sdmbn_notify_flow_destroyed()
{
    sdmbn_stats.flows_active--;
}

static byte base64_encode_bits(byte data)
{
    if (data < 26)
        return (data + 'A');
    if (data < 52)
        return (data - 26 + 'a');
    if (data < 62)
        return (data - 52 + '0');
    if (data == 62)
        return '+';
    if (data == 63)
        return '/';
    return 0;
}

char *sdmbn_base64_encode(void *blob, int size)
{
    if (NULL == blob || size < 1)
    { return NULL; }
    byte *ptrBlob = (byte *)blob;
    char *result = (char *)malloc(size*2+1);
    if (NULL == result)
    { return NULL; }
    char *ptrResult = result;
    while (size > 0)
    {
        byte lower = *ptrBlob & 0x0F;
        byte upper = (*ptrBlob & 0xF0) >> 4;
        *ptrResult = base64_encode_bits(lower);
        ptrResult++;
        *ptrResult = base64_encode_bits(upper);
        ptrResult++;
        ptrBlob++;
        size += -1;
    }
    *ptrResult = 0;
    return result;
}

static byte base64_decode_bits(byte data)
{
    if (data >= 'A' && data <= 'Z')
        return (data - 'A');
    if (data >= 'a' && data <= 'z')
        return (data + 26 - 'a');
    if (data >= '0' && data <= '9')
        return (data + 52 - '0');
    if (data == '+')
        return 62;
    if (data == '/')
        return 63;
    return 0;
}

void *sdmbn_base64_decode(char *blob)
{
    if (NULL == blob)
    { return NULL; }
    byte *ptrBlob = (byte *)blob;
    int size = strlen(blob);
    void *result = malloc(size/2);
    if (NULL == result)
    { return NULL; }
    char *ptrResult = (char *)result;
    while (size > 0)
    {
        byte lower = base64_decode_bits(*ptrBlob);
        ptrBlob++;
        byte upper = base64_decode_bits(*ptrBlob);
        ptrBlob++;
        *ptrResult = (upper << 4) | lower;
        ptrResult++;
        size += -2;
    }
//    *ptrResult = 0;
    return result;
}

