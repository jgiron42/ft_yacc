#include "Scanner.hpp"
#include <stdio.h>
#include <stdexcept>
#include <cstring>
#include <iostream>

int  		yylex(void);
extern char *yytext;
extern FILE *yyin;
extern int yyleng;
extern int lineno;

int  yywrap(void) {return 1;}

Scanner::Scanner(const std::string &f) : filename(f) {
	yyin = fopen(filename.c_str(), "r");
	if (!yyin)
		throw std::runtime_error(strerror(errno));
}

void Scanner::scan() {
	int t;
	while((t = yylex()))
		this->tokens.emplace_back(token{t, yytext, filename, lineno - (int)std::count(yytext, yytext + yyleng, '\n')});
	this->tokens.emplace_back(token{token::END, "end of file", filename, lineno});
}

std::list<Scanner::token> Scanner::get_tokens() {return this->tokens;}
