#include "Generator.hpp"
#include <regex>

Generator::Generator(Parser::Config &c, CommandLine &cl, LALR &l) : config(c), commandLine(cl), lalr(l) {}

void Generator::generate_c(std::ostream &out) const {
	put_common_headers(out);
	put_cpp_headers(out);
	put_cpp_tables(out);
	put_cpp_routines(out);
	put_yyparse(out);
	out << config.tail_code;
}

void Generator::generate_h(std::ostream &out) const {
	out << "#ifndef YY_TAB_H" << std::endl;
	out << "# define YY_TAB_H" << std::endl;
	put_common_headers(out);
	out << "#endif" << std::endl;
}

void Generator::put_common_headers(std::ostream &out) const {
	if (commandLine.debug_defines)
		out << "#define YYDEBUG 1" << std::endl;
	out << R"(
#ifndef YYSTYPE
typedef )" + (this->config.union_enabled ? this->config.stack_type : "int") + R"( YYSTYPE;
#endif
#if YYDEBUG
extern int yydebug;
#endif
extern YYSTYPE			)" + this->commandLine.sym_prefix + R"(lval;
int			)" + this->commandLine.sym_prefix + R"(parse(void);
)";
	out << "enum yy_token_definitions {" << std::endl;
	for (auto &token : lalr.get_token_definitions())
		out << token.first << " = " << token.second << "," << std::endl;
	out << "};" << std::endl;
}


void Generator::put_cpp_headers(std::ostream &out) const {
	if (commandLine.sym_prefix != "yy") {
		auto syms =
				{
						"_state_t",
						"_stack_type",
						"_stack",
						"_stack_size",
						"_stack_cap",
						"lval",
						"_lookahead",
						"_rule",
						"parse",
						"internal_yyparse",
						"_yacc_transitions",
						"_terminals_limit",
						"_reduce_tokens",
						"_reduce_table",
						"_map_token",
						"_reduce",
						"_shift",
						"_stack_top",
						"_stack_pop",
						"_stack_push",
						"lex",
						"in"
				};
		for (std::string sym: syms)
			out << "#define yy" << sym << " " << commandLine.sym_prefix << sym << std::endl;
	}
	out  << config.header_code
	<< R"(
#include <sys/types.h>
#include <string.h>
#define YY_STACK_ELEMENT(N) (yy_stack[yy_stack_size - 1 + (N - 1)].value)
#define YYRECOVERING (yy_error_state != 0)
#define YYERROR do {yy_error_state=4;goto yy_end_of_semantic_action;} while (0)
#define yyerrok do {yy_error_state = 0;} while (0)
#define yyclearin do {if (yy_stack_size) yy_stack_top()->token = -1;} while (0)
#define YYACCEPT do {return 0;} while (0)
#define YYABORT do {return 1;} while (0)
#define YY_END 0
#define YY_EPSILON 1
)";
	out << "#define YY_ACCEPT " << this->lalr.get_accept() << std::endl;
	out << R"(typedef int yy_state_t;

typedef struct {
	yy_state_t		state;
	int				token;
	YYSTYPE			value;
}				yy_stack_type;

yy_stack_type	*yy_stack = NULL;
size_t			yy_stack_size = 0;
size_t			yy_stack_cap = 0;
YYSTYPE			yylval;
#if YYDEBUG
int yydebug = 0;
#endif

)";
}

void Generator::put_cpp_routines(std::ostream &out) const {
	out << R"(

int yy_stack_push(yy_stack_type value)
{
	if (yy_stack_size >= yy_stack_cap) {
		yy_stack_cap = yy_stack_cap ? 2 * yy_stack_cap : 50;
		yy_stack = realloc(yy_stack, yy_stack_cap * sizeof(yy_stack_type));
		if (!yy_stack)
			return 1;
	}
	yy_stack[yy_stack_size] = value;
	yy_stack_size++;
	return 0;
}

void yy_stack_pop(size_t n)
{
//	if (n > yy_stack_size)
//		yy_stack_size = 0;
//	else
		yy_stack_size -= n;
}

yy_stack_type *yy_stack_top()
{
	return yy_stack + yy_stack_size - 1;
}

int	yy_lookahead()
{
	if (yy_stack_top()->token == -1)
	{
		int yytok = yylex();
#ifdef YYDEBUG
		if (yydebug)
			fprintf(stderr, "lex %d\n", yytok);
#endif
		if (yytok < 0)
			yytok = 0;
		if (yytok >= (sizeof(yy_map_token) / sizeof(*yy_map_token)) || (yytok != 0 && !yy_map_token[yytok]))
			yy_stack_top()->token = -2;
		else
			yy_stack_top()->token = yy_map_token[yytok];
		yy_stack_top()->value = yylval;
	}
	return yy_stack_top()->token;
}

int	yy_shift(int state)
{
	return yy_stack_push((yy_stack_type){state, -1});
}

)";
}

