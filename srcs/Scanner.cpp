#include "Scanner.hpp"
#include <algorithm>

extern int lineno;

int  yywrap(void) {return 1;}

Scanner::Scanner(const std::string &f) : filename(f), yyLexer(&stream), stream(f) {}

void Scanner::scan() {
	int t;
	while((t = yylex()))
		this->tokens.emplace_back(token{t, yytext, filename, lineno - (int)std::count(yytext, yytext + yyleng, '\n')});
	this->tokens.emplace_back(token{token::END, "end of file", filename, lineno});
}

std::list<Scanner::token> Scanner::get_tokens() {return this->tokens;}
