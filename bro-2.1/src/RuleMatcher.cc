#include <algorithm>

#include "config.h"

#include "Analyzer.h"
#include "RuleMatcher.h"
#include "DFA.h"
#include "NetVar.h"
#include "Scope.h"
#include "File.h"
#include "Reporter.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/export.hpp>

// FIXME: Things that are not fully implemented/working yet:
//
//		  - "ip-options" always evaluates to false
//		  - offsets for payload patterns are ignored
//			(but simulated by snort2bro by leading dots)
//		  - if a rule contains "PayloadSize" and application
//			specific patterns (like HTTP), but no "payload" patterns,
//			it may fail to match. Work-around: Insert an always
//			matching "payload" pattern (not done in snort2bro yet)
//		  - tcp-state always evaluates to true
//			(implemented but deactivated for comparision to Snort)

uint32 RuleHdrTest::idcounter = 0;

RuleHdrTest::RuleHdrTest(Prot arg_prot, uint32 arg_offset, uint32 arg_size,
				Comp arg_comp, maskedvalue_list* arg_vals)
	{
	prot = arg_prot;
	offset = arg_offset;
	size = arg_size;
	comp = arg_comp;
	vals = arg_vals;
	sibling = 0;
	child = 0;
	pattern_rules = 0;
	pure_rules = 0;
	ruleset = new IntSet;
	id = ++idcounter;
	level = 0;
	}

Val* RuleMatcher::BuildRuleStateValue(const Rule* rule,
					const RuleEndpointState* state) const
	{
	RecordVal* val = new RecordVal(signature_state);
	val->Assign(0, new StringVal(rule->ID()));
	val->Assign(1, state->GetAnalyzer()->BuildConnVal());
	val->Assign(2, new Val(state->is_orig, TYPE_BOOL));
	val->Assign(3, new Val(state->payload_size, TYPE_COUNT));
	return val;
	}

RuleHdrTest::RuleHdrTest(RuleHdrTest& h)
	{
	prot = h.prot;
	offset = h.offset;
	size = h.size;
	comp = h.comp;

	vals = new maskedvalue_list;
	loop_over_list(*h.vals, i)
		vals->append(new MaskedValue(*(*h.vals)[i]));

	for ( int j = 0; j < Rule::TYPES; ++j )
		{
		loop_over_list(h.psets[j], k)
			{
			PatternSet* orig_set = h.psets[j][k];
			PatternSet* copied_set = new PatternSet;
			copied_set->re = 0;
			copied_set->ids = orig_set->ids;
			loop_over_list(orig_set->patterns, l)
				copied_set->patterns.append(copy_string(orig_set->patterns[l]));
			delete copied_set;
			// TODO: Why do we create copied_set only to then
			// never use it?
			}
		}

	sibling = 0;
	child = 0;
	pattern_rules = 0;
	pure_rules = 0;
	ruleset = new IntSet;
	id = ++idcounter;
	level = 0;
	}

RuleHdrTest::~RuleHdrTest()
	{
	loop_over_list(*vals, i)
		delete (*vals)[i];
	delete vals;

	for ( int i = 0; i < Rule::TYPES; ++i )
		{
		loop_over_list(psets[i], j)
			delete psets[i][j]->re;
		}

	delete ruleset;
	}

bool RuleHdrTest::operator==(const RuleHdrTest& h)
	{
	if ( prot != h.prot || offset != h.offset || size != h.size ||
	     comp != h.comp || vals->length() != h.vals->length() )
		return false;

	loop_over_list(*vals, i)
		if ( (*vals)[i]->val != (*h.vals)[i]->val ||
		     (*vals)[i]->mask != (*h.vals)[i]->mask )
			return false;

	return true;
	}

void RuleHdrTest::PrintDebug()
	{
	static const char* str_comp[] = { "<=", ">=", "<", ">", "==", "!=" };
	static const char* str_prot[] = { "", "ip", "icmp", "tcp", "udp" };

	fprintf(stderr, "	RuleHdrTest %s[%d:%d] %s",
			str_prot[prot], offset, size, str_comp[comp]);

	loop_over_list(*vals, i)
		fprintf(stderr, " 0x%08x/0x%08x",
				(*vals)[i]->val, (*vals)[i]->mask);

	fprintf(stderr, "\n");
	}

RuleEndpointState::RuleEndpointState(Analyzer* arg_analyzer, bool arg_is_orig,
					  RuleEndpointState* arg_opposite,
					  ::PIA* arg_PIA)
	{
	payload_size = -1;
	analyzer = arg_analyzer;
	is_orig = arg_is_orig;

	opposite = arg_opposite;
	if ( opposite )
		opposite->opposite = this;

	pia = arg_PIA;
	}

