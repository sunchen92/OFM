#include "SDMBN.h"
#include "event.h"
#include "state.h"
#include "SDMBNCore.h"
#include "SDMBNJson.h"
#include "SDMBNDebug.h"
#include "SDMBNConn.h"

#include <stdlib.h>
#include <string.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <pcap/pcap.h>
#include <errno.h>

// Thread for handling event messages
static pthread_t event_thread;

// Connection for event messages
int sdmbn_conn_event = -1;

// List of event filters
static EventFilter *sdmbn_event_filters = NULL;

static pthread_mutex_t lock_processing_pkt;
//static pthread_mutex_t lock_event_filters;

static void send_error(int id, int hashkey, char *cause)
{
    json_object *error = json_compose_error(id, hashkey, cause);
    json_send(error, sdmbn_conn_event);
    json_object_put(error);
}

static void send_events_ack(int id)
{
    json_object *ack = json_compose_events_ack(id);
    json_send(ack, sdmbn_conn_state);
    json_object_put(ack);
}

static int events_add_filter(PerflowKey *key, char *action, int id)
{
    EventFilter* filter = (EventFilter*)malloc(sizeof(EventFilter));
    memcpy(&(filter->key), key, sizeof(PerflowKey));
    if (0 == strcmp(action, CONSTANT_ACTION_DROP))
    { filter->action = EVENT_ACTION_DROP; }
    else if (0 == strcmp(action, CONSTANT_ACTION_BUFFER))
    { filter->action = EVENT_ACTION_BUFFER; }
    else if (0 == strcmp(action, CONSTANT_ACTION_PROCESS))
    { filter->action = EVENT_ACTION_PROCESS; }
    filter->id = id;

    filter->bufferHead = NULL;
    filter->bufferTail = NULL;
    if (pthread_mutex_init(&(filter->bufferLock), NULL) < 0)
    { 
        ERROR_PRINT("Failed to initialize buffer lock for filter"); 
        return -1;
    }

//    pthread_mutex_lock(&lock_event_filters);
    filter->next = sdmbn_event_filters;
    sdmbn_event_filters = filter;
//    pthread_mutex_unlock(&lock_event_filters);
    return 1;
}

static void events_remove_filter(PerflowKey *key)
{
    double receiveTime, releaseTime;
    int write_len = -1;

    // Find filter
//    pthread_mutex_lock(&lock_event_filters);
    EventFilter *filter = sdmbn_event_filters;
    while (filter != NULL)
    {
        if (0 == memcmp(&(filter->key), key, sizeof(PerflowKey)))
        { break; }
        filter = filter->next;
    }
//    pthread_mutex_unlock(&lock_event_filters);

    if (filter != NULL)
    {
        // Release bufferred events
        pthread_mutex_lock(&(filter->bufferLock));
        INFO_PRINT("Release buffered events (head=%p)", filter->bufferHead);
        while (filter->bufferHead != NULL)
        {
            BufferedPkt *entry = filter->bufferHead;
            pthread_mutex_unlock(&(filter->bufferLock));

            if (sdmbn_locals->process_packet != NULL)
            {
                write_len = sdmbn_locals->process_packet(&(entry->hdr), 
                        entry->pkt);
            }

            if (write_len < 0)
            {
                ERROR_PRINT("Failed to release buffered packet of len %d",
                        entry->hdr.caplen)
            }
            else
            {
                DEBUG_PRINT("Released buffered packet of len %d", 
                        entry->hdr.caplen);
            
                receiveTime = entry->hdr.ts.tv_sec +
                        0.000001 * entry->hdr.ts.tv_usec;
                struct timeval ts;
                gettimeofday(&ts, NULL);
                releaseTime = ts.tv_sec + 0.000001 * ts.tv_usec;
                INFO_PRINT("WAIT TIME = %f", releaseTime - receiveTime);
                sdmbn_stats.pkts_release_count++;
            }

			free((void*)entry->pkt);
            pthread_mutex_lock(&(filter->bufferLock));
            filter->bufferHead  = entry->next;
            if (NULL == filter->bufferHead)
            { filter->bufferTail = NULL; }
            free(entry);
        }
        pthread_mutex_unlock(&(filter->bufferLock));

        // Remove filter
        EventFilter *prev = NULL;
//        pthread_mutex_lock(&lock_event_filters);
        EventFilter *curr = sdmbn_event_filters;
        while (curr != NULL)
        {
            if (curr == filter)
            {
                if (NULL == prev)
                { sdmbn_event_filters = curr->next; }
                else
                { prev->next = curr->next; }
                break;
            }
            prev = curr;
            curr = curr->next;
        }
//        pthread_mutex_unlock(&lock_event_filters);

        // Free filter
//        pthread_mutex_lock(&lock_processing_pkt);
        pthread_mutex_destroy(&(filter->bufferLock));
        free(filter);
//        pthread_mutex_unlock(&lock_processing_pkt);
    }

    return;
}

