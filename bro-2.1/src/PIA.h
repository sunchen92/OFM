// An analyzer for application-layer protocol-detection.

#ifndef PIA_H
#define PIA_H

#include "Analyzer.h"
#include "TCP.h"
#include <boost/serialization/access.hpp>
#include <boost/serialization/export.hpp>

class RuleEndpointState;

// Abstract PIA class providing common functionality for both TCP and UDP.
// Accepts only packet input.
//
// Note that the PIA provides our main interface to the signature engine and
// also keeps the matching state.  This is because (i) it needs to match
// itself, and (ii) in case of tunnel-decapsulation we may have multiple
// PIAs and then each needs its own matching-state.
class PIA : public RuleMatcherState {
public:
	PIA(Analyzer* as_analyzer);
	virtual ~PIA();

	// Called when PIA wants to put an Analyzer in charge.  rule is the
	// signature that triggered the activitation, if any.
	virtual void ActivateAnalyzer(AnalyzerTag::Tag tag,
					const Rule* rule = 0) = 0;

	// Called when PIA wants to remove an Analyzer.
	virtual void DeactivateAnalyzer(AnalyzerTag::Tag tag) = 0;

	void Match(Rule::PatternType type, const u_char* data, int len,
			bool is_orig, bool bol, bool eol, bool clear_state);

	void ReplayPacketBuffer(Analyzer* analyzer);

	// Children are also derived from Analyzer. Return this object
	// as pointer to an Analyzer.
	Analyzer* AsAnalyzer()	{ return as_analyzer; }

	static bool Available()	{ return true; }

protected:
	void PIA_Done();
	void PIA_DeliverPacket(int len, const u_char* data, bool is_orig,
				int seq, const IP_Hdr* ip, int caplen);

	enum State { INIT, BUFFERING, MATCHING_ONLY, SKIPPING } state;

	// Buffers one chunk of data.  Used both for packet payload (incl.
	// sequence numbers for TCP) and chunks of a reassembled stream.
	struct DataBlock {
		const u_char* data;
		bool is_orig;
		int len;
		int seq;
		DataBlock* next;
	};

	struct Buffer {
		Buffer() { head = tail = 0; size = 0; state = INIT; }

		DataBlock* head;
		DataBlock* tail;
		int size;
		State state;
	};

	void AddToBuffer(Buffer* buffer, int seq, int len,
				const u_char* data, bool is_orig);
	void AddToBuffer(Buffer* buffer, int len,
				const u_char* data, bool is_orig);
	void ClearBuffer(Buffer* buffer);

	DataBlock* CurrentPacket()	{ return &current_packet; }

	void DoMatch(const u_char* data, int len, bool is_orig, bool bol,
			bool eol, bool clear_state, const IP_Hdr* ip = 0);

	void SetConn(Connection* c)	{ conn = c; }

	Buffer pkt_buffer;

    PIA() {}; // Dummy default constructor for serialization

private:
	Analyzer* as_analyzer;
	Connection* conn;
	DataBlock current_packet;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

// PIA for UDP.
class PIA_UDP : public PIA, public Analyzer {
public:
	PIA_UDP(Connection* conn)
	: PIA(this), Analyzer(AnalyzerTag::PIA_UDP, conn)
		{ SetConn(conn); }
	virtual ~PIA_UDP()	{ }

	static Analyzer* InstantiateAnalyzer(Connection* conn)
		{ return new PIA_UDP(conn); }

protected:
	virtual void Done()
		{
		Analyzer::Done();
		PIA_Done();
		}

	virtual void DeliverPacket(int len, const u_char* data, bool is_orig,
					int seq, const IP_Hdr* ip, int caplen)
		{
		Analyzer::DeliverPacket(len, data, is_orig, seq, ip, caplen);
		PIA_DeliverPacket(len, data, is_orig, seq, ip, caplen);
		}

	virtual void ActivateAnalyzer(AnalyzerTag::Tag tag, const Rule* rule);
	virtual void DeactivateAnalyzer(AnalyzerTag::Tag tag);

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
    
    PIA_UDP() {}; // Dummy default constructor for serialization
};

// PIA for TCP.  Accepts both packet and stream input (and reassembles
// packets before passing payload on to children).
class PIA_TCP : public PIA, public TCP_ApplicationAnalyzer {
public:
	PIA_TCP(Connection* conn)
	: PIA(this), TCP_ApplicationAnalyzer(AnalyzerTag::PIA_TCP, conn)
		{ stream_mode = false; SetConn(conn); }

	virtual ~PIA_TCP();

	virtual void Init();

	// The first packet for each direction of a connection is passed
	// in here.
	//
	// (This is a bit crude as it doesn't really fit nicely into the
	// analyzer interface.  Yet we need it for initializing the packet
	// matcher in the case that we already get reassembled input,
	// and making it part of the general analyzer interface seems
	// to be unnecessary overhead.)
	void FirstPacket(bool is_orig, const IP_Hdr* ip);

	void ReplayStreamBuffer(Analyzer* analyzer);

	static Analyzer* InstantiateAnalyzer(Connection* conn)
		{ return new PIA_TCP(conn); }

protected:
	virtual void Done()
		{
		Analyzer::Done();
		PIA_Done();
		}

	virtual void DeliverPacket(int len, const u_char* data, bool is_orig,
					int seq, const IP_Hdr* ip, int caplen)
		{
		Analyzer::DeliverPacket(len, data, is_orig, seq, ip, caplen);
		PIA_DeliverPacket(len, data, is_orig, seq, ip, caplen);
		}

	virtual void DeliverStream(int len, const u_char* data, bool is_orig);
	virtual void Undelivered(int seq, int len, bool is_orig);

	virtual void ActivateAnalyzer(AnalyzerTag::Tag tag,
					const Rule* rule = 0);
	virtual void DeactivateAnalyzer(AnalyzerTag::Tag tag);

private:
	// FIXME: Not sure yet whether we need both pkt_buffer and stream_buffer.
	// In any case, it's easier this way...
	Buffer stream_buffer;

	bool stream_mode;
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    PIA_TCP() {}; // Dummy default constructor for serialization
};

#endif
