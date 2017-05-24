// See the file "COPYING" in the main distribution directory for copyright.
//

#ifndef CONNSTATS_H
#define CONNSTATS_H

#include "Analyzer.h"
#include "NetVar.h"
#include <boost/serialization/access.hpp>


class ConnSize_Analyzer : public Analyzer {
public:
	ConnSize_Analyzer(Connection* c);
	virtual ~ConnSize_Analyzer();

	virtual void Init();
	virtual void Done();

	// from Analyzer.h
	virtual void UpdateConnVal(RecordVal *conn_val);
	virtual void FlipRoles();

	static Analyzer* InstantiateAnalyzer(Connection* conn)
		{ return new ConnSize_Analyzer(conn); }

	static bool Available()	{ return BifConst::use_conn_size_analyzer ; }

protected:
	virtual void DeliverPacket(int len, const u_char* data, bool is_orig,
					int seq, const IP_Hdr* ip, int caplen);


	uint64_t orig_bytes;
	uint64_t resp_bytes;
	uint64_t orig_pkts;
	uint64_t resp_pkts;
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    ConnSize_Analyzer() {}; // Dummy default constructor for serialization
};

#endif