RuleEndpointState::~RuleEndpointState()
	{
	loop_over_list(matchers, i)
		{
		delete matchers[i]->state;
		delete matchers[i];
		}

	loop_over_list(matched_text, j)
		delete matched_text[j];
	}

BOOST_CLASS_EXPORT_GUID(RuleEndpointState,"RuleEndpointState")
template<class Archive>
void RuleEndpointState::serialize(Archive & ar, const unsigned int version)
    {
        ar & is_orig;
        ar & analyzer;
        ar & opposite;
        //ar & pia; //FIXME
        //ar & matchers; //FIXME
        //ar & hdr_tests; //FIXME
        //ar & matched_by_patterns; //FIXME
        //ar & matched_test; //FIXME
        ar & payload_size;
        //ar & matched_rules; //FIXME
    }
template void RuleEndpointState::serialize<boost::archive::text_oarchive>(
        boost::archive::text_oarchive & ar, 
        const unsigned int file_version);
template void RuleEndpointState::serialize<boost::archive::text_iarchive>(
        boost::archive::text_iarchive & ar, 
        const unsigned int file_version);

RuleMatcher::RuleMatcher(int arg_RE_level)
	{
	root = new RuleHdrTest(RuleHdrTest::NOPROT, 0, 0, RuleHdrTest::EQ,
				new maskedvalue_list);
	RE_level = arg_RE_level;
	}

RuleMatcher::~RuleMatcher()
	{
#ifdef MATCHER_PRINT_STATS
	DumpStats(stderr);
#endif
	Delete(root);

	loop_over_list(rules, i)
		delete rules[i];
	}

void RuleMatcher::Delete(RuleHdrTest* node)
	{
	RuleHdrTest* next;
	for ( RuleHdrTest* h = node->child; h; h = next )
		{
		next = h->sibling;
		Delete(h);
		}

	delete node;
	}

bool RuleMatcher::ReadFiles(const name_list& files)
	{
#ifdef USE_PERFTOOLS_DEBUG
	HeapLeakChecker::Disabler disabler;
#endif

	parse_error = false;

	for ( int i = 0; i < files.length(); ++i )
		{
		rules_in = search_for_file(files[i], "sig", 0, false, 0);
		if ( ! rules_in )
			{
			reporter->Error("Can't open signature file %s", files[i]);
			return false;
			}

		rules_line_number = 0;
		current_rule_file = files[i];
		rules_parse();
		}

	if ( parse_error )
		return false;

	BuildRulesTree();

	string_list exprs[Rule::TYPES];
	int_list ids[Rule::TYPES];
	BuildRegEx(root, exprs, ids);

	return ! parse_error;
	}

void RuleMatcher::AddRule(Rule* rule)
	{
	if ( rules_by_id.Lookup(rule->ID()) )
		{
		rules_error("rule defined twice");
		return;
		}

	rules.append(rule);
	rules_by_id.Insert(rule->ID(), rule);
	}

void RuleMatcher::BuildRulesTree()
	{
	loop_over_list(rules, r)
		{
		if ( ! rules[r]->Active() )
			continue;

		rules[r]->SortHdrTests();
		InsertRuleIntoTree(rules[r], 0, root, 0);
		}
	}

void RuleMatcher::InsertRuleIntoTree(Rule* r, int testnr,
					RuleHdrTest* dest, int level)
	{
	// Initiliaze the preconditions
	loop_over_list(r->preconds, i)
		{
		Rule::Precond* pc = r->preconds[i];

		Rule* pc_rule = rules_by_id.Lookup(pc->id);
		if ( ! pc_rule )
			{
			rules_error(r, "unknown rule referenced");
			return;
			}

		pc->rule = pc_rule;
		pc_rule->dependents.append(r);
		}

	// All tests in tree already?
	if ( testnr >= r->hdr_tests.length() )
		{ // then insert it into the right list of the test
		if ( r->patterns.length() )
			{
			r->next = dest->pattern_rules;
			dest->pattern_rules = r;
			}
		else
			{
			r->next = dest->pure_rules;
			dest->pure_rules = r;
			}

		dest->ruleset->Insert(r->Index());
		return;
		}

	// Look for matching child.
	for ( RuleHdrTest* h = dest->child; h; h = h->sibling )
		if ( *h == *r->hdr_tests[testnr] )
			{
			InsertRuleIntoTree(r, testnr + 1, h, level + 1);
			return;
			}

	// Insert new child.
	RuleHdrTest* newtest = new RuleHdrTest(*r->hdr_tests[testnr]);
	newtest->sibling = dest->child;
	newtest->level = level + 1;
	dest->child = newtest;

	InsertRuleIntoTree(r, testnr + 1, newtest, level + 1);
	}

