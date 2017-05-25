#include "SSL.h"
#include "TCP_Reassembler.h"
#include "Reporter.h"
#include "util.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/export.hpp>

SSL_Analyzer::SSL_Analyzer(Connection* c)
: TCP_ApplicationAnalyzer(AnalyzerTag::SSL, c)
	{
	interp = new binpac::SSL::SSL_Conn(this);
	had_gap = false;
	}

SSL_Analyzer::~SSL_Analyzer()
	{
	delete interp;
	}

void SSL_Analyzer::Done()
	{
	TCP_ApplicationAnalyzer::Done();

	interp->FlowEOF(true);
	interp->FlowEOF(false);
	}

void SSL_Analyzer::EndpointEOF(bool is_orig)
	{
	TCP_ApplicationAnalyzer::EndpointEOF(is_orig);
	interp->FlowEOF(is_orig);
	}

void SSL_Analyzer::DeliverStream(int len, const u_char* data, bool orig)
	{
	TCP_ApplicationAnalyzer::DeliverStream(len, data, orig);

	assert(TCP());
	if ( TCP()->IsPartial() )
		return;

	if ( had_gap )
		// If only one side had a content gap, we could still try to
		// deliver data to the other side if the script layer can handle this.
		return;

	try
		{
		interp->NewData(orig, data, data + len);
		}
	catch ( const binpac::Exception& e )
		{
		ProtocolViolation(fmt("Binpac exception: %s", e.c_msg()));
		}
	}

void SSL_Analyzer::Undelivered(int seq, int len, bool orig)
	{
	TCP_ApplicationAnalyzer::Undelivered(seq, len, orig);
	had_gap = true;
	interp->NewGap(orig, len);
	}

BOOST_CLASS_EXPORT_GUID(SSL_Analyzer,"SSL_Analyzer")
template<class Archive>
void SSL_Analyzer::serialize(Archive & ar, const unsigned int version)
    {
        // Serialize TCP_ApplicationAnalyzer
        ar & boost::serialization::base_object<TCP_ApplicationAnalyzer>(*this);
        //ar & interp; //FIXME
        if (Archive::is_loading::value) { interp = new binpac::SSL::SSL_Conn(this); } //TMPHACK
        ar & had_gap;
    }
template void SSL_Analyzer::serialize<boost::archive::text_oarchive>(
        boost::archive::text_oarchive & ar, 
        const unsigned int file_version);
template void SSL_Analyzer::serialize<boost::archive::text_iarchive>(
        boost::archive::text_iarchive & ar, 
        const unsigned int file_version);
