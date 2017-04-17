#include "SDMBNConn.h"
#include "SDMBNDebug.h"
#include "SDMBNJson.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <json/json.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pcap.h>

json_object *json_compose_perflow_key(PerflowKey *key)
{
    json_object *field = json_object_new_object();

    if (!(key->wildcards & WILDCARD_DL_TYPE))
    { 
        json_object_object_add(field, "dl_type", 
                json_object_new_int(key->dl_type)); 
    }

    if (!(key->wildcards & WILDCARD_NW_SRC))
    {

        struct in_addr addr;
        bzero(&addr, sizeof(addr));
        addr.s_addr = key->nw_src;
        char *nw_src = inet_ntoa(addr);
        json_object_object_add(field, "nw_src", 
                json_object_new_string(nw_src));
    }
    
    if (!(key->wildcards & WILDCARD_NW_SRC_MASK))
    {
        json_object_object_add(field, "nw_src_mask", 
                json_object_new_int(key->nw_src_mask));
    }

    if (!(key->wildcards & WILDCARD_NW_DST))
    {

        struct in_addr addr;
        bzero(&addr, sizeof(addr));
        addr.s_addr = key->nw_dst;
        char *nw_dst = inet_ntoa(addr);
        json_object_object_add(field, "nw_dst", 
                json_object_new_string(nw_dst));
    }

    if (!(key->wildcards & WILDCARD_NW_DST_MASK))
    {
        json_object_object_add(field, "nw_dst_mask", 
                json_object_new_int(key->nw_dst_mask));
    }


    if (!(key->wildcards & WILDCARD_NW_PROTO))
    { 
        json_object_object_add(field, "nw_proto", 
                json_object_new_int(key->nw_proto)); 
    }

    if (!(key->wildcards & WILDCARD_TP_SRC))
    { 
        json_object_object_add(field, "tp_src", 
                json_object_new_int(ntohs(key->tp_src))); 
    }

    if (!(key->wildcards & WILDCARD_TP_DST))
    { 
        json_object_object_add(field, "tp_dst", 
                json_object_new_int(ntohs(key->tp_dst))); 
    }

    if (!(key->wildcards & WILDCARD_TP_SRC) 
            || !(key->wildcards & WILDCARD_TP_DST))
    { 
        json_object_object_add(field, "tp_flip", 
                json_object_new_int(key->tp_flip)); 
    }


    return field;
}

void json_parse_perflow_key(json_object *field, PerflowKey *key)
{
    bzero(key, sizeof(*key));

	json_object *dl_type_obj = json_object_object_get(field, "dl_type");
    if (dl_type_obj != NULL)
    { key->dl_type = json_object_get_int(dl_type_obj); }
    else
    { key->wildcards |= WILDCARD_DL_TYPE; }

	json_object *nw_src_obj = json_object_object_get(field, "nw_src");
    if (nw_src_obj != NULL)
    {
        const char *nw_src = json_object_get_string(nw_src_obj);
        struct in_addr addr;
        bzero(&addr, sizeof(addr));
        inet_aton(nw_src, &addr);
        key->nw_src = (addr.s_addr);
    }
    else
    { key->wildcards |= WILDCARD_NW_SRC; }

    json_object *nw_src_mask_obj = json_object_object_get(field,
            "nw_src_mask");
    if (nw_src_mask_obj != NULL)
    { key->nw_src_mask = json_object_get_int(nw_src_mask_obj); }
    else
    { key->wildcards |= WILDCARD_NW_SRC_MASK; }

    json_object *nw_dst_obj = json_object_object_get(field, "nw_dst");
    if (nw_dst_obj != NULL)
    {
        const char *nw_dst = json_object_get_string(nw_dst_obj);
        struct in_addr addr;
        bzero(&addr, sizeof(addr));
        inet_aton(nw_dst, &addr);
        key->nw_dst = (addr.s_addr);
    }
    else
    { key->wildcards |= WILDCARD_NW_DST; }

    json_object *nw_dst_mask_obj = json_object_object_get(field,
            "nw_dst_mask");
    if (nw_dst_mask_obj != NULL)
    { key->nw_dst_mask = json_object_get_int(nw_dst_mask_obj); }
    else
    { key->wildcards |= WILDCARD_NW_DST_MASK; }

    json_object *nw_proto_obj = json_object_object_get(field, "nw_proto");
    if (nw_proto_obj != NULL)
    { key->nw_proto = json_object_get_int(nw_proto_obj); }
    else
    { key->wildcards |= WILDCARD_NW_PROTO; }

    json_object *tp_src_obj = json_object_object_get(field, "tp_src");
    if (tp_src_obj != NULL)
    { key->tp_src = htons(json_object_get_int(tp_src_obj)); }
    else
    { key->wildcards |= WILDCARD_TP_SRC; }

    json_object *tp_dst_obj = json_object_object_get(field, "tp_dst");
    if (tp_dst_obj != NULL)
    { key->tp_dst = htons(json_object_get_int(tp_dst_obj)); }
    else
    { key->wildcards |= WILDCARD_TP_DST; }

    json_object *tp_flip_obj = json_object_object_get(field, "tp_flip");
    if (tp_flip_obj != NULL)
    { key->tp_flip = json_object_get_int(tp_flip_obj); }
    else
    { key->tp_flip = 0; }

    return;
}