void RuleMatcher::BuildRegEx(RuleHdrTest* hdr_test, string_list* exprs,
				int_list* ids)
	{
	// For each type, get all patterns on this node.
	for ( Rule* r = hdr_test->pattern_rules; r; r = r->next )
		{
		loop_over_list(r->patterns, j)
			{
			Rule::Pattern* p = r->patterns[j];
			exprs[p->type].append(p->pattern);
			ids[p->type].append(p->id);
			}
		}

	// If we're above the RE_level, these patterns will form the regexprs.
	if ( hdr_test->level < RE_level )
		{
		for ( int i = 0; i < Rule::TYPES; ++i )
			if ( exprs[i].length() )
				BuildPatternSets(&hdr_test->psets[i], exprs[i], ids[i]);
		}

	// Get the patterns on all of our children.
	for ( RuleHdrTest* h = hdr_test->child; h; h = h->sibling )
		{
		string_list child_exprs[Rule::TYPES];
		int_list child_ids[Rule::TYPES];

		BuildRegEx(h, child_exprs, child_ids);

		for ( int i = 0; i < Rule::TYPES; ++i )
			{
			loop_over_list(child_exprs[i], j)
				{
				exprs[i].append(child_exprs[i][j]);
				ids[i].append(child_ids[i][j]);
				}
			}
		}

	// If we're on the RE_level, all patterns gathered now
	// form the regexprs.
	if ( hdr_test->level == RE_level )
		{
		for ( int i = 0; i < Rule::TYPES; ++i )
			if ( exprs[i].length() )
				BuildPatternSets(&hdr_test->psets[i], exprs[i], ids[i]);
		}

	// If we're below the RE_level, the regexprs remains empty.
	}

void RuleMatcher::BuildPatternSets(RuleHdrTest::pattern_set_list* dst,
				const string_list& exprs, const int_list& ids)
	{
	assert(exprs.length() == ids.length());

	// We build groups of at most sig_max_group_size regexps.

	string_list group_exprs;
	int_list group_ids;

	for ( int i = 0; i < exprs.length() + 1 /* sic! */; i++ )
		{
		if ( i < exprs.length() )
			{
			group_exprs.append(exprs[i]);
			group_ids.append(ids[i]);
			}

		if ( group_exprs.length() > sig_max_group_size ||
		     i == exprs.length() )
			{
			RuleHdrTest::PatternSet* set =
				new RuleHdrTest::PatternSet;
			set->re = new Specific_RE_Matcher(MATCH_EXACTLY, 1);
			set->re->CompileSet(group_exprs, group_ids);
			set->patterns = group_exprs;
			set->ids = group_ids;
			dst->append(set);

			group_exprs.clear();
			group_ids.clear();
			}
		}
	}

// Get a 8/16/32-bit value from the given position in the packet header
static inline uint32 getval(const u_char* data, int size)
	{
	switch ( size ) {
	case 1:
		return *(uint8*) data;

	case 2:
		return ntohs(*(uint16*) data);

	case 4:
		return ntohl(*(uint32*) data);

	default:
		reporter->InternalError("illegal HdrTest size");
	}

	// Should not be reached.
	return 0;
	}


// A line which can be inserted into the macros below for debugging
// fprintf(stderr, "%.06f %08x & %08x %s %08x\n", network_time, v, (mvals)[i]->mask, #op, (mvals)[i]->val);

// Evaluate a value list (matches if at least one value matches).
#define DO_MATCH_OR( mvals, v, op )	\
	{	\
	loop_over_list((mvals), i)	\
		{	\
		if ( ((v) & (mvals)[i]->mask) op (mvals)[i]->val )	\
			goto match;	\
		}	\
	goto no_match;	\
	}

// Evaluate a value list (doesn't match if any value matches).
#define DO_MATCH_NOT_AND( mvals, v, op )	\
	{	\
	loop_over_list((mvals), i)	\
		{	\
		if ( ((v) & (mvals)[i]->mask) op (mvals)[i]->val )	\
			goto no_match;	\
		}	\
	goto match;	\
	}