static void build_packet_key(const unsigned char* pkt, PerflowKey* key)
{
    key->wildcards = WILDCARD_ALL;
    unsigned char *nxthdr = (unsigned char*)pkt;

    // Ethernet
    struct ether_header* eth = (struct ether_header*)nxthdr;
    //key->dl_dst = eth->ether_dhost;
    //key->dl_src = eth->ether_shost;
    key->dl_type = ntohs(eth->ether_type);
    key->wildcards &= ~WILDCARD_DL_TYPE;
    nxthdr += ETHER_HDR_LEN;

    // Network layer
    switch (ntohs(eth->ether_type))
    {
    case ETHERTYPE_IP:
        {
        struct iphdr* ip = (struct iphdr*)nxthdr;
        key->nw_src = ntohl(ip->saddr);
        key->nw_dst = ntohl(ip->daddr);
        key->nw_proto = ip->protocol;
        key->wildcards &= ~(WILDCARD_NW_SRC | WILDCARD_NW_DST 
                | WILDCARD_NW_PROTO);
        nxthdr += ip->ihl * 4;
        break;
        }
    default:
        break;
    }

    // Transport layer
    switch (key->nw_proto)
    {
    case IPPROTO_ICMP:
        break; 
    case IPPROTO_TCP:
        {
        struct tcphdr* tcp = (struct tcphdr*)nxthdr;
        key->tp_src = ntohs(tcp->source);
        key->tp_dst = ntohs(tcp->dest);
        key->wildcards &= ~(WILDCARD_TP_SRC | WILDCARD_TP_DST);
        nxthdr += tcp->doff * 4;
        break; 
        }
    case IPPROTO_UDP:
        {
        struct udphdr* udp = (struct udphdr*)nxthdr;
        key->tp_src = ntohs(udp->source);
        key->tp_dst = ntohs(udp->dest);
        key->wildcards &= ~(WILDCARD_TP_SRC | WILDCARD_TP_DST);
        nxthdr += sizeof(struct udphdr);
        break; 
        }
    default:
        break;
    }
}

static EventFilter* matches_filter(const struct pcap_pkthdr* hdr, 
        const unsigned char* pkt)
{
    if (NULL == sdmbn_event_filters)
    { return NULL; }

    PerflowKey pKey;
    build_packet_key(pkt, &pKey);

//    pthread_mutex_lock(&lock_event_filters);

    EventFilter *filter = sdmbn_event_filters;
    while (filter != NULL)
    {
        PerflowKey* fKey = &(filter->key);

        // Check dl_type
        if (!(fKey->wildcards & WILDCARD_DL_TYPE)
                && (fKey->dl_type != pKey.dl_type))
        { 
            filter = filter->next;
            continue; 
        }

        // Check nw_src
        int nw_src_mask = 0xFFFFFFFF;
        if (!(fKey->wildcards & WILDCARD_NW_SRC_MASK))
        { nw_src_mask = nw_src_mask << (32 - fKey->nw_src_mask); }
        if (!(fKey->wildcards & WILDCARD_NW_SRC)
                && ((fKey->nw_src & nw_src_mask) 
                    != (pKey.nw_src & nw_src_mask)))
        { 
            filter = filter->next;
            continue; 
        }

        // Check nw_dst
        int nw_dst_mask = 0xFFFFFFFF;
        if (!(fKey->wildcards & WILDCARD_NW_DST_MASK))
        { nw_dst_mask = nw_dst_mask << (32 - fKey->nw_dst_mask); }
        if (!(fKey->wildcards & WILDCARD_NW_DST)
                && ((fKey->nw_dst & nw_dst_mask) 
                    != (pKey.nw_dst & nw_dst_mask)))
        { 
            filter = filter->next;
            continue; 
        }

        // Check nw_proto
        if (!(fKey->wildcards & WILDCARD_NW_PROTO)
                && (fKey->nw_proto != pKey.nw_proto))
        { 
            filter = filter->next;
            continue; 
        }

        // Check tp_src
        if (!(fKey->wildcards & WILDCARD_TP_SRC) &&
                !(fKey->tp_src == pKey.tp_src
                    || (pKey.tp_flip && fKey->tp_dst == pKey.tp_src)))
        { 
            filter = filter->next;
            continue; 
        }

        // Check tp_dst
        if (!(fKey->wildcards & WILDCARD_TP_DST) &&
                !(fKey->tp_dst == pKey.tp_dst
                    || (pKey.tp_flip && fKey->tp_src == pKey.tp_dst)))
        { 
            filter = filter->next;
            continue; 
        }

//        pthread_mutex_unlock(&lock_event_filters);
        return filter;
    }

//    pthread_mutex_unlock(&lock_event_filters);
    return NULL;
}

