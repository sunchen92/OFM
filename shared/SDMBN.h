#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SDMBN_H_
#define _SDMBN_H_

#include <stdint.h>
#include <sys/time.h>
#include <pcap/pcap.h>

typedef struct {
	uint32_t nw_src;
	uint32_t nw_src_mask;
	uint32_t nw_dst;
	uint32_t nw_dst_mask;
	uint16_t tp_src;
	uint16_t tp_dst;
    uint16_t dl_type;
	uint8_t nw_proto;
    uint8_t wildcards;
	uint8_t tp_flip;
} PerflowKey;

#define WILDCARD_DL_TYPE    0x01
#define WILDCARD_NW_SRC     0x02
#define WILDCARD_NW_DST     0x04
#define WILDCARD_NW_PROTO   0x08
#define WILDCARD_TP_SRC     0x10
#define WILDCARD_TP_DST     0x20
#define WILDCARD_NW_SRC_MASK    0x40
#define WILDCARD_NW_DST_MASK    0x80
#define WILDCARD_NONE       0x00
#define WILDCARD_ALL        0xFF

typedef struct SDMBNExt {
    char id;
    void *data;
    struct SDMBNExt *next;
} SDMBNExt;

#define SDMBN_EXT_MAX_CHUNKS    0x01

typedef struct {
    int (*init)();
    int (*cleanup)();
    int (*get_perflow)(PerflowKey *, int, int, SDMBNExt *);
    int (*put_perflow)(int, PerflowKey *, char *); 
    int (*get_allflows)(int);
    int (*put_allflows)(int, char *);
    int (*delete_all_flows)(PerflowKey *, int, int);
    int (*delete_select_flow)(PerflowKey *, int);
    int (*get_multiflow)(PerflowKey *, int); 
    int (*put_multiflow)(int, PerflowKey *, char *); 
    int (*delete_multiflow)(PerflowKey *, int, int);
    int (*delete_perflow)(PerflowKey *, int);
    int (*get_config)(int);
    int (*put_config)(int, char *); 
    int (*process_packet)(const struct pcap_pkthdr *, const unsigned char *);
    char *device;
    
} SDMBNLocals;

typedef struct {
    char injected;
    char stop;
    char event;
} ProcessContext;

///// FUNCTION PROTOTYPES ////////////////////////////////////////////////////
int sdmbn_init(SDMBNLocals *locals);
int sdmbn_cleanup();
int sdmbn_raise_reprocess(int id, int hashkey, 
        const struct pcap_pkthdr *header, const unsigned char *packet);
int sdmbn_send_perflow(int id, PerflowKey *key, char *state, int hashkey, 
        int seq);
int sdmbn_send_allflows(int id, char *state, int seq);
int sdmbn_send_multiflow(int id, PerflowKey *key, char *state, int hashkey, 
        int seq);
int sdmbn_send_config(int id, char *state, int seq);

void sdmbn_notify_packet_received(char *type, struct timeval *recv_time);
void sdmbn_notify_flow_created();
void sdmbn_notify_flow_destroyed();

void sdmbn_preprocess_packet(const struct pcap_pkthdr *hdr, 
        const unsigned char *pkt, ProcessContext *context);

void sdmbn_postprocess_packet(const struct pcap_pkthdr *hdr, 
        const unsigned char *pkt, ProcessContext *context);

char *sdmbn_base64_encode(void *blob, int size);
void *sdmbn_base64_decode(char *blob);
#endif

#ifdef __cplusplus
}
#endif