RuleEndpointState* RuleMatcher::InitEndpoint(Analyzer* analyzer,
						const IP_Hdr* ip, int caplen,
						RuleEndpointState* opposite,
						bool from_orig, PIA* pia)
	{
	RuleEndpointState* state =
		new RuleEndpointState(analyzer, from_orig, opposite, pia);

	if ( rule_bench == 3 )
		return state;

	rule_hdr_test_list tests;
	tests.append(root);

	loop_over_list(tests, h)
		{
		RuleHdrTest* hdr_test = tests[h];

		DBG_LOG(DBG_RULES, "HdrTest %d matches (%s%s)", hdr_test->id,
				hdr_test->pattern_rules ? "+" : "-",
				hdr_test->pure_rules ? "+" : "-");

		// Current HdrTest node matches the packet, so remember it
		// if we have any rules on it.
		if ( hdr_test->pattern_rules || hdr_test->pure_rules )
			state->hdr_tests.append(hdr_test);

		// Evaluate all rules on this node which don't contain
		// any patterns.
		for ( Rule* r = hdr_test->pure_rules; r; r = r->next )
			if ( EvalRuleConditions(r, state, 0, 0, 0) )
				ExecRuleActions(r, state, 0, 0, 0);

		// If we're on or above the RE_level, we may have some
		// pattern matching to do.
		if ( hdr_test->level <= RE_level )
			{
			for ( int i = 0; i < Rule::TYPES; ++i )
				{
				loop_over_list(hdr_test->psets[i], j)
					{
					RuleHdrTest::PatternSet* set =
						hdr_test->psets[i][j];

					assert(set->re);

					RuleEndpointState::Matcher* m =
						new RuleEndpointState::Matcher;
					m->state = new RE_Match_State(set->re);
					m->type = (Rule::PatternType) i;
					state->matchers.append(m);
					}
				}
			}

		if ( ip )
			{
			// Get start of transport layer.
			const u_char* transport = ip->Payload();

			// Descend the RuleHdrTest tree further.
			for ( RuleHdrTest* h = hdr_test->child; h;
			      h = h->sibling )
				{
				const u_char* data;

				// Evaluate the header test.
				switch ( h->prot ) {
				case RuleHdrTest::IP:
					data = (const u_char*) ip->IP4_Hdr();
					break;

				case RuleHdrTest::ICMP:
				case RuleHdrTest::TCP:
				case RuleHdrTest::UDP:
					data = transport;
					break;

				default:
					data = 0;
					reporter->InternalError("unknown protocol");
				}

				// ### data can be nil here if it's an
				// IPv6 packet and we're doing an IP test.
				if ( ! data )
					continue;

				// Sorry for the hidden gotos :-)
				switch ( h->comp ) {
					case RuleHdrTest::EQ:
						DO_MATCH_OR(*h->vals, getval(data + h->offset, h->size), ==);

					case RuleHdrTest::NE:
						DO_MATCH_NOT_AND(*h->vals, getval(data + h->offset, h->size), ==);

					case RuleHdrTest::LT:
						DO_MATCH_OR(*h->vals, getval(data + h->offset, h->size), <);

					case RuleHdrTest::GT:
						DO_MATCH_OR(*h->vals, getval(data + h->offset, h->size), >);

					case RuleHdrTest::LE:
						DO_MATCH_OR(*h->vals, getval(data + h->offset, h->size), <=);

					case RuleHdrTest::GE:
						DO_MATCH_OR(*h->vals, getval(data + h->offset, h->size), >=);

					default:
						reporter->InternalError("unknown comparision type");
				}

no_match:
				continue;

match:
				tests.append(h);
				}
			}
		}
	// Save some memory.
	state->hdr_tests.resize(0);
	state->matchers.resize(0);

	// Send BOL to payload matchers.
	Match(state, Rule::PAYLOAD, (const u_char *) "", 0, true, false, false);

	return state;
	}

