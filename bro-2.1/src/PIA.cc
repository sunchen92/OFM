#include "PIA.h"
#include "RuleMatcher.h"
#include "TCP_Reassembler.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/export.hpp>
#include "SDMBNlocal.h"

PIA::PIA(Analyzer* arg_as_analyzer)
	{
	current_packet.data = 0;
	as_analyzer = arg_as_analyzer;
	}

PIA::~PIA()
	{
	ClearBuffer(&pkt_buffer);
	}

void PIA::ClearBuffer(Buffer* buffer)
	{
	DataBlock* next = 0;
	for ( DataBlock* b = buffer->head; b; b = next )
		{
		next = b->next;
		delete [] b->data;
		delete b;
		}

	buffer->head = buffer->tail = 0;
	buffer->size = 0;
	}

void PIA::AddToBuffer(Buffer* buffer, int seq, int len, const u_char* data,
			bool is_orig)
	{
	u_char* tmp = 0;

	if ( data )
		{
		tmp = new u_char[len];
		memcpy(tmp, data, len);
		}

	DataBlock* b = new DataBlock;
	b->data = tmp;
	b->is_orig = is_orig;
	b->len = len;
	b->seq = seq;
	b->next = 0;

	if ( buffer->tail )
		{
		buffer->tail->next = b;
		buffer->tail = b;
		}
	else
		buffer->head = buffer->tail = b;

	buffer->size += len;
	}

void PIA::AddToBuffer(Buffer* buffer, int len, const u_char* data, bool is_orig)
	{
	AddToBuffer(buffer, -1, len, data, is_orig);
	}

void PIA::ReplayPacketBuffer(Analyzer* analyzer)
	{
	DBG_LOG(DBG_DPD, "PIA replaying %d total packet bytes", pkt_buffer.size);

	for ( DataBlock* b = pkt_buffer.head; b; b = b->next )
		analyzer->DeliverPacket(b->len, b->data, b->is_orig, -1, 0, 0);
	}

void PIA::PIA_Done()
	{
	FinishEndpointMatcher();
	}

void PIA::PIA_DeliverPacket(int len, const u_char* data, bool is_orig, int seq,
				const IP_Hdr* ip, int caplen)
	{
	if ( pkt_buffer.state == SKIPPING )
		return;

	current_packet.data = data;
	current_packet.len = len;
	current_packet.seq = seq;
	current_packet.is_orig = is_orig;

	State new_state = pkt_buffer.state;

	if ( pkt_buffer.state == INIT )
		new_state = BUFFERING;

	if ( (pkt_buffer.state == BUFFERING || new_state == BUFFERING) &&
	     len > 0 )
		{
		AddToBuffer(&pkt_buffer, seq, len, data, is_orig);
		if ( pkt_buffer.size > dpd_buffer_size )
			new_state = dpd_match_only_beginning ?
						SKIPPING : MATCHING_ONLY;
		}

	// FIXME: I'm not sure why it does not work with eol=true...
	DoMatch(data, len, is_orig, true, false, false, ip);

	pkt_buffer.state = new_state;

	current_packet.data = 0;
	}

void PIA::Match(Rule::PatternType type, const u_char* data, int len,
		bool is_orig, bool bol, bool eol, bool clear_state)
	{
	if ( ! MatcherInitialized(is_orig) )
		InitEndpointMatcher(AsAnalyzer(), 0, 0, is_orig, this);

	RuleMatcherState::Match(type, data, len, is_orig, bol, eol, clear_state);
	}

void PIA::DoMatch(const u_char* data, int len, bool is_orig, bool bol, bool eol,
			bool clear_state, const IP_Hdr* ip)
	{
	if ( ! rule_matcher )
		return;

	if ( ! MatcherInitialized(is_orig) )
		InitEndpointMatcher(AsAnalyzer(), ip, len, is_orig, this);

	RuleMatcherState::Match(Rule::PAYLOAD, data, len, is_orig,
				bol, eol, clear_state);
	}

