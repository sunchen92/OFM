// See the file "COPYING" in the main distribution directory for copyright.

#ifndef udp_h
#define udp_h

#include "Analyzer.h"
#include <netinet/udp.h>
#include <boost/serialization/access.hpp>

typedef enum {
	UDP_INACTIVE,	// no packet seen
	UDP_ACTIVE,	// packets seen
} UDP_EndpointState;

class UDP_Analyzer : public TransportLayerAnalyzer {
public:
	UDP_Analyzer(Connection* conn);
	virtual ~UDP_Analyzer();

	virtual void Init();

	virtual void UpdateConnVal(RecordVal *conn_val);

	static Analyzer* InstantiateAnalyzer(Connection* conn)
		{ return new UDP_Analyzer(conn); }

	static bool Available() { return true; }

protected:
	virtual void Done();
	virtual void DeliverPacket(int len, const u_char* data, bool orig,
					int seq, const IP_Hdr* ip, int caplen);
	virtual bool IsReuse(double t, const u_char* pkt);
	virtual unsigned int MemoryAllocation() const;

	// Returns true if the checksum is valid, false if not
	static bool ValidateChecksum(const IP_Hdr* ip, const struct udphdr* up,
	                             int len);

	bro_int_t request_len, reply_len;

private:
	void UpdateEndpointVal(RecordVal* endp, int is_orig);

#define HIST_ORIG_DATA_PKT 0x1
#define HIST_RESP_DATA_PKT 0x2
#define HIST_ORIG_CORRUPT_PKT 0x4
#define HIST_RESP_CORRUPT_PKT 0x8

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    UDP_Analyzer() {}; // Dummy default constructor for serialization
};

#endif
