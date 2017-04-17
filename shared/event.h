#ifdef __cplusplus
extern "C" {
#endif

#ifndef _event_H_
#define _event_H_

#include "SDMBN.h"
#include "SDMBNJson.h"
#include <pthread.h>

struct _BufferedPkt {
    const struct pcap_pkthdr hdr;
    const unsigned char *pkt;
    struct _BufferedPkt *next;
};
typedef struct _BufferedPkt BufferedPkt;

typedef struct _EventFilter {
    PerflowKey key;
    enum {EVENT_ACTION_DROP,EVENT_ACTION_BUFFER,EVENT_ACTION_PROCESS} action;
    int id;
    BufferedPkt *bufferHead;
    BufferedPkt *bufferTail;
    pthread_mutex_t bufferLock;
    struct _EventFilter *next;
} EventFilter;

///// DEFINES ///////////////////////////////////////////////////////////////
#define PACKET_FLAG_DO_NOT_BUFFER    (1 << 2)
#define PACKET_FLAG_DO_NOT_DROP    (1 << 3)

///// GLOBALS ///////////////////////////////////////////////////////////////
extern int sdmbn_conn_event;

///// FUNCTION PROTOTYPES ///////////////////////////////////////////////////
int handle_enable_events(json_object *msg, int id);
int handle_disable_events(json_object *msg, int id);
int events_init();
int events_cleanup();

#endif

#ifdef __cplusplus
}
#endif
