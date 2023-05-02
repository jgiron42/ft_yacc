#include "utils.hpp"

int	parse_literal(const std::string &s)
{
	if (s[0] != '\'')
		return -1;
	if (s[1] != '\\')
		return s[1];
	if (s[2] == 'x')
	{
		int n = 0;
		for (int i = 3;s[i]; i++)
		{
			n *= 16;
			if (isdigit(s[i]))
				n += s[i] - '0';
			else if (islower(s[i]))
				n += s[i] - 'a';
			else if (isupper(s[i]))
				n += s[i] - 'A';
		}
		return n;
	}
	else if (s[2] >= '0' && s[2] <= '8')
	{
		int n = 0;
		for (int i = 2;s[i]; i++)
			n = n * 8 + s[i] - '0';
		return n;
	}
	else switch (s[2])
		{
			case '\\':
				return '\\';
			case 'a':
				return '\a';
			case 'b':
				return '\b';
			case 'f':
				return '\f';
			case 'n':
				return '\n';
			case 't':
				return '\t';
			case 'r':
				return '\r';
			case 'v':
				return '\v';
			default:
				return s[1];
		}
}