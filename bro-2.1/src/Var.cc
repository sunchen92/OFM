// See the file "COPYING" in the main distribution directory for copyright.

#include "config.h"

#include "Var.h"
#include "Func.h"
#include "Stmt.h"
#include "Scope.h"
#include "Serializer.h"
#include "RemoteSerializer.h"
#include "EventRegistry.h"

extern int generate_documentation;

static Val* init_val(Expr* init, const BroType* t, Val* aggr)
	{
	return init->InitVal(t, aggr);
	}

static void make_var(ID* id, BroType* t, init_class c, Expr* init,
			attr_list* attr, decl_type dt, int do_init)
	{
	if ( id->Type() )
		{
		if ( id->IsRedefinable() || (! init && attr) )
			{
			BroObj* redef_obj = init ? (BroObj*) init : (BroObj*) t;
			if ( dt != VAR_REDEF )
				id->Warn("redefinition requires \"redef\"", redef_obj, 1);
			}

		else if ( dt != VAR_REDEF || init || ! attr )
			{
			id->Error("already defined", init);
			return;
			}
		}

	if ( dt == VAR_REDEF )
		{
		if ( ! id->Type() )
			{
			id->Error("\"redef\" used but not previously defined");
			return;
			}

		if ( ! t )
			t = id->Type();
		}

	if ( id->Type() && id->Type()->Tag() != TYPE_ERROR )
		{
		if ( dt != VAR_REDEF &&
		     (! init || ! do_init || (! t && ! (t = init_type(init)))) )
			{
			id->Error("already defined", init);
			return;
			}

		// Allow redeclaration in order to initialize.
		if ( ! same_type(t, id->Type()) )
			{
			id->Error("redefinition changes type", init);
			return;
			}
		}

	if ( init && optimize )
		init = init->Simplify(SIMPLIFY_GENERAL);

	if ( t && t->IsSet() )
		{ // Check for set with explicit elements.
		SetType* st = t->AsTableType()->AsSetType();
		ListExpr* elements = st->SetElements();

		if ( elements )
			{
			if ( init )
				{
				id->Error("double initialization", init);
				return;
				}

			Ref(elements);
			init = elements;
			}
		}

	if ( ! t )
		{ // Take type from initialization.
		if ( ! init )
			{
			id->Error("no type given");
			return;
			}

		t = init_type(init);
		if ( ! t )
			{
			id->SetType(error_type());
			return;
			}
		}
	else
		Ref(t);

	id->SetType(t);

	if ( attr )
		id->AddAttrs(new Attributes(attr, t, false));

	if ( id->FindAttr(ATTR_PERSISTENT) || id->FindAttr(ATTR_SYNCHRONIZED) )
		{
		if ( dt == VAR_CONST )
			{
			id->Error("&persistant/synchronized with constant");
			return;
			}

		if ( ! id->IsGlobal() )
			{
			id->Error("&persistant/synchronized with non-global");
			return;
			}
		}

	if ( do_init )
		{
		if ( (c == INIT_EXTRA && id->FindAttr(ATTR_ADD_FUNC)) ||
		     (c == INIT_REMOVE && id->FindAttr(ATTR_DEL_FUNC)) )
			// Just apply the function.
			id->SetVal(init, c);

		else if ( dt != VAR_REDEF || init || ! attr )
			{
			Val* aggr;
			if ( t->Tag() == TYPE_RECORD )
				aggr = new RecordVal(t->AsRecordType());

			else if ( t->Tag() == TYPE_TABLE )
				aggr = new TableVal(t->AsTableType(), id->Attrs());

			else if ( t->Tag() == TYPE_VECTOR )
				aggr = new VectorVal(t->AsVectorType());

			else
				aggr = 0;

			Val* v = 0;
			if ( init )
				{
				v = init_val(init, t, aggr);
				if ( ! v )
					return;
				}

			if ( aggr )
				id->SetVal(aggr, c);
			else if ( v )
				id->SetVal(v, c);
			}
		}

	if ( dt == VAR_CONST )
		{
		if ( ! init && ! id->IsRedefinable() )
			id->Error("const variable must be initialized");

		id->SetConst();
		}

	id->UpdateValAttrs();

	if ( t && t->Tag() == TYPE_FUNC && t->AsFuncType()->IsEvent() )
		{
		// For events, add a function value (without any body) here so that
		// we can later access the ID even if no implementations have been
		// defined.
		Func* f = new BroFunc(id, 0, 0, 0, 0);
		id->SetVal(new Val(f));
		id->SetConst();
		}
	}


void add_global(ID* id, BroType* t, init_class c, Expr* init,
		attr_list* attr, decl_type dt)
	{
	make_var(id, t, c, init, attr, dt, 1);
	}

Stmt* add_local(ID* id, BroType* t, init_class c, Expr* init,
		attr_list* attr, decl_type dt)
	{
	make_var(id, t, c, init, attr, dt, 0);

	if ( init )
		{
		if ( c != INIT_FULL )
			id->Error("can't use += / -= for initializations of local variables");

		Ref(id);

		Stmt* stmt =
		    new ExprStmt(new AssignExpr(new NameExpr(id), init, 0, 0,
		        id->Attrs() ? id->Attrs()->Attrs() : 0 ));
		stmt->SetLocationInfo(init->GetLocationInfo());

		return stmt;
		}

	else
		{
		if ( t->Tag() == TYPE_RECORD || t->Tag() == TYPE_TABLE ||
		     t->Tag() == TYPE_VECTOR )
			current_scope()->AddInit(id);

		return new NullStmt;
		}
	}

extern Expr* add_and_assign_local(ID* id, Expr* init, Val* val)
	{
	make_var(id, 0, INIT_FULL, init, 0, VAR_REGULAR, 0);
	Ref(id);
	return new AssignExpr(new NameExpr(id), init, 0, val);
	}

void add_type(ID* id, BroType* t, attr_list* attr, int /* is_event */)
	{
	BroType* tnew = t;

	// In "documentation mode", we'd like to to be able to associate
	// an identifier name with a declared type.  Dealing with declared
	// types that are "aliases" to a builtin type requires that the BroType
	// is cloned before setting the identifier name that resolves to it.
	// And still this is not enough to document cases where the declared type
	// is an alias for another declared type -- but that's not a natural/common
	// practice.  If documenting that corner case is desired, one way
	// is to add an ID* to class ID that tracks aliases and set it here if
	// t->GetTypeID() is true.
	if ( generate_documentation )
		{
		switch ( t->Tag() ) {
		// Only "shallow" copy types that may contain records because
		// we want to be able to see additions to the original record type's
		// list of fields
		case TYPE_RECORD:
			tnew = new RecordType(t->AsRecordType()->Types());
			break;
		case TYPE_TABLE:
			tnew = new TableType(t->AsTableType()->Indices(),
			                     t->AsTableType()->YieldType());
			break;
		case TYPE_VECTOR:
			tnew = new VectorType(t->AsVectorType()->YieldType());
			break;
		case TYPE_FUNC:
			tnew = new FuncType(t->AsFuncType()->Args(),
			                    t->AsFuncType()->YieldType(),
			                    t->AsFuncType()->IsEvent());
			break;
		default:
			SerializationFormat* form = new BinarySerializationFormat();
			form->StartWrite();
			CloneSerializer ss(form);
			SerialInfo sinfo(&ss);
			sinfo.cache = false;

			t->Serialize(&sinfo);
			char* data;
			uint32 len = form->EndWrite(&data);
			form->StartRead(data, len);

			UnserialInfo uinfo(&ss);
			uinfo.cache = false;
			tnew = t->Unserialize(&uinfo);

			delete [] data;
		}

		tnew->SetTypeID(copy_string(id->Name()));
		}

	id->SetType(tnew);
	id->MakeType();

	if ( attr )
		id->SetAttrs(new Attributes(attr, tnew, false));
	}

void begin_func(ID* id, const char* module_name, function_flavor flavor,
		int is_redef, FuncType* t)
	{
	if ( flavor == FUNC_FLAVOR_EVENT )
		{
		const BroType* yt = t->YieldType();

		if ( yt && yt->Tag() != TYPE_VOID )
			id->Error("event cannot yield a value", t);

		t->ClearYieldType();
		}

	if ( id->Type() )
		{
		if ( ! same_type(id->Type(), t) )
			id->Type()->Error("incompatible types", t);
		}

	else if ( is_redef )
		id->Error("redef of not-previously-declared value");

	if ( id->HasVal() )
		{
		int id_is_event = id->ID_Val()->AsFunc()->IsEvent();

		if ( id_is_event != (flavor == FUNC_FLAVOR_EVENT) )
			id->Error("inconsistency between event and function", t);
		if ( id_is_event )
			{
			if ( is_redef )
				// Clear out value so it will be replaced.
				id->SetVal(0);
			}
		else
			{
			if ( ! id->IsRedefinable() )
				id->Error("already defined");
			}
		}
	else
		id->SetType(t);

	push_scope(id);

	RecordType* args = t->Args();
	int num_args = args->NumFields();
	for ( int i = 0; i < num_args; ++i )
		{
		TypeDecl* arg_i = args->FieldDecl(i);
		ID* arg_id = lookup_ID(arg_i->id, module_name);

		if ( arg_id && ! arg_id->IsGlobal() )
			arg_id->Error("argument name used twice");

		arg_id = install_ID(copy_string(arg_i->id), module_name,
					false, false);
		arg_id->SetType(arg_i->type->Ref());
		}
	}

void end_func(Stmt* body, attr_list* attrs)
	{
	int frame_size = current_scope()->Length();
	id_list* inits = current_scope()->GetInits();

	Scope* scope = pop_scope();
	ID* id = scope->ScopeID();

	int priority = 0;
	if ( attrs )
		{
		loop_over_list(*attrs, i)
			{
			Attr* a = (*attrs)[i];
			if ( a->Tag() != ATTR_PRIORITY )
				{
				a->Error("illegal attribute for function body");
				continue;
				}

			Val* v = a->AttrExpr()->Eval(0);
			if ( ! v )
				{
				a->Error("cannot evaluate attribute expression");
				continue;
				}

			if ( ! IsIntegral(v->Type()->Tag()) )
				{
				a->Error("expression is not of integral type");
				continue;
				}

			priority = v->InternalInt();
			}
		}

	if ( id->HasVal() )
		id->ID_Val()->AsFunc()->AddBody(body, inits, frame_size, priority);
	else
		{
		Func* f = new BroFunc(id, body, inits, frame_size, priority);
		id->SetVal(new Val(f));
		id->SetConst();
		}

	id->ID_Val()->AsFunc()->SetScope(scope);
	}

Val* internal_val(const char* name)
	{
	ID* id = lookup_ID(name, GLOBAL_MODULE_NAME);
	if ( ! id )
		reporter->InternalError("internal variable %s missing", name);

	return id->ID_Val();
	}

Val* internal_const_val(const char* name)
	{
	ID* id = lookup_ID(name, GLOBAL_MODULE_NAME);
	if ( ! id )
		reporter->InternalError("internal variable %s missing", name);

	if ( ! id->IsConst() )
		reporter->InternalError("internal variable %s is not constant", name);

	return id->ID_Val();
	}

Val* opt_internal_val(const char* name)
	{
	ID* id = lookup_ID(name, GLOBAL_MODULE_NAME);
	return id ? id->ID_Val() : 0;
	}

double opt_internal_double(const char* name)
	{
	Val* v = opt_internal_val(name);
	return v ? v->InternalDouble() : 0.0;
	}

bro_int_t opt_internal_int(const char* name)
	{
	Val* v = opt_internal_val(name);
	return v ? v->InternalInt() : 0;
	}

bro_uint_t opt_internal_unsigned(const char* name)
	{
	Val* v = opt_internal_val(name);
	return v ? v->InternalUnsigned() : 0;
	}

StringVal* opt_internal_string(const char* name)
	{
	Val* v = opt_internal_val(name);
	return v ? v->AsStringVal() : 0;
	}

TableVal* opt_internal_table(const char* name)
	{
	Val* v = opt_internal_val(name);
	return v ? v->AsTableVal() : 0;
	}

ListVal* internal_list_val(const char* name)
	{
	ID* id = lookup_ID(name, GLOBAL_MODULE_NAME);
	if ( ! id )
		return 0;

	Val* v = id->ID_Val();
	if ( v )
		{
		if ( v->Type()->Tag() == TYPE_LIST )
			return (ListVal*) v;

		else if ( v->Type()->IsSet() )
			{
			TableVal* tv = v->AsTableVal();
			ListVal* lv = tv->ConvertToPureList();
			return lv;
			}

		else
			reporter->InternalError("internal variable %s is not a list", name);
		}

	return 0;
	}

BroType* internal_type(const char* name)
	{
	ID* id = lookup_ID(name, GLOBAL_MODULE_NAME);
	if ( ! id )
		reporter->InternalError("internal type %s missing", name);

	return id->Type();
	}

Func* internal_func(const char* name)
	{
	Val* v = internal_val(name);
	if ( v )
		return v->AsFunc();
	else
		return 0;
	}

EventHandlerPtr internal_handler(const char* name)
	{
	// If there already is an entry in the registry, we have a
	// local handler on the script layer.
	EventHandler* h = event_registry->Lookup(name);
	if ( h )
		{
		h->SetUsed();
		return h;
		}

	h = new EventHandler(name);
	event_registry->Register(h);

	h->SetUsed();

	return h;
	}
