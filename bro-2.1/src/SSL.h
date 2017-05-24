#ifndef ssl_h
#define ssl_h

#include "TCP.h"
#include "ssl_pac.h"
#include <boost/serialization/access.hpp>

class SSL_Analyzer : public TCP_ApplicationAnalyzer {
public:
	SSL_Analyzer(Connection* conn);
	virtual ~SSL_Analyzer();

	// Overriden from Analyzer.
	virtual void Done();
	virtual void DeliverStream(int len, const u_char* data, bool orig);
	virtual void Undelivered(int seq, int len, bool orig);

	// Overriden from TCP_ApplicationAnalyzer.
	virtual void EndpointEOF(bool is_orig);

	static Analyzer* InstantiateAnalyzer(Connection* conn)
		{ return new SSL_Analyzer(conn); }

	static bool Available()
		{
		return ( ssl_client_hello || ssl_server_hello ||
			ssl_established || ssl_extension || ssl_alert ||
			x509_certificate || x509_extension || x509_error );
		}

protected:
	binpac::SSL::SSL_Conn* interp;
	bool had_gap;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    SSL_Analyzer() {}; // Dummy default constructor for serialization
};

#endif
