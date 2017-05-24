#include <algorithm>

#include "Analyzer.h"
#include "PIA.h"
#include "Event.h"

#include "AYIYA.h"
#include "BackDoor.h"
#include "BitTorrent.h"
#include "BitTorrentTracker.h"
#include "Finger.h"
#include "InterConn.h"
#include "NTP.h"
#include "HTTP.h"
#include "HTTP-binpac.h"
#include "ICMP.h"
#include "SteppingStone.h"
#include "IRC.h"
#include "SMTP.h"
#include "FTP.h"
#include "FileAnalyzer.h"
#include "DNS.h"
#include "DNS-binpac.h"
#include "DHCP-binpac.h"
#include "Telnet.h"
#include "Rlogin.h"
#include "RSH.h"
#include "DCE_RPC.h"
#include "Gnutella.h"
#include "Ident.h"
#include "NCP.h"
#include "NetbiosSSN.h"
#include "SMB.h"
#include "NFS.h"
#include "Portmap.h"
#include "POP3.h"
#include "SOCKS.h"
#include "SSH.h"
#include "SSL.h"
#include "Syslog-binpac.h"
#include "Teredo.h"
#include "ConnSizeAnalyzer.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/list.hpp>
#include "SDMBNlocal.h"

// Keep same order here as in AnalyzerTag definition!
const Analyzer::Config Analyzer::analyzer_configs[] = {
	{ AnalyzerTag::Error, "<ERROR>", 0, 0, 0, false },

	{ AnalyzerTag::PIA_TCP, "PIA_TCP", PIA_TCP::InstantiateAnalyzer,
		PIA_TCP::Available, 0, false },
	{ AnalyzerTag::PIA_UDP, "PIA_UDP", PIA_UDP::InstantiateAnalyzer,
		PIA_UDP::Available, 0, false },

	{ AnalyzerTag::ICMP, "ICMP", ICMP_Analyzer::InstantiateAnalyzer,
		ICMP_Analyzer::Available, 0, false },

	{ AnalyzerTag::TCP, "TCP", TCP_Analyzer::InstantiateAnalyzer,
		TCP_Analyzer::Available, 0, false },
	{ AnalyzerTag::UDP, "UDP", UDP_Analyzer::InstantiateAnalyzer,
		UDP_Analyzer::Available, 0, false },

	{ AnalyzerTag::BitTorrent, "BITTORRENT",
		BitTorrent_Analyzer::InstantiateAnalyzer,
		BitTorrent_Analyzer::Available, 0, false },
	{ AnalyzerTag::BitTorrentTracker, "BITTORRENTTRACKER",
		BitTorrentTracker_Analyzer::InstantiateAnalyzer,
		BitTorrentTracker_Analyzer::Available, 0, false },
	{ AnalyzerTag::DCE_RPC, "DCE_RPC",
		DCE_RPC_Analyzer::InstantiateAnalyzer,
		DCE_RPC_Analyzer::Available, 0, false },
	{ AnalyzerTag::DNS, "DNS", DNS_Analyzer::InstantiateAnalyzer,
		DNS_Analyzer::Available, 0, false },
	{ AnalyzerTag::Finger, "FINGER", Finger_Analyzer::InstantiateAnalyzer,
		Finger_Analyzer::Available, 0, false },
	{ AnalyzerTag::FTP, "FTP", FTP_Analyzer::InstantiateAnalyzer,
		FTP_Analyzer::Available, 0, false },
	{ AnalyzerTag::Gnutella, "GNUTELLA",
		Gnutella_Analyzer::InstantiateAnalyzer,
		Gnutella_Analyzer::Available, 0, false },
	{ AnalyzerTag::HTTP, "HTTP", HTTP_Analyzer::InstantiateAnalyzer,
		HTTP_Analyzer::Available, 0, false },
	{ AnalyzerTag::Ident, "IDENT", Ident_Analyzer::InstantiateAnalyzer,
		Ident_Analyzer::Available, 0, false },
	{ AnalyzerTag::IRC, "IRC", IRC_Analyzer::InstantiateAnalyzer,
		IRC_Analyzer::Available, 0, false },
	{ AnalyzerTag::Login, "LOGIN", 0, 0, 0, false },  // just a base class
	{ AnalyzerTag::NCP, "NCP", NCP_Analyzer::InstantiateAnalyzer,
		NCP_Analyzer::Available, 0, false },
	{ AnalyzerTag::NetbiosSSN, "NetbiosSSN",
		NetbiosSSN_Analyzer::InstantiateAnalyzer,
		NetbiosSSN_Analyzer::Available, 0, false },
	{ AnalyzerTag::NFS, "NFS", NFS_Analyzer::InstantiateAnalyzer,
		NFS_Analyzer::Available, 0, false },
	{ AnalyzerTag::NTP, "NTP", NTP_Analyzer::InstantiateAnalyzer,
		NTP_Analyzer::Available, 0, false },
	{ AnalyzerTag::POP3, "POP3", POP3_Analyzer::InstantiateAnalyzer,
		POP3_Analyzer::Available, 0, false },
	{ AnalyzerTag::Portmapper, "PORTMAPPER",
		Portmapper_Analyzer::InstantiateAnalyzer,
		Portmapper_Analyzer::Available, 0, false },
	{ AnalyzerTag::Rlogin, "RLOGIN", Rlogin_Analyzer::InstantiateAnalyzer,
		Rlogin_Analyzer::Available, 0, false },
	{ AnalyzerTag::RPC, "RPC", 0, 0, 0, false },
	{ AnalyzerTag::Rsh, "RSH", Rsh_Analyzer::InstantiateAnalyzer,
		Rsh_Analyzer::Available, 0, false },
	{ AnalyzerTag::SMB, "SMB", SMB_Analyzer::InstantiateAnalyzer,
		SMB_Analyzer::Available, 0, false },
	{ AnalyzerTag::SMTP, "SMTP", SMTP_Analyzer::InstantiateAnalyzer,
		SMTP_Analyzer::Available, 0, false },
	{ AnalyzerTag::SSH, "SSH", SSH_Analyzer::InstantiateAnalyzer,
		SSH_Analyzer::Available, 0, false },
	{ AnalyzerTag::Telnet, "TELNET", Telnet_Analyzer::InstantiateAnalyzer,
		Telnet_Analyzer::Available, 0, false },

	{ AnalyzerTag::DHCP_BINPAC, "DHCP_BINPAC",
		DHCP_Analyzer_binpac::InstantiateAnalyzer,
		DHCP_Analyzer_binpac::Available, 0, false },
	{ AnalyzerTag::DNS_TCP_BINPAC, "DNS_TCP_BINPAC",
		DNS_TCP_Analyzer_binpac::InstantiateAnalyzer,
		DNS_TCP_Analyzer_binpac::Available, 0, false },
	{ AnalyzerTag::DNS_UDP_BINPAC, "DNS_UDP_BINPAC",
		DNS_UDP_Analyzer_binpac::InstantiateAnalyzer,
		DNS_UDP_Analyzer_binpac::Available, 0, false },
	{ AnalyzerTag::HTTP_BINPAC, "HTTP_BINPAC",
		HTTP_Analyzer_binpac::InstantiateAnalyzer,
		HTTP_Analyzer_binpac::Available, 0, false },
	{ AnalyzerTag::SSL, "SSL",
		SSL_Analyzer::InstantiateAnalyzer,
		SSL_Analyzer::Available, 0, false },
	{ AnalyzerTag::SYSLOG_BINPAC, "SYSLOG_BINPAC",
		Syslog_Analyzer_binpac::InstantiateAnalyzer,
		Syslog_Analyzer_binpac::Available, 0, false },

	{ AnalyzerTag::AYIYA, "AYIYA",
		AYIYA_Analyzer::InstantiateAnalyzer,
		AYIYA_Analyzer::Available, 0, false },
	{ AnalyzerTag::SOCKS, "SOCKS",
		SOCKS_Analyzer::InstantiateAnalyzer,
		SOCKS_Analyzer::Available, 0, false },
	{ AnalyzerTag::Teredo, "TEREDO",
		Teredo_Analyzer::InstantiateAnalyzer,
		Teredo_Analyzer::Available, 0, false },

	{ AnalyzerTag::File, "FILE", File_Analyzer::InstantiateAnalyzer,
		File_Analyzer::Available, 0, false },
	{ AnalyzerTag::Backdoor, "BACKDOOR",
		BackDoor_Analyzer::InstantiateAnalyzer,
		BackDoor_Analyzer::Available, 0, false },
	{ AnalyzerTag::InterConn, "INTERCONN",
		InterConn_Analyzer::InstantiateAnalyzer,
		InterConn_Analyzer::Available, 0, false },
	{ AnalyzerTag::SteppingStone, "STEPPINGSTONE",
		SteppingStone_Analyzer::InstantiateAnalyzer,
		SteppingStone_Analyzer::Available, 0, false },
	{ AnalyzerTag::TCPStats, "TCPSTATS",
		TCPStats_Analyzer::InstantiateAnalyzer,
		TCPStats_Analyzer::Available, 0, false },
	{ AnalyzerTag::ConnSize, "CONNSIZE",
		ConnSize_Analyzer::InstantiateAnalyzer,
		ConnSize_Analyzer::Available, 0, false },

	{ AnalyzerTag::Contents, "CONTENTS", 0, 0, 0, false },
	{ AnalyzerTag::ContentLine, "CONTENTLINE", 0, 0, 0, false },
	{ AnalyzerTag::NVT, "NVT", 0, 0, 0, false },
	{ AnalyzerTag::Zip, "ZIP", 0, 0, 0, false },
	{ AnalyzerTag::Contents_DNS, "CONTENTS_DNS", 0, 0, 0, false },
	{ AnalyzerTag::Contents_NetbiosSSN, "CONTENTS_NETBIOSSSN", 0, 0, 0, false },
	{ AnalyzerTag::Contents_NCP, "CONTENTS_NCP", 0, 0, 0, false },
	{ AnalyzerTag::Contents_Rlogin, "CONTENTS_Rlogin", 0, 0, 0, false },
	{ AnalyzerTag::Contents_Rsh, "CONTENTS_RSH", 0, 0, 0, false },
	{ AnalyzerTag::Contents_DCE_RPC, "CONTENTS_DCE_RPC", 0, 0, 0, false },
	{ AnalyzerTag::Contents_SMB, "CONTENTS_SMB", 0, 0, 0, false },
	{ AnalyzerTag::Contents_RPC, "CONTENTS_RPC", 0, 0, 0, false },
	{ AnalyzerTag::Contents_NFS, "CONTENTS_NFS", 0, 0, 0, false },
};