static int is_packet_flag_set(const unsigned char* pkt, int flag)
{
    unsigned char *nxthdr = (unsigned char*)pkt;

    // Ethernet
    struct ether_header* eth = (struct ether_header*)nxthdr;
    nxthdr += ETHER_HDR_LEN;

    // Network layer
    switch (ntohs(eth->ether_type))
    {
    case ETHERTYPE_IP:
        {
        struct iphdr* ip = (struct iphdr*)nxthdr;
        if (ip->tos & flag)
        { return 1; }
        break;
        }
    default:
        break;
    }
    return 0;
}

void sdmbn_preprocess_packet(const struct pcap_pkthdr *hdr, 
        const unsigned char *pkt, ProcessContext *context)
{
    pthread_mutex_lock(&lock_processing_pkt);

    EventFilter* filter = matches_filter(hdr, pkt);
    if (NULL == filter)
    { 
        DEBUG_PRINT("Packet does not match filter"); 
        context->stop = 0;
        context->event = 0;
        sdmbn_stats.pkts_nofilter_count++;
    }
    else
    {
        DEBUG_PRINT("Packet matches filter"); 
        struct timeval ts;
        gettimeofday(&ts, NULL);
        switch (filter->action)
        {
        case EVENT_ACTION_BUFFER:
            {
            // Check if packet should not be buffered
            if (is_packet_flag_set(pkt, PACKET_FLAG_DO_NOT_BUFFER))
            { 
                context->stop = 0;
                context->event = filter->id; 
                DEBUG_PRINT("Packet should not be buffered");
                sdmbn_stats.pkts_nobuffer_count++;
                break;
            }

            // Check if we just dequeued this packet from the buffer
            if (context->injected)
            { 
                context->stop = 0;
                context->event = 0; 
                DEBUG_PRINT("Packet was just dequeued; do not buffer");
                break; 
            }

            // Create buffer entry
            BufferedPkt *entry = (BufferedPkt *)malloc(sizeof(BufferedPkt));
            memcpy((void *)&(entry->hdr), hdr, sizeof(struct pcap_pkthdr));
            entry->pkt = (unsigned char *)malloc(hdr->caplen);
            memcpy((void *)entry->pkt, pkt, hdr->caplen);
            entry->next = NULL;

            // Add to buffer
            pthread_mutex_lock(&(filter->bufferLock));
            if (NULL == filter->bufferTail)
            {
                filter->bufferHead = entry;
                filter->bufferTail = entry;
            }
            else
            {
                filter->bufferTail->next = entry;
                filter->bufferTail = entry;
            }
            pthread_mutex_unlock(&(filter->bufferLock));
            STATS_PRINT("BUF,%ld.%ld", ts.tv_sec,ts.tv_usec);
            sdmbn_stats.pkts_buffer_count++;

//            sdmbn_raise_reprocess(filter->id, -1, hdr, pkt);
            context->stop = 1; 
            context->event = -1;
            break;
            }
        case EVENT_ACTION_DROP:
			// Check if packet should not be dropped
            if (is_packet_flag_set(pkt, PACKET_FLAG_DO_NOT_DROP))
            { 
                context->stop = 0;
                context->event = filter->id; 
                DEBUG_PRINT("Packet should not be dropped");
                break;
            }
            sdmbn_raise_reprocess(filter->id, -1, hdr, pkt);
            context->stop = 1; 
            context->event = -1;
            STATS_PRINT("DROP,%ld.%ld", ts.tv_sec,ts.tv_usec);
            sdmbn_stats.pkts_drop_count++;
            break;
        case EVENT_ACTION_PROCESS:
            context->stop = 0;
            context->event = filter->id; 
            sdmbn_stats.pkts_process_count++;
            break;
        }
    }

    if (context->stop)
    { pthread_mutex_unlock(&lock_processing_pkt); }
}

