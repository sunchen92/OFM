// See the file "COPYING" in the main distribution directory for copyright.

#ifndef sessions_h
#define sessions_h

#include "Dict.h"
#include "CompHash.h"
#include "IP.h"
#include "ARP.h"
#include "Frag.h"
#include "PacketFilter.h"
#include "Stats.h"
#include "NetVar.h"
#include "TunnelEncapsulation.h"
#include <utility>

struct pcap_pkthdr;

class EncapsulationStack;
class Connection;
class OSFingerprint;
class ConnCompressor;
struct ConnID;

declare(PDict,Connection);
declare(PDict,FragReassembler);

class Discarder;
class SteppingStoneManager;
class PacketFilter;

class PacketSortElement;

struct SessionStats {
	int num_TCP_conns;
	int num_UDP_conns;
	int num_ICMP_conns;
	int num_fragments;
	int num_packets;
	int num_timers;
	int num_events_queued;
	int num_events_dispatched;

	int max_TCP_conns;
	int max_UDP_conns;
	int max_ICMP_conns;
	int max_fragments;
	int max_timers;
};

// Drains and deletes a timer manager if it hasn't seen any advances
// for an interval timer_mgr_inactivity_timeout.
class TimerMgrExpireTimer : public Timer {
public:
	TimerMgrExpireTimer(double t, TimerMgr* arg_mgr)
		: Timer(t, TIMER_TIMERMGR_EXPIRE)
		{
		mgr = arg_mgr;
		}

	virtual void Dispatch(double t, int is_expire);

protected:
	double interval;
	TimerMgr* mgr;
};

class NetSessions {
public:
	NetSessions();
	~NetSessions();

	// Main entry point for packet processing. Dispatches the packet
	// either through NextPacket() or NextPacketSecondary(), optionally
	// employing the packet sorter first.
	void DispatchPacket(double t, const struct pcap_pkthdr* hdr,
			const u_char* const pkt, int hdr_size,
			PktSrc* src_ps, PacketSortElement* pkt_elem);

	void Done();	// call to drain events before destructing

	// Returns a reassembled packet, or nil if there are still
	// some missing fragments.
	FragReassembler* NextFragment(double t, const IP_Hdr* ip,
				const u_char* pkt);

	int Get_OS_From_SYN(struct os_type* retval,
			uint16 tot, uint8 DF_flag, uint8 TTL, uint16 WSS,
			uint8 ocnt, uint8* op, uint16 MSS, uint8 win_scale,
			uint32 tstamp, /* uint8 TOS, */ uint32 quirks,
			uint8 ECN) const;

	bool CompareWithPreviousOSMatch(const IPAddr& addr, int id) const;

	// Looks up the connection referred to by the given Val,
	// which should be a conn_id record.  Returns nil if there's
	// no such connection or the Val is ill-formed.
	Connection* FindConnection(Val* v);

	void Remove(Connection* c);
	void Remove(FragReassembler* f);

	void Insert(Connection* c);

	// Generating connection_pending events for all connections
	// that are still active.
	void Drain();

	void GetStats(SessionStats& s) const;

	void Weird(const char* name, const struct pcap_pkthdr* hdr,
	    const u_char* pkt, const EncapsulationStack* encap = 0);
	void Weird(const char* name, const IP_Hdr* ip,
	    const EncapsulationStack* encap = 0);

	PacketFilter* GetPacketFilter()
		{
		if ( ! packet_filter )
			packet_filter = new PacketFilter(packet_filter_default);
		return packet_filter;
		}

	// Looks up timer manager associated with tag.  If tag is unknown and
	// "create" is true, creates new timer manager and stores it.  Returns
	// global timer manager if tag is nil.
	TimerMgr* LookupTimerMgr(const TimerMgr::Tag* tag, bool create = true);

	void ExpireTimerMgrs();

	SteppingStoneManager* GetSTPManager()	{ return stp_manager; }

	unsigned int CurrentConnections()
		{
		return tcp_conns.Length() + udp_conns.Length() +
			icmp_conns.Length();
		}

	void DoNextPacket(double t, const struct pcap_pkthdr* hdr,
			const IP_Hdr* ip_hdr, const u_char* const pkt,
			int hdr_size, const EncapsulationStack* encapsulation);

	/**
	 * Wrapper that recurses on DoNextPacket for encapsulated IP packets.
	 *
	 * @param t Network time.
	 * @param hdr If the outer pcap header is available, this pointer can be set
	 *        so that the fake pcap header passed to DoNextPacket will use
	 *        the same timeval.  The caplen and len fields of the fake pcap
	 *        header are always set to the TotalLength() of \a inner.
	 * @param inner Pointer to IP header wrapper of the inner packet, ownership
	 *        of the pointer's memory is assumed by this function.
	 * @param prev Any previous encapsulation stack of the caller, not including
	 *        the most-recently found depth of encapsulation.
	 * @param ec The most-recently found depth of encapsulation.
	 */
	void DoNextInnerPacket(double t, const struct pcap_pkthdr* hdr,
	                      const IP_Hdr* inner, const EncapsulationStack* prev,
	                      const EncapsulatingConn& ec);

