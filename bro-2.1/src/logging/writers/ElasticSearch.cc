// See the file "COPYING" in the main distribution directory for copyright.
//
// This is experimental code that is not yet ready for production usage.
//


#include "config.h"

#ifdef USE_ELASTICSEARCH

#include "util.h" // Needs to come first for stdint.h

#include <string>
#include <errno.h>

#include "BroString.h"
#include "NetVar.h"
#include "threading/SerialTypes.h"

#include <curl/curl.h>
#include <curl/easy.h>

#include "ElasticSearch.h"

using namespace logging;
using namespace writer;
using threading::Value;
using threading::Field;

ElasticSearch::ElasticSearch(WriterFrontend* frontend) : WriterBackend(frontend)
	{
	cluster_name_len = BifConst::LogElasticSearch::cluster_name->Len();
	cluster_name = new char[cluster_name_len + 1];
	memcpy(cluster_name, BifConst::LogElasticSearch::cluster_name->Bytes(), cluster_name_len);
	cluster_name[cluster_name_len] = 0;

	index_prefix = string((const char*) BifConst::LogElasticSearch::index_prefix->Bytes(), BifConst::LogElasticSearch::index_prefix->Len());

	es_server = string(Fmt("http://%s:%d", BifConst::LogElasticSearch::server_host->Bytes(),
	                                       (int) BifConst::LogElasticSearch::server_port));
	bulk_url = string(Fmt("%s/_bulk", es_server.c_str()));

	http_headers = curl_slist_append(NULL, "Content-Type: text/json; charset=utf-8");
	buffer.Clear();
	counter = 0;
	current_index = string();
	prev_index = string();
	last_send = current_time();
	failing = false;

	transfer_timeout = BifConst::LogElasticSearch::transfer_timeout * 1000;

	curl_handle = HTTPSetup();
}

ElasticSearch::~ElasticSearch()
	{
	delete [] cluster_name;
	}

bool ElasticSearch::DoInit(const WriterInfo& info, int num_fields, const threading::Field* const* fields)
	{
	return true;
	}

bool ElasticSearch::DoFlush(double network_time)
	{
	BatchIndex();
	return true;
	}

bool ElasticSearch::DoFinish(double network_time)
	{
	BatchIndex();
	curl_slist_free_all(http_headers);
	curl_easy_cleanup(curl_handle);
	return true;
	}

bool ElasticSearch::BatchIndex()
	{
	curl_easy_reset(curl_handle);
	curl_easy_setopt(curl_handle, CURLOPT_URL, bulk_url.c_str());
	curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)buffer.Len());
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, buffer.Bytes());
	failing = ! HTTPSend(curl_handle);

	// We are currently throwing the data out regardless of if the send failed.  Fire and forget!
	buffer.Clear();
	counter = 0;
	last_send = current_time();

	return true;
	}

