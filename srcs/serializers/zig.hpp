#ifndef FT_YACC_POC_ZIGSERIALIZER_HPP
#define FT_YACC_POC_ZIGSERIALIZER_HPP
#include "LALR.hpp"
#include "Parser.hpp"
#include <ostream>
#include "CommandLine.hpp"
#include "Generator.hpp"

class ZigSerializer {
private:
	const LALR				&lalr;
	const Parser::Config	&config;
	Generator				generator;
	std::map<std::string, int>			types; // for c++ variants and zig unions
public:
	ZigSerializer(Parser::Config &, LALR &);
	virtual void generate();
private:
	void build();
	std::string serialize_zig_union() const;
	std::string serialize_yystype() const;
	std::string serialize_rules();
	std::string serialize_config() const;
	std::string serialize_transitions(size_t &size) const;
	std::string serialize_variant() const;
	std::string substitute_action(const LALR::Rule &rule);
	template <typename T>
	std::string serialize_array(const std::vector<T> &array) const;
};


#endif
