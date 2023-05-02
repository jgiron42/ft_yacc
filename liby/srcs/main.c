#include <stdio.h>
int yyparse(void);
extern FILE *yyin;

int main()
{
	yyin = stdin;
	yyparse();
}