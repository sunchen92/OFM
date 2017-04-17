#ifdef __cplusplus
extern "C" {
#endif

#ifndef _discovery_H_
#define _discovery_H_

///// DEFINES ////////////////////////////////////////////////////////////////
#define DISCOVERY_ETHERTYPE  0x88B5
#define DISCOVERY_FREQUENCY  30

///// FUNCTION PROTOTYPES ////////////////////////////////////////////////////
void *discovery_send(void *arg);

#endif

#ifdef __cplusplus
}
#endif