AnalyzerTimer::~AnalyzerTimer()
	{
	analyzer->RemoveTimer(this);
	Unref(analyzer->Conn());
	}

void AnalyzerTimer::Dispatch(double t, int is_expire)
	{
	if ( is_expire && ! do_expire )
		return;

	// Remove ourselves from the connection's set of timers so
	// it doesn't try to cancel us.
	analyzer->RemoveTimer(this);

	(analyzer->*timer)(t);
	}

void AnalyzerTimer::Init(Analyzer* arg_analyzer, analyzer_timer_func arg_timer,
				int arg_do_expire)
	{
	analyzer = arg_analyzer;
	timer = arg_timer;
	do_expire = arg_do_expire;

	// We need to Ref the connection as the analyzer doesn't do it and
	// we need to have it around until we expire.
	Ref(analyzer->Conn());
	}

BOOST_CLASS_EXPORT_GUID(AnalyzerTimer,"AnalyzerTimer")
template<class Archive>
void AnalyzerTimer::serialize(Archive & ar, const unsigned int version)
    {
        SERIALIZE_PRINT("%s:AnalyzerTimer:%d",__FILE__,__LINE__);
        // Serialize Timer
        ar & boost::serialization::base_object<Timer>(*this);

        ar & analyzer;
       
        //ar & timer; //FIXME

        ar & do_expire;
    }
