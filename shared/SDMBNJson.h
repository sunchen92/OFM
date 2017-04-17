#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SDMBNJson_H_
#define _SDMBNJson_H_

#include "SDMBN.h"
#include <json/json.h>
#include <pcap.h>

///// DEFINES ////////////////////////////////////////////////////////////////
#define FIELD_ID                "id"
#define FIELD_TYPE              "type"
#define FIELD_KEY               "key"
#define FIELD_RAISEEVENTS		"raiseEvents"
#define FIELD_HASHKEY           "hashkey"
#define FIELD_STATE             "state"
#define FIELD_SEQ               "seq"
#define FIELD_COUNT             "count"
#define FIELD_CAUSE             "cause"
#define FIELD_HOST              "host"
#define FIELD_HEADER            "header"
#define FIELD_PACKET            "packet"
#define FIELD_PID               "pid"
#define FIELD_DELTYPE           "deltype"
#define FIELD_ACTION            "action"

#define COMMAND_GET_PERFLOW     	"get-perflow"
#define COMMAND_PUT_PERFLOW     	"put-perflow"
#define COMMAND_GET_MULTIFLOW     	"get-multiflow"
#define COMMAND_PUT_MULTIFLOW     	"put-multiflow"
#define COMMAND_GET_ALLFLOWS      	"get-allflows"
#define COMMAND_PUT_ALLFLOWS      	"put-allflows"
#define COMMAND_GET_CONFIG      	"get-config"
#define COMMAND_PUT_CONFIG      	"put-config"
#define COMMAND_DELETE_PERFLOW    	"delete-perflow"
#define COMMAND_DELETE_MULTIFLOW  	"delete-multiflow"
#define COMMAND_ENABLE_EVENTS       "enable-events"
#define COMMAND_DISABLE_EVENTS      "disable-events"


#define RESPONSE_DELETE_PERFLOW_ACK     "delete-perflow-ack"
#define RESPONSE_DELETE_MULTIFLOW_ACK   "delete-multiflow-ack"
#define RESPONSE_GET_PERFLOW_ACK   		"get-perflow-ack"
#define RESPONSE_PUT_PERFLOW_ACK        "put-perflow-ack"
#define RESPONSE_GET_MULTIFLOW_ACK   	"get-multiflow-ack"
#define RESPONSE_PUT_MULTIFLOW_ACK     "put-multiflow-ack"
#define RESPONSE_GET_ALLFLOWS_ACK   	"get-allflows-ack"
#define RESPONSE_PUT_ALLFLOWS_ACK       "put-allflows-ack"
#define RESPONSE_GET_CONFIG_ACK        	"get-config-ack"
#define RESPONSE_PUT_CONFIG_ACK        	"put-config-ack"
#define RESPONSE_EVENTS_ACK        	    "events-ack"
#define RESPONSE_SYN            		"syn"
#define RESPONSE_ERROR           		"error"
#define RESPONSE_STATE_PERFLOW  		"state-perflow"
#define RESPONSE_STATE_MULTIFLOW  		"state-multiflow"
#define RESPONSE_STATE_ALLFLOWS   		"state-allflows"
#define RESPONSE_STATE_CONFIG   		"state-config"
#define RESPONSE_DISCOVER       		"discover"

#define EVENT_REPROCESS         "reprocess"

#define CONSTANT_ACTION_DROP            "drop"
#define CONSTANT_ACTION_BUFFER          "buffer"
#define CONSTANT_ACTION_PROCESS         "process"

#define ERROR_MALFORMED          "malformed"
#define ERROR_UNSUPPORTED        "unsupported"
#define ERROR_INTERNAL           "internal-failure"

///// TYPEDEFS ///////////////////////////////////////////////////////////////
typedef unsigned char byte;

///// FUNCTION PROTOTYPES ////////////////////////////////////////////////////
json_object *json_compose_perflow_key(PerflowKey *key);
void json_parse_perflow_key(json_object *field, PerflowKey *key);
json_object* json_compose_perflow_state(int id, PerflowKey *key, char *state,
        int hashkey, int seq);
json_object* json_compose_allflows_state(int id, char *state, int seq);
json_object* json_compose_multiflow_state(int id, PerflowKey *key, 
        char *state, int hashkey, int seq);
json_object* json_compose_config_state(int id, char *state, int seq);
json_object* json_compose_syn();
json_object* json_compose_delete_perflow_ack(int id, int count);
json_object* json_compose_delete_multiflow_ack(int id, int count);
json_object* json_compose_get_perflow_ack(int id, int count);
json_object* json_compose_get_multiflow_ack(int id, int count);
json_object* json_compose_get_allflows_ack(int id, int count);
json_object* json_compose_get_config_ack(int id, int count);
json_object* json_compose_put_perflow_ack(int id, int hashkey);
json_object* json_compose_put_multiflow_ack(int id, int hashkey);
json_object* json_compose_put_allflows_ack(int id, int hashkey);
json_object* json_compose_put_config_ack(int id, int hashkey);
json_object* json_compose_events_ack(int id);
json_object* json_compose_error(int id, int hashkey, char *reason);
json_object* json_compose_reprocess_event(int id, int hashkey, 
        const struct pcap_pkthdr *hdr, const unsigned char *pkt);
json_object* json_compose_discover();
int json_send(json_object *msg, int conn);

#endif

#ifdef __cplusplus
}
#endif
