// See the file "COPYING" in the main distribution directory for copyright.

#ifndef type_h
#define type_h

#include <string>
#include <list>
#include <map>

#include "Obj.h"
#include "Attr.h"
#include "BroList.h"
#include "Dict.h"
#include <boost/serialization/access.hpp>

// BRO types.

typedef enum {
	TYPE_VOID,
	TYPE_BOOL, TYPE_INT, TYPE_COUNT, TYPE_COUNTER, TYPE_DOUBLE,
	TYPE_TIME, TYPE_INTERVAL,
	TYPE_STRING, TYPE_PATTERN,
	TYPE_ENUM,
	TYPE_TIMER,
	TYPE_PORT, TYPE_ADDR, TYPE_SUBNET,
	TYPE_ANY,
	TYPE_TABLE,
	TYPE_UNION,
	TYPE_RECORD,
	TYPE_LIST,
	TYPE_FUNC,
	TYPE_FILE,
	TYPE_VECTOR,
	TYPE_TYPE,
	TYPE_ERROR
#define NUM_TYPES (int(TYPE_ERROR) + 1)
} TypeTag;

typedef enum { FUNC_FLAVOR_FUNCTION, FUNC_FLAVOR_EVENT } function_flavor;

typedef enum {
	TYPE_INTERNAL_VOID,
	TYPE_INTERNAL_INT, TYPE_INTERNAL_UNSIGNED, TYPE_INTERNAL_DOUBLE,
	TYPE_INTERNAL_STRING, TYPE_INTERNAL_ADDR, TYPE_INTERNAL_SUBNET,
	TYPE_INTERNAL_OTHER, TYPE_INTERNAL_ERROR
} InternalTypeTag;

// Returns the name of the type.
extern const char* type_name(TypeTag t);

class Expr;
class Attributes;
class TypeList;
class TableType;
class SetType;
class RecordType;
class SubNetType;
class FuncType;
class ListExpr;
class EnumType;
class Serializer;
class VectorType;
class TypeType;

const int DOES_NOT_MATCH_INDEX = 0;
const int MATCHES_INDEX_SCALAR = 1;
const int MATCHES_INDEX_VECTOR = 2;

class BroType : public BroObj {
public:
	BroType(TypeTag tag, bool base_type = false);
	~BroType();

	TypeTag Tag() const		{ return tag; }
	InternalTypeTag InternalType() const	{ return internal_tag; }

	// Whether it's stored in network order.
	int IsNetworkOrder() const	{ return is_network_order; }

	// Type-checks the given expression list, returning
	// MATCHES_INDEX_SCALAR = 1 if it matches this type's index
	// and produces a scalar result (and promoting its
	// subexpressions as necessary); MATCHES_INDEX_VECTOR = 2
	// if it matches and produces a vector result; and
	// DOES_NOT_MATCH_INDEX = 0 if it can't match (or the type
	// is not an indexable type).
	virtual int MatchesIndex(ListExpr*& index) const;

	// Returns the type yielded by this type.  For example, if
	// this type is a table[string] of port, then returns the "port"
	// type.  Returns nil if this is not an index type.
	virtual BroType* YieldType();
	const BroType* YieldType() const
		{ return ((BroType*) this)->YieldType(); }

	// Returns true if this type is a record and contains the
	// given field, false otherwise.
	virtual int HasField(const char* field) const;

	// Returns the type of the given field, or nil if no such field.
	virtual BroType* FieldType(const char* field) const;

#define CHECK_TYPE_TAG(tag_type, func_name) \
	CHECK_TAG(tag, tag_type, func_name, type_name)

	const TypeList* AsTypeList() const
		{
		CHECK_TYPE_TAG(TYPE_LIST, "BroType::AsTypeList");
		return (const TypeList*) this;
		}
	TypeList* AsTypeList()
		{
		CHECK_TYPE_TAG(TYPE_LIST, "BroType::AsTypeList");
		return (TypeList*) this;
		}

	const TableType* AsTableType() const
		{
		CHECK_TYPE_TAG(TYPE_TABLE, "BroType::AsTableType");
		return (const TableType*) this;
		}
	TableType* AsTableType()
		{
		CHECK_TYPE_TAG(TYPE_TABLE, "BroType::AsTableType");
		return (TableType*) this;
		}

	SetType* AsSetType()
		{
		if ( ! IsSet() )
			BadTag("BroType::AsSetType", type_name(tag));
		return (SetType*) this;
		}
	const SetType* AsSetType() const
		{
		if ( ! IsSet() )
			BadTag("BroType::AsSetType", type_name(tag));
		return (const SetType*) this;
		}

	const RecordType* AsRecordType() const
		{
		CHECK_TYPE_TAG(TYPE_RECORD, "BroType::AsRecordType");
		return (const RecordType*) this;
		}
	RecordType* AsRecordType()
		{
		CHECK_TYPE_TAG(TYPE_RECORD, "BroType::AsRecordType");
		return (RecordType*) this;
		}

	const SubNetType* AsSubNetType() const
		{
		CHECK_TYPE_TAG(TYPE_SUBNET, "BroType::AsSubNetType");
		return (const SubNetType*) this;
		}

	SubNetType* AsSubNetType()
		{
		CHECK_TYPE_TAG(TYPE_SUBNET, "BroType::AsSubNetType");
		return (SubNetType*) this;
		}

	const FuncType* AsFuncType() const
		{
		CHECK_TYPE_TAG(TYPE_FUNC, "BroType::AsFuncType");
		return (const FuncType*) this;
		}
	FuncType* AsFuncType()
		{
		CHECK_TYPE_TAG(TYPE_FUNC, "BroType::AsFuncType");
		return (FuncType*) this;
		}

	const EnumType* AsEnumType() const
		{
		CHECK_TYPE_TAG(TYPE_ENUM, "BroType::AsEnumType");
		return (EnumType*) this;
		}

	EnumType* AsEnumType()
		{
		CHECK_TYPE_TAG(TYPE_ENUM, "BroType::AsEnumType");
		return (EnumType*) this;
		}

	const VectorType* AsVectorType() const
	        {
		CHECK_TYPE_TAG(TYPE_VECTOR, "BroType::AsVectorType");
		return (VectorType*) this;
		}

	VectorType* AsVectorType()
	        {
		CHECK_TYPE_TAG(TYPE_VECTOR, "BroType::AsVectorType");
		return (VectorType*) this;
		}

	const TypeType* AsTypeType() const
	        {
		CHECK_TYPE_TAG(TYPE_TYPE, "BroType::AsTypeType");
		return (TypeType*) this;
		}

	TypeType* AsTypeType()
	        {
		CHECK_TYPE_TAG(TYPE_TYPE, "BroType::AsTypeType");
		return (TypeType*) this;
		}

	int IsSet() const
		{
		return tag == TYPE_TABLE && (YieldType() == 0);
		}

	BroType* Ref()		{ ::Ref(this); return this; }

	virtual void Describe(ODesc* d) const;
	virtual void DescribeReST(ODesc* d) const;

	virtual unsigned MemoryAllocation() const;

	bool Serialize(SerialInfo* info) const;
	static BroType* Unserialize(UnserialInfo* info, TypeTag want = TYPE_ANY);

	void SetTypeID(const char* id)	{ type_id = id; }
	const char* GetTypeID() const	{ return type_id; }

protected:
	BroType()	{ type_id = 0; }

	void SetError();

	DECLARE_SERIAL(BroType)

private:
	TypeTag tag;
	InternalTypeTag internal_tag;
	bool is_network_order;
	bool base_type;

	// This type_id field is only used by the documentation framework to
	// track the names of declared types.
	const char* type_id;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

class TypeList : public BroType {
public:
	TypeList(BroType* arg_pure_type = 0) : BroType(TYPE_LIST)
		{
		pure_type = arg_pure_type;
		if ( pure_type )
			pure_type->Ref();
		}
	~TypeList();

	const type_list* Types() const	{ return &types; }
	type_list* Types()		{ return &types; }

	int IsPure() const		{ return pure_type != 0; }

	// Returns the underlying pure type, or nil if the list
	// is not pure or is empty.
	BroType* PureType()		{ return pure_type; }
	const BroType* PureType() const	{ return pure_type; }

	// True if all of the types match t, false otherwise.  If
	// is_init is true, then the matching is done in the context
	// of an initialization.
	int AllMatch(const BroType* t, int is_init) const;

	void Append(BroType* t);
	void AppendEvenIfNotPure(BroType* t);

	void Describe(ODesc* d) const;

	unsigned int MemoryAllocation() const
		{
		return BroType::MemoryAllocation()
			+ padded_sizeof(*this) - padded_sizeof(BroType)
			+ types.MemoryAllocation() - padded_sizeof(types);
		}

protected:
	DECLARE_SERIAL(TypeList)

	BroType* pure_type;
	type_list types;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

class IndexType : public BroType {
public:
	int MatchesIndex(ListExpr*& index) const;

	TypeList* Indices() const		{ return indices; }
	const type_list* IndexTypes() const	{ return indices->Types(); }
	BroType* YieldType();

	void Describe(ODesc* d) const;
	void DescribeReST(ODesc* d) const;

	// Returns true if this table is solely indexed by subnet.
	bool IsSubNetIndex() const;

protected:
	IndexType(){ indices = 0; yield_type = 0; }
	IndexType(TypeTag t, TypeList* arg_indices, BroType* arg_yield_type) :
		BroType(t)
		{
		indices = arg_indices;
		yield_type = arg_yield_type;
		}
	~IndexType();

	DECLARE_SERIAL(IndexType)

	TypeList* indices;
	BroType* yield_type;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

class TableType : public IndexType {
public:
	TableType(TypeList* ind, BroType* yield);

	// Returns true if this table type is "unspecified", which is
	// what one gets using an empty "set()" or "table()" constructor.
	bool IsUnspecifiedTable() const;

protected:
	TableType()	{}

	TypeList* ExpandRecordIndex(RecordType* rt) const;

	DECLARE_SERIAL(TableType)

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

class SetType : public TableType {
public:
	SetType(TypeList* ind, ListExpr* arg_elements);
	~SetType();

	ListExpr* SetElements() const	{ return elements; }

protected:
	SetType()	{}

	ListExpr* elements;

	DECLARE_SERIAL(SetType)

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

class FuncType : public BroType {
public:
	FuncType(RecordType* args, BroType* yield, int is_event);

	~FuncType();

	RecordType* Args() const	{ return args; }
	BroType* YieldType();
	void SetYieldType(BroType* arg_yield)	{ yield = arg_yield; }
	int IsEvent() const		{ return is_event; }

	// Used to convert a function type to an event type.
	void ClearYieldType()
		{ Unref(yield); yield = 0; is_event = 1; }

	int MatchesIndex(ListExpr*& index) const;
	int CheckArgs(const type_list* args) const;

	TypeList* ArgTypes()	{ return arg_types; }

	ID* GetReturnValueID() const;

	void Describe(ODesc* d) const;
	void DescribeReST(ODesc* d) const;

protected:
	FuncType()	{ args = 0; arg_types = 0; yield = 0; return_value = 0; }
	DECLARE_SERIAL(FuncType)

	RecordType* args;
	TypeList* arg_types;
	BroType* yield;
	int is_event;
	ID* return_value;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

class TypeType : public BroType {
public:
	TypeType(BroType* t) : BroType(TYPE_TYPE)	{ type = t->Ref(); }
	~TypeType()	{ Unref(type); }

	BroType* Type()	{ return type; }

protected:
	TypeType()	{}

	BroType* type;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

class TypeDecl {
public:
	TypeDecl(BroType* t, const char* i, attr_list* attrs = 0, bool in_record = false);
	virtual ~TypeDecl();

	const Attr* FindAttr(attr_tag a) const
		{ return attrs ? attrs->FindAttr(a) : 0; }

	bool Serialize(SerialInfo* info) const;
	static TypeDecl* Unserialize(UnserialInfo* info);

	virtual void DescribeReST(ODesc* d) const;

	BroType* type;
	Attributes* attrs;
	const char* id;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
    
    TypeDecl() {}; // Dummy default constructor for serialization
};

class CommentedTypeDecl : public TypeDecl {
public:
	CommentedTypeDecl(BroType* t, const char* i, attr_list* attrs = 0,
			bool in_record = false, std::list<std::string>* cmnt_list = 0);
	virtual ~CommentedTypeDecl();

	void DescribeReST(ODesc* d) const;

	std::list<std::string>* comments;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    CommentedTypeDecl() {}; // Dummy default constructor for serialization
};

class RecordType : public BroType {
public:
	RecordType(type_decl_list* types);

	~RecordType();

	int HasField(const char* field) const;
	BroType* FieldType(const char* field) const;
	BroType* FieldType(int field) const;
	Val* FieldDefault(int field) const; // Ref's the returned value; 0 if none.

	// A field's offset is its position in the type_decl_list,
	// starting at 0.  Returns negative if the field doesn't exist.
	int FieldOffset(const char* field) const;

	// Given an offset, returns the field's name.
	const char* FieldName(int field) const;

	type_decl_list* Types() { return types; }

	// Given an offset, returns the field's TypeDecl.
	const TypeDecl* FieldDecl(int field) const;
	TypeDecl* FieldDecl(int field);

	int NumFields() const			{ return num_fields; }

	// Returns 0 if all is ok, otherwise a pointer to an error message.
	// Takes ownership of list.
	const char* AddFields(type_decl_list* types, attr_list* attr);

	void Describe(ODesc* d) const;
	void DescribeReST(ODesc* d) const;
	void DescribeFields(ODesc* d) const;
	void DescribeFieldsReST(ODesc* d, bool func_args) const;

protected:
	RecordType() { types = 0; }

	DECLARE_SERIAL(RecordType)

	int num_fields;
	type_decl_list* types;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

class SubNetType : public BroType {
public:
	SubNetType();
	void Describe(ODesc* d) const;
protected:
	DECLARE_SERIAL(SubNetType)

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

class FileType : public BroType {
public:
	FileType(BroType* yield_type);
	~FileType();

	BroType* YieldType();

	void Describe(ODesc* d) const;

protected:
	FileType()	{ yield = 0; }

	DECLARE_SERIAL(FileType)

	BroType* yield;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

class EnumType : public BroType {
public:
	EnumType();
	~EnumType();

	// The value of this name is next internal counter value, starting
	// with zero. The internal counter is incremented.
	void AddName(const string& module_name, const char* name, bool is_export);

	// The value of this name is set to val. Once a value has been
	// explicitly assigned using this method, no further names can be
	// added that aren't likewise explicitly initalized.
	void AddName(const string& module_name, const char* name, bro_int_t val, bool is_export);

	// -1 indicates not found.
	bro_int_t Lookup(const string& module_name, const char* name);
	const char* Lookup(bro_int_t value); // Returns 0 if not found

protected:
	DECLARE_SERIAL(EnumType)

	virtual void AddNameInternal(const string& module_name,
			const char* name, bro_int_t val, bool is_export);

	typedef std::map< const char*, bro_int_t, ltstr > NameMap;
	NameMap names;

	// The counter is initialized to 0 and incremented on every implicit
	// auto-increment name that gets added (thus its > 0 if
	// auto-increment is used).  Once an explicit value has been
	// specified, the counter is set to -1. This way counter can be used
	// as a flag to prevent mixing of auto-increment and explicit
	// enumerator specifications.
	bro_int_t counter;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

class CommentedEnumType: public EnumType {
public:
	CommentedEnumType() {}
	~CommentedEnumType();

	void DescribeReST(ODesc* d) const;
	void AddComment(const string& module_name, const char* name,
			std::list<std::string>* comments);

protected:
	// This overriden method does not install the given ID name into a
	// scope and it also does not do any kind of checking that the
	// provided name already exists.
	void AddNameInternal(const string& module_name, const char* name,
			bro_int_t val, bool is_export);

	// Comments are only filled when in "documentation mode".
	typedef std::map< const char*, std::list<std::string>*, ltstr > CommentMap;
	CommentMap comments;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

class VectorType : public BroType {
public:
	VectorType(BroType* t);
	virtual ~VectorType();
	BroType* YieldType()	{ return yield_type; }

	int MatchesIndex(ListExpr*& index) const;

	// Returns true if this table type is "unspecified", which is what one
	// gets using an empty "vector()" constructor.
	bool IsUnspecifiedVector() const;

	void Describe(ODesc* d) const;

protected:
	VectorType()	{ yield_type = 0; }

	DECLARE_SERIAL(VectorType)

	BroType* yield_type;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

// Returns the BRO basic (non-parameterized) type with the given type.
extern BroType* base_type(TypeTag tag);

// Returns the BRO basic error type.
inline BroType* error_type()	{ return base_type(TYPE_ERROR); }

// True if the two types are equivalent.  If is_init is true then the
// test is done in the context of an initialization.
extern int same_type(const BroType* t1, const BroType* t2, int is_init=0);

// True if the two attribute lists are equivalent.
extern int same_attrs(const Attributes* a1, const Attributes* a2);

// Returns true if the record sub_rec can be promoted to the record
// super_rec.
extern int record_promotion_compatible(const RecordType* super_rec,
					const RecordType* sub_rec);

// If the given BroType is a TypeList with just one element, returns
// that element, otherwise returns the type.
extern const BroType* flatten_type(const BroType* t);
extern BroType* flatten_type(BroType* t);

// Returns the "maximum" of two type tags, in a type-promotion sense.
extern TypeTag max_type(TypeTag t1, TypeTag t2);

// Given two types, returns the "merge", in which promotable types
// are promoted to the maximum of the two.  Returns nil (and generates
// an error message) if the types are incompatible.
extern BroType* merge_types(const BroType* t1, const BroType* t2);

// Given a list of expressions, returns a (ref'd) type reflecting
// a merged type consistent across all of them, or nil if this
// cannot be done.
BroType* merge_type_list(ListExpr* elements);

// Given an expression, infer its type when used for an initialization.
extern BroType* init_type(Expr* init);

// True if the given type tag corresponds to an integral type.
#define IsIntegral(t)	(t == TYPE_INT || t == TYPE_COUNT || t == TYPE_COUNTER)

// True if the given type tag corresponds to an arithmetic type.
#define IsArithmetic(t)	(IsIntegral(t) || t == TYPE_DOUBLE)

// True if the given type tag corresponds to a boolean type.
#define IsBool(t)	(t == TYPE_BOOL)

// True if the given type tag corresponds to an interval type.
#define IsInterval(t)	(t == TYPE_INTERVAL)

// True if the given type tag corresponds to a record type.
#define IsRecord(t)	(t == TYPE_RECORD || t == TYPE_UNION)

// True if the given type tag corresponds to a function type.
#define IsFunc(t)	(t == TYPE_FUNC)

// True if the given type tag corresponds to mutable type.
#define IsMutable(t)	\
	(t == TYPE_RECORD || t == TYPE_TABLE || t == TYPE_VECTOR)

// True if the given type type is a vector.
#define IsVector(t)	(t == TYPE_VECTOR)

// True if the given type type is a string.
#define IsString(t)	(t == TYPE_STRING)

// True if the given type tag corresponds to type that can be assigned to.
extern int is_assignable(BroType* t);

// True if the given type tag corresponds to the error type.
#define IsErrorType(t)	(t == TYPE_ERROR)

// True if both tags are integral types.
#define BothIntegral(t1, t2) (IsIntegral(t1) && IsIntegral(t2))

// True if both tags are arithmetic types.
#define BothArithmetic(t1, t2) (IsArithmetic(t1) && IsArithmetic(t2))

// True if either tags is an arithmetic type.
#define EitherArithmetic(t1, t2) (IsArithmetic(t1) || IsArithmetic(t2))

// True if both tags are boolean types.
#define BothBool(t1, t2) (IsBool(t1) && IsBool(t2))

// True if both tags are interval types.
#define BothInterval(t1, t2) (IsInterval(t1) && IsInterval(t2))

// True if both tags are string types.
#define BothString(t1, t2) (IsString(t1) && IsString(t2))

// True if either tag is the error type.
#define EitherError(t1, t2) (IsErrorType(t1) || IsErrorType(t2))

#endif