void Generator::put_cpp_tables(std::ostream &out) const {
	out << "int yy_map_token[] = ";
	put_array(out, this->lalr.get_token_map());
	out << ";" << std::endl;
	out << "int yy_reduce_table[] = ";
	put_array(out, this->lalr.get_reduce_table());
	out << ";" << std::endl;
	out << "int yy_reduce_tokens[] = ";
	put_array(out, this->lalr.get_reduce_tokens());
	out << ";" << std::endl;
	out << std::endl;
	out << "int yy_terminals_limit = " << this->lalr.get_terminals_limit() << ";" << std::endl;
	out << std::endl;
	auto table = this->lalr.get_table();
	out << "yy_state_t yy_yacc_transitions[][" << table[0].size() << "] = {" << std::endl;
	for (auto &s : table)
	{
		put_array(out, s);
		out << ", " << std::endl;
	}
	out << "};" << std::endl;
	out << std::endl;
}

template<typename T>
void Generator::put_array(std::ostream &out, const std::vector<T> &array) const {
	out << "{";
	for (const auto &e : array)
		out << e << ", ";
	out << "}";
}

void Generator::put_yyparse(std::ostream &out) const {
	out << R"(
int yyinternal_yyparse(void)
{
	int yy_error_state = 0;
	yy_shift(0);
	while (yy_stack_top()->state != YY_ACCEPT)
	{
		int yytok;
#ifdef YYDEBUG
		if (yydebug)
		{
			fprintf(stderr, "state: %d\n", yy_stack_top()->state);
			if (yy_yacc_transitions[yy_stack_top()->state][YY_EPSILON] >= 0)
				fprintf(stderr, "lookahead: %d\n", yy_lookahead());
		}
#endif
		if (yy_yacc_transitions[yy_stack_top()->state][YY_EPSILON] < 0) // epsilon reduce
		{
			yytok = YY_EPSILON;
			goto yy_reduce;
		}
		else if (yy_lookahead() > 0 && yy_yacc_transitions[yy_stack_top()->state][yy_lookahead()] > 0) // shift
		{
#ifdef YYDEBUG
			if (yydebug)
				fprintf(stderr, "shift and go to %d\n", yy_yacc_transitions[yy_stack_top()->state][yy_lookahead()]);
#endif
			int yystate = yy_yacc_transitions[yy_stack_top()->state][yy_lookahead()];
			yy_shift(yystate);
			if (YYRECOVERING)
				yy_error_state--;
		}
		else if (yy_lookahead() > 0 && yy_yacc_transitions[yy_stack_top()->state][yy_lookahead()] < 0) // reduce
		{
			yytok = yy_lookahead();
			yy_reduce:;
#ifdef YYDEBUG
			if (yydebug)
				fprintf(stderr, "reduce with rule %d\n", -yy_yacc_transitions[yy_stack_top()->state][yytok]);
#endif
			int yy_rule = -yy_yacc_transitions[yy_stack_top()->state][yytok];
			int yy_lookahead = yy_stack_top()->token;
			yy_stack_pop(yy_reduce_table[-yy_yacc_transitions[yy_stack_top()->state][yytok]]);
			switch (yy_rule)
			{
)";
	auto action = this->lalr.get_actions();
	for (size_t i = 0; i < action.size(); i++)
		out << "case " << i << ":" << std::endl
			<< action[i] << "break;" << std::endl;
	out << R"(
			}
			yy_end_of_semantic_action:;
			yy_stack_top()->token = yy_reduce_tokens[yy_rule];
			yy_stack_top()->value = yylval;
#ifdef YYDEBUG
			if (yydebug)
				fprintf(stderr, "goto %d\n", yy_yacc_transitions[yy_stack_top()->state][yy_stack_top()->token]);
#endif
			yy_stack_push((yy_stack_type){ yy_yacc_transitions[yy_stack_top()->state][yy_stack_top()->token], yy_lookahead });
			if (yy_error_state == 4) // special value, set by YYERROR
			{
				yy_error_state = 0;
				goto yy_error_lbl;
			}
		}
		else // error
		{
			if (!YYRECOVERING
#ifdef YYDEBUG
			|| yydebug
#endif
			)
				yyerror("syntax error");
			yy_error_lbl:;
			if (yy_error_state == 3)
			{
				if (yy_lookahead() == YY_END)
					return 1;
				yy_stack_top()->token = -1;
#ifdef YYDEBUG
				if (yydebug)
					fprintf(stderr, "discard\n");
#endif
			}
//			else
//			{
				int save_lookahead = yy_stack_top()->token;
				YYSTYPE save_lookahead_value = yy_stack_top()->value;
				while (yy_stack_size > 0 &&	yy_yacc_transitions[yy_stack_top()->state][yy_map_token[error]] <= 0)
					yy_stack_pop(1);
				if (yy_stack_size == 0)
				{
#ifdef YYDEBUG
					if (yydebug)
						fprintf(stderr, "stack is empty, stop the parsing\n");
#endif
					return 1;
				}
				yy_shift(yy_yacc_transitions[yy_stack_top()->state][yy_map_token[error]]);
#ifdef YYDEBUG
				if (yydebug)
					fprintf(stderr, "shift error token\n");
#endif
				yy_stack_top()->token = save_lookahead;
				yy_stack_top()->value = save_lookahead_value;
				yy_error_state = 3;
//			}
		}
	}
#ifdef YYDEBUG
	if (yydebug)
		fprintf(stderr, "accept\n");
#endif
	return 0;
}

int yyparse(void)
{
	yy_stack = NULL;
	yy_stack_size = 0;
	yy_stack_cap = 0;
	int ret = yyinternal_yyparse();
	free(yy_stack);
	return ret;
}
)";
}