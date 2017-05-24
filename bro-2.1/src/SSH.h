// See the file "COPYING" in the main distribution directory for copyright.

#ifndef ssh_h
#define ssh_h

#include "TCP.h"
#include "ContentLine.h"
#include <boost/serialization/access.hpp>

class SSH_Analyzer : public TCP_ApplicationAnalyzer {
public:
	SSH_Analyzer(Connection* conn);

	virtual void DeliverStream(int len, const u_char* data, bool orig);

	static Analyzer* InstantiateAnalyzer(Connection* conn)
		{ return new SSH_Analyzer(conn); }

	static bool Available()
		{ return  ssh_client_version || ssh_server_version; }

private:
	ContentLine_Analyzer* orig;
	ContentLine_Analyzer* resp;
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    SSH_Analyzer() {}; // Dummy default constructor for serialization
};

#endif
