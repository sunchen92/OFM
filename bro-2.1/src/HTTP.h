// See the file "COPYING" in the main distribution directory for copyright.

#ifndef http_h
#define http_h

#include "TCP.h"
#include "ContentLine.h"
#include "MIME.h"
#include "binpac_bro.h"
#include "ZIP.h"
#include "IPAddr.h"
#include <boost/serialization/access.hpp>

enum CHUNKED_TRANSFER_STATE {
	NON_CHUNKED_TRANSFER,
	BEFORE_CHUNK,
	EXPECT_CHUNK_SIZE,
	EXPECT_CHUNK_DATA,
	EXPECT_CHUNK_DATA_CRLF,
	EXPECT_CHUNK_TRAILER,
	EXPECT_NOTHING,
};

typedef enum { IDENTITY, GZIP, COMPRESS, DEFLATE } HTTPEncoding;

class HTTP_Entity;
class HTTP_Message;
class HTTP_Analyzer;

class HTTP_Entity : public MIME_Entity {
public:
	HTTP_Entity(HTTP_Message* msg, MIME_Entity* parent_entity,
			int expect_body);
	~HTTP_Entity()
		{
		if ( zip )
			{ zip->Done(); delete zip; }
		}

	void EndOfData();
	void Deliver(int len, const char* data, int trailing_CRLF);
	int Undelivered(int64_t len);
	int64_t BodyLength() const 		{ return body_length; }
	int64_t HeaderLength() const 	{ return header_length; }
	void SkipBody() 		{ deliver_body = 0; }

protected:
	class UncompressedOutput;
	friend class UncompressedOutput;

	HTTP_Message* http_message;
	int chunked_transfer_state;
	int64_t content_length;
	int64_t expect_data_length;
	int expect_body;
	int64_t body_length;
	int64_t header_length;
	int deliver_body;
	HTTPEncoding encoding;
	ZIP_Analyzer* zip;

	MIME_Entity* NewChildEntity() { return new HTTP_Entity(http_message, this, 1); }

	void DeliverBody(int len, const char* data, int trailing_CRLF);
	void DeliverBodyClear(int len, const char* data, int trailing_CRLF);

	void SubmitData(int len, const char* buf);

	void SetPlainDelivery(int64_t length);

	void SubmitHeader(MIME_Header* h);
	void SubmitAllHeaders();

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    HTTP_Entity() {}; // Dummy default constructor for serialization
};

enum {
	HTTP_BODY_NOT_EXPECTED,
	HTTP_BODY_EXPECTED,
	HTTP_BODY_MAYBE,
};

// Finishing HTTP Messages:
//
// HTTP_Entity::SubmitAllHeaders	-> EndOfData (no body)
// HTTP_Entity::Deliver	-> EndOfData (end of body)
// HTTP_Analyzer::Done	-> {Request,Reply}Made (connection terminated)
// {Request,Reply}Made	-> HTTP_Message::Done
// HTTP_Message::Done	-> MIME_Message::Done, EndOfData, HTTP_MessageDone
// MIME_Entity::EndOfData	-> Message::EndEntity
// HTTP_Message::EndEntity	-> Message::Done
// HTTP_MessageDone	-> {Request,Reply}Made

class HTTP_Message : public MIME_Message {
public:
	HTTP_Message(HTTP_Analyzer* analyzer, ContentLine_Analyzer* cl,
			 bool is_orig, int expect_body, int64_t init_header_length);
	~HTTP_Message();
	void Done(const int interrupted, const char* msg);
	void Done() { Done(0, "message ends normally"); }

	int Undelivered(int64_t len);

	void BeginEntity(MIME_Entity* /* entity */);
	void EndEntity(MIME_Entity* entity);
	void SubmitHeader(MIME_Header* h);
	void SubmitAllHeaders(MIME_HeaderList& /* hlist */);
	void SubmitData(int len, const char* buf);
	int RequestBuffer(int* plen, char** pbuf);
	void SubmitAllData();
	void SubmitEvent(int event_type, const char* detail);

	void SubmitTrailingHeaders(MIME_HeaderList& /* hlist */);
	void SetPlainDelivery(int64_t length);
	void SkipEntityData();

	HTTP_Analyzer* MyHTTP_Analyzer() const
		{ return (HTTP_Analyzer*) analyzer; }

	void Weird(const char* msg);
	bool IsOrig()	{ return is_orig; }

    int GetBufferDiff(char *buf) 
    { 
        if (0 == data_buffer)
            {
            if (0 == buffers.size())
            { return -1; }
            return buf - (char *)buffers.back()->Bytes();
            }
        return buf - (char *)data_buffer->Bytes();
    }
    char* GetBuffer(int diff)
    { 
        if (0 == data_buffer)
            {
            if (0 == buffers.size())
            { return NULL; }
            return (char *)buffers.back()->Bytes() + diff;
            }
        return (char *)data_buffer->Bytes() + diff; 
    }

protected:
	HTTP_Analyzer* analyzer;
	ContentLine_Analyzer* content_line;
	bool is_orig;