BOOST_CLASS_EXPORT_GUID(PIA,"PIA")
template<class Archive>
void PIA::serialize(Archive & ar, const unsigned int version)
    {
        SERIALIZE_PRINT("%s:PIA:%d",__FILE__,__LINE__);

        // Serialize RuleMatcherState
        ar & boost::serialization::base_object<RuleMatcherState>(*this);

        //ar & pkt_buffer.head //FIXME
        //ar & pkt_buffer.tail //FIXME
        ar & pkt_buffer.size;
        ar & pkt_buffer.state;

        // Special check for analyzer serializability
        if (!Archive::is_loading::value)
        {
            if (as_analyzer != NULL)
            { assert(sdmbn_can_serialize(as_analyzer->GetTag())); }
        }
        SERIALIZE_PRINT("PIA:%d",__LINE__);
        ar & as_analyzer;

        ar & conn;
        //ar & current_packet; //FIXME
        SERIALIZE_PRINT("\t\tPIA:Done");
    }
template void PIA::serialize<boost::archive::text_oarchive>(
        boost::archive::text_oarchive & ar, 
        const unsigned int file_version);
template void PIA::serialize<boost::archive::text_iarchive>(
        boost::archive::text_iarchive & ar, 
        const unsigned int file_version);

void PIA_UDP::ActivateAnalyzer(AnalyzerTag::Tag tag, const Rule* rule)
	{
	if ( pkt_buffer.state == MATCHING_ONLY )
		{
		DBG_LOG(DBG_DPD, "analyzer found but buffer already exceeded");
		// FIXME: This is where to check whether an analyzer
		// supports partial connections once we get such.
		return;
		}

	if ( Parent()->HasChildAnalyzer(tag) )
		return;

	Analyzer* a = Parent()->AddChildAnalyzer(tag);
	a->SetSignature(rule);

	if ( a )
		ReplayPacketBuffer(a);
	}

void PIA_UDP::DeactivateAnalyzer(AnalyzerTag::Tag tag)
	{
	reporter->InternalError("PIA_UDP::Deact not implemented yet");
	}

BOOST_CLASS_EXPORT_GUID(PIA_UDP,"PIA_UDP")
template<class Archive>
void PIA_UDP::serialize(Archive & ar, const unsigned int version)
    {
        SERIALIZE_PRINT("%s:PIA_UDP:%d",__FILE__,__LINE__);

        // Serialize PIA
        ar & boost::serialization::base_object<PIA>(*this);
        // Serialize Analyzer
        ar & boost::serialization::base_object<Analyzer>(*this);
        SERIALIZE_PRINT("\t\tPIA_UDP:Done");
    }
template void PIA_UDP::serialize<boost::archive::text_oarchive>(
        boost::archive::text_oarchive & ar, 
        const unsigned int file_version);
template void PIA_UDP::serialize<boost::archive::text_iarchive>(
        boost::archive::text_iarchive & ar, 
        const unsigned int file_version);

//// TCP PIA

PIA_TCP::~PIA_TCP()
	{
	ClearBuffer(&stream_buffer);
	}

void PIA_TCP::Init()
	{
	TCP_ApplicationAnalyzer::Init();

	if ( Parent()->GetTag() == AnalyzerTag::TCP )
		{
		TCP_Analyzer* tcp = static_cast<TCP_Analyzer*>(Parent());
		SetTCP(tcp);
		tcp->SetPIA(this);
		}
	}

