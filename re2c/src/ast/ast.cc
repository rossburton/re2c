#include <limits>

#include "src/ast/ast.h"

namespace re2c
{

free_list<AST*> AST::flist;

const uint32_t AST::MANY = std::numeric_limits<uint32_t>::max();

AST::AST(uint32_t l, uint32_t c, type_t t)
	: type(t), line(l), column(c)
{
	flist.insert(this);
}

AST::~AST()
{
	flist.erase(this);
	if (type == TAG) {
		delete tag.name;
	} else if (type == REF) {
		delete ref.name;
	} else if (type == STR) {
		delete str.chars;
	} else if (type == CLS) {
		delete cls.ranges;
	}
}

const AST *ast_nil(uint32_t l, uint32_t c)
{
	return new AST(l, c, AST::NIL);
}

const AST *ast_str(uint32_t l, uint32_t c, std::vector<ASTChar> *chars, bool icase)
{
	AST *ast = new AST(l, c, AST::STR);
	ast->str.chars = chars;
	ast->str.icase = icase;
	return ast;
}

const AST *ast_cls(uint32_t l, uint32_t c, std::vector<ASTRange> *ranges, bool negated)
{
	AST *ast = new AST(l, c, AST::CLS);
	ast->cls.ranges = ranges;
	ast->cls.negated = negated;
	return ast;
}

const AST *ast_dot(uint32_t l, uint32_t c)
{
	return new AST(l, c, AST::DOT);
}

const AST *ast_default(uint32_t l, uint32_t c)
{
	return new AST(l, c, AST::DEFAULT);
}

const AST *ast_alt(const AST *a1, const AST *a2)
{
	if (!a1) return a2;
	if (!a2) return a1;
	AST *ast = new AST(a1->line, a1->column, AST::ALT);
	ast->alt.ast1 = a1;
	ast->alt.ast2 = a2;
	return ast;
}

const AST *ast_cat(const AST *a1, const AST *a2)
{
	if (!a1) return a2;
	if (!a2) return a1;
	AST *ast = new AST(a1->line, a1->column, AST::CAT);
	ast->cat.ast1 = a1;
	ast->cat.ast2 = a2;
	return ast;
}

const AST *ast_iter(const AST *r, uint32_t n, uint32_t m)
{
	AST *ast = new AST(r->line, r->column, AST::ITER);
	ast->iter.ast = r;
	ast->iter.min = n;
	ast->iter.max = m;
	return ast;
}

const AST *ast_diff(const AST *a1, const AST *a2)
{
	AST *ast = new AST(a1->line, a1->column, AST::DIFF);
	ast->cat.ast1 = a1;
	ast->cat.ast2 = a2;
	return ast;
}

const AST *ast_tag(uint32_t l, uint32_t c, const std::string *n, bool h)
{
	AST *ast = new AST(l, c, AST::TAG);
	ast->tag.name = n;
	ast->tag.history = h;
	return ast;
}

const AST *ast_cap(const AST *r)
{
	AST *ast = new AST(r->line, r->column, AST::CAP);
	ast->cap = r;
	return ast;
}

const AST *ast_ref(const AST *r, const std::string &n)
{
	AST *ast = new AST(r->line, r->column, AST::REF);
	ast->ref.ast = r;
	ast->ref.name = new std::string(n);
	return ast;
}

bool ast_need_wrap(const AST *ast)
{
	switch (ast->type) {
		case AST::ITER:
		case AST::NIL:
		case AST::STR:
		case AST::CLS:
		case AST::DOT:
		case AST::DEFAULT:
		case AST::TAG:
		case AST::CAP:
			return false;
		case AST::ALT:
		case AST::CAT:
		case AST::DIFF:
		case AST::REF:
			return true;
	}
	return false; /* unreachable */
}

} // namespace re2c