	vector<BroString*> buffers;

	// Controls the total buffer size within http_entity_data_delivery_size.
	int total_buffer_size;

	int buffer_offset, buffer_size;
	BroString* data_buffer;

	double start_time;

	int64_t body_length;	// total length of entity bodies
	int64_t header_length;	// total length of headers, including the request/reply line

	// Total length of content gaps that are "successfully" skipped.
	// Note: this might NOT include all content gaps!
	int64_t content_gap_length;

	HTTP_Entity* current_entity;

	int InitBuffer(int64_t length);
	void DeliverEntityData();

	Val* BuildMessageStat(const int interrupted, const char* msg);

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    HTTP_Message() { 
        analyzer = 0;
        content_line = 0;
        is_orig = 0;
        total_buffer_size = 0;
        buffer_offset = 0;
        buffer_size = 0;
    //    data_buffer = 0; 
        start_time = 0;
        body_length = 0;
        header_length = 0;
        content_gap_length = 0;
        current_entity = 0; 
    }; // Dummy default constructor for serialization
};

class HTTP_Analyzer : public TCP_ApplicationAnalyzer {
public:
	HTTP_Analyzer(Connection* conn);
	~HTTP_Analyzer();

	void Undelivered(TCP_Endpoint* sender, int seq, int len);

	void HTTP_Header(int is_orig, MIME_Header* h);
	void HTTP_EntityData(int is_orig, const BroString* entity_data);
	void HTTP_MessageDone(int is_orig, HTTP_Message* message);
	void HTTP_Event(const char* category, const char* detail);
	void HTTP_Event(const char* category, StringVal *detail);

	void SkipEntityData(int is_orig);

	int IsConnectionClose()		{ return connection_close; }
	int HTTP_ReplyCode() const { return reply_code; };

	// Overriden from Analyzer.
	virtual void Done();
	virtual void DeliverStream(int len, const u_char* data, bool orig);
	virtual void Undelivered(int seq, int len, bool orig);

	// Overriden from TCP_ApplicationAnalyzer
	virtual void EndpointEOF(bool is_orig);
	virtual void ConnectionFinished(int half_finished);
	virtual void ConnectionReset();
	virtual void PacketWithRST();

	static Analyzer* InstantiateAnalyzer(Connection* conn)
		{ return new HTTP_Analyzer(conn); }

	static bool Available()
		{ return (http_request || http_reply || http_header ||
			http_all_headers || http_begin_entity || http_end_entity ||
			http_content_type || http_entity_data || http_message_done ||
			http_event || http_stats) && !FLAGS_use_binpac; }

protected:
	void GenStats();

	int HTTP_RequestLine(const char* line, const char* end_of_line);
	int HTTP_ReplyLine(const char* line, const char* end_of_line);

	void InitHTTPMessage(ContentLine_Analyzer* cl, HTTP_Message*& message, bool is_orig,
				int expect_body, int64_t init_header_length);

	const char* PrefixMatch(const char* line, const char* end_of_line,
				const char* prefix);
	const char* PrefixWordMatch(const char* line, const char* end_of_line,
				const char* prefix);

	int ParseRequest(const char* line, const char* end_of_line);
	double HTTP_Version(int len, const char* data);

	void SetVersion(double& version, double new_version);

	int RequestExpected() const { return num_requests == 0 || keep_alive; }

	void HTTP_Request();
	void HTTP_Reply();

	void RequestMade(const int interrupted, const char* msg);
	void ReplyMade(const int interrupted, const char* msg);
	void RequestClash(Val* clash_val);

	const BroString* UnansweredRequestMethod();

	void ParseVersion(data_chunk_t ver, const IPAddr& host, bool user_agent);
	int HTTP_ReplyCode(const char* code_str);
	int ExpectReplyMessageBody();

	StringVal* TruncateURI(StringVal* uri);

	int request_state, reply_state;
	int num_requests, num_replies;
	int num_request_lines, num_reply_lines;
	double request_version, reply_version;
	int keep_alive;
	int connection_close;
	int request_ongoing, reply_ongoing;

	StringVal* request_method;

	// request_URI is in the original form (may contain '%<hex><hex>'
	// sequences).
	Val* request_URI;

	// unescaped_URI does not contain escaped sequences.
	Val* unescaped_URI;

	std::queue<Val*> unanswered_requests;

	int reply_code;
	Val* reply_reason_phrase;

	ContentLine_Analyzer* content_line_orig;
	ContentLine_Analyzer* content_line_resp;

	HTTP_Message* request_message;
	HTTP_Message* reply_message;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    HTTP_Analyzer() {}; // Dummy default constructor for serialization
};

extern int is_reserved_URI_char(unsigned char ch);
extern int is_unreserved_URI_char(unsigned char ch);
extern void escape_URI_char(unsigned char ch, unsigned char*& p);
extern BroString* unescape_URI(const u_char* line, const u_char* line_end,
				Analyzer* analyzer);

#endif
