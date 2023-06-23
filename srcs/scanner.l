%pointer
%s	RULES READ_CODE
%x	TAIL_SECTION SNIPPET COMMENT

DIGIT    		[[:digit:]]
C_IDENTIFIER	[[:alpha:]_][[:alnum:]_]*
LITERAL			'([^\\']|\\.|\\[0-7]{1,3}|\\x[[:xdigit:]]{1,2})'
STRING			\"([^"\]|\\.)*\"

%{
	#include <regex>
	#include <string.h>
	#include "Scanner.hpp"
	int is_balanced(char *s);
	char *strappend(char *s1, char *s2);
	int lineno = 1;
%}

%%

%{
static char *code_buf = NULL;
static int	old_state = -1;
%}


<INITIAL>%%[[:blank:]]*/\n BEGIN RULES;return Scanner::token::MARK;

<RULES>"%%"[[:blank:]]*/\n BEGIN TAIL_SECTION;return Scanner::token::MARK;

<INITIAL>"%{" BEGIN SNIPPET;

<SNIPPET>%}[[:blank:]]*/\n BEGIN INITIAL;

<SNIPPET>.*\n  lineno++;return Scanner::token::HEADER_CODE;

<INITIAL>%left return Scanner::token::LEFT;

<INITIAL>%right return Scanner::token::RIGHT;

<INITIAL>%variant return Scanner::token::VARIANT;

<INITIAL>%use_cpp_lex return Scanner::token::USE_CPP_LEX;

<INITIAL>{STRING} return Scanner::token::FILENAME;

<INITIAL>%type return Scanner::token::TYPE;

<INITIAL>%nonassoc return Scanner::token::NONASSOC;

<INITIAL>%token return Scanner::token::TOKEN;

<RULES>%prec return Scanner::token::PREC;

<INITIAL>%start return Scanner::token::START;

<INITIAL>%union return Scanner::token::UNION;

<RULES>{C_IDENTIFIER}/[[:space:]]*: return Scanner::token::C_IDENTIFIER;

<RULES,INITIAL>{LITERAL}		|
<RULES,INITIAL>{C_IDENTIFIER}	return Scanner::token::IDENTIFIER;

"<"[^>]+">" {
	yytext[yyleng-1] = 0;
	yytext++;
	return Scanner::token::TAG;
}

<RULES,INITIAL>{DIGIT}+	return Scanner::token::NUMBER;

<READ_CODE>[^\n}]*[\n}] |
\{[^\n}]*[\n}] {
	if (yytext[yyleng] == '\n')
		lineno++;
	code_buf = strappend(code_buf, yytext);
	if (is_balanced(code_buf))
	{
		yytext = strdup(code_buf);
		code_buf[0] = 0;
		if (old_state != -1)
		{
			BEGIN old_state;
			old_state = -1;
		}
		return Scanner::token::ACTION;
	}
	else
	{
		if (old_state == -1)
			old_state = CURRENT_START_CONDITION;
		BEGIN READ_CODE;
	}
}

<TAIL_SECTION>(.|\n)*	return Scanner::token::TAIL;

\/\*([^*]|\*[^/])*\*\/	for (char *s = strchr(yytext, '\n'); s; s = strchr(s + 1, '\n'))lineno++;

[[:blank:]\f] ;

\n	lineno++;

.	return yytext[0];

%%

int is_balanced(char *s)
{
	static int init = 0;
	static std::regex string_and_comment("(/\\*([^*]|\\*[^/])*\\*/|\"(\\\\([^\n]|[0-7]{1,3}|x[[:xdigit:]]{1,2})|[^\n\"\\\\])*\")", std::regex_constants::extended);
	static std::regex partial_string_and_comment("(/\\*([^*]|\\*[^/])*|\"(\\\\([^\n]|[0-7]{1,3}|x[[:xdigit:]]{1,2})|[^\n\"\\\\])*)$", std::regex_constants::extended);

	int bracket = 0;
	for (int i = 0; s[i]; i++)
	{
		if (s[i] == '{')
			bracket++;
		if (s[i] == '}')
			bracket--;
		std::cmatch mr;
		if (std::regex_search(s + i, mr, string_and_comment, std::regex_constants::match_continuous))
			i += mr[0].str().size() - 1;
		else if (std::regex_search(s + i, mr, partial_string_and_comment, std::regex_constants::match_continuous))
			return 0;
	}
	return bracket == 0;
}

char *strappend(char *s1, char *s2)
{
	if (!s1)
		return strdup(s2);
	s1 = (char*)realloc(s1, strlen(s1) + strlen(s2) + 1);
	return (strcat(s1, s2));
}