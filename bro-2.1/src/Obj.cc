// See the file "COPYING" in the main distribution directory for copyright.

#include "config.h"

#include <stdlib.h>

#include "Obj.h"
#include "Serializer.h"
#include "File.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/binary_object.hpp>
#include "SDMBNlocal.h"

Location no_location("<no location>", 0, 0, 0, 0);
Location start_location("<start uninitialized>", 0, 0, 0, 0);
Location end_location("<end uninitialized>", 0, 0, 0, 0);

bool Location::Serialize(SerialInfo* info) const
	{
	return SerialObj::Serialize(info);
	}

Location* Location::Unserialize(UnserialInfo* info)
	{
	return (Location*) SerialObj::Unserialize(info, SER_LOCATION);
	}

IMPLEMENT_SERIAL(Location, SER_LOCATION);

bool Location::DoSerialize(SerialInfo* info) const
	{
	DO_SERIALIZE(SER_LOCATION, SerialObj);
	info->s->WriteOpenTag("Location");
	SERIALIZE(filename);
	SERIALIZE(first_line);
	SERIALIZE(last_line);
	SERIALIZE(first_column);
	SERIALIZE(last_column);
	info->s->WriteCloseTag("Location");
	return true;
	}

bool Location::DoUnserialize(UnserialInfo* info)
	{
	DO_UNSERIALIZE(SerialObj);

	delete_data = true;

	return UNSERIALIZE_STR(&filename, 0)
		&& UNSERIALIZE(&first_line)
		&& UNSERIALIZE(&last_line)
		&& UNSERIALIZE(&first_column)
		&& UNSERIALIZE(&last_column);
	}

void Location::Describe(ODesc* d) const
	{
	if ( filename )
		{
		d->Add(filename);

		if ( first_line == 0 )
			return;

		d->AddSP(",");
		}

	if ( last_line != first_line )
		{
		d->Add("lines ");
		d->Add(first_line);
		d->Add("-");
		d->Add(last_line);
		}
	else
		{
		d->Add("line ");
		d->Add(first_line);
		}
	}

bool Location::operator==(const Location& l) const
	{
	if ( filename == l.filename ||
	     (filename && l.filename && streq(filename, l.filename)) )
		return first_line == l.first_line && last_line == l.last_line;
	else
		return false;
	}

BOOST_CLASS_EXPORT_GUID(Location,"Location")
template<class Archive>
void Location::serialize(Archive & ar, const unsigned int version)
    {
        SERIALIZE_PRINT("%s:Location:%d",__FILE__,__LINE__);
        // Serialize SerialObj?
        //ar & boost::serialization::base_object<>(*this);
        
        // Special handling of filename & delete_data
        int filename_len;
        if (!Archive::is_loading::value)
        { filename_len = strlen(filename)+1; }
        ar & filename_len;
        if (!Archive::is_loading::value)
        { 
            ar & boost::serialization::make_binary_object((void *)filename, 
                    filename_len); 
        }
        if (Archive::is_loading::value)
        {
            filename = new char[filename_len];
            ar & boost::serialization::make_binary_object(
                    const_cast<char* &>(filename), filename_len);
            delete_data = true;
        }

        SERIALIZE_PRINT("\t\tLocation:%d",__LINE__);
        ar & first_line;
        ar & last_line;
        ar & first_column;
        ar & last_column;
        ar & timestamp;

        // Special handling of text 
        int text_len;
        if (!Archive::is_loading::value)
        {
            if (text)
            { text_len = strlen(text); }
            else
            { text_len = 0; }
        }
        ar & text_len;
        if (!Archive::is_loading::value)
        { 
            if (text_len > 0)
            { ar & boost::serialization::make_binary_object(text, text_len); }
        }
        if (Archive::is_loading::value)
        {
            if (text_len > 0)
            {
                text = new char[text_len];
                ar & boost::serialization::make_binary_object(text, text_len);
            }
            else
            { text = NULL; }
        }

        SERIALIZE_PRINT("\t\tLocation:Done");
    }
template void Location::serialize<boost::archive::text_oarchive>(
        boost::archive::text_oarchive & ar, 
        const unsigned int file_version);
template void Location::serialize<boost::archive::text_iarchive>(
        boost::archive::text_iarchive & ar, 
        const unsigned int file_version);

int BroObj::suppress_errors = 0;

BroObj::~BroObj()
	{
	delete location;
	}

void BroObj::Warn(const char* msg, const BroObj* obj2, int pinpoint_only) const
	{
	ODesc d;
	DoMsg(&d, msg, obj2, pinpoint_only);
	reporter->Warning("%s", d.Description());
	reporter->PopLocation();
	}

