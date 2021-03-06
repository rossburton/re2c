#include "src/conf/msg.h"
#include "src/re/re.h"
#include "src/re/empty_class_policy.h"
#include "src/re/encoding/case.h"
#include "src/re/encoding/enc.h"
#include "src/re/encoding/utf16/utf16_regexp.h"
#include "src/re/encoding/utf8/utf8_regexp.h"

namespace re2c {

/* note [default regexp]
 *
 * Create a byte range that includes all possible input characters.
 * This may include characters, which do not map to any valid symbol
 * in current encoding. For encodings, which directly map symbols to
 * input characters (ASCII, EBCDIC, UTF-32), it equals [^]. For other
 * encodings (UTF-16, UTF-8), [^] and this range are different.
 *
 * Also note that default range doesn't respect encoding policy
 * (the way invalid code points are treated).
 */

/* note [POSIX subexpression hierarchy]
 *
 * POSIX treats subexpressions with and without captures as equal,
 * therefore we have to insert missing captures in subexpressions
 * that influence disambiguation of existing captures. Such cases are:
 * left alternative in union (unless it is already a capture) and first
 * operand in concatenation (unless it is a capture or the length of
 * strings accepted by it is fixed). Of course, this insertion only
 * applies to subexpressions that have nested captures.
 */

static bool has_tags(const AST *ast)
{
	switch (ast->type) {
		case AST::NIL:
		case AST::STR:
		case AST::CLS:
		case AST::DOT:
		case AST::DEFAULT:
		case AST::DIFF: return false;
		case AST::TAG:
		case AST::CAP: return true;
		case AST::ALT: return has_tags(ast->alt.ast1) || has_tags(ast->alt.ast2);
		case AST::CAT: return has_tags(ast->cat.ast1) || has_tags(ast->cat.ast2);
		case AST::REF: return has_tags(ast->ref.ast);
		case AST::ITER: return has_tags(ast->iter.ast);
	}
	return false; /* unreachable */
}

static size_t fixlen(const AST *ast)
{
	switch (ast->type) {
		case AST::NIL:
		case AST::TAG: return 0;
		case AST::CLS:
		case AST::DOT:
		case AST::DEFAULT:
		case AST::DIFF: return 1;
		case AST::STR: return ast->str.chars->size();
		case AST::ALT: {
			const size_t
				l1 = fixlen(ast->alt.ast1),
				l2 = fixlen(ast->alt.ast2);
			return l1 == l2 ? l1 : Tag::VARDIST;
		}
		case AST::CAT: {
			const size_t
				l1 = fixlen(ast->cat.ast1),
				l2 = fixlen(ast->cat.ast2);
			return l1 == Tag::VARDIST || l2 == Tag::VARDIST
				? Tag::VARDIST : l1 + l2;
		}
		case AST::REF: return fixlen(ast->ref.ast);
		case AST::ITER: {
			const size_t l = fixlen(ast->iter.ast);
			const uint32_t m = ast->iter.min, n = ast->iter.max;
			return l == Tag::VARDIST || m != n
				? Tag::VARDIST : l * (n - m);
		}
		case AST::CAP: return fixlen(ast->cap);
	}
	return Tag::VARDIST; /* unreachable */
}

static RE *ast_to_re(RESpec &spec, const AST *ast, size_t &ncap)
{
	RE::alc_t &alc = spec.alc;
	std::vector<Tag> &tags = spec.tags;
	const opt_t *opts = spec.opts;
	Warn &warn = spec.warn;

	switch (ast->type) {
		case AST::NIL:
			return re_nil(alc);
		case AST::STR: {
			const bool icase = opts->bCaseInsensitive
				|| (ast->str.icase != opts->bCaseInverted);
			RE *x = NULL;
			std::vector<ASTChar>::const_iterator
				i = ast->str.chars->begin(),
				e = ast->str.chars->end();
			for (; i != e; ++i) {
				x = re_cat(alc, x, icase
					? re_ichar(alc, ast->line, i->column, i->chr, opts)
					: re_schar(alc, ast->line, i->column, i->chr, opts));
			}
			return x ? x : re_nil(alc);
		}
		case AST::CLS: {
			Range *r = NULL;
			std::vector<ASTRange>::const_iterator
				i = ast->cls.ranges->begin(),
				e = ast->cls.ranges->end();
			for (; i != e; ++i) {
				Range *s = opts->encoding.encodeRange(i->lower, i->upper);
				if (!s) fatal_lc(ast->line, i->column,
					"bad code point range: '0x%X - 0x%X'", i->lower, i->upper);
				r = Range::add(r, s);
			}
			if (ast->cls.negated) {
				r = Range::sub(opts->encoding.fullRange(), r);
			}
			return re_class(alc, ast->line, ast->column, r, opts, warn);
		}
		case AST::DOT: {
			uint32_t c = '\n';
			if (!opts->encoding.encode(c)) {
				fatal_lc(ast->line, ast->column, "bad code point: '0x%X'", c);
			}
			return re_class(alc, ast->line, ast->column,
				Range::sub(opts->encoding.fullRange(), Range::sym(c)), opts, warn);
		}
		case AST::DEFAULT:
			// see note [default regexp]
			return re_sym(alc, Range::ran(0, opts->encoding.nCodeUnits()));
		case AST::ALT: {
			RE *t1 = NULL, *t2 = NULL, *x, *y;
			if (opts->posix_captures && has_tags(ast)
				&& ast->alt.ast1->type != AST::CAP) {
				// see note [POSIX subexpression hierarchy]
				t1 = re_tag(alc, tags.size(), false);
				tags.push_back(Tag(Tag::FICTIVE, false));
				t2 = re_tag(alc, tags.size(), false);
				tags.push_back(Tag(Tag::FICTIVE, false));
			}
			x = ast_to_re(spec, ast->alt.ast1, ncap);
			x = re_cat(alc, t1, re_cat(alc, x, t2));
			y = ast_to_re(spec, ast->alt.ast2, ncap);
			return re_alt(alc, x, y);
		}
		case AST::DIFF: {
			RE *x = ast_to_re(spec, ast->diff.ast1, ncap);
			RE *y = ast_to_re(spec, ast->diff.ast2, ncap);
			if (x->type != RE::SYM || y->type != RE::SYM) {
				fatal_lc(ast->line, ast->column, "can only difference char sets");
			}
			return re_class(alc, ast->line, ast->column, Range::sub(x->sym, y->sym), opts, warn);
		}
		case AST::CAT: {
			RE *t1 = NULL, *t2 = NULL, *x, *y;
			const AST *a1 = ast->alt.ast1;
			if (opts->posix_captures && has_tags(ast)
				&& a1->type != AST::CAP && fixlen(a1) == Tag::VARDIST) {
				// see note [POSIX subexpression hierarchy]
				t1 = re_tag(alc, tags.size(), false);
				tags.push_back(Tag(Tag::FICTIVE, false));
				t2 = re_tag(alc, tags.size(), false);
				tags.push_back(Tag(Tag::FICTIVE, false));
			}
			x = ast_to_re(spec, ast->cat.ast1, ncap);
			x = re_cat(alc, t1, re_cat(alc, x, t2));
			y = ast_to_re(spec, ast->cat.ast2, ncap);
			return re_cat(alc, x, y);
		}
		case AST::TAG: {
			if (ast->tag.name && !opts->tags) {
				fatal_lc(ast->line, ast->column,
					"tags are only allowed with '-T, --tags' option");
			}
			if (opts->posix_captures) {
				fatal_lc(ast->line, ast->column,
					"simple tags are not allowed with '--posix-captures' option");
			}
			RE *t = re_tag(alc, tags.size(), false);
			tags.push_back(Tag(ast->tag.name, ast->tag.history));
			return t;
		}
		case AST::CAP: {
			if (!opts->posix_captures) {
				return ast_to_re(spec, ast->cap, ncap);
			}
			const AST *x = ast->cap;
			if (x->type == AST::REF) x = x->ref.ast;

			RE *t1 = re_tag(alc, tags.size(), false);
			tags.push_back(Tag(2 * ncap, false));

			RE *t2 = re_tag(alc, tags.size(), false);
			tags.push_back(Tag(2 * ncap + 1, false));

			++ncap;
			return re_cat(alc, t1, re_cat(alc, ast_to_re(spec, x, ncap), t2));
		}
		case AST::REF:
			if (!opts->posix_captures) {
				return ast_to_re(spec, ast->ref.ast, ncap);
			}
			fatal_l(ast->line,
				"implicit grouping is forbidden with '--posix-captures'"
				" option, please wrap '%s' in capturing parenthesis",
				ast->ref.name->c_str());
			return NULL;
		case AST::ITER: {
			const uint32_t
				n = ast->iter.min,
				n1 = std::max(n, 1u),
				m = std::max(n, ast->iter.max);
			const AST *x = ast->iter.ast;

			RE *t1 = NULL, *t2 = NULL;
			if (opts->posix_captures && x->type == AST::CAP) {
				x = x->cap;
				if (x->type == AST::REF) x = x->ref.ast;

				t1 = re_tag(alc, tags.size(), false);
				tags.push_back(Tag(2 * ncap, m > 1));

				t2 = re_tag(alc, tags.size(), false);
				tags.push_back(Tag(2 * ncap + 1, false));

				++ncap;
			}

			RE *y = NULL;
			if (m == 0) {
				y = re_cat(alc, t1, t2);
			} else if (m == 1) {
				y = ast_to_re(spec, x, ncap);
				y = re_cat(alc, t1, re_cat(alc, y, t2));
			} else  {
				y = ast_to_re(spec, x, ncap);
				y = re_cat(alc, t1, y);
				y = re_iter(alc, y, n1, m);
				y = re_cat(alc, y, t2);
			}
			if (n == 0) {
				y = re_alt(alc, y, re_nil(alc));
			}
			return y;
		}
	}
	return NULL; /* unreachable */
}

RE *re_schar(RE::alc_t &alc, uint32_t line, uint32_t column, uint32_t c, const opt_t *opts)
{
	if (!opts->encoding.encode(c)) {
		fatal_lc(line, column, "bad code point: '0x%X'", c);
	}
	switch (opts->encoding.type()) {
		case Enc::UTF16:
			return UTF16Symbol(alc, c);
		case Enc::UTF8:
			return UTF8Symbol(alc, c);
		case Enc::ASCII:
		case Enc::EBCDIC:
		case Enc::UTF32:
		case Enc::UCS2:
			return re_sym(alc, Range::sym(c));
	}
	return NULL; /* unreachable */
}

RE *re_ichar(RE::alc_t &alc, uint32_t line, uint32_t column, uint32_t c, const opt_t *opts)
{
	if (is_alpha(c)) {
		return re_alt(alc,
			re_schar(alc, line, column, to_lower_unsafe(c), opts),
			re_schar(alc, line, column, to_upper_unsafe(c), opts));
	} else {
		return re_schar(alc, line, column, c, opts);
	}
}

RE *re_class(RE::alc_t &alc, uint32_t line, uint32_t column, const Range *r, const opt_t *opts, Warn &warn)
{
	if (!r) {
		switch (opts->empty_class_policy) {
			case EMPTY_CLASS_MATCH_EMPTY:
				warn.empty_class(line);
				return re_nil(alc);
			case EMPTY_CLASS_MATCH_NONE:
				warn.empty_class(line);
				break;
			case EMPTY_CLASS_ERROR:
				fatal_lc(line, column, "empty character class");
		}
	}
	switch (opts->encoding.type()) {
		case Enc::UTF16:
			return UTF16Range(alc, r);
		case Enc::UTF8:
			return UTF8Range(alc, r);
		case Enc::ASCII:
		case Enc::EBCDIC:
		case Enc::UTF32:
		case Enc::UCS2:
			return re_sym(alc, r);
	}
	return NULL; /* unreachable */
}

static void assert_tags_used_once(const Rule &rule, const std::vector<Tag> &tags)
{
	std::set<std::string> names;
	const std::string *name = NULL;

	for (size_t t = rule.ltag; t < rule.htag; ++t) {
		name = tags[t].name;
		if (name && !names.insert(*name).second) {
			fatal_l(rule.code->fline,
				"tag '%s' is used multiple times in the same rule",
				name->c_str());
		}
	}
}

static void init_rule(Rule &rule, const Code *code, const std::vector<Tag> &tags,
	size_t ltag, size_t ncap)
{
	rule.code = code;
	rule.ltag = ltag;
	rule.htag = tags.size();
	for (rule.ttag = ltag; rule.ttag < rule.htag && !trailing(tags[rule.ttag]); ++rule.ttag);
	rule.ncap = ncap;
	assert_tags_used_once(rule, tags);
}

RESpec::RESpec(const std::vector<ASTRule> &ast, const opt_t *o, Warn &w)
	: alc()
	, res()
	, charset(*new std::vector<uint32_t>)
	, tags(*new std::vector<Tag>)
	, rules(*new std::valarray<Rule>(ast.size()))
	, opts(o)
	, warn(w)
{
	for (size_t i = 0; i < ast.size(); ++i) {
		size_t ltag = tags.size(), ncap = 0;
		res.push_back(ast_to_re(*this, ast[i].ast, ncap));
		init_rule(rules[i], ast[i].code, tags, ltag, ncap);
	}
}

} // namespace re2c
