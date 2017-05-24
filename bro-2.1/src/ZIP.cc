// See the file "COPYING" in the main distribution directory for copyright.

#include "ZIP.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/export.hpp>
#include "SDMBNlocal.h"

ZIP_Analyzer::ZIP_Analyzer(Connection* conn, bool orig, Method arg_method)
: TCP_SupportAnalyzer(AnalyzerTag::Zip, conn, orig)
	{
	zip = 0;
	zip_status = Z_OK;
	method = arg_method;

	zip = new z_stream;
	zip->zalloc = 0;
	zip->zfree = 0;
	zip->opaque = 0;
	zip->next_out = 0;
	zip->avail_out = 0;
	zip->next_in = 0;
	zip->avail_in = 0;

	// "15" here means maximum compression.  "32" is a gross overload
	// hack that means "check it for whether it's a gzip file".  Sheesh.
	zip_status = inflateInit2(zip, 15 + 32);
	if ( zip_status != Z_OK )
		{
		Weird("inflate_init_failed");
		delete zip;
		zip = 0;
		}
	}

ZIP_Analyzer::~ZIP_Analyzer()
	{
	delete zip;
	}

void ZIP_Analyzer::Done()
	{
	Analyzer::Done();

	if ( zip )
		inflateEnd(zip);
	}

void ZIP_Analyzer::DeliverStream(int len, const u_char* data, bool orig)
	{
	TCP_SupportAnalyzer::DeliverStream(len, data, orig);

	if ( ! len || zip_status != Z_OK )
		return;

	static unsigned int unzip_size = 4096;
	Bytef unzipbuf[unzip_size];

	zip->next_in = (Bytef*) data;
	zip->avail_in = len;

	do
		{
		zip->next_out = unzipbuf;
		zip->avail_out = unzip_size;

		zip_status = inflate(zip, Z_SYNC_FLUSH);

		if ( zip_status != Z_STREAM_END &&
		     zip_status != Z_OK &&
		     zip_status != Z_BUF_ERROR )
			{
			Weird("inflate_failed");
			inflateEnd(zip);
			break;
			}

		int have = unzip_size - zip->avail_out;
		if ( have )
			ForwardStream(have, unzipbuf, IsOrig());

		if ( zip_status == Z_STREAM_END )
			{
			inflateEnd(zip);
			delete zip;
			zip = 0;
			break;
			}

		zip_status = Z_OK;
		}
	while ( zip->avail_out == 0 );
	}

BOOST_CLASS_EXPORT_GUID(ZIP_Analyzer,"ZIP_Analyzer")
template<class Archive>
void ZIP_Analyzer::serialize(Archive & ar, const unsigned int version)
    {
        SERIALIZE_PRINT("%s:ZIP_Analzyer:%d\n",__FILE__,__LINE__);
        // Serialize TCP_SupportAnalyzer 
        ar & boost::serialization::base_object<TCP_SupportAnalyzer>(*this);

        //ar & zip; //FIXME
//        int zipnull;
//        if (!Archive::is_loading::value)
//        {
//            if (NULL == zip)
//            { zipnull = 1; }
//            else
//            { zipnull = 0; }
//        }
//        ar & zipnull;
//        if (!zipnull)
//        {
//            //ar & zip->next_in; //FIXME
//            ar & zip->avail_in;
//            ar & zip->total_in;
//            //ar & zip->next_out; //FIXME
//            ar & zip->avail_out;
//            ar & zip->total_out;
//            //ar & zip->msg; //FIXME
//            //ar & zip->state; //FIXME
//            //zalloc, zfree, and opaque are always null? //FIXME
//            ar & zip->data_type;
//            ar & zip->adler;
//            ar & zip->reserved;
//        }
//        else
//        { zip = NULL; }
//
        if (Archive::is_loading::value) { zip = NULL; } //TMPHACK
        ar & zip_status;
        if (Archive::is_loading::value) { zip_status = Z_BUF_ERROR; } //TMPHACK
        ar & method;
    }
template void ZIP_Analyzer::serialize<boost::archive::text_oarchive>(
        boost::archive::text_oarchive & ar, 
        const unsigned int file_version);
template void ZIP_Analyzer::serialize<boost::archive::text_iarchive>(
        boost::archive::text_iarchive & ar, 
        const unsigned int file_version);
