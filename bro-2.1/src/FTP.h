// See the file "COPYING" in the main distribution directory for copyright.

#ifndef ftp_h
#define ftp_h

#include "NVT.h"
#include "TCP.h"
#include <boost/serialization/access.hpp>

class FTP_Analyzer : public TCP_ApplicationAnalyzer {
public:
	FTP_Analyzer(Connection* conn);

	virtual void Done();
	virtual void DeliverStream(int len, const u_char* data, bool orig);

	static Analyzer* InstantiateAnalyzer(Connection* conn)
		{
		return new FTP_Analyzer(conn);
		}

	static bool Available()	{ return ftp_request || ftp_reply; }


protected:
	FTP_Analyzer()	{}

	NVT_Analyzer* nvt_orig;
	NVT_Analyzer* nvt_resp;
	uint32 pending_reply;	// code associated with multi-line reply, or 0
	string auth_requested;	// AUTH method requested

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#endif
