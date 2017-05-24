#include <algorithm>

#include "FileAnalyzer.h"
#include "Reporter.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/export.hpp>

magic_t File_Analyzer::magic = 0;
magic_t File_Analyzer::magic_mime = 0;

File_Analyzer::File_Analyzer(Connection* conn)
: TCP_ApplicationAnalyzer(AnalyzerTag::File, conn)
	{
	buffer_len = 0;

	if ( ! magic )
		{
		InitMagic(&magic, MAGIC_NONE);
		InitMagic(&magic_mime, MAGIC_MIME);
		}
	}

void File_Analyzer::DeliverStream(int len, const u_char* data, bool orig)
	{
	TCP_ApplicationAnalyzer::DeliverStream(len, data, orig);

	int n = min(len, BUFFER_SIZE - buffer_len);

	if ( n )
		{
		strncpy(buffer + buffer_len, (const char*) data, n);
		buffer_len += n;

		if ( buffer_len == BUFFER_SIZE )
			Identify();
		}
	return;
	}

void File_Analyzer::Done()
	{
	TCP_ApplicationAnalyzer::Done();

	if ( buffer_len && buffer_len != BUFFER_SIZE )
		Identify();
	}

void File_Analyzer::Identify()
	{
	const char* descr = 0;
	const char* mime = 0;

	if ( magic )
		descr = magic_buffer(magic, buffer, buffer_len);

	if ( magic_mime )
		mime = magic_buffer(magic_mime, buffer, buffer_len);

	val_list* vl = new val_list;
	vl->append(BuildConnVal());
	vl->append(new StringVal(buffer_len, buffer));
	vl->append(new StringVal(descr ? descr : "<unknown>"));
	vl->append(new StringVal(mime ? mime : "<unknown>"));
	ConnectionEvent(file_transferred, vl);
	}

void File_Analyzer::InitMagic(magic_t* magic, int flags)
	{
	*magic = magic_open(flags);

	if ( ! *magic )
		reporter->Error("can't init libmagic: %s", magic_error(*magic));

	else if ( magic_load(*magic, 0) < 0 )
		{
		reporter->Error("can't load magic file: %s", magic_error(*magic));
		magic_close(*magic);
		*magic = 0;
		}
	}

BOOST_CLASS_EXPORT_GUID(File_Analyzer,"File_Analyzer")
template<class Archive>
void File_Analyzer::serialize(Archive & ar, const unsigned int version)
    {
        // Serialize TCP_ApplicationAnalyzer
        ar & boost::serialization::base_object<TCP_ApplicationAnalyzer>(*this);

        ar & buffer;
        ar & buffer_len;
    }
template void File_Analyzer::serialize<boost::archive::text_oarchive>(
        boost::archive::text_oarchive & ar, 
        const unsigned int file_version);
template void File_Analyzer::serialize<boost::archive::text_iarchive>(
        boost::archive::text_iarchive & ar, 
        const unsigned int file_version);
