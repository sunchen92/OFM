#ifndef base64_h
#define base64_h

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "util.h"
#include "BroString.h"
#include "Analyzer.h"
#include <boost/serialization/access.hpp>

// Maybe we should have a base class for generic decoders?

class Base64Decoder {
public:
	// <analyzer> is used for error reporting, and it should be zero when
	// the decoder is called by the built-in function decode_base64().
	// Empty alphabet indicates the default base64 alphabet.
	Base64Decoder(Analyzer* analyzer, const string& alphabet = "");
	~Base64Decoder();

	// A note on Decode():
	//
	// The input is specified by <len> and <data> and the output
	// buffer by <blen> and <buf>.  If *buf is nil, a buffer of
	// an appropriate size will be new'd and *buf will point
	// to the buffer on return. *blen holds the length of
	// decoded data on return.  The function returns the number of
	// input bytes processed, since the decoding will stop when there
	// is not enough output buffer space.

	int Decode(int len, const char* data, int* blen, char** buf);

	int Done(int* pblen, char** pbuf);
	int HasData() const { return base64_group_next != 0; }

	// True if an error has occurred.
	int Errored() const	{ return errored; }

	const char* ErrorMsg() const	{ return error_msg; }
	void IllegalEncoding(const char* msg)
		{ 
		// strncpy(error_msg, msg, sizeof(error_msg));
		if ( analyzer )
			analyzer->Weird("base64_illegal_encoding", msg);
		else
			reporter->Error("%s", msg);
		}

protected:
	char error_msg[256];

protected:
	char base64_group[4];
	int base64_group_next;
	int base64_padding;
	int base64_after_padding;
	int errored;	// if true, we encountered an error - skip further processing
	Analyzer* analyzer;
	int* base64_table;

	static int* InitBase64Table(const string& alphabet);
	static int default_base64_table[256];
	static const string default_alphabet;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    Base64Decoder() {}; // Dummy default constructor for serialization
};

BroString* decode_base64(const BroString* s, const BroString* a = 0);

#endif /* base64_h */
