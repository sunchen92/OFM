##! Base DNS analysis script which tracks and logs DNS queries along with
##! their responses.

@load ./consts

module DNS;

export {
	## The DNS logging stream identifier.
	redef enum Log::ID += { LOG };

	## The record type which contains the column fields of the DNS log.
	type Info: record {
		## The earliest time at which a DNS protocol message over the
		## associated connection is observed.
		ts:            time               &log;
		## A unique identifier of the connection over which DNS messages
		## are being transferred.
		uid:           string             &log;
		## The connection's 4-tuple of endpoint addresses/ports.
		id:            conn_id            &log;
		## The transport layer protocol of the connection.
		proto:         transport_proto    &log;
		## A 16 bit identifier assigned by the program that generated the
		## DNS query.  Also used in responses to match up replies to
		## outstanding queries.
		trans_id:      count              &log &optional;
		## The domain name that is the subject of the DNS query.
		query:         string             &log &optional;
		## The QCLASS value specifying the class of the query.
		qclass:        count              &log &optional;
		## A descriptive name for the class of the query.
		qclass_name:   string             &log &optional;
		## A QTYPE value specifying the type of the query.
		qtype:         count              &log &optional;
		## A descriptive name for the type of the query.
		qtype_name:    string             &log &optional;
		## The response code value in DNS response messages.
		rcode:         count              &log &optional;
		## A descriptive name for the response code value.
		rcode_name:    string             &log &optional;
		## The Authoritative Answer bit for response messages specifies that
		## the responding name server is an authority for the domain name
		## in the question section.
		AA:            bool               &log &default=F;
		## The Truncation bit specifies that the message was truncated.
		TC:            bool               &log &default=F;
		## The Recursion Desired bit in a request message indicates that
		## the client wants recursive service for this query.
		RD:            bool               &log &default=F;
		## The Recursion Available bit in a response message indicates that
		## the name server supports recursive queries.
		RA:            bool               &log &default=F;
		## A reserved field that is currently supposed to be zero in all
		## queries and responses.
		Z:             count              &log &default=0;
		## The set of resource descriptions in the query answer.
		answers:       vector of string   &log &optional;
		## The caching intervals of the associated RRs described by the
		## ``answers`` field.
		TTLs:          vector of interval &log &optional;

		## This value indicates if this request/response pair is ready to be
		## logged.
		ready:         bool            &default=F;
		## The total number of resource records in a reply message's answer
		## section.
		total_answers: count           &optional;
		## The total number of resource records in a reply message's answer,
		## authority, and additional sections.
		total_replies: count           &optional;
	};

	## A record type which tracks the status of DNS queries for a given
	## :bro:type:`connection`.
	type State: record {
		## Indexed by query id, returns Info record corresponding to
		## query/response which haven't completed yet.
		pending: table[count] of Info &optional;

		## This is the list of DNS responses that have completed based on the
		## number of responses declared and the number received.  The contents
		## of the set are transaction IDs.
		finished_answers: set[count] &optional;
	};

	## An event that can be handled to access the :bro:type:`DNS::Info`
	## record as it is sent to the logging framework.
	global log_dns: event(rec: Info);

	## This is called by the specific dns_*_reply events with a "reply" which
	## may not represent the full data available from the resource record, but
	## it's generally considered a summarization of the response(s).
	##
	## c: The connection record for which to fill in DNS reply data.
	##
	## msg: The DNS message header information for the response.
	##
	## ans: The general information of a RR response.
	##
	## reply: The specific response information according to RR type/class.
	global do_reply: event(c: connection, msg: dns_msg, ans: dns_answer, reply: string);
}

redef record connection += {
	dns:       Info  &optional;
	dns_state: State &optional;
};

# DPD configuration.
redef capture_filters += {
	["dns"] = "port 53",
	["mdns"] = "udp and port 5353",
	["llmns"] = "udp and port 5355",
	["netbios-ns"] = "udp port 137",
};

const dns_ports = { 53/udp, 53/tcp, 137/udp, 5353/udp, 5355/udp };
redef dpd_config += { [ANALYZER_DNS] = [$ports = dns_ports] };

const dns_udp_ports = { 53/udp, 137/udp, 5353/udp, 5355/udp };
const dns_tcp_ports = { 53/tcp };
redef dpd_config += { [ANALYZER_DNS_UDP_BINPAC] = [$ports = dns_udp_ports] };
redef dpd_config += { [ANALYZER_DNS_TCP_BINPAC] = [$ports = dns_tcp_ports] };

redef likely_server_ports += { 53/udp, 53/tcp, 137/udp, 5353/udp, 5355/udp };

event bro_init() &priority=5
	{
	Log::create_stream(DNS::LOG, [$columns=Info, $ev=log_dns]);
	}

function new_session(c: connection, trans_id: count): Info
	{
	if ( ! c?$dns_state )
		{
		local state: State;
		state$pending=table();
		state$finished_answers=set();
		c$dns_state = state;
		}

	local info: Info;
	info$ts       = network_time();
	info$id       = c$id;
	info$uid      = c$uid;
	info$proto    = get_conn_transport_proto(c$id);
	info$trans_id = trans_id;
	return info;
	}

