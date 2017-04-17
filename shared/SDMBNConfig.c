#include "SDMBNCore.h"
#include "SDMBNConfig.h"
#include "SDMBNDebug.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifndef CONFIG_DIR
#define CONFIG_DIR "/etc/sdmbn/"
#endif

int sdmbn_parse_config()
{
    // Set defaults
    snprintf(sdmbn_config.ctrl_ip, CONF_CTRL_IP_LEN, CONF_CTRL_IP_DEFAULT);
    sdmbn_config.ctrl_port_state = CONF_CTRL_PORT_STATE_DEFAULT;
    sdmbn_config.ctrl_port_event = CONF_CTRL_PORT_EVENT_DEFAULT;
    sdmbn_config.release_get_pkts = CONF_RELEASE_GET_PKTS_DEFAULT;
    sdmbn_config.release_get_flows = CONF_RELEASE_GET_FLOWS_DEFAULT;
    sdmbn_config.max_get_flows = CONF_MAX_GET_FLOWS_DEFAULT;

#define FILEPATH_LEN 256
    char filepath[FILEPATH_LEN];
    snprintf(filepath, FILEPATH_LEN, "%s/%s", CONFIG_DIR, CONFIG_FILE);

    FILE *file = fopen(filepath, "r");
    if (NULL == file)
    { 
        ERROR_PRINT("Failed to open config file '%s': %s", filepath, 
                strerror(errno));
        return -1; 
    }

    char key[CONF_KEY_LEN];
    char value[CONF_VALUE_LEN];
    while (2 == fscanf(file, "%s = %s\n", key, value))
    {
        if (0 == strncmp(key, CONF_CTRL_IP, 7))
        { strncpy(sdmbn_config.ctrl_ip, value, CONF_CTRL_IP_LEN); } 
        else if (0 == strncmp(key, CONF_CTRL_PORT_STATE, 15))
        { sdmbn_config.ctrl_port_state = atoi(value); }
        else if (0 == strncmp(key, CONF_CTRL_PORT_EVENT, 15))
        { sdmbn_config.ctrl_port_event = atoi(value); }
        else if (0 == strncmp(key, CONF_RELEASE_GET_PKTS, 16))
        { sdmbn_config.release_get_pkts = atoi(value); }
        else if (0 == strncmp(key, CONF_RELEASE_GET_FLOWS, 17))
        { sdmbn_config.release_get_flows = atoi(value); }
        else if (0 == strncmp(key, CONF_MAX_GET_FLOWS, 13))
        { sdmbn_config.max_get_flows = atoi(value); }
    }

    if (fclose(file) != 0)
    {
        ERROR_PRINT("Failed to close config file '%s': %s", filepath, 
                strerror(errno));
        return -1; 
    }

    return 0;
}