void BroObj::Error(const char* msg, const BroObj* obj2, int pinpoint_only) const
	{
	if ( suppress_errors )
		return;

	ODesc d;
	DoMsg(&d, msg, obj2, pinpoint_only);
	reporter->Error("%s", d.Description());
	reporter->PopLocation();
	}

void BroObj::BadTag(const char* msg, const char* t1, const char* t2) const
	{
	char out[512];

	if ( t2 )
		snprintf(out, sizeof(out), "%s (%s/%s)", msg, t1, t2);
	else if ( t1 )
		snprintf(out, sizeof(out), "%s (%s)", msg, t1);
	else
		snprintf(out, sizeof(out), "%s", msg);

	ODesc d;
	DoMsg(&d, out);
	reporter->FatalError("%s", d.Description());
	reporter->PopLocation();
	}

void BroObj::Internal(const char* msg) const
	{
	ODesc d;
	DoMsg(&d, msg);
	reporter->InternalError("%s", d.Description());
	reporter->PopLocation();
	}

void BroObj::InternalWarning(const char* msg) const
	{
	ODesc d;
	DoMsg(&d, msg);
	reporter->InternalWarning("%s", d.Description());
	reporter->PopLocation();
	}

void BroObj::AddLocation(ODesc* d) const
	{
	if ( ! location )
		{
		d->Add("<no location>");
		return;
		}

	location->Describe(d);
	}

bool BroObj::SetLocationInfo(const Location* start, const Location* end)
	{
	if ( ! start || ! end )
		return false;

	if ( end->filename && ! streq(start->filename, end->filename) )
		return false;

	if ( location && (start == &no_location || end == &no_location) )
		// We already have a better location, so don't use this one.
		return true;

	delete location;

	location = new Location(start->filename,
				start->first_line, end->last_line,
				start->first_column, end->last_column);

	return true;
	}

void BroObj::UpdateLocationEndInfo(const Location& end)
	{
	if ( ! location )
		SetLocationInfo(&end, &end);

	location->last_line = end.last_line;
	location->last_column = end.last_column;
	}

void BroObj::DoMsg(ODesc* d, const char s1[], const BroObj* obj2,
			int pinpoint_only) const
	{
	d->SetShort();

	d->Add(s1);
	PinPoint(d, obj2, pinpoint_only);

	const Location* loc2 = 0;
	if ( obj2 && obj2->GetLocationInfo() != &no_location &&
		 *obj2->GetLocationInfo() != *GetLocationInfo() )
		loc2 = obj2->GetLocationInfo();

	reporter->PushLocation(GetLocationInfo(), loc2);
	}

void BroObj::PinPoint(ODesc* d, const BroObj* obj2, int pinpoint_only) const
	{
	d->Add(" (");
	Describe(d);
	if ( obj2 && ! pinpoint_only )
		{
		d->Add(" and ");
		obj2->Describe(d);
		}

	d->Add(")");
	}

bool BroObj::DoSerialize(SerialInfo* info) const
	{
	DO_SERIALIZE(SER_BRO_OBJ, SerialObj);

	info->s->WriteOpenTag("Object");

	Location* loc = info->include_locations ? location : 0;
	SERIALIZE_OPTIONAL(loc);
	info->s->WriteCloseTag("Object");

	return true;
	}

bool BroObj::DoUnserialize(UnserialInfo* info)
	{
	DO_UNSERIALIZE(SerialObj);

	delete location;

	UNSERIALIZE_OPTIONAL(location, Location::Unserialize(info));
	return true;
	}

BOOST_CLASS_EXPORT_GUID(BroObj,"BroObj")
template<class Archive>
void BroObj::serialize(Archive & ar, const unsigned int version)
    {
        SERIALIZE_PRINT("%s:BroObj:%d",__FILE__,__LINE__);
        // Serialize SerialObj?
        //ar & boost::serialization::base_object<>(*this);
    
        // Mark object as not in serialization cache
        if (Archive::is_loading::value) { in_ser_cache = false; }

        ar & location;
        ar & ref_cnt;
        SERIALIZE_PRINT("\t\t:BroObj:Done");
    }
template void BroObj::serialize<boost::archive::text_oarchive>(
        boost::archive::text_oarchive & ar, 
        const unsigned int file_version);
template void BroObj::serialize<boost::archive::text_iarchive>(
        boost::archive::text_iarchive & ar, 
        const unsigned int file_version);

void print(const BroObj* obj)
	{
	static BroFile fstderr(stderr);
	ODesc d(DESC_READABLE, &fstderr);
	obj->Describe(&d);
	d.Add("\n");
	}

void bad_ref(int type)
	{
	reporter->InternalError("bad reference count [%d]", type);
	abort();
	}

void bro_obj_delete_func(void* v)
	{
	Unref((BroObj*) v);
	}
