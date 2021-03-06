#ifndef _RE2C_DFA_DUMP_
#define _RE2C_DFA_DUMP_

#include "src/dfa/closure.h"
#include "src/dfa/dfa.h"

namespace re2c
{

struct dump_dfa_t
{
	const bool debug;
	const dfa_t &dfa;
	const Tagpool &tagpool;
	uint32_t uniqidx;
	const nfa_state_t *base;
	closure_t *shadow;

	dump_dfa_t(const dfa_t &d, const Tagpool &pool, const nfa_t &n);
	~dump_dfa_t();
	void closure_tags(cclositer_t c);
	void closure(const closure_t &clos, uint32_t state, bool isnew);
	void state0(const closure_t &clos);
	void state(const closure_t &clos, size_t state, size_t symbol, bool isnew);
	void final(size_t state, const nfa_state_t *port);
	uint32_t index(const nfa_state_t *s);
	FORBID_COPY(dump_dfa_t);
};

void dump_dfa(const dfa_t &dfa);
void dump_tcmd(const tcmd_t *p);

} // namespace re2c

#endif // _RE2C_DFA_DUMP_
