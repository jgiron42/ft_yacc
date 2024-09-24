#include "Scanner.hpp"
#include <algorithm>

extern int lineno;

int  Scanner::yywrap(void) {
	if (this->stream_queue.empty())
		return 1;
	else {
		this->in = this->stream_queue.front();
		this->stream_queue.pop();
		return 0;
	}
}

Scanner::Scanner(const std::string &f) : filename(f), yyLexer(&stream), stream(f) {}

Scanner::token Scanner::next() {
	if (auto t= yylex())
		return token{t, yytext, filename, lineno - (int)std::count(yytext, yytext + yyleng, '\n')};
	else
		return token{token::END, "end of file", filename, lineno};
}

void Scanner::scan() {
	int t;
	while((t = yylex()))
		this->tokens.emplace_back(token{t, yytext, filename, lineno - (int)std::count(yytext, yytext + yyleng, '\n')});
	this->tokens.emplace_back(token{token::END, "end of file", filename, lineno});
}

std::list<Scanner::token> Scanner::get_tokens() {return this->tokens;}

void	Scanner::append_stream(std::istream * s) {
	this->stream_queue.push(s);
}