template void AnalyzerTimer::serialize<boost::archive::text_oarchive>(
        boost::archive::text_oarchive & ar, 
        const unsigned int file_version);
template void AnalyzerTimer::serialize<boost::archive::text_iarchive>(
        boost::archive::text_iarchive & ar, 
        const unsigned int file_version);

AnalyzerID Analyzer::id_counter = 0;;

Analyzer* Analyzer::InstantiateAnalyzer(AnalyzerTag::Tag tag, Connection* c)
	{
	Analyzer* a = analyzer_configs[tag].factory(c);
	assert(a);
	return a;
	}

const char* Analyzer::GetTagName(AnalyzerTag::Tag tag)
	{
	return analyzer_configs[tag].name;
	}

AnalyzerTag::Tag Analyzer::GetTag(const char* name)
	{
	for ( int i = 1; i < int(AnalyzerTag::LastAnalyzer); i++ )
		if ( strcasecmp(analyzer_configs[i].name, name) == 0 )
			return analyzer_configs[i].tag;

	return AnalyzerTag::Error;
	}

// Used in debugging output.
static string fmt_analyzer(Analyzer* a)
	{
	return string(a->GetTagName()) + fmt("[%d]", a->GetID());
	}

Analyzer::Analyzer(AnalyzerTag::Tag arg_tag, Connection* arg_conn)
	{
	// Don't Ref conn here to avoid circular ref'ing. It can't be deleted
	// before us.
	conn = arg_conn;
	tag = arg_tag;
	id = ++id_counter;
	protocol_confirmed = false;
	skip = false;
	finished = false;
	removing = false;
	parent = 0;
	orig_supporters = 0;
	resp_supporters = 0;
	signature = 0;
	output_handler = 0;
    timers_canceled = false;
    moved = false;
	}

Analyzer::~Analyzer()
	{
	//assert(moved || finished); // SDMBN-FIXME

	LOOP_OVER_CHILDREN(i)
		delete *i;

	SupportAnalyzer* next = 0;

	for ( SupportAnalyzer* a = orig_supporters; a; a = next )
		{
		next = a->sibling;
		delete a;
		}

	for ( SupportAnalyzer* a = resp_supporters; a; a = next)
		{
		next = a->sibling;
		delete a;
		}

	delete output_handler;

    if (moved)
    { CancelTimers(); }
	}