static json_object *json_compose_base64_field(void *blob, int size)
{
    char *encoded = sdmbn_base64_encode(blob, size);
    json_object *field = json_object_new_string(encoded);
    free(encoded);
    return field;
}

//void *json_parse_encoded_blob(json_object *field)
//{
//    char *encoded = (char *)json_object_get_string(field);
//    void *blob = base64_decode_blob(encoded);
//    return blob;
//}

json_object* json_compose_perflow_state(int id, PerflowKey *key, char *state, 
        int hashkey, int seq)
{
    if (NULL == key || NULL == state)
    { return NULL; }

    json_object *msg;
    msg = json_object_new_object();
    json_object_object_add(msg, FIELD_ID, json_object_new_int(id));
    json_object_object_add(msg, FIELD_TYPE, 
            json_object_new_string(RESPONSE_STATE_PERFLOW));
    json_object *key_field = json_compose_perflow_key(key);
    json_object_object_add(msg, FIELD_KEY, key_field);
    json_object_object_add(msg, FIELD_HASHKEY, 
            json_object_new_int(hashkey));
    json_object_object_add(msg, FIELD_STATE,
            json_object_new_string(state));
    json_object_object_add(msg, FIELD_SEQ, json_object_new_int(seq));
    return msg;
}

json_object* json_compose_multiflow_state(int id, PerflowKey *key,
        char *state, int hashkey, int seq)
{
    if (NULL == key || NULL == state)
    { return NULL; }

    json_object *msg;
    msg = json_object_new_object();
    json_object_object_add(msg, FIELD_ID, json_object_new_int(id));
    json_object_object_add(msg, FIELD_TYPE, 
            json_object_new_string(RESPONSE_STATE_MULTIFLOW));
    json_object *key_field = json_compose_perflow_key(key);
    json_object_object_add(msg, FIELD_KEY, key_field);
    json_object_object_add(msg, FIELD_HASHKEY,
            json_object_new_int(hashkey));
    json_object_object_add(msg, FIELD_STATE,
            json_object_new_string(state));
    json_object_object_add(msg, FIELD_SEQ, json_object_new_int(seq));
    return msg;
}

json_object* json_compose_allflows_state(int id, char *state, int seq)
{
    if (NULL == state)
    { return NULL; }

    json_object *msg;
    msg = json_object_new_object();
    json_object_object_add(msg, FIELD_ID, json_object_new_int(id));
    json_object_object_add(msg, FIELD_TYPE, 
            json_object_new_string(RESPONSE_STATE_ALLFLOWS));
    json_object_object_add(msg, FIELD_STATE,
            json_object_new_string(state));
    json_object_object_add(msg, FIELD_SEQ, json_object_new_int(seq));
    return msg;
}