function set_session(c: connection, msg: dns_msg, is_query: bool)
	{
	if ( ! c?$dns_state || msg$id !in c$dns_state$pending )
		{
		c$dns_state$pending[msg$id] = new_session(c, msg$id);
		# Try deleting this transaction id from the set of finished answers.
		# Sometimes hosts will reuse ports and transaction ids and this should
		# be considered to be a legit scenario (although bad practice).
		delete c$dns_state$finished_answers[msg$id];
		}

	c$dns = c$dns_state$pending[msg$id];

	if ( ! is_query )
		{
		c$dns$rcode = msg$rcode;
		c$dns$rcode_name = base_errors[msg$rcode];

		if ( ! c$dns?$total_answers )
			c$dns$total_answers = msg$num_answers;

		if ( c$dns?$total_replies &&
		     c$dns$total_replies != msg$num_answers + msg$num_addl + msg$num_auth )
			{
			event conn_weird("dns_changed_number_of_responses", c,
			                      fmt("The declared number of responses changed from %d to %d",
			                          c$dns$total_replies,
			                          msg$num_answers + msg$num_addl + msg$num_auth));
			}
		else
			{
			# Store the total number of responses expected from the first reply.
			c$dns$total_replies = msg$num_answers + msg$num_addl + msg$num_auth;
			}
		}
	}

event DNS::do_reply(c: connection, msg: dns_msg, ans: dns_answer, reply: string) &priority=5
	{
	set_session(c, msg, F);

	if ( ans$answer_type == DNS_ANS )
		{
		c$dns$AA    = msg$AA;
		c$dns$RA    = msg$RA;

		if ( msg$id in c$dns_state$finished_answers )
			event conn_weird("dns_reply_seen_after_done", c, "");

		if ( reply != "" )
			{
			if ( ! c$dns?$answers )
				c$dns$answers = vector();
			c$dns$answers[|c$dns$answers|] = reply;

			if ( ! c$dns?$TTLs )
				c$dns$TTLs = vector();
			c$dns$TTLs[|c$dns$TTLs|] = ans$TTL;
			}

		if ( c$dns?$answers && |c$dns$answers| == c$dns$total_answers )
			{
			add c$dns_state$finished_answers[c$dns$trans_id];
			# Indicate this request/reply pair is ready to be logged.
			c$dns$ready = T;
			}
		}
	}

event DNS::do_reply(c: connection, msg: dns_msg, ans: dns_answer, reply: string) &priority=-5
	{
	if ( c$dns$ready )
		{
		Log::write(DNS::LOG, c$dns);
		# This record is logged and no longer pending.
		delete c$dns_state$pending[c$dns$trans_id];
		}
	}

event dns_request(c: connection, msg: dns_msg, query: string, qtype: count, qclass: count) &priority=5
	{
	set_session(c, msg, T);

	c$dns$RD          = msg$RD;
	c$dns$TC          = msg$TC;
	c$dns$qclass      = qclass;
	c$dns$qclass_name = classes[qclass];
	c$dns$qtype       = qtype;
	c$dns$qtype_name  = query_types[qtype];

	# Decode netbios name queries
	# Note: I'm ignoring the name type for now.  Not sure if this should be
	#       worked into the query/response in some fashion.
	if ( c$id$resp_p == 137/udp )
		query = decode_netbios_name(query);
	c$dns$query    = query;

	c$dns$Z = msg$Z;
	}

event dns_A_reply(c: connection, msg: dns_msg, ans: dns_answer, a: addr) &priority=5
	{
	event DNS::do_reply(c, msg, ans, fmt("%s", a));
	}

event dns_TXT_reply(c: connection, msg: dns_msg, ans: dns_answer, str: string) &priority=5
	{
	event DNS::do_reply(c, msg, ans, str);
	}

event dns_AAAA_reply(c: connection, msg: dns_msg, ans: dns_answer, a: addr) &priority=5
	{
	event DNS::do_reply(c, msg, ans, fmt("%s", a));
	}

event dns_A6_reply(c: connection, msg: dns_msg, ans: dns_answer, a: addr) &priority=5
	{
	event DNS::do_reply(c, msg, ans, fmt("%s", a));
	}

event dns_NS_reply(c: connection, msg: dns_msg, ans: dns_answer, name: string) &priority=5
	{
	event DNS::do_reply(c, msg, ans, name);
	}

event dns_CNAME_reply(c: connection, msg: dns_msg, ans: dns_answer, name: string) &priority=5
	{
	event DNS::do_reply(c, msg, ans, name);
	}

event dns_MX_reply(c: connection, msg: dns_msg, ans: dns_answer, name: string,
                   preference: count) &priority=5
	{
	event DNS::do_reply(c, msg, ans, name);
	}

event dns_PTR_reply(c: connection, msg: dns_msg, ans: dns_answer, name: string) &priority=5
	{
	event DNS::do_reply(c, msg, ans, name);
	}

event dns_SOA_reply(c: connection, msg: dns_msg, ans: dns_answer, soa: dns_soa) &priority=5
	{
	event DNS::do_reply(c, msg, ans, soa$mname);
	}

event dns_WKS_reply(c: connection, msg: dns_msg, ans: dns_answer) &priority=5
	{
	event DNS::do_reply(c, msg, ans, "");
	}

event dns_SRV_reply(c: connection, msg: dns_msg, ans: dns_answer) &priority=5
	{
	event DNS::do_reply(c, msg, ans, "");
	}

# TODO: figure out how to handle these
#event dns_EDNS(c: connection, msg: dns_msg, ans: dns_answer)
#	{
#
#	}
#
#event dns_EDNS_addl(c: connection, msg: dns_msg, ans: dns_edns_additional)
#	{
#
#	}
#
#event dns_TSIG_addl(c: connection, msg: dns_msg, ans: dns_tsig_additional)
#	{
#
#	}


event dns_rejected(c: connection, msg: dns_msg,
                   query: string, qtype: count, qclass: count) &priority=5
	{
	set_session(c, msg, F);
	}

event connection_state_remove(c: connection) &priority=-5
	{
	if ( ! c?$dns_state )
		return;

	# If Bro is expiring state, we should go ahead and log all unlogged
	# request/response pairs now.
	for ( trans_id in c$dns_state$pending )
		Log::write(DNS::LOG, c$dns_state$pending[trans_id]);
	}