void RuleMatcher::Match(RuleEndpointState* state, Rule::PatternType type,
			const u_char* data, int data_len,
			bool bol, bool eol, bool clear)
	{
	if ( ! state )
		{
		reporter->Warning("RuleEndpointState not initialized yet.");
		return;
		}

	// FIXME: There is probably some room for performance improvements
	// in this method.  For example, it *may* help to use an IntSet
	// for 'accepted' (that depends on the average number of matching
	// patterns).

	if ( rule_bench >= 2 )
		return;

	bool newmatch = false;

#ifdef DEBUG
	if ( debug_logger.IsEnabled(DBG_RULES) )
		{
		const char* s =
			fmt_bytes((const char *) data, min(40, data_len));

		DBG_LOG(DBG_RULES, "Matching %s rules [%d,%d] on |%s%s|",
				Rule::TypeToString(type), bol, eol, s,
				data_len > 40 ? "..." : "");
		}
#endif

	// Remember size of first non-null data.
	if ( type == Rule::PAYLOAD )
		{
		bol = state->payload_size < 0;

		if ( state->payload_size <= 0 && data_len )
			state->payload_size = data_len;

		else if ( state->payload_size < 0 )
			state->payload_size = 0;
		}

	// Feed data into all relevant matchers.
	loop_over_list(state->matchers, x)
		{
		RuleEndpointState::Matcher* m = state->matchers[x];
		if ( m->type == type &&
		     m->state->Match((const u_char*) data, data_len,
					bol, eol, clear) )
			newmatch = true;
		}

	// If no new match found, we're already done.
	if ( ! newmatch )
		return;

	DBG_LOG(DBG_RULES, "New pattern match found");

	// Build a joined AcceptingSet.
	AcceptingSet accepted;
	int_list matchpos;

	loop_over_list(state->matchers, y)
		{
		RuleEndpointState::Matcher* m = state->matchers[y];
		const AcceptingSet* ac = m->state->Accepted();

		loop_over_list(*ac, k)
			{
			if ( ! accepted.is_member((*ac)[k]) )
				{
				accepted.append((*ac)[k]);
				matchpos.append((*m->state->MatchPositions())[k]);
				}
			}
		}

	// Determine the rules for which all patterns have matched.
	// This code should be fast enough as long as there are only very few
	// matched patterns per connection (which is a plausible assumption).

	rule_list matched;

	loop_over_list(accepted, i)
		{
		Rule* r = Rule::rule_table[accepted[i] - 1];

		DBG_LOG(DBG_RULES, "Checking rule: %s", r->id);

		// Check whether all patterns of the rule have matched.
		loop_over_list(r->patterns, j)
			{
			if ( ! accepted.is_member(r->patterns[j]->id) )
				goto next_pattern;

			// See if depth is satisfied.
			if ( (unsigned int) matchpos[i] >
			     r->patterns[j]->offset + r->patterns[j]->depth )
				goto next_pattern;

			DBG_LOG(DBG_RULES, "All patterns of rule satisfied");

			// FIXME: How to check for offset ??? ###
			}

		// If not already in the list of matching rules, add it.
		if ( ! matched.is_member(r) )
			matched.append(r);

next_pattern:
		continue;
		}

	// Check which of the matching rules really belong to any of our nodes.

	loop_over_list(matched, j)
		{
		Rule* r = matched[j];

		DBG_LOG(DBG_RULES, "Accepted rule: %s", r->id);

		loop_over_list(state->hdr_tests, k)
			{
			RuleHdrTest* h = state->hdr_tests[k];

			DBG_LOG(DBG_RULES, "Checking for accepted rule on HdrTest %d", h->id);

			// Skip if rule does not belong to this node.
			if ( ! h->ruleset->Contains(r->Index()) )
				continue;

			DBG_LOG(DBG_RULES, "On current node");

			// Skip if rule already fired for this connection.
			if ( state->matched_rules.is_member(r->Index()) )
				continue;

			// Remember that all patterns have matched.
			if ( ! state->matched_by_patterns.is_member(r) )
				{
				state->matched_by_patterns.append(r);
				BroString* s = new BroString(data, data_len, 0);
				state->matched_text.append(s);
				}

			DBG_LOG(DBG_RULES, "And has not already fired");
			// Eval additional conditions.
			if ( ! EvalRuleConditions(r, state, data, data_len, 0) )
				continue;

			// Found a match.
			ExecRuleActions(r, state, data, data_len, 0);
			}
		}
	}

void RuleMatcher::FinishEndpoint(RuleEndpointState* state)
	{
	if ( rule_bench == 3 )
		return;

	// Send EOL to payload matchers.
	Match(state, Rule::PAYLOAD, (const u_char *) "", 0, false, true, false);

	// Some of the pure rules may match at the end of the connection,
	// although they have not matched at the beginning. So, we have
	// to test the candidates here.

	ExecPureRules(state, 1);

	loop_over_list(state->matched_by_patterns, i)
		ExecRulePurely(state->matched_by_patterns[i],
				state->matched_text[i], state, 1);
	}

void RuleMatcher::ExecPureRules(RuleEndpointState* state, bool eos)
	{
	loop_over_list(state->hdr_tests, i)
		{
		RuleHdrTest* hdr_test = state->hdr_tests[i];
		for ( Rule* r = hdr_test->pure_rules; r; r = r->next )
			ExecRulePurely(r, 0, state, eos);
		}
	}

bool RuleMatcher::ExecRulePurely(Rule* r, BroString* s,
				 RuleEndpointState* state, bool eos)
	{
	if ( state->matched_rules.is_member(r->Index()) )
		return false;

	DBG_LOG(DBG_RULES, "Checking rule %s purely", r->ID());

	if ( EvalRuleConditions(r, state, 0, 0, eos) )
		{
		DBG_LOG(DBG_RULES, "MATCH!");

		if ( s )
			ExecRuleActions(r, state, s->Bytes(), s->Len(), eos);
		else
			ExecRuleActions(r, state, 0, 0, eos);

		return true;
		}

	return false;
	}

