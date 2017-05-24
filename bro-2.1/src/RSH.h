// See the file "COPYING" in the main distribution directory for copyright.

#ifndef rsh_h
#define rsh_h

#include "Login.h"
#include "ContentLine.h"

typedef enum {
	RSH_FIRST_NULL,		// waiting to see first NUL
	RSH_CLIENT_USER_NAME,	// scanning client user name up to NUL
	RSH_SERVER_USER_NAME,	// scanning server user name up to NUL
	RSH_INITIAL_CMD,	// scanning initial command up to NUL

	RSH_LINE_MODE,		// switch to line-oriented processing

	RSH_PRESUMED_REJECTED,	// apparently server said No Way

	RSH_UNKNOWN,	// we don't know what state we're in
} rsh_state;

class Rsh_Analyzer;

class Contents_Rsh_Analyzer : public ContentLine_Analyzer {
public:
	Contents_Rsh_Analyzer(Connection* conn, bool orig, Rsh_Analyzer* analyzer);
	~Contents_Rsh_Analyzer();

	rsh_state RshSaveState() const	{ return save_state; }

protected:
	virtual void DoDeliver(int len, const u_char* data);
	void BadProlog();

	rsh_state state, save_state;
	int num_bytes_to_scan;

	Rsh_Analyzer* analyzer;
};

class Rsh_Analyzer : public Login_Analyzer {
public:
	Rsh_Analyzer(Connection* conn);

	virtual void DeliverStream(int len, const u_char* data, bool orig);

	void ClientUserName(const char* s);
	void ServerUserName(const char* s);

	static Analyzer* InstantiateAnalyzer(Connection* conn)
		{ return new Rsh_Analyzer(conn); }

	static bool Available()
		{ return login_failure || login_success || login_input_line || login_output_line; }

	Contents_Rsh_Analyzer* contents_orig;
	Contents_Rsh_Analyzer* contents_resp;
};

#endif
