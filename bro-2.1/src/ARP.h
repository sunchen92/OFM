// See the file "COPYING" in the main distribution directory for copyright.

#ifndef arp_h
#define arp_h

#include "config.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h>
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#elif defined(HAVE_SYS_ETHERNET_H)
#include <sys/ethernet.h>
#elif defined(HAVE_NETINET_IF_ETHER_H)
#include <netinet/if_ether.h>
#elif defined(HAVE_NET_ETHERTYPES_H)
#include <net/ethertypes.h>
#endif

#ifndef arp_pkthdr
#define arp_pkthdr arphdr
#endif

#include "NetVar.h"


class ARP_Analyzer : public BroObj {
public:
	ARP_Analyzer();
	virtual ~ARP_Analyzer();

	void NextPacket(double t, const struct pcap_pkthdr* hdr,
			const u_char* const pkt, int hdr_size);

	void Describe(ODesc* d) const;
	void RREvent(EventHandlerPtr e, const u_char* src, const u_char* dst,
			const char* spa, const char* sha,
			const char* tpa, const char* tha);

	// Whether a packet is of interest for ARP analysis.
	static bool IsARP(const u_char* pkt, int hdr_size);

protected:
	AddrVal* ConstructAddrVal(const void* addr);
	StringVal* EthAddrToStr(const u_char* addr);
	void BadARP(const struct arp_pkthdr* hdr, const char* string);
	void Corrupted(const char* string);

	EventHandlerPtr arp_corrupted_packet;
	EventHandlerPtr arp_request;
	EventHandlerPtr arp_reply;
};

#endif