json_object* json_compose_config_state(int id, char *state, int seq)
{
    if (NULL == state)
    { return NULL; }

    json_object *msg;
    msg = json_object_new_object();
    json_object_object_add(msg, FIELD_ID, json_object_new_int(id));
    json_object_object_add(msg, FIELD_TYPE, 
            json_object_new_string(RESPONSE_STATE_CONFIG));
    json_object_object_add(msg, FIELD_STATE,
            json_object_new_string(state));
    json_object_object_add(msg, FIELD_SEQ, json_object_new_int(seq));
    return msg;
}

json_object* json_compose_syn()
{
    json_object *msg;
    msg = json_object_new_object();
    json_object_object_add(msg, FIELD_TYPE, 
            json_object_new_string(RESPONSE_SYN));

    char hostname[1024];
    memset(hostname, 0, 1024);
    gethostname(hostname, 1024);
    json_object_object_add(msg, FIELD_HOST, json_object_new_string(hostname));
    json_object_object_add(msg, FIELD_PID, json_object_new_int(getpid()));

    return msg;
}

json_object* json_compose_delete_perflow_ack(int id, int count)
{
    json_object *msg;
    msg = json_object_new_object();
    json_object_object_add(msg, FIELD_ID,
            json_object_new_int(id));
    json_object_object_add(msg, FIELD_TYPE,
            json_object_new_string(RESPONSE_DELETE_PERFLOW_ACK));
    json_object_object_add(msg, FIELD_COUNT,
            json_object_new_int(count));
    return msg;
}

json_object* json_compose_delete_multiflow_ack(int id, int count)
{
    json_object *msg;
    msg = json_object_new_object();
    json_object_object_add(msg, FIELD_ID,
            json_object_new_int(id));
    json_object_object_add(msg, FIELD_TYPE,
            json_object_new_string(RESPONSE_DELETE_MULTIFLOW_ACK));
    json_object_object_add(msg, FIELD_COUNT,
            json_object_new_int(count));
    return msg;
}

json_object* json_compose_get_perflow_ack(int id, int count)
{
    json_object *msg;
    msg = json_object_new_object();
    json_object_object_add(msg, FIELD_ID, 
            json_object_new_int(id));
    json_object_object_add(msg, FIELD_TYPE, 
            json_object_new_string(RESPONSE_GET_PERFLOW_ACK));
    json_object_object_add(msg, FIELD_COUNT, 
            json_object_new_int(count));
    return msg;
}

json_object* json_compose_put_perflow_ack(int id, int hashkey)
{
    json_object *msg;
    msg = json_object_new_object();
    json_object_object_add(msg, FIELD_ID, 
            json_object_new_int(id));
    json_object_object_add(msg, FIELD_TYPE, 
            json_object_new_string(RESPONSE_PUT_PERFLOW_ACK));
    json_object_object_add(msg, FIELD_HASHKEY, 
            json_object_new_int(hashkey));
    return msg;
}

json_object* json_compose_get_multiflow_ack(int id, int count)
{
    json_object *msg;
    msg = json_object_new_object();
    json_object_object_add(msg, FIELD_ID,
            json_object_new_int(id));
    json_object_object_add(msg, FIELD_TYPE,
            json_object_new_string(RESPONSE_GET_MULTIFLOW_ACK));
    json_object_object_add(msg, FIELD_COUNT,
            json_object_new_int(count));
    return msg;
}

json_object* json_compose_put_multiflow_ack(int id, int hashkey)
{
    json_object *msg;
    msg = json_object_new_object();
    json_object_object_add(msg, FIELD_ID,
            json_object_new_int(id));
    json_object_object_add(msg, FIELD_TYPE,
            json_object_new_string(RESPONSE_PUT_MULTIFLOW_ACK));
    json_object_object_add(msg, FIELD_HASHKEY,
            json_object_new_int(hashkey));
    return msg;
}

json_object* json_compose_get_allflows_ack(int id, int count)
{
    json_object *msg;
    msg = json_object_new_object();
    json_object_object_add(msg, FIELD_ID,
            json_object_new_int(id));
    json_object_object_add(msg, FIELD_TYPE,
            json_object_new_string(RESPONSE_GET_ALLFLOWS_ACK));
    json_object_object_add(msg, FIELD_COUNT,
            json_object_new_int(count));
    return msg;
}