void sdmbn_postprocess_packet(const struct pcap_pkthdr *hdr, 
        const unsigned char *pkt, ProcessContext *context)
{
    if (context->event > 0)
    { sdmbn_raise_reprocess(context->event, -1, hdr, pkt); }

    pthread_mutex_unlock(&lock_processing_pkt);
}

int sdmbn_raise_reprocess(int id, int hashkey, 
        const struct pcap_pkthdr *header, const unsigned char *packet)
{
    if (NULL == header || NULL == packet)
    { return -1; }

    //TMPHACK
    //hashkey = sdmbn_stats.reprocess_raised;
    //TMPHACK

    // Send reprocess event
    json_object *event = json_compose_reprocess_event(id, hashkey, header, 
            packet);
    int result = json_send(event, sdmbn_conn_event);

    // Free JSON object
    json_object_put(event);

    // Update statistics 
    sdmbn_stats.reprocess_raised++;
    struct timeval evt_time;
    gettimeofday(&evt_time, NULL);
    STATS_PRINT("EVTPRO,%ld.%ld", evt_time.tv_sec, evt_time.tv_usec);

    return result;
}

int handle_enable_events(json_object *msg, int id)
{
    DEBUG_PRINT("Got enable-events");

    // Parse key
    json_object *key_field = json_object_object_get(msg, FIELD_KEY);
    if (NULL == key_field)
    {
        ERROR_PRINT("Enable events has no key");
        send_error(id, -1, ERROR_MALFORMED);
        return -1;
    }
    PerflowKey key;
    json_parse_perflow_key(key_field, &key);

    // Parse action
    json_object *action_field = json_object_object_get(msg, FIELD_ACTION);
    if (NULL == action_field)
    {
        ERROR_PRINT("Enable events has no action");
        send_error(id, -1, ERROR_MALFORMED);
        return -1;
    }
    char *action = (char *)json_object_get_string(action_field);

    // Add event filter
    if (events_add_filter(&key, action, id) < 0)
    {
        ERROR_PRINT("Failed to add event filter");
        send_error(id, -1, ERROR_INTERNAL);
        return -1;
    }

    // Send ACK
    send_events_ack(id);

    return 1;
}

int handle_disable_events(json_object *msg, int id)
{
    DEBUG_PRINT("Got disable-events");

    // Parse key
    json_object *key_field = json_object_object_get(msg, FIELD_KEY);
    if (NULL == key_field)
    {
        ERROR_PRINT("Disable events has no key");
        send_error(id, -1, ERROR_MALFORMED);
        return -1;
    }
    PerflowKey key;
    json_parse_perflow_key(key_field, &key);

    // Add event filter
    events_remove_filter(&key);

    // Send ACK
    send_events_ack(id);

    return 1;
}

static int handle_reprocess_event(json_object *msg, int id)
{
    // Parse hashkey
    json_object *hashkey_field = json_object_object_get(msg, FIELD_HASHKEY);
    if (NULL == hashkey_field)
    {
        ERROR_PRINT("Reprocess event has no hashkey");
        send_error(id, -1, ERROR_MALFORMED);
        return -1;
    }
    int hashkey = json_object_get_int(hashkey_field);

    // Parse header
    json_object *header_field = json_object_object_get(msg, FIELD_HEADER);
    if (NULL == header_field)
    {
        ERROR_PRINT("Reprocess event has no header");
        send_error(id, hashkey, ERROR_MALFORMED);
        return -1;
    }
    char *encoded_header = (char *)json_object_get_string(header_field);
    struct pcap_pkthdr *header = (struct pcap_pkthdr *)sdmbn_base64_decode(
            encoded_header);

    // Parse packet
    json_object *packet_field = json_object_object_get(msg, FIELD_PACKET);
    if (NULL == packet_field)
    {
        ERROR_PRINT("Reprocess event has no packet");
        send_error(id, hashkey, ERROR_MALFORMED);
        return -1;
    }
    char *encoded_packet = (char *)json_object_get_string(packet_field);
    unsigned char *packet = (unsigned char *)sdmbn_base64_decode(
            encoded_packet);

    sdmbn_stats.events_count++;

    int write_len = -1;
    if (sdmbn_locals->process_packet != NULL)
    { write_len = sdmbn_locals->process_packet(header, packet); }

    if (write_len < 0)
    {
        ERROR_PRINT("Failed to inject packet");
        return -1;
    }

    INFO_PRINT("Injected packet id=%d hashkey=0x%08X len=%d", 
            id, hashkey, write_len); 
    return write_len;
}