bool ElasticSearch::AddValueToBuffer(ODesc* b, Value* val)
	{
	switch ( val->type )
		{
		// ES treats 0 as false and any other value as true so bool types go here.
		case TYPE_BOOL:
		case TYPE_INT:
			b->Add(val->val.int_val);
			break;

		case TYPE_COUNT:
		case TYPE_COUNTER:
			{
			// ElasticSearch doesn't seem to support unsigned 64bit ints.
			if ( val->val.uint_val >= INT64_MAX )
				{
				Error(Fmt("count value too large: %" PRIu64, val->val.uint_val));
				b->AddRaw("null", 4);
				}
			else
				b->Add(val->val.uint_val);
			break;
			}

		case TYPE_PORT:
			b->Add(val->val.port_val.port);
			break;

		case TYPE_SUBNET:
			b->AddRaw("\"", 1);
			b->Add(Render(val->val.subnet_val));
			b->AddRaw("\"", 1);
			break;

		case TYPE_ADDR:
			b->AddRaw("\"", 1);
			b->Add(Render(val->val.addr_val));
			b->AddRaw("\"", 1);
			break;

		case TYPE_DOUBLE:
		case TYPE_INTERVAL:
			b->Add(val->val.double_val);
			break;

		case TYPE_TIME:
			{
			// ElasticSearch uses milliseconds for timestamps and json only
			// supports signed ints (uints can be too large).
			uint64_t ts = (uint64_t) (val->val.double_val * 1000);
			if ( ts >= INT64_MAX )
				{
				Error(Fmt("time value too large: %" PRIu64, ts));
				b->AddRaw("null", 4);
				}
			else
				b->Add(ts);
			break;
			}

		case TYPE_ENUM:
		case TYPE_STRING:
		case TYPE_FILE:
		case TYPE_FUNC:
			{
			b->AddRaw("\"", 1);
			for ( int i = 0; i < val->val.string_val.length; ++i )
				{
				char c = val->val.string_val.data[i];
				// 2byte Unicode escape special characters.
				if ( c < 32 || c > 126 || c == '\n' || c == '"' || c == '\'' || c == '\\' || c == '&' )
					{
					static const char hex_chars[] = "0123456789abcdef";
					b->AddRaw("\\u00", 4);
					b->AddRaw(&hex_chars[(c & 0xf0) >> 4], 1);
					b->AddRaw(&hex_chars[c & 0x0f], 1);
					}
				else
					b->AddRaw(&c, 1);
				}
			b->AddRaw("\"", 1);
			break;
			}

		case TYPE_TABLE:
			{
			b->AddRaw("[", 1);
			for ( int j = 0; j < val->val.set_val.size; j++ )
				{
				if ( j > 0 )
					b->AddRaw(",", 1);
				AddValueToBuffer(b, val->val.set_val.vals[j]);
				}
			b->AddRaw("]", 1);
			break;
			}

		case TYPE_VECTOR:
			{
			b->AddRaw("[", 1);
			for ( int j = 0; j < val->val.vector_val.size; j++ )
				{
				if ( j > 0 )
					b->AddRaw(",", 1);
				AddValueToBuffer(b, val->val.vector_val.vals[j]);
				}
			b->AddRaw("]", 1);
			break;
			}

		default:
			return false;
		}
	return true;
	}

bool ElasticSearch::AddFieldToBuffer(ODesc *b, Value* val, const Field* field)
	{
	if ( ! val->present )
		return false;

	b->AddRaw("\"", 1);
	b->Add(field->name);
	b->AddRaw("\":", 2);
	AddValueToBuffer(b, val);
	return true;
	}

bool ElasticSearch::DoWrite(int num_fields, const Field* const * fields,
			     Value** vals)
	{
	if ( current_index.empty() )
		UpdateIndex(network_time, Info().rotation_interval, Info().rotation_base);

	// Our action line looks like:
	buffer.AddRaw("{\"index\":{\"_index\":\"", 20);
	buffer.Add(current_index);
	buffer.AddRaw("\",\"_type\":\"", 11);
	buffer.Add(Info().path);
	buffer.AddRaw("\"}}\n", 4);

	buffer.AddRaw("{", 1);
	for ( int i = 0; i < num_fields; i++ )
		{
		if ( i > 0 && buffer.Bytes()[buffer.Len()] != ',' && vals[i]->present )
			buffer.AddRaw(",", 1);
		AddFieldToBuffer(&buffer, vals[i], fields[i]);
		}
	buffer.AddRaw("}\n", 2);

	counter++;
	if ( counter >= BifConst::LogElasticSearch::max_batch_size ||
	     uint(buffer.Len()) >= BifConst::LogElasticSearch::max_byte_size )
		BatchIndex();

	return true;
	}

bool ElasticSearch::UpdateIndex(double now, double rinterval, double rbase)
	{
	if ( rinterval == 0 )
		{
		// if logs aren't being rotated, don't use a rotation oriented index name.
		current_index = index_prefix;
		}
	else
		{
		double nr = calc_next_rotate(now, rinterval, rbase);
		double interval_beginning = now - (rinterval - nr);

		struct tm tm;
		char buf[128];
		time_t teatime = (time_t)interval_beginning;
		localtime_r(&teatime, &tm);
		strftime(buf, sizeof(buf), "%Y%m%d%H%M", &tm);

		prev_index = current_index;
		current_index = index_prefix + "-" + buf;

		// Send some metadata about this index.
		buffer.AddRaw("{\"index\":{\"_index\":\"@", 21);
		buffer.Add(index_prefix);
		buffer.AddRaw("-meta\",\"_type\":\"index\",\"_id\":\"", 30);
		buffer.Add(current_index);
		buffer.AddRaw("-", 1);
		buffer.Add(Info().rotation_base);
		buffer.AddRaw("-", 1);
		buffer.Add(Info().rotation_interval);
		buffer.AddRaw("\"}}\n{\"name\":\"", 13);
		buffer.Add(current_index);
		buffer.AddRaw("\",\"start\":", 10);
		buffer.Add(interval_beginning);
		buffer.AddRaw(",\"end\":", 7);
		buffer.Add(interval_beginning+rinterval);
		buffer.AddRaw("}\n", 2);
		}

	//printf("%s - prev:%s current:%s\n", Info().path.c_str(), prev_index.c_str(), current_index.c_str());
	return true;
	}


