#include "SOCKS.h"
#include "socks_pac.h"
#include "TCP_Reassembler.h"

SOCKS_Analyzer::SOCKS_Analyzer(Connection* conn)
: TCP_ApplicationAnalyzer(AnalyzerTag::SOCKS, conn)
	{
	interp = new binpac::SOCKS::SOCKS_Conn(this);
	orig_done = resp_done = false;
	pia = 0;
	}

SOCKS_Analyzer::~SOCKS_Analyzer()
	{
	delete interp;
	}

void SOCKS_Analyzer::EndpointDone(bool orig)
	{
	if ( orig )
		orig_done = true;
	else
		resp_done = true;
	}

void SOCKS_Analyzer::Done()
	{
	TCP_ApplicationAnalyzer::Done();

	interp->FlowEOF(true);
	interp->FlowEOF(false);
	}

void SOCKS_Analyzer::EndpointEOF(bool is_orig)
	{
	TCP_ApplicationAnalyzer::EndpointEOF(is_orig);
	interp->FlowEOF(is_orig);
	}

void SOCKS_Analyzer::DeliverStream(int len, const u_char* data, bool orig)
	{
	TCP_ApplicationAnalyzer::DeliverStream(len, data, orig);

	assert(TCP());

	if ( TCP()->IsPartial() )
		// punt on partial.
		return;

	if ( orig_done && resp_done )
		{
		// Finished decapsulating tunnel layer. Now do standard processing
		// with the rest of the conneciton.
		//
		// Note that we assume that no payload data arrives before both endpoints
		// are done with there part of the SOCKS protocol.

		if ( ! pia )
			{
			pia = new PIA_TCP(Conn());
			AddChildAnalyzer(pia);
			pia->FirstPacket(true, 0);
			pia->FirstPacket(false, 0);
			}

		ForwardStream(len, data, orig);
		}
	else
		{
		try
			{
			interp->NewData(orig, data, data + len);
			}
		catch ( const binpac::Exception& e )
			{
			ProtocolViolation(fmt("Binpac exception: %s", e.c_msg()));
			}
		}
	}

void SOCKS_Analyzer::Undelivered(int seq, int len, bool orig)
	{
	TCP_ApplicationAnalyzer::Undelivered(seq, len, orig);
	interp->NewGap(orig, len);
	}