bool RuleMatcher::EvalRuleConditions(Rule* r, RuleEndpointState* state,
		const u_char* data, int len, bool eos)
	{
	DBG_LOG(DBG_RULES, "Evaluating conditions for rule %s", r->ID());

	// Check for other rules which have to match first.
	loop_over_list(r->preconds, i)
		{
		Rule::Precond* pc = r->preconds[i];

		RuleEndpointState* pc_state = state;

		if ( pc->opposite_dir )
			{
			if ( ! state->opposite )
				// No rule matching for other direction yet.
				return false;

			pc_state = state->opposite;
			}

		if ( ! pc->negate )
			{
			if ( ! pc_state->matched_rules.is_member(pc->rule->Index()) )
				// Precond rule has not matched yet.
				return false;
			}
		else
			{
			// Only at eos can we decide about negated conditions.
			if ( ! eos )
				return false;

			if ( pc_state->matched_rules.is_member(pc->rule->Index()) )
				return false;
			}
		}

	loop_over_list(r->conditions, l)
		if ( ! r->conditions[l]->DoMatch(r, state, data, len) )
			return false;

	DBG_LOG(DBG_RULES, "Conditions met: MATCH! %s", r->ID());
	return true;
	}

void RuleMatcher::ExecRuleActions(Rule* r, RuleEndpointState* state,
				const u_char* data, int len, bool eos)
	{
	if ( state->opposite &&
	     state->opposite->matched_rules.is_member(r->Index()) )
		// We have already executed the actions.
		return;

	state->matched_rules.append(r->Index());

	loop_over_list(r->actions, i)
		r->actions[i]->DoAction(r, state, data, len);

	// This rule may trigger some other rules; check them.
	loop_over_list(r->dependents, j)
		{
		Rule* dep = (r->dependents)[j];
		ExecRule(dep, state, eos);
		if ( state->opposite )
			ExecRule(dep, state->opposite, eos);
		}
	}

void RuleMatcher::ExecRule(Rule* rule, RuleEndpointState* state, bool eos)
	{
	// Nothing to do if it has already matched.
	if ( state->matched_rules.is_member(rule->Index()) )
		return;

	loop_over_list(state->hdr_tests, i)
		{
		RuleHdrTest* h = state->hdr_tests[i];

		// Is it on this HdrTest at all?
		if ( ! h->ruleset->Contains(rule->Index()) )
			continue;

		// Is it a pure rule?
		for ( Rule* r = h->pure_rules; r; r = r->next )
			if ( r == rule )
				{ // found, so let's evaluate it
				ExecRulePurely(rule, 0, state, eos);
				return;
				}

		// It must be a non-pure rule. It can only match right now if
		// all its patterns are satisfied already.
		int pos = state->matched_by_patterns.member_pos(rule);
		if ( pos >= 0 )
			{ // they are, so let's evaluate it
			ExecRulePurely(rule, state->matched_text[pos], state, eos);
			return;
			}
		}
	}

void RuleMatcher::ClearEndpointState(RuleEndpointState* state)
	{
	if ( rule_bench == 3 )
		return;

	ExecPureRules(state, 1);
	state->payload_size = -1;
	state->matched_by_patterns.clear();
	loop_over_list(state->matched_text, i)
		delete state->matched_text[i];
	state->matched_text.clear();

	loop_over_list(state->matchers, j)
		state->matchers[j]->state->Clear();
	}

void RuleMatcher::PrintDebug()
	{
	loop_over_list(rules, i)
		rules[i]->PrintDebug();

	fprintf(stderr, "\n---------------\n");

	PrintTreeDebug(root);
	}

static inline void indent(int level)
	{
	for ( int i = level * 2; i; --i )
		fputc(' ', stderr);
	}