	/**
	 * Returns a wrapper IP_Hdr object if \a pkt appears to be a valid IPv4
	 * or IPv6 header based on whether it's long enough to contain such a header
	 * and also that the payload length field of that header matches the actual
	 * length of \a pkt given by \a caplen.
	 *
	 * @param caplen The length of \a pkt in bytes.
	 * @param pkt The inner IP packet data.
	 * @param proto Either IPPROTO_IPV6 or IPPROTO_IPV4 to indicate which IP
	 *        protocol \a pkt corresponds to.
	 * @param inner The inner IP packet wrapper pointer to be allocated/assigned
	 *        if \a pkt looks like a valid IP packet or at least long enough
	 *        to hold an IP header.
	 * @return 0 If the inner IP packet appeared valid, else -1 if \a caplen
	 *         is greater than the supposed IP packet's payload length field or
	 *         1 if \a caplen is less than the supposed packet's payload length.
	 *         In the -1 case, \a inner may still be non-null if \a caplen was
	 *         long enough to be an IP header, and \a inner is always non-null
	 *         for other return values.
	 */
	int ParseIPPacket(int caplen, const u_char* const pkt, int proto,
	                  IP_Hdr*& inner);

	unsigned int ConnectionMemoryUsage();
	unsigned int ConnectionMemoryUsageConnVals();
	unsigned int MemoryAllocation();
	TCPStateStats tcp_stats;	// keeps statistics on TCP states

	PDict(Connection) tcp_conns;
	PDict(Connection) udp_conns;
	PDict(Connection) icmp_conns;

protected:
	friend class RemoteSerializer;
	friend class ConnCompressor;
	friend class TimerMgrExpireTimer;
	friend class IPTunnelTimer;

	Connection* NewConn(HashKey* k, double t, const ConnID* id,
			const u_char* data, int proto, uint32 flow_lable,
			const EncapsulationStack* encapsulation);

	// Check whether the tag of the current packet is consistent with
	// the given connection.  Returns:
	//    -1   if current packet is to be completely ignored.
	//     0   if tag is not consistent and new conn should be instantiated.
	//     1   if tag is consistent, i.e., packet is part of connection.
	int CheckConnectionTag(Connection* conn);

	// Returns true if the port corresonds to an application
	// for which there's a Bro analyzer (even if it might not
	// be used by the present policy script), or it's more
	// generally a likely server port, false otherwise.
	//
	// Note, port is in host order.
	bool IsLikelyServerPort(uint32 port,
				TransportProto transport_proto) const;

	// Upon seeing the first packet of a connection, checks whether
	// we want to analyze it (e.g., we may not want to look at partial
	// connections), and, if yes, whether we should flip the roles of
	// originator and responder (based on known ports or such).
	// Use tcp_flags=0 for non-TCP.
	bool WantConnection(uint16 src_port, uint16 dest_port,
				TransportProto transport_proto,
				uint8 tcp_flags, bool& flip_roles);

	void NextPacket(double t, const struct pcap_pkthdr* hdr,
			const u_char* const pkt, int hdr_size,
			PacketSortElement* pkt_elem);

	void NextPacketSecondary(double t, const struct pcap_pkthdr* hdr,
			const u_char* const pkt, int hdr_size,
			const PktSrc* src_ps);

	// Record the given packet (if a dumper is active).  If len=0
	// then the whole packet is recorded, otherwise just the first
	// len bytes.
	void DumpPacket(const struct pcap_pkthdr* hdr, const u_char* pkt,
			int len=0);

	void Internal(const char* msg, const struct pcap_pkthdr* hdr,
			const u_char* pkt);

	// For a given protocol, checks whether the header's length as derived
	// from lower-level headers or the length actually captured is less
	// than that protocol's minimum header size.
	bool CheckHeaderTrunc(int proto, uint32 len, uint32 caplen,
			      const struct pcap_pkthdr* hdr, const u_char* pkt,
			      const EncapsulationStack* encap);

	CompositeHash* ch;
	PDict(FragReassembler) fragments;

	typedef pair<IPAddr, IPAddr> IPPair;
	typedef pair<EncapsulatingConn, double> TunnelActivity;
	typedef std::map<IPPair, TunnelActivity> IPTunnelMap;
	IPTunnelMap ip_tunnels;

	ARP_Analyzer* arp_analyzer;

	SteppingStoneManager* stp_manager;
	Discarder* discarder;
	PacketFilter* packet_filter;
	OSFingerprint* SYN_OS_Fingerprinter;
	int build_backdoor_analyzer;
	int dump_this_packet;	// if true, current packet should be recorded
	int num_packets_processed;
	PacketProfiler* pkt_profiler;

	// We may use independent timer managers for different sets of related
	// activity.  The managers are identified by an unique tag.
	typedef std::map<TimerMgr::Tag, TimerMgr*> TimerMgrMap;
	TimerMgrMap timer_mgrs;
};


class IPTunnelTimer : public Timer {
public:
	IPTunnelTimer(double t, NetSessions::IPPair p)
	: Timer(t + BifConst::Tunnel::ip_tunnel_timeout,
			TIMER_IP_TUNNEL_INACTIVITY), tunnel_idx(p) {}

	~IPTunnelTimer() {}

	void Dispatch(double t, int is_expire);

protected:
	NetSessions::IPPair tunnel_idx;
};

// Manager for the currently active sessions.
extern NetSessions* sessions;

#endif
