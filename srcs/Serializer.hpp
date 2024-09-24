#ifndef FT_YACC_POC_SERIALIZER_HPP
#define FT_YACC_POC_SERIALIZER_HPP
#include "LALR.hpp"
#include "Parser.hpp"
#include <ostream>
#include "CommandLine.hpp"
#include "Generator.hpp"

class Serializer {
public:
//	Serializer(Parser::Config &, LALR &);
	virtual void generate() = 0;
};


#endif