json_object* json_compose_put_allflows_ack(int id, int hashkey)
{
    json_object *msg;
    msg = json_object_new_object();
    json_object_object_add(msg, FIELD_ID,
            json_object_new_int(id));
    json_object_object_add(msg, FIELD_TYPE,
            json_object_new_string(RESPONSE_PUT_ALLFLOWS_ACK));
    json_object_object_add(msg, FIELD_HASHKEY,
            json_object_new_int(hashkey));
    return msg;
}

json_object* json_compose_get_config_ack(int id, int count)
{
    json_object *msg;
    msg = json_object_new_object();
    json_object_object_add(msg, FIELD_ID, 
            json_object_new_int(id));
    json_object_object_add(msg, FIELD_TYPE, 
            json_object_new_string(RESPONSE_GET_CONFIG_ACK));
    json_object_object_add(msg, FIELD_COUNT, 
            json_object_new_int(count));
    return msg;
}

json_object* json_compose_put_config_ack(int id, int hashkey)
{
    json_object *msg;
    msg = json_object_new_object();
    json_object_object_add(msg, FIELD_ID, 
            json_object_new_int(id));
    json_object_object_add(msg, FIELD_TYPE, 
            json_object_new_string(RESPONSE_PUT_CONFIG_ACK));
    json_object_object_add(msg, FIELD_HASHKEY, 
            json_object_new_int(hashkey));
    return msg;
}

json_object* json_compose_events_ack(int id)
{
    json_object *msg;
    msg = json_object_new_object();
    json_object_object_add(msg, FIELD_ID, 
            json_object_new_int(id));
    json_object_object_add(msg, FIELD_TYPE, 
            json_object_new_string(RESPONSE_EVENTS_ACK));
    return msg;
}

json_object* json_compose_error(int id, int hashkey, char *reason)
{
    json_object *msg;
    msg = json_object_new_object();
    json_object_object_add(msg, FIELD_ID, 
            json_object_new_int(id));
    json_object_object_add(msg, FIELD_TYPE, 
            json_object_new_string(RESPONSE_ERROR));
    json_object_object_add(msg, FIELD_HASHKEY, 
            json_object_new_int(hashkey));
    json_object_object_add(msg, FIELD_CAUSE, 
            json_object_new_string(reason));
    return msg;
}

json_object* json_compose_reprocess_event(int id, int hashkey, 
        const struct pcap_pkthdr *hdr, const unsigned char *pkt)
{
    json_object *msg;
    msg = json_object_new_object();
    json_object_object_add(msg, FIELD_TYPE, 
            json_object_new_string(EVENT_REPROCESS));
    json_object_object_add(msg, FIELD_ID, 
            json_object_new_int(id));
    json_object_object_add(msg, FIELD_HASHKEY, json_object_new_int(hashkey));
    json_object_object_add(msg, FIELD_HEADER, 
            json_compose_base64_field((void *)hdr, sizeof(*hdr)));
    json_object_object_add(msg, FIELD_PACKET, 
            json_compose_base64_field((void *)pkt, hdr->caplen));
    return msg;
}

json_object* json_compose_discover()
{
    json_object *msg;
    msg = json_object_new_object();
    json_object_object_add(msg, FIELD_TYPE, 
            json_object_new_string(RESPONSE_DISCOVER));

    char hostname[1024];
    memset(hostname, 0, 1024);
    gethostname(hostname, 1024);
    json_object_object_add(msg, FIELD_HOST, json_object_new_string(hostname));
    json_object_object_add(msg, FIELD_PID, json_object_new_int(getpid()));

    return msg;
}

int json_send(json_object *msg, int conn)
{
    if (NULL == msg)
    { return -1; }

    char *jsonstr = (char*)json_object_to_json_string(msg);
    conn_write_append_newline(conn, jsonstr, strlen(jsonstr));

    DEBUG_PRINT("Sent: %s", jsonstr);

    return 1;
}