void RuleMatcher::PrintTreeDebug(RuleHdrTest* node)
	{
	for ( int i = 0; i < Rule::TYPES; ++i )
		{
		indent(node->level);
		loop_over_list(node->psets[i], j)
			{
			RuleHdrTest::PatternSet* set = node->psets[i][j];

			fprintf(stderr,
				"[%d patterns in %s group %d from %d rules]\n",
				set->patterns.length(),
				Rule::TypeToString((Rule::PatternType) i), j,
				set->ids.length());
			}
		}

	for ( Rule* r = node->pattern_rules; r; r = r->next )
		{
		indent(node->level);
		fprintf(stderr, "Pattern rule %s (%d/%d)\n", r->id, r->idx,
				node->ruleset->Contains(r->Index()));
		}

	for ( Rule* r = node->pure_rules; r; r = r->next )
		{
		indent(node->level);
		fprintf(stderr, "Pure rule %s (%d/%d)\n", r->id, r->idx,
				node->ruleset->Contains(r->Index()));
		}

	for ( RuleHdrTest* h = node->child; h; h = h->sibling )
		{
		indent(node->level);
		fprintf(stderr, "Test %4d\n", h->id);
		PrintTreeDebug(h);
		}
	}

void RuleMatcher::GetStats(Stats* stats, RuleHdrTest* hdr_test)
	{
	if ( ! hdr_test )
		{
		stats->matchers = 0;
		stats->dfa_states = 0;
		stats->computed = 0;
		stats->mem = 0;
		stats->hits = 0;
		stats->misses = 0;
		stats->avg_nfa_states = 0;
		hdr_test = root;
		}

	DFA_State_Cache::Stats cstats;

	for ( int i = 0; i < Rule::TYPES; ++i )
		{
		loop_over_list(hdr_test->psets[i], j)
			{
			RuleHdrTest::PatternSet* set = hdr_test->psets[i][j];
			assert(set->re);

			++stats->matchers;
			set->re->DFA()->Cache()->GetStats(&cstats);

			stats->dfa_states += cstats.dfa_states;
			stats->computed += cstats.computed;
			stats->mem += cstats.mem;
			stats->hits += cstats.hits;
			stats->misses += cstats.misses;
			stats->avg_nfa_states += cstats.nfa_states;
			}
		}

	if (  stats->dfa_states )
		stats->avg_nfa_states /= stats->dfa_states;
	else
		stats->avg_nfa_states = 0;

	for ( RuleHdrTest* h = hdr_test->child; h; h = h->sibling )
		GetStats(stats, h);
	}

void RuleMatcher::DumpStats(BroFile* f)
	{
	Stats stats;
	GetStats(&stats);

	f->Write(fmt("%.6f computed dfa states = %d; classes = ??; "
			"computed trans. = %d; matchers = %d; mem = %d\n",
			network_time, stats.dfa_states, stats.computed,
			stats.matchers, stats.mem));
	f->Write(fmt("%.6f DFA cache hits = %d; misses = %d\n", network_time,
			stats.hits, stats.misses));

	DumpStateStats(f, root);
	}

void RuleMatcher::DumpStateStats(BroFile* f, RuleHdrTest* hdr_test)
	{
	if ( ! hdr_test )
		return;

	for ( int i = 0; i < Rule::TYPES; i++ )
		{
		loop_over_list(hdr_test->psets[i], j)
			{
			RuleHdrTest::PatternSet* set = hdr_test->psets[i][j];
			assert(set->re);

			f->Write(fmt("%.6f %d DFA states in %s group %d from sigs ", network_time,
					 set->re->DFA()->NumStates(),
					 Rule::TypeToString((Rule::PatternType)i), j));

			loop_over_list(set->ids, k)
				{
				Rule* r = Rule::rule_table[set->ids[k] - 1];
				f->Write(fmt("%s ", r->ID()));
				}
			
			f->Write("\n");
			}
		}

	for ( RuleHdrTest* h = hdr_test->child; h; h = h->sibling )
		DumpStateStats(f, h);
	}

static Val* get_bro_val(const char* label)
	{
	ID* id = lookup_ID(label, GLOBAL_MODULE_NAME, false);
	if ( ! id )
		{
		rules_error("unknown script-level identifier", label);
		return 0;
		}

	return id->ID_Val();
	}


// Converts an atomic Val and appends it to the list
static bool val_to_maskedval(Val* v, maskedvalue_list* append_to)
	{
	MaskedValue* mval = new MaskedValue;

	switch ( v->Type()->Tag() ) {
		case TYPE_PORT:
			mval->val = v->AsPortVal()->Port();
			mval->mask = 0xffffffff;
			break;

		case TYPE_BOOL:
		case TYPE_COUNT:
		case TYPE_ENUM:
		case TYPE_INT:
			mval->val = v->CoerceToUnsigned();
			mval->mask = 0xffffffff;
			break;

		case TYPE_SUBNET:
			{
			const uint32* n;
			uint32 m[4];
			v->AsSubNet().Prefix().GetBytes(&n);
			v->AsSubNetVal()->Mask().CopyIPv6(m);

			for ( unsigned int i = 0; i < 4; ++i )
				m[i] = ntohl(m[i]);

			bool is_v4_mask = m[0] == 0xffffffff &&
						m[1] == m[0] && m[2] == m[0];

			if ( v->AsSubNet().Prefix().GetFamily() == IPv4 &&
			     is_v4_mask )
				{
				mval->val = ntohl(*n);
				mval->mask = m[3];
				}

			else
				{
				rules_error("IPv6 subnets not supported");
				mval->val = 0;
				mval->mask = 0;
				}
			}
			break;

		default:
			rules_error("Wrong type of identifier");
			return false;
	}

	append_to->append(mval);

	return true;
	}

