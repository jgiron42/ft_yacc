#ifndef FT_YACC_POC_GENERATOR_HPP
#define FT_YACC_POC_GENERATOR_HPP
#include "LALR.hpp"
#include "Parser.hpp"
#include <ostream>
#include "CommandLine.hpp"

class Generator {
private:
	const LALR				&lalr;
	const CommandLine		&commandLine;
	const Parser::Config	&config;
public:
	Generator(Parser::Config &, CommandLine &, LALR &);
	void generate_c(std::ostream &) const;
	void generate_h(std::ostream &) const;
private:
	void put_cpp_headers(std::ostream &) const;
	void put_common_headers(std::ostream &) const;
	void put_cpp_routines(std::ostream &) const;
	void put_yyparse(std::ostream &) const;
	void put_cpp_tables(std::ostream &) const;
	static std::string substitute(const std::string &) ;
	template <typename T>
	void put_array(std::ostream &, const std::vector<T> &) const;
};


#endif
