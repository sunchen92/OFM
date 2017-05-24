// Support-analyzer to split a reassembled stream into lines.

#ifndef CONTENTLINE_H
#define CONTENTLINE_H

#include "TCP.h"
#include <boost/serialization/access.hpp>

#define CR_as_EOL 1
#define LF_as_EOL 2

class ContentLine_Analyzer : public TCP_SupportAnalyzer {
public:
	ContentLine_Analyzer(Connection* conn, bool orig);
	~ContentLine_Analyzer();

	// If enabled, flag (first) line with embedded NUL. Default off.
	void SetIsNULSensitive(bool enable)
		{ flag_NULs = enable; }

	// If enabled, skip data above a hole. Default off.
	void SetSkipPartial(bool enable)
		{ skip_partial = enable; }

	// If true, single CR / LF are considered as EOL. Default on for both.
	void SetCRLFAsEOL(int crlf = (CR_as_EOL | LF_as_EOL))
		{ CR_LF_as_EOL = crlf; }

	int CRLFAsEOL()
		{ return CR_LF_as_EOL ; }

	int HasPartialLine() const;

	bool SkipDeliveries() const
		{ return skip_deliveries; }

	void SetSkipDeliveries(int should_skip)
		{ skip_deliveries = should_skip; }

	// We actually have two delivery modes: line delivery and plain
	// delivery for data portions which are not line-separated.
	// SetPlainDelivery() keeps the ContentLine_Analyzer in plain delivery
	// mode for next <length> bytes.  Plain-delivery data is also passed
	// via DeliverStream() and can differentiated by calling
	// IsPlainDelivery().
	void SetPlainDelivery(int64_t length);
	int64_t GetPlainDeliveryLength() const	{ return plain_delivery_length; }
	bool IsPlainDelivery()			{ return is_plain; }

	// Skip <length> bytes after this line.
	// Can be used to skip HTTP data for performance considerations.
	void SkipBytesAfterThisLine(int64_t length);
	void SkipBytes(int64_t length);

	bool IsSkippedContents(int64_t seq, int64_t length)
		{ return seq + length <= seq_to_skip; }

protected:
	ContentLine_Analyzer(AnalyzerTag::Tag tag, Connection* conn, bool orig);

	virtual void DeliverStream(int len, const u_char* data, bool is_orig);
	virtual void Undelivered(int seq, int len, bool orig);
	virtual void EndpointEOF(bool is_orig);

	class State;
	void InitState();
	void InitBuffer(int size);
	virtual void DoDeliver(int len, const u_char* data);
	int DoDeliverOnce(int len, const u_char* data);
	void CheckNUL();

	// Returns the sequence number delivered so far.
	int64_t SeqDelivered() const	{ return seq_delivered_in_lines; }

	u_char* buf;	// where we build up the body of the request
	int offset;	// where we are in buf
	int buf_len;	// how big buf is, total
	unsigned int last_char;	// last (non-option) character scanned

	int64_t seq;	// last seq number
	int64_t seq_to_skip;

	// Seq delivered up to through NewLine() -- it is adjusted
	// *before* NewLine() is called.
	int64_t seq_delivered_in_lines;

	// Number of bytes to be skipped after this line. See
	// comments in SkipBytesAfterThisLine().
	int64_t skip_pending;

	// Remaining bytes to deliver plain.
	int64_t plain_delivery_length;
	int is_plain;

	// Don't deliver further data.
	int skip_deliveries;

	// If true, flag (first) line with embedded NUL.
	unsigned int flag_NULs:1;

	// Whether single CR / LF are considered as EOL.
	unsigned int CR_LF_as_EOL:2;

	// Whether to skip partial conns.
	unsigned int skip_partial:1;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    ContentLine_Analyzer() 
    {
        buf_len = 0;
        offset = 0;
        last_char = 0;
        flag_NULs = 0;
        CR_LF_as_EOL = (CR_as_EOL | LF_as_EOL);
        skip_deliveries = 0;
        skip_partial = 0;
        buf = 0;
        seq_delivered_in_lines = 0;
        skip_pending = 0;
        seq = 0;
        seq_to_skip = 0;
        plain_delivery_length = 0;
        is_plain = 0;
    }; // Dummy default constructor for serialization
};

#endif
