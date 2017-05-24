// Analyzer for connections that transfer binary data.

#ifndef FILEANALYZER_H
#define FILEANALYZER_H

#include "TCP.h"
#include <boost/serialization/access.hpp>

#include <magic.h>

class File_Analyzer : public TCP_ApplicationAnalyzer {
public:
	File_Analyzer(Connection* conn);

	virtual void Done();

	virtual void DeliverStream(int len, const u_char* data, bool orig);

	static Analyzer* InstantiateAnalyzer(Connection* conn)
		{ return new File_Analyzer(conn); }

	static bool Available()	{ return file_transferred; }

protected:
	File_Analyzer()	{}

	void Identify();

	static const int BUFFER_SIZE = 1024;
	char buffer[BUFFER_SIZE];
	int buffer_len;

	static void InitMagic(magic_t* magic, int flags);

	static magic_t magic;
	static magic_t magic_mime;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#endif