void Analyzer::Init()
	{
	}

void Analyzer::InitChildren()
	{
	AppendNewChildren();

	LOOP_OVER_CHILDREN(i)
		{
		(*i)->Init();
		(*i)->InitChildren();
		}
	}

void Analyzer::Done()
	{
    if (moved)
    { return; }

	assert(!finished);

	if ( ! skip )
		{
		EndOfData(true);
		EndOfData(false);
		}

	CancelTimers();

	AppendNewChildren();

	LOOP_OVER_CHILDREN(i)
		if ( ! (*i)->finished )
			(*i)->Done();

	for ( SupportAnalyzer* a = orig_supporters; a; a = a->sibling )
		if ( ! a->finished )
			a->Done();

	for ( SupportAnalyzer* a = resp_supporters; a; a = a->sibling )
		if ( ! a->finished )
			a->Done();

	finished = true;
	}

void Analyzer::NextPacket(int len, const u_char* data, bool is_orig, int seq,
				const IP_Hdr* ip, int caplen)
	{
	if ( skip )
		return;

	// If we have support analyzers, we pass it to them.
	if ( is_orig && orig_supporters )
		orig_supporters->NextPacket(len, data, is_orig, seq, ip, caplen);
	else if ( ! is_orig && resp_supporters )
		resp_supporters->NextPacket(len, data, is_orig, seq, ip, caplen);
	else
		{
		try
			{
			DeliverPacket(len, data, is_orig, seq, ip, caplen);
			}
		catch ( binpac::Exception const &e )
			{
			Weird(e.c_msg());
			}
		}
	}

const char* Analyzer::GetTagName() const
	{
	return GetTagName(tag);
	}

void Analyzer::NextStream(int len, const u_char* data, bool is_orig)
	{
	if ( skip )
		return;

	// If we have support analyzers, we pass it to them.
	if ( is_orig && orig_supporters )
		orig_supporters->NextStream(len, data, is_orig);
	else if ( ! is_orig && resp_supporters )
		resp_supporters->NextStream(len, data, is_orig);
	else
		{
		try
			{
			DeliverStream(len, data, is_orig);
			}
		catch ( binpac::Exception const &e )
			{
			Weird(e.c_msg());
			}
		}
	}

void Analyzer::NextUndelivered(int seq, int len, bool is_orig)
	{
	if ( skip )
		return;

	// If we have support analyzers, we pass it to them.
	if ( is_orig && orig_supporters )
		orig_supporters->NextUndelivered(seq, len, is_orig);
	else if ( ! is_orig && resp_supporters )
		resp_supporters->NextUndelivered(seq, len, is_orig);
	else
		{
		try
			{
			Undelivered(seq, len, is_orig);
			}
		catch ( binpac::Exception const &e )
			{
			Weird(e.c_msg());
			}
		}
	}

void Analyzer::NextEndOfData(bool is_orig)
	{
	if ( skip )
		return;

	// If we have support analyzers, we pass it to them.
	if ( is_orig && orig_supporters )
		orig_supporters->NextEndOfData(is_orig);
	else if ( ! is_orig && resp_supporters )
		resp_supporters->NextEndOfData(is_orig);
	else
		EndOfData(is_orig);
	}

void Analyzer::ForwardPacket(int len, const u_char* data, bool is_orig,
				int seq, const IP_Hdr* ip, int caplen)
	{
	if ( output_handler )
		output_handler->DeliverPacket(len, data, is_orig, seq,
						ip, caplen);

	AppendNewChildren();

	// Pass to all children.
	analyzer_list::iterator next;
	for ( analyzer_list::iterator i = children.begin();
	      i != children.end(); i = next )
		{
		Analyzer* current = *i;
		next = ++i;

		if ( ! (current->finished || current->removing ) )
			current->NextPacket(len, data, is_orig, seq, ip, caplen);
		else
			DeleteChild(--i);
		}

	AppendNewChildren();
	}

void Analyzer::ForwardStream(int len, const u_char* data, bool is_orig)
	{
	if ( output_handler )
		output_handler->DeliverStream(len, data, is_orig);

	AppendNewChildren();

	analyzer_list::iterator next;
	for ( analyzer_list::iterator i = children.begin();
	      i != children.end(); i = next )
		{
		Analyzer* current = *i;
		next = ++i;

		if ( ! (current->finished || current->removing ) )
			current->NextStream(len, data, is_orig);
		else
			DeleteChild(--i);
		}

	AppendNewChildren();
	}

void Analyzer::ForwardUndelivered(int seq, int len, bool is_orig)
	{
	if ( output_handler )
		output_handler->Undelivered(seq, len, is_orig);

	AppendNewChildren();

	analyzer_list::iterator next;
	for ( analyzer_list::iterator i = children.begin();
	      i != children.end(); i = next )
		{
		Analyzer* current = *i;
		next = ++i;

		if ( ! (current->finished || current->removing ) )
			current->NextUndelivered(seq, len, is_orig);
		else
			DeleteChild(--i);
		}

	AppendNewChildren();
	}

