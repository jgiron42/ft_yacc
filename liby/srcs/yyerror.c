#include <stdio.h>

int  yyerror(const char *s)
{
	return fprintf(stderr, "%s\n", s);
}