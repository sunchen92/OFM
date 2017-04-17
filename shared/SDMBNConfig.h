#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SDMBNConfig_H_
#define _SDMBNConfig_H_

#include <stdint.h>
#include <sys/time.h>
#include <pcap.h>

///// DEFINES ////////////////////////////////////////////////////////////////
#define CONFIG_FILE "sdmbn.conf"
#define CONF_KEY_LEN    32
#define CONF_VALUE_LEN  32

#define CONF_CTRL_IP_DEFAULT            "127.0.0.1"
#define CONF_CTRL_PORT_STATE_DEFAULT    7790
#define CONF_CTRL_PORT_EVENT_DEFAULT    7791
#define CONF_RELEASE_GET_PKTS_DEFAULT   -1
#define CONF_RELEASE_GET_FLOWS_DEFAULT  -1
#define CONF_MAX_GET_FLOWS_DEFAULT      -1

#define CONF_CTRL_IP_LEN  16

#define CONF_CTRL_IP            "ctrl_ip"
#define CONF_CTRL_PORT_STATE    "ctrl_port_state"
#define CONF_CTRL_PORT_EVENT    "ctrl_port_event"
#define CONF_RELEASE_GET_PKTS   "release_get_pkts"
#define CONF_RELEASE_GET_FLOWS  "release_get_flows"
#define CONF_MAX_GET_FLOWS      "max_get_flows"

///// STRUCTURES /////////////////////////////////////////////////////////////
typedef struct {
    char ctrl_ip[CONF_CTRL_IP_LEN];
	uint16_t ctrl_port_state;
	uint16_t ctrl_port_event;
    int release_get_pkts;
    int release_get_flows;
    int max_get_flows;
} SDMBNConfig;

///// FUNCTION PROTOTYPES ////////////////////////////////////////////////////
int sdmbn_parse_config();

#endif

#ifdef __cplusplus
}
#endif