void Analyzer::ForwardEndOfData(bool orig)
	{
	AppendNewChildren();

	analyzer_list::iterator next;
	for ( analyzer_list::iterator i = children.begin();
	      i != children.end(); i = next )
		{
		Analyzer* current = *i;
		next = ++i;

		if ( ! (current->finished || current->removing ) )
			current->NextEndOfData(orig);
		else
			DeleteChild(--i);
		}

	AppendNewChildren();
	}

void Analyzer::AddChildAnalyzer(Analyzer* analyzer, bool init)
	{
	if ( HasChildAnalyzer(analyzer->GetTag()) )
		{
		analyzer->Done();
		delete analyzer;
		return;
		}

	// We add new children to new_children first.  They are then
	// later copied to the "real" child list.  This is necessary
	// because this method may be called while somebody is iterating
	// over the children and we might confuse the caller by modifying
	// the list.

	analyzer->parent = this;
	children.push_back(analyzer);

	if ( init )
		analyzer->Init();

	DBG_LOG(DBG_DPD, "%s added child %s",
			fmt_analyzer(this).c_str(), fmt_analyzer(analyzer).c_str());
	}

Analyzer* Analyzer::AddChildAnalyzer(AnalyzerTag::Tag analyzer)
	{
	if ( ! HasChildAnalyzer(analyzer) )
		{
		Analyzer* a = InstantiateAnalyzer(analyzer, conn);
		AddChildAnalyzer(a);
		return a;
		}

	return 0;
	}

void Analyzer::RemoveChildAnalyzer(Analyzer* analyzer)
	{
	LOOP_OVER_CHILDREN(i)
		if ( *i == analyzer && ! (analyzer->finished || analyzer->removing) )
			{
			DBG_LOG(DBG_DPD, "%s disabling child %s",
					fmt_analyzer(this).c_str(), fmt_analyzer(*i).c_str());
			// We just flag it as being removed here but postpone
			// actually doing that to later. Otherwise, we'd need
			// to call Done() here, which then in turn might
			// cause further code to be executed that may assume
			// something not true because of a violation that
			// triggered the removal in the first place.
			(*i)->removing = true;
			return;
			}
	}

void Analyzer::RemoveChildAnalyzer(AnalyzerID id)
	{
	LOOP_OVER_CHILDREN(i)
		if ( (*i)->id == id && ! ((*i)->finished || (*i)->removing) )
			{
			DBG_LOG(DBG_DPD, "%s  disabling child %s", GetTagName(), id,
					fmt_analyzer(this).c_str(), fmt_analyzer(*i).c_str());
			// See comment above.
			(*i)->removing = true;
			return;
			}
	}

bool Analyzer::HasChildAnalyzer(AnalyzerTag::Tag tag)
	{
	LOOP_OVER_CHILDREN(i)
		if ( (*i)->tag == tag )
			return true;

	LOOP_OVER_GIVEN_CHILDREN(i, new_children)
		if ( (*i)->tag == tag )
			return true;

	return false;
	}

Analyzer* Analyzer::FindChild(AnalyzerID arg_id)
	{
	if ( id == arg_id )
		return this;

	LOOP_OVER_CHILDREN(i)
		{
		Analyzer* child = (*i)->FindChild(arg_id);
		if ( child )
			return child;
		}

	return 0;
	}

Analyzer* Analyzer::FindChild(AnalyzerTag::Tag arg_tag)
	{
	if ( tag == arg_tag )
		return this;

	LOOP_OVER_CHILDREN(i)
		{
		Analyzer* child = (*i)->FindChild(arg_tag);
		if ( child )
			return child;
		}

	return 0;
	}

void Analyzer::DeleteChild(analyzer_list::iterator i)
	{
	Analyzer* child = *i;

	// Analyzer must have already been finished or marked for removal.
	assert(child->finished || child->removing);

	if ( child->removing )
		{
		child->Done();
		child->removing = false;
		}

	DBG_LOG(DBG_DPD, "%s deleted child %s 3",
		fmt_analyzer(this).c_str(), fmt_analyzer(child).c_str());

	children.erase(i);
	delete child;
	}