void PIA_TCP::FirstPacket(bool is_orig, const IP_Hdr* ip)
	{
	static char dummy_packet[sizeof(struct ip) + sizeof(struct tcphdr)];
	static struct ip* ip4 = 0;
	static struct tcphdr* tcp4 = 0;
	static IP_Hdr* ip4_hdr = 0;

	DBG_LOG(DBG_DPD, "PIA_TCP[%d] FirstPacket(%s)", GetID(), (is_orig ? "T" : "F"));

	if ( ! ip )
		{
		// Create a dummy packet.  Not very elegant, but everything
		// else would be *really* ugly ...
		if ( ! ip4_hdr )
			{
			ip4 = (struct ip*) dummy_packet;
			tcp4 = (struct tcphdr*)
				(dummy_packet + sizeof(struct ip));
			ip4->ip_len = sizeof(struct ip) + sizeof(struct tcphdr);
			ip4->ip_hl = sizeof(struct ip) >> 2;
			ip4->ip_p = IPPROTO_TCP;

			// Cast to const so that it doesn't delete it.
			ip4_hdr = new IP_Hdr(ip4, false);
			}

		if ( is_orig )
			{
			Conn()->OrigAddr().CopyIPv4(&ip4->ip_src);
			Conn()->RespAddr().CopyIPv4(&ip4->ip_dst);
			tcp4->th_sport = htons(Conn()->OrigPort());
			tcp4->th_dport = htons(Conn()->RespPort());
			}
		else
			{
			Conn()->RespAddr().CopyIPv4(&ip4->ip_src);
			Conn()->OrigAddr().CopyIPv4(&ip4->ip_dst);
			tcp4->th_sport = htons(Conn()->RespPort());
			tcp4->th_dport = htons(Conn()->OrigPort());
			}

		ip = ip4_hdr;
		}

	if ( ! MatcherInitialized(is_orig) )
		DoMatch((const u_char*)"", 0, is_orig, true, false, false, ip);
	}

void PIA_TCP::DeliverStream(int len, const u_char* data, bool is_orig)
	{
	TCP_ApplicationAnalyzer::DeliverStream(len, data, is_orig);

	if ( stream_buffer.state == SKIPPING )
		return;

	stream_mode = true;

	State new_state = stream_buffer.state;

	if ( stream_buffer.state == INIT )
		{
		// FIXME: clear payload-matching state here...
		new_state = BUFFERING;
		}

	if ( stream_buffer.state == BUFFERING || new_state == BUFFERING )
		{
		AddToBuffer(&stream_buffer, len, data, is_orig);
		if ( stream_buffer.size > dpd_buffer_size )
			new_state = dpd_match_only_beginning ?
						SKIPPING : MATCHING_ONLY;
		}

	DoMatch(data, len, is_orig, false, false, false, 0);

	stream_buffer.state = new_state;
	}

void PIA_TCP::Undelivered(int seq, int len, bool is_orig)
	{
	TCP_ApplicationAnalyzer::Undelivered(seq, len, is_orig);

	if ( stream_buffer.state == BUFFERING )
		// We use data=nil to mark an undelivered.
		AddToBuffer(&stream_buffer, seq, len, 0, is_orig);

	// No check for buffer overrun here. I think that's ok.
	}