bool ElasticSearch::DoRotate(const char* rotated_path, double open, double close, bool terminating)
	{
	// Update the currently used index to the new rotation interval.
	UpdateIndex(close, Info().rotation_interval, Info().rotation_base);

	// Only do this stuff if there was a previous index.
	if ( ! prev_index.empty() )
		{
		// FIXME: I think this section is taking too long and causing the thread to die.

		// Compress the previous index
		//curl_easy_reset(curl_handle);
		//curl_easy_setopt(curl_handle, CURLOPT_URL, Fmt("%s/%s/_settings", es_server.c_str(), prev_index.c_str()));
		//curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "PUT");
		//curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, "{\"index\":{\"store.compress.stored\":\"true\"}}");
		//curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t) 42);
		//HTTPSend(curl_handle);

		// Optimize the previous index.
		// TODO: make this into variables.
		//curl_easy_reset(curl_handle);
		//curl_easy_setopt(curl_handle, CURLOPT_URL, Fmt("%s/%s/_optimize?max_num_segments=1&wait_for_merge=false", es_server.c_str(), prev_index.c_str()));
		//HTTPSend(curl_handle);
		}

	if ( ! FinishedRotation(current_index.c_str(), prev_index.c_str(), open, close, terminating) )
		Error(Fmt("error rotating %s to %s", prev_index.c_str(), current_index.c_str()));

	return true;
	}

bool ElasticSearch::DoSetBuf(bool enabled)
	{
	// Nothing to do.
	return true;
	}

bool ElasticSearch::DoHeartbeat(double network_time, double current_time)
	{
	if ( last_send > 0 && buffer.Len() > 0 &&
	     current_time-last_send > BifConst::LogElasticSearch::max_batch_interval )
		{
		BatchIndex();
		}

	return true;
	}


CURL* ElasticSearch::HTTPSetup()
	{
	CURL* handle = curl_easy_init();
	if ( ! handle )
		{
		Error("cURL did not initialize correctly.");
		return 0;
		}

	return handle;
	}

size_t ElasticSearch::HTTPReceive(void* ptr, int size, int nmemb, void* userdata)
	{
	//TODO: Do some verification on the result?
	return size;
	}

bool ElasticSearch::HTTPSend(CURL *handle)
	{
	curl_easy_setopt(handle, CURLOPT_HTTPHEADER, http_headers);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &logging::writer::ElasticSearch::HTTPReceive); // This gets called with the result.
	// HTTP 1.1 likes to use chunked encoded transfers, which aren't good for speed.
	// The best (only?) way to disable that is to just use HTTP 1.0
	curl_easy_setopt(handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);

	// Some timeout options.  These will need more attention later.
	curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT_MS, transfer_timeout);
	curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, transfer_timeout*2);
	curl_easy_setopt(handle, CURLOPT_DNS_CACHE_TIMEOUT, 60*60);

	CURLcode return_code = curl_easy_perform(handle);

	switch ( return_code )
		{
		case CURLE_COULDNT_CONNECT:
		case CURLE_COULDNT_RESOLVE_HOST:
		case CURLE_WRITE_ERROR:
		case CURLE_RECV_ERROR:
			{
			if ( ! failing )
				Error(Fmt("ElasticSearch server may not be accessible."));

			break;
			}

		case CURLE_OPERATION_TIMEDOUT:
			{
			if ( ! failing )
				Warning(Fmt("HTTP operation with elasticsearch server timed out at %" PRIu64 " msecs.", transfer_timeout));

			break;
			}

		case CURLE_OK:
			{
			uint http_code = 0;
			curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
			if ( http_code == 200 )
				// Hopefully everything goes through here.
				return true;
			else if ( ! failing )
				Error(Fmt("Received a non-successful status code back from ElasticSearch server, check the elasticsearch server log."));

			break;
			}

		default:
			{
			break;
			}
		}
		// The "successful" return happens above
		return false;
	}

#endif