void Analyzer::AddSupportAnalyzer(SupportAnalyzer* analyzer)
	{
	if ( HasSupportAnalyzer(analyzer->GetTag(), analyzer->IsOrig()) )
		{
		DBG_LOG(DBG_DPD, "%s already has %s %s",
			fmt_analyzer(this).c_str(),
			analyzer->IsOrig() ? "originator" : "responder",
			fmt_analyzer(analyzer).c_str());

		analyzer->Done();
		delete analyzer;
		return;
		}

	SupportAnalyzer** head =
		analyzer->IsOrig() ? &orig_supporters : &resp_supporters;

	// Find end of the list.
	SupportAnalyzer* prev = 0;
	SupportAnalyzer* s;
	for ( s = *head; s; prev = s, s = s->sibling )
		;

	if ( prev )
		prev->sibling = analyzer;
	else
		*head = analyzer;

	analyzer->parent = this;

	analyzer->Init();

	DBG_LOG(DBG_DPD, "%s added %s support %s",
			fmt_analyzer(this).c_str(),
			analyzer->IsOrig() ? "originator" : "responder",
			fmt_analyzer(analyzer).c_str());
	}

void Analyzer::RemoveSupportAnalyzer(SupportAnalyzer* analyzer)
	{
	SupportAnalyzer** head =
		analyzer->IsOrig() ? &orig_supporters : &resp_supporters;

	SupportAnalyzer* prev = 0;
	SupportAnalyzer* s;
	for ( s = *head; s && s != analyzer; prev = s, s = s->sibling )
		;

	if ( ! s )
		return;

	if ( prev )
		prev->sibling = s->sibling;
	else
		*head = s->sibling;

	DBG_LOG(DBG_DPD, "%s removed support %s",
			fmt_analyzer(this).c_str(),
			analyzer->IsOrig() ? "originator" : "responder",
			fmt_analyzer(analyzer).c_str());

	if ( ! analyzer->finished )
		analyzer->Done();

	delete analyzer;
	return;
	}

bool Analyzer::HasSupportAnalyzer(AnalyzerTag::Tag tag, bool orig)
	{
	SupportAnalyzer* s = orig ? orig_supporters : resp_supporters;
	for ( ; s; s = s->sibling )
		if ( s->tag == tag )
			return true;

	return false;
	}

void Analyzer::DeliverPacket(int len, const u_char* data, bool is_orig,
				int seq, const IP_Hdr* ip, int caplen)
	{
	DBG_LOG(DBG_DPD, "%s DeliverPacket(%d, %s, %d, %p, %d) [%s%s]",
			fmt_analyzer(this).c_str(), len, is_orig ? "T" : "F", seq, ip, caplen,
			fmt_bytes((const char*) data, min(40, len)), len > 40 ? "..." : "");
	}

void Analyzer::DeliverStream(int len, const u_char* data, bool is_orig)
	{
	DBG_LOG(DBG_DPD, "%s DeliverStream(%d, %s) [%s%s]",
			fmt_analyzer(this).c_str(), len, is_orig ? "T" : "F",
			fmt_bytes((const char*) data, min(40, len)), len > 40 ? "..." : "");
	}

void Analyzer::Undelivered(int seq, int len, bool is_orig)
	{
	DBG_LOG(DBG_DPD, "%s Undelivered(%d, %d, %s)",
			fmt_analyzer(this).c_str(), seq, len, is_orig ? "T" : "F");
	}

void Analyzer::EndOfData(bool is_orig)
	{
	DBG_LOG(DBG_DPD, "%s EndOfData(%s)",
			fmt_analyzer(this).c_str(), is_orig ? "T" : "F");
	}

void Analyzer::FlipRoles()
	{
	DBG_LOG(DBG_DPD, "%s FlipRoles()");

	LOOP_OVER_CHILDREN(i)
		(*i)->FlipRoles();

	LOOP_OVER_GIVEN_CHILDREN(i, new_children)
		(*i)->FlipRoles();

	for ( SupportAnalyzer* a = orig_supporters; a; a = a->sibling )
		a->FlipRoles();

	for ( SupportAnalyzer* a = resp_supporters; a; a = a->sibling )
		a->FlipRoles();

	SupportAnalyzer* tmp = orig_supporters;
	orig_supporters = resp_supporters;
	resp_supporters = tmp;
	}

void Analyzer::ProtocolConfirmation()
	{
	if ( protocol_confirmed )
		return;

	val_list* vl = new val_list;
	vl->append(BuildConnVal());
	vl->append(new Val(tag, TYPE_COUNT));
	vl->append(new Val(id, TYPE_COUNT));

	// We immediately raise the event so that the analyzer can quickly
	// react if necessary.
	::Event* e = new ::Event(protocol_confirmation, vl, SOURCE_LOCAL);
	mgr.Dispatch(e);

	protocol_confirmed = true;
	}