void PIA_TCP::ActivateAnalyzer(AnalyzerTag::Tag tag, const Rule* rule)
	{
	if ( stream_buffer.state == MATCHING_ONLY )
		{
		DBG_LOG(DBG_DPD, "analyzer found but buffer already exceeded");
		// FIXME: This is where to check whether an analyzer supports
		// partial connections once we get such.
		return;
		}

	if ( Parent()->HasChildAnalyzer(tag) )
		return;

	Analyzer* a = Parent()->AddChildAnalyzer(tag);
	a->SetSignature(rule);

	// We have two cases here:
	//
	// (a) We have already got stream input.
	//     => Great, somebody's already reassembling and we can just
	//		replay our stream buffer to the new analyzer.
	if ( stream_mode )
		{
		ReplayStreamBuffer(a);
		return;
		}

	// (b) We have only got packet input so far (or none at all).
	//     => We have to switch from packet-mode to stream-mode.
	//
	// Here's what we do:
	//
	//   (1) We create new TCP_Reassemblers and feed them the buffered
	//       packets.
	//
	//   (2) The reassembler will give us their results via the
	//       stream-interface and we buffer it as usual.
	//
	//   (3) We replay the now-filled stream-buffer to the analyzer.
	//
	//   (4) We hand the two reassemblers to the TCP Analyzer (our parent),
	//       turning reassembly now on for all subsequent data.

	DBG_LOG(DBG_DPD, "DPM_TCP switching from packet-mode to stream-mode");
	stream_mode = true;

	// FIXME: The reassembler will query the endpoint for state. Not sure
	// if this is works in all cases...

	if ( Parent()->GetTag() != AnalyzerTag::TCP )
		{
		// Our parent is not the TCP analyzer, which can only mean
		// we have been inserted somewhere further down in the
		// analyzer tree.  In this case, we will never have seen
		// any input at this point (because we don't get packets).
		assert(!pkt_buffer.head);
		assert(!stream_buffer.head);
		return;
		}

	TCP_Analyzer* tcp = (TCP_Analyzer*) Parent();

	TCP_Reassembler* reass_orig =
		new TCP_Reassembler(this, tcp, TCP_Reassembler::Direct,
					true, tcp->Orig());

	TCP_Reassembler* reass_resp =
		new TCP_Reassembler(this, tcp, TCP_Reassembler::Direct,
					false, tcp->Resp());

	int orig_seq = 0;
	int resp_seq = 0;

	for ( DataBlock* b = pkt_buffer.head; b; b = b->next )
		{
		if ( b->is_orig )
			reass_orig->DataSent(network_time, orig_seq = b->seq,
						b->len, b->data, true);
		else
			reass_resp->DataSent(network_time, resp_seq = b->seq,
						b->len, b->data, true);
		}

	// We also need to pass the current packet on.
	DataBlock* current = CurrentPacket();
	if ( current->data )
		{
		if ( current->is_orig )
			reass_orig->DataSent(network_time,
					orig_seq = current->seq,
					current->len, current->data, true);
		else
			reass_resp->DataSent(network_time,
					resp_seq = current->seq,
					current->len, current->data, true);
		}

	ClearBuffer(&pkt_buffer);

	ReplayStreamBuffer(a);
	reass_orig->AckReceived(orig_seq);
	reass_resp->AckReceived(resp_seq);

	reass_orig->SetType(TCP_Reassembler::Forward);
	reass_resp->SetType(TCP_Reassembler::Forward);

	tcp->SetReassembler(reass_orig, reass_resp);
	}

void PIA_TCP::DeactivateAnalyzer(AnalyzerTag::Tag tag)
	{
	reporter->InternalError("PIA_TCP::Deact not implemented yet");
	}

void PIA_TCP::ReplayStreamBuffer(Analyzer* analyzer)
	{
	DBG_LOG(DBG_DPD, "PIA_TCP replaying %d total stream bytes", stream_buffer.size);

	for ( DataBlock* b = stream_buffer.head; b; b = b->next )
		{
		if ( b->data )
			analyzer->NextStream(b->len, b->data, b->is_orig);
		else
			analyzer->NextUndelivered(b->seq, b->len, b->is_orig);
		}
	}

BOOST_CLASS_EXPORT_GUID(PIA_TCP,"PIA_TCP")
template<class Archive>
void PIA_TCP::serialize(Archive & ar, const unsigned int version)
    {
        SERIALIZE_PRINT("%s:PIA_TCP:%d",__FILE__,__LINE__);

        // Serialize PIA
        ar & boost::serialization::base_object<PIA>(*this);
        // Serialize TCP_ApplicationAnalyzer
        ar & boost::serialization::base_object<TCP_ApplicationAnalyzer>(*this);

        //ar & stream_buffer.head; //FIXME
        //ar & stream_buffer.tail; //FIXME
        SERIALIZE_PRINT("\t\tPIA_TCP:%d",__LINE__);
        ar & stream_buffer.size;
        ar & stream_buffer.state;
        ar & stream_mode;
        SERIALIZE_PRINT("\t\tPIA_TCP:Done");
    }
template void PIA_TCP::serialize<boost::archive::text_oarchive>(
        boost::archive::text_oarchive & ar, 
        const unsigned int file_version);
template void PIA_TCP::serialize<boost::archive::text_iarchive>(
        boost::archive::text_iarchive & ar, 
        const unsigned int file_version);
