#include "SDMBN.h"
#include "SDMBNCore.h"
#include "discovery.h"
#include "SDMBNDebug.h"
#include "SDMBNJson.h"
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <linux/if_packet.h>
#include <sys/ioctl.h>
#include <unistd.h>

void *discovery_send(void *arg)
{
    INFO_PRINT("Discovery sending thread started");

    /*
     * Code for sending a raw packet was customized based on the code at
     * http://austinmarton.wordpress.com/2011/09/14/
     *  sending-raw-ethernet-packets-from-a-specific-interface-in-c-on-linux/
     */

    // Open RAW socket to send on
    int rawsock = socket(AF_PACKET, SOCK_RAW, htons(DISCOVERY_ETHERTYPE));
    if (rawsock < 0) 
    {
        ERROR_PRINT("Failed to open discovery socket");
        return NULL;
    }

    // Get the index of the interface to send on
    struct ifreq if_idx;
    memset(&if_idx, 0, sizeof(struct ifreq));
    strcpy(if_idx.ifr_name, sdmbn_locals->device);
    if (ioctl(rawsock, SIOCGIFINDEX, &if_idx) < 0)
    {
        ERROR_PRINT("Failed to set interface %s for discovery socket",
                sdmbn_locals->device);
        return NULL;
    }

    // Get the MAC address of the interface to send on
    struct ifreq if_mac;
    memset(&if_mac, 0, sizeof(struct ifreq));
    strcpy(if_mac.ifr_name, sdmbn_locals->device);
    if (ioctl(rawsock, SIOCGIFHWADDR, &if_mac) < 0)
    {
        ERROR_PRINT("Failed to set MAC address for discovery socket");
        return NULL;
    } 

    // Construct the Ethernet header 
    int tx_len = 0;
    char sendbuf[1500];
    struct ether_header *eh = (struct ether_header *) sendbuf;
    memset(sendbuf, 0, 1500);
    eh->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
    eh->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
    eh->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
    eh->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
    eh->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
    eh->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
    eh->ether_dhost[0] = 0xFF; 
    eh->ether_dhost[1] = 0xFF;
    eh->ether_dhost[2] = 0xFF;
    eh->ether_dhost[3] = 0xFF;
    eh->ether_dhost[4] = 0xFF;
    eh->ether_dhost[5] = 0xFF;
    eh->ether_type = htons(DISCOVERY_ETHERTYPE);
    tx_len += sizeof(struct ether_header);

    // Construct the discover message
    json_object *discover = json_compose_discover();
    char *jsonstr = (char*)json_object_to_json_string(discover);
    int jsonlen = strlen(jsonstr);
    strncpy(sendbuf + tx_len, jsonstr, jsonlen);
    tx_len += jsonlen;
    sendbuf[tx_len] = '\n';
    tx_len++;
    // Free JSON object
    json_object_put(discover);

    // Prepare to send the raw Ethernet packet
    // Destination address
    struct sockaddr_ll socket_address;
    // Index of the network device
    socket_address.sll_ifindex = if_idx.ifr_ifindex;
    // Address length
    socket_address.sll_halen = ETH_ALEN;
    // Destination MAC
    socket_address.sll_addr[0] = 0xFF;
    socket_address.sll_addr[1] = 0xFF;
    socket_address.sll_addr[2] = 0xFF;
    socket_address.sll_addr[3] = 0xFF;
    socket_address.sll_addr[4] = 0xFF;
    socket_address.sll_addr[5] = 0xFF;

    while(1)
    {
        // Send packet
        if (sendto(rawsock, sendbuf, tx_len, 0, 
                    (struct sockaddr*)&socket_address, 
                    sizeof(struct sockaddr_ll)) < 0)
        {
            ERROR_PRINT("Failed to send discovery packet");
            return NULL;
        }
        DEBUG_PRINT("Sent discovery packet");
        sleep(DISCOVERY_FREQUENCY);
    }
}
