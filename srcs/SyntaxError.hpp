#ifndef FT_YACC_POC_SYNTAXERROR_HPP
#define FT_YACC_POC_SYNTAXERROR_HPP
#include <string>
#include <map>
#include "Scanner.hpp"

struct SyntaxError {
	enum Severity {
		Notice,
		Warning,
		Error
	};

	std::string	file;
	int			line;
	std::string	description;
	Severity	severity;

	SyntaxError(std::string f, int l, std::string d, Severity s);
	operator std::string() const;
};


#endif
