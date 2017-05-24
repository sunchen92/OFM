// See the file "COPYING" in the main distribution directory for copyright.

// The hash function works as follows:
//
// 1) For short data we have a number of universal hash functions:
// UHASH_CW (ax + b (mod p)), H3, Dietzfelbinger and UMAC_NH (UMAC_NH is
// not as strongly universal as the others, but probably enough). All
// these functions require number of random bits linear to the data
// length. And we use them for data no longer than UHASH_KEY_SIZE.
// They are faster than HMAC/MD5 used for longer data, and most hash
// operations are on short data.
//
// 2) As a fall-back, we use HMAC/MD5 (keyed MD5) for data of arbitrary
// length. MD5 is used as a scrambling scheme so that it is difficult
// for the adversary to construct conflicts, though I do not know if
// HMAC/MD5 is provably universal.

#include "config.h"

#include "Hash.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/binary_object.hpp>

#include "H3.h"
const H3<hash_t, UHASH_KEY_SIZE>* h3;

void init_hash_function()
	{
	// Make sure we have already called init_random_seed().
	ASSERT(hmac_key_set);
	h3 = new H3<hash_t, UHASH_KEY_SIZE>();
	}

HashKey::HashKey(bro_int_t i)
	{
	key_u.i = i;
	key = (void*) &key_u;
	size = sizeof(i);
	hash = HashBytes(key, size);
	is_our_dynamic = 0;
    type = HASH_KEY_TYPE_INT;
	}

HashKey::HashKey(bro_uint_t u)
	{
	key_u.i = bro_int_t(u);
	key = (void*) &key_u;
	size = sizeof(u);
	hash = HashBytes(key, size);
	is_our_dynamic = 0;
    type = HASH_KEY_TYPE_INT;
	}

HashKey::HashKey(uint32 u)
	{
	key_u.u32 = u;
	key = (void*) &key_u;
	size = sizeof(u);
	hash = HashBytes(key, size);
	is_our_dynamic = 0;
    type = HASH_KEY_TYPE_UINT;
	}

HashKey::HashKey(const uint32 u[], int n)
	{
	size = n * sizeof(u[0]);
	key = (void*) u;
	hash = HashBytes(key, size);
	is_our_dynamic = 0;
    type = HASH_KEY_TYPE_NONE;
	}

HashKey::HashKey(double d)
	{
	union {
		double d;
		int i[2];
	} u;

	key_u.d = u.d = d;
	key = (void*) &key_u;
	size = sizeof(d);
	hash = HashBytes(key, size);
	is_our_dynamic = 0;
    type = HASH_KEY_TYPE_DOUBLE;
	}

HashKey::HashKey(const void* p)
	{
	key_u.p = p;
	key = (void*) &key_u;
	size = sizeof(p);
	hash = HashBytes(key, size);
	is_our_dynamic = 0;
    type = HASH_KEY_TYPE_PTR;
	}

HashKey::HashKey(const char* s)
	{
	size = strlen(s);	// note - skip final \0
	key = (void*) s;
	hash = HashBytes(key, size);
	is_our_dynamic = 0;
    type = HASH_KEY_TYPE_NONE;
	}

HashKey::HashKey(const BroString* s)
	{
	size = s->Len();
	key = (void*) s->Bytes();
	hash = HashBytes(key, size);
	is_our_dynamic = 0;
    type = HASH_KEY_TYPE_NONE;
	}

HashKey::HashKey(int copy_key, void* arg_key, int arg_size)
	{
	size = arg_size;
	is_our_dynamic = 1;

	if ( copy_key )
		{
		key = (void*) new char[size];
		memcpy(key, arg_key, size);
		}
	else
		key = arg_key;

	hash = HashBytes(key, size);
    type = HASH_KEY_TYPE_NONE;
	}

HashKey::HashKey(const void* arg_key, int arg_size, hash_t arg_hash)
	{
	size = arg_size;
	hash = arg_hash;
	key = CopyKey(arg_key, size);
	is_our_dynamic = 1;
    type = HASH_KEY_TYPE_NONE;
	}

HashKey::HashKey(const void* arg_key, int arg_size, hash_t arg_hash,
		bool /* dont_copy */)
	{
	size = arg_size;
	hash = arg_hash;
	key = const_cast<void*>(arg_key);
	is_our_dynamic = 0;
    type = HASH_KEY_TYPE_NONE;
	}

HashKey::HashKey(const void* bytes, int arg_size)
	{
	size = arg_size;
	key = CopyKey(bytes, size);
	hash = HashBytes(key, size);
	is_our_dynamic = 1;
    type = HASH_KEY_TYPE_NONE;
	}

void* HashKey::TakeKey()
	{
	if ( is_our_dynamic )
		{
		is_our_dynamic = 0;
		return key;
		}
	else
		return CopyKey(key, size);
	}

void* HashKey::CopyKey(const void* k, int s) const
	{
	void* k_copy = (void*) new char[s];
	memcpy(k_copy, k, s);
	return k_copy;
	}

hash_t HashKey::HashBytes(const void* bytes, int size)
	{
	if ( size <= UHASH_KEY_SIZE )
		{
		// H3 doesn't check if size is zero
		return ( size == 0 ) ? 0 : (*h3)(bytes, size);
		}

	// Fall back to HMAC/MD5 for longer data (which is usually rare).
	hash_t digest[16];
	hmac_md5(size, (const unsigned char*) bytes, (unsigned char*) digest);
	return digest[0];
	}

BOOST_CLASS_EXPORT_GUID(HashKey,"HashKey")
template<class Archive>
void HashKey::serialize(Archive & ar, const unsigned int version)
    {
        ar & size;
        ar & type;

        // Special handling of void * pointer to key
        if (!Archive::is_loading::value)
        {
            switch(type)
            {
            case HASH_KEY_TYPE_INT:
                ar & key_u.i;
                break;
            case HASH_KEY_TYPE_UINT:
                ar & key_u.u32;
                break;
            case HASH_KEY_TYPE_DOUBLE:
                ar & key_u.d;
                break;
            case HASH_KEY_TYPE_PTR:
            case HASH_KEY_TYPE_NONE:
                char *ptr = (char *)key;
                ar & boost::serialization::make_binary_object(ptr, size);
                break;
            }
        }
        if (Archive::is_loading::value)
        {
            switch(type)
            {
            case HASH_KEY_TYPE_INT:
                ar & key_u.i;
                key = (void*) &key_u;
                break;
            case HASH_KEY_TYPE_UINT:
                ar & key_u.u32;
                key = (void*) &key_u;
                break;
            case HASH_KEY_TYPE_DOUBLE:
                ar & key_u.d;
                key = (void*) &key_u;
                break;
            case HASH_KEY_TYPE_PTR:
            case HASH_KEY_TYPE_NONE:
                char *ptr = new char[size];
                is_our_dynamic = true; // Force deallocation when destroyed
                ar & boost::serialization::make_binary_object(ptr, size);
                if (HASH_KEY_TYPE_PTR == type)
                { 
                    key_u.p = (void *)ptr; 
                    key = (void*) &key_u;
                }
                else if (HASH_KEY_TYPE_NONE == type)
                { key = (void *)ptr; }
                break;
            }
        }
        
        ar & hash;
    }
template void HashKey::serialize<boost::archive::text_oarchive>(
        boost::archive::text_oarchive & ar, 
        const unsigned int file_version);
template void HashKey::serialize<boost::archive::text_iarchive>(
        boost::archive::text_iarchive & ar, 
        const unsigned int file_version);
