// See the file "COPYING" in the main distribution directory for copyright.

#include "TunnelEncapsulation.h"
#include "util.h"
#include "Conn.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/vector.hpp>

EncapsulatingConn::EncapsulatingConn(Connection* c, BifEnum::Tunnel::Type t)
		: src_addr(c->OrigAddr()), dst_addr(c->RespAddr()),
		  src_port(c->OrigPort()), dst_port(c->RespPort()),
		  proto(c->ConnTransport()), type(t), uid(c->GetUID())
	{
	if ( ! uid )
		{
		uid = calculate_unique_id();
		c->SetUID(uid);
		}
	}

RecordVal* EncapsulatingConn::GetRecordVal() const
	{
	RecordVal *rv = new RecordVal(BifType::Record::Tunnel::EncapsulatingConn);

	RecordVal* id_val = new RecordVal(conn_id);
	id_val->Assign(0, new AddrVal(src_addr));
	id_val->Assign(1, new PortVal(ntohs(src_port), proto));
	id_val->Assign(2, new AddrVal(dst_addr));
	id_val->Assign(3, new PortVal(ntohs(dst_port), proto));
	rv->Assign(0, id_val);
	rv->Assign(1, new EnumVal(type, BifType::Enum::Tunnel::Type));

	char tmp[20];
	rv->Assign(2, new StringVal(uitoa_n(uid, tmp, sizeof(tmp), 62)));

	return rv;
	}

BOOST_CLASS_EXPORT_GUID(EncapsulatingConn,"EncapsulatingConn")
template<class Archive>
void EncapsulatingConn::serialize(Archive & ar, const unsigned int version)
    {
        ar & src_addr;
        ar & dst_addr;
        ar & src_port;
        ar & dst_port;
        ar & proto;
        ar & type;
        ar & uid;
    }
template void EncapsulatingConn::serialize<boost::archive::text_oarchive>(
        boost::archive::text_oarchive & ar, 
        const unsigned int file_version);
template void EncapsulatingConn::serialize<boost::archive::text_iarchive>(
        boost::archive::text_iarchive & ar, 
        const unsigned int file_version);

bool operator==(const EncapsulationStack& e1, const EncapsulationStack& e2)
	{
	if ( ! e1.conns )
		return e2.conns;

	if ( ! e2.conns )
		return false;

	if ( e1.conns->size() != e2.conns->size() )
		return false;

	for ( size_t i = 0; i < e1.conns->size(); ++i )
		{
		if ( (*e1.conns)[i] != (*e2.conns)[i] )
			return false;
		}

	return true;
	}

BOOST_CLASS_EXPORT_GUID(EncapsulationStack,"EncapsulationStack")
template<class Archive>
void EncapsulationStack::serialize(Archive & ar, const unsigned int version)
    {
        printf("%s:EncapsulationStack:%d\n",__FILE__,__LINE__);
//        ar & conns; //REINCLUDE
    }
template void EncapsulationStack::serialize<boost::archive::text_oarchive>(
        boost::archive::text_oarchive & ar, 
        const unsigned int file_version);
template void EncapsulationStack::serialize<boost::archive::text_iarchive>(
        boost::archive::text_iarchive & ar, 
        const unsigned int file_version);