void id_to_maskedvallist(const char* id, maskedvalue_list* append_to)
	{
	Val* v = get_bro_val(id);
	if ( ! v )
		return;

	if ( v->Type()->Tag() == TYPE_TABLE )
		{
		val_list* vals = v->AsTableVal()->ConvertToPureList()->Vals();
		loop_over_list(*vals, i )
			if ( ! val_to_maskedval((*vals)[i], append_to) )
			{
				delete_vals(vals);
				return;
			}

		delete_vals(vals);
		}

	else
		val_to_maskedval(v, append_to);
	}

char* id_to_str(const char* id)
	{
	const BroString* src;
	char* dst;

	Val* v = get_bro_val(id);
	if ( ! v )
		goto error;

	if ( v->Type()->Tag() != TYPE_STRING )
		{
		rules_error("Identifier must refer to string");
		goto error;
		}

	src = v->AsString();
	dst = new char[src->Len()+1];
	memcpy(dst, src->Bytes(), src->Len());
	*(dst+src->Len()) = '\0';
	return dst;

error:
	char* dummy = copy_string("<error>");
	return dummy;
	}

uint32 id_to_uint(const char* id)
	{
	Val* v = get_bro_val(id);
	if ( ! v )
		return 0;

	TypeTag t = v->Type()->Tag();

	if ( t == TYPE_BOOL || t == TYPE_COUNT || t == TYPE_ENUM ||
	     t == TYPE_INT || t == TYPE_PORT )
		return v->CoerceToUnsigned();

	rules_error("Identifier must refer to integer");
	return 0;
	}

void RuleMatcherState::InitEndpointMatcher(Analyzer* analyzer, const IP_Hdr* ip,
					int caplen, bool from_orig, PIA* pia)
	{
	if ( ! rule_matcher )
		return;

	if ( from_orig )
		{
		if ( orig_match_state )
			{
			rule_matcher->FinishEndpoint(orig_match_state);
			delete orig_match_state;
			}

		orig_match_state =
			rule_matcher->InitEndpoint(analyzer, ip, caplen,
					resp_match_state, from_orig, pia);
		}

	else
		{
		if ( resp_match_state )
			{
			rule_matcher->FinishEndpoint( resp_match_state );
			delete resp_match_state;
			}

		resp_match_state =
			rule_matcher->InitEndpoint(analyzer, ip, caplen,
					orig_match_state, from_orig, pia);
		}
	}

void RuleMatcherState::FinishEndpointMatcher()
	{
	if ( ! rule_matcher )
		return;

	if ( orig_match_state )
		rule_matcher->FinishEndpoint(orig_match_state);

	if ( resp_match_state )
		rule_matcher->FinishEndpoint(resp_match_state);

	delete orig_match_state;
	delete resp_match_state;

	orig_match_state = resp_match_state = 0;
	}

void RuleMatcherState::Match(Rule::PatternType type, const u_char* data,
				int data_len, bool from_orig,
				bool bol, bool eol, bool clear)
	{
	if ( ! rule_matcher )
		return;

	rule_matcher->Match(from_orig ? orig_match_state : resp_match_state,
					type, data, data_len, bol, eol, clear);
	}

void RuleMatcherState::ClearMatchState(bool orig)
	{
	if ( ! rule_matcher )
		return;

	if ( orig_match_state )
		rule_matcher->ClearEndpointState(orig_match_state);
	if ( resp_match_state )
		rule_matcher->ClearEndpointState(resp_match_state);
	}

BOOST_CLASS_EXPORT_GUID(RuleMatcherState,"RuleMatcherState")
template<class Archive>
void RuleMatcherState::serialize(Archive & ar, const unsigned int version)
    {
        ar & orig_match_state;
        ar & resp_match_state;
    }
template void RuleMatcherState::serialize<boost::archive::text_oarchive>(
        boost::archive::text_oarchive & ar, 
        const unsigned int file_version);
template void RuleMatcherState::serialize<boost::archive::text_iarchive>(
        boost::archive::text_iarchive & ar, 
        const unsigned int file_version);