void Analyzer::ProtocolViolation(const char* reason, const char* data, int len)
	{
	StringVal* r;

	if ( data && len )
		{
		const char *tmp = copy_string(reason);
		r = new StringVal(fmt("%s [%s%s]", tmp,
					fmt_bytes(data, min(40, len)),
					len > 40 ? "..." : ""));
		delete [] tmp;
		}
	else
		r = new StringVal(reason);

	val_list* vl = new val_list;
	vl->append(BuildConnVal());
	vl->append(new Val(tag, TYPE_COUNT));
	vl->append(new Val(id, TYPE_COUNT));
	vl->append(r);

	// We immediately raise the event so that the analyzer can quickly be
	// disabled if necessary.
	::Event* e = new ::Event(protocol_violation, vl, SOURCE_LOCAL);
	mgr.Dispatch(e);
	}

void Analyzer::AddTimer(analyzer_timer_func timer, double t,
			int do_expire, TimerType type)
	{
	Timer* analyzer_timer = new
		AnalyzerTimer(this, timer, t, do_expire, type);

	Conn()->GetTimerMgr()->Add(analyzer_timer);
	timers.append(analyzer_timer);
	}

void Analyzer::RemoveTimer(Timer* t)
	{
	timers.remove(t);
	}

void Analyzer::CancelTimers()
	{
	// We are going to cancel our timers which, in turn, may cause them to
	// call RemoveTimer(), which would then modify the list we're just
	// traversing.  Thus, we first make a copy of the list which we then
	// iterate through.
	timer_list tmp(timers.length());
	loop_over_list(timers, j)
		tmp.append(timers[j]);

	loop_over_list(tmp, i)
		Conn()->GetTimerMgr()->Cancel(tmp[i]);

	timers_canceled = 1;
	timers.clear();
	}

void Analyzer::AppendNewChildren()
	{
	LOOP_OVER_GIVEN_CHILDREN(i, new_children)
		children.push_back(*i);
	new_children.clear();
	}

unsigned int Analyzer::MemoryAllocation() const
	{
	unsigned int mem = padded_sizeof(*this)
		+ (timers.MemoryAllocation() - padded_sizeof(timers));

	LOOP_OVER_CONST_CHILDREN(i)
		mem += (*i)->MemoryAllocation();

	for ( SupportAnalyzer* a = orig_supporters; a; a = a->sibling )
		mem += a->MemoryAllocation();

	for ( SupportAnalyzer* a = resp_supporters; a; a = a->sibling )
		mem += a->MemoryAllocation();

	return mem;
	}

void Analyzer::UpdateConnVal(RecordVal *conn_val)
	{
	LOOP_OVER_CHILDREN(i)
		(*i)->UpdateConnVal(conn_val);
	}

void Analyzer::Moved()
    {
    moved = true;
	
	for ( SupportAnalyzer* a = orig_supporters; a; a = a->sibling )
    { a->Moved(); }

	for ( SupportAnalyzer* a = resp_supporters; a; a = a->sibling )
    { a->Moved(); }

    for (std::list<Analyzer*>::iterator it = children.begin(); 
              it != children.end(); it++)
    { (*it)->Moved(); }
        
    for (std::list<Analyzer*>::iterator it = new_children.begin(); 
            it != new_children.end(); it++)
    { (*it)->Moved(); }
    }

BOOST_CLASS_EXPORT_GUID(Analyzer,"Analyzer")
template<class Archive>
void Analyzer::serialize(Archive & ar, const unsigned int version)
    {
        SERIALIZE_PRINT("%s:Analyzer:%d",__FILE__,__LINE__);

        ar & tag;
        ar & id;
        ar & conn; 

        // Special check for analyzer serializability
        if (!Archive::is_loading::value)
        {
            if (parent != NULL)
            { assert(sdmbn_can_serialize(parent->GetTag())); }
        }
        SERIALIZE_PRINT("\t\tAnalyzer:%d",__LINE__);
        ar & parent;
        //ar & signature; //Include? Does not seem to be used
        if (Archive::is_loading::value) { signature = NULL; }
        //ar & output_handler //Include? Only used in HTTP.cc:190
        if (Archive::is_loading::value) { output_handler = NULL; }
        
        // Special check for analyzer serializability
        if (!Archive::is_loading::value)
        {
            for (std::list<Analyzer*>::iterator it = children.begin(); 
                    it != children.end(); it++)
            { assert(sdmbn_can_serialize((*it)->GetTag())); }

            if (orig_supporters != NULL)
            { assert(sdmbn_can_serialize(orig_supporters->GetTag())); }
        
            if (resp_supporters != NULL)
            { assert(sdmbn_can_serialize(resp_supporters->GetTag())); }
        
            for (std::list<Analyzer*>::iterator it = new_children.begin(); 
                    it != new_children.end(); it++)
            { assert(sdmbn_can_serialize((*it)->GetTag())); }
        }
        ar & children;
        ar & orig_supporters;
        ar & resp_supporters;
        ar & new_children;

        SERIALIZE_PRINT("\t\tAnalyzer:%d",__LINE__);
        ar & protocol_confirmed;
        //ar & timers; //FIXME //Should include?
        ar & timers_canceled;
        ar & skip;
        ar & finished;
        ar & removing;
        SERIALIZE_PRINT("\t\tAnalyzer:Done");
    }
