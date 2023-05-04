#ifndef FT_YACC_POC_SERIALIZER_HPP
#define FT_YACC_POC_SERIALIZER_HPP
#include "LALR.hpp"
#include "Parser.hpp"
#include <ostream>
#include "CommandLine.hpp"
#include "Generator.hpp"

class Serializer {
private:
	const LALR				&lalr;
	const CommandLine		&commandLine;
	const Parser::Config	&config;
	Generator				generator;
public:
	Serializer(Parser::Config &, CommandLine &, LALR &);
	void build();
	Generator &get_generator();
private:
	std::string serialize_sym_def() const;
	std::string serialize_variant() const;
	std::string serialize_yystype() const;
	std::string serialize_rules() const;
	std::string serialize_transitions(size_t &size) const;
	template <typename T>
	std::string serialize_array(const std::vector<T> &array) const;
};


#endif
