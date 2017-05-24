// See the file "COPYING" in the main distribution directory for copyright.

#ifndef zip_h
#define zip_h

#include "config.h"

#include "zlib.h"
#include "TCP.h"
#include <boost/serialization/access.hpp>

class ZIP_Analyzer : public TCP_SupportAnalyzer {
public:
	enum Method { GZIP, DEFLATE };

	ZIP_Analyzer(Connection* conn, bool orig, Method method = GZIP);
	~ZIP_Analyzer();

	virtual void Done();

	virtual void DeliverStream(int len, const u_char* data, bool orig);

protected:
	enum { NONE, ZIP_OK, ZIP_FAIL };
	z_stream* zip;
	int zip_status;
	Method method;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    ZIP_Analyzer() {}; // Dummy default constructor for serialization
};

#endif