static void *events_handler(void *arg)
{
    INFO_PRINT("Event handling thread started");

    while (1)
    {
        // Attempt to read a JSON string
        char* buffer =  conn_read(sdmbn_conn_event);
        if (NULL == buffer)
        {
            ERROR_PRINT("Failed to read from event socket");
            break;
        }

        // Attempt to parse the JSON string
        struct json_object *msg;
        msg = json_tokener_parse(buffer);
        free(buffer);
        if (NULL == msg)
        {
            ERROR_PRINT("Failed to parse JSON from controller: %s", buffer);
            continue;
        }
        DEBUG_PRINT("Parsed msg");

        // Get message id
        json_object *id_field = json_object_object_get(msg, FIELD_ID);
        if (NULL == id_field)
        {
            ERROR_PRINT("JSON from controller has no id: %s", buffer);
            send_error(-1, -1, ERROR_MALFORMED);
            // Free JSON object
            json_object_put(msg);
            continue;
        }
        int id = json_object_get_int(id_field);

        // Get message type
        json_object *type_field = json_object_object_get(msg, FIELD_TYPE);
        if (NULL == type_field)
        {
            ERROR_PRINT("JSON from controller has no type: %s", buffer);
            send_error(id, -1, ERROR_MALFORMED);
            // Free JSON object
            json_object_put(msg);
            continue;
        }
        char *type = (char *)json_object_get_string(type_field);
        DEBUG_PRINT("Message: id=%d, type=%s", id, type);

        // Take appropriate action
        if (0 == strcmp(type, EVENT_REPROCESS))
        { handle_reprocess_event(msg, id); }
        else
        {
            ERROR_PRINT("Unknown type: %s", type);
            send_error(id, -1, ERROR_MALFORMED);
        }

        // Free JSON object
        json_object_put(msg);
    }

    INFO_PRINT("Event handling thread finished");
    pthread_exit(NULL);
}

int events_init()
{
    // Open events communication channel
    sdmbn_conn_event = conn_active_open(sdmbn_config.ctrl_ip, 
            sdmbn_config.ctrl_port_event);
    if (sdmbn_conn_event < 0)
    { 
        ERROR_PRINT("Failed to open event communication channel");
        return sdmbn_conn_event; 
    }

    // Create SYN
    json_object *syn = json_compose_syn();

    // Send SYN
    json_send(syn, sdmbn_conn_event);

    // Free JSON object
    json_object_put(syn);

    int result = pthread_mutex_init(&lock_processing_pkt, NULL);
    if (result != 0)
    { 
        ERROR_PRINT("Failed to initialize processing packet lock");
        return result;
    }

    // Create SDMBN event handling thread
    result = pthread_create(&event_thread, NULL, events_handler, NULL);
    if (result != 0)
    { 
        ERROR_PRINT("Failed to initialize event handling thread");
        return result;
    }

    if (sdmbn_locals->init != NULL)
    { sdmbn_locals->init(); }

    return 1;
}

int events_cleanup()
{
    // Close event communication channel
    if (conn_close(sdmbn_conn_event) >= 0)
    { sdmbn_conn_event = -1; }
    else
    { ERROR_PRINT("Failed to close event communication channel"); }

    STATS_PRINT("PKTS_NOFILTER=%d", sdmbn_stats.pkts_nofilter_count);
    STATS_PRINT("PKTS_DROP=%d", sdmbn_stats.pkts_drop_count);
    STATS_PRINT("PKTS_BUFFER=%d", sdmbn_stats.pkts_buffer_count);
    STATS_PRINT("PKTS_NOBUFFER=%d", sdmbn_stats.pkts_nobuffer_count);
    STATS_PRINT("PKTS_RELEASE=%d", sdmbn_stats.pkts_release_count);
    STATS_PRINT("PKTS_PROCESS=%d", sdmbn_stats.pkts_process_count);
    STATS_PRINT("EVENTS=%d", sdmbn_stats.events_count);

    // Destroy events thread
    //pthread_kill(events_thread, SIGKILL);

    return 1;
}