template void Analyzer::serialize<boost::archive::text_oarchive>(
        boost::archive::text_oarchive & ar,
        const unsigned int file_version);
template void Analyzer::serialize<boost::archive::text_iarchive>(
        boost::archive::text_iarchive & ar,
        const unsigned int file_version);

void SupportAnalyzer::ForwardPacket(int len, const u_char* data, bool is_orig,
					int seq, const IP_Hdr* ip, int caplen)
	{
	// We do not call parent's method, as we're replacing the functionality.
	if ( GetOutputHandler() )
		GetOutputHandler()->DeliverPacket(len, data, is_orig, seq,
							ip, caplen);
	else if ( sibling )
		// Pass to next in chain.
		sibling->NextPacket(len, data, is_orig, seq, ip, caplen);
	else
		// Finished with preprocessing - now it's the parent's turn.
		Parent()->DeliverPacket(len, data, is_orig, seq, ip, caplen);
	}

void SupportAnalyzer::ForwardStream(int len, const u_char* data, bool is_orig)
	{
	// We do not call parent's method, as we're replacing the functionality.
	if ( GetOutputHandler() )
		GetOutputHandler()->DeliverStream(len, data, is_orig);

	else if ( sibling )
		// Pass to next in chain.
		sibling->NextStream(len, data, is_orig);
	else
		// Finished with preprocessing - now it's the parent's turn.
		Parent()->DeliverStream(len, data, is_orig);
	}

void SupportAnalyzer::ForwardUndelivered(int seq, int len, bool is_orig)
	{
	// We do not call parent's method, as we're replacing the functionality.
	if ( GetOutputHandler() )
		GetOutputHandler()->Undelivered(seq, len, is_orig);

	else if ( sibling )
		// Pass to next in chain.
		sibling->NextUndelivered(seq, len, is_orig);
	else
		// Finished with preprocessing - now it's the parent's turn.
		Parent()->Undelivered(seq, len, is_orig);
	}

BOOST_CLASS_EXPORT_GUID(SupportAnalyzer,"SupportAnalyzer")
template<class Archive>
void SupportAnalyzer::serialize(Archive & ar, const unsigned int version)
    {
        SERIALIZE_PRINT("%s:SupportAnalyzer:%d",__FILE__,__LINE__);

        // Serialize Analyzer
        ar & boost::serialization::base_object<Analyzer>( *this );

        SERIALIZE_PRINT("\t\tSupportAnalyzer:%d",__LINE__);
        ar & orig;
        ar & sibling;
        SERIALIZE_PRINT("\t\tSupportAnalyzer:Done");
    }
template void SupportAnalyzer::serialize<boost::archive::text_oarchive>(
        boost::archive::text_oarchive & ar,
        const unsigned int file_version);
template void SupportAnalyzer::serialize<boost::archive::text_iarchive>(
        boost::archive::text_iarchive & ar,
        const unsigned int file_version);

void TransportLayerAnalyzer::Done()
	{
	Analyzer::Done();
	}

void TransportLayerAnalyzer::SetContentsFile(unsigned int /* direction */,
						BroFile* /* f */)
	{
	reporter->Error("analyzer type does not support writing to a contents file");
	}

BroFile* TransportLayerAnalyzer::GetContentsFile(unsigned int /* direction */) const
	{
	reporter->Error("analyzer type does not support writing to a contents file");
	return 0;
	}

void TransportLayerAnalyzer::PacketContents(const u_char* data, int len)
	{
	if ( packet_contents && len > 0 )
		{
		BroString* cbs = new BroString(data, len, 1);
		Val* contents = new StringVal(cbs);
		Event(packet_contents, contents);
		}
	}

BOOST_CLASS_EXPORT_GUID(TransportLayerAnalyzer,"TransportLayerAnalyzer")
template<class Archive>
void TransportLayerAnalyzer::serialize(Archive & ar, 
        const unsigned int version)
    {
        SERIALIZE_PRINT("%s:TransportLayerAnalyzer:%d",__FILE__,__LINE__);

        // Serialize Analyzer
        ar & boost::serialization::base_object<Analyzer>(*this);

        SERIALIZE_PRINT("\t\tTransportLayerAnalyzer:%d",__LINE__);
        ar & pia;
        SERIALIZE_PRINT("\t\tTransportLayerAnalyzer:Done");
    }
template void TransportLayerAnalyzer::serialize<boost::archive::text_oarchive>(
        boost::archive::text_oarchive & ar,
        const unsigned int file_version);
template void TransportLayerAnalyzer::serialize<boost::archive::text_iarchive>(
        boost::archive::text_iarchive & ar,
        const unsigned int file_version);
