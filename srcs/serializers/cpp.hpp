#ifndef FT_YACC_POC_CPPSERIALIZER_HPP
#define FT_YACC_POC_CPPSERIALIZER_HPP
#include "LALR.hpp"
#include "Parser.hpp"
#include <ostream>
#include "CommandLine.hpp"
#include "Generator.hpp"
#include "../Serializer.hpp"

class CppSerializer : public Serializer {
private:
	const LALR				&lalr;
	const Parser::Config	&config;
	Generator				generator;
	std::map<std::string, int>			types; // for c++ variants and zig unions
public:
	CppSerializer(Parser::Config &, LALR &);
	virtual void generate();
private:
	void build();
	std::string serialize_sym_def() const;
	std::string serialize_variant() const;
	std::string serialize_zig_union() const;
	std::string serialize_yystype() const;
	std::string serialize_rules();
	std::string serialize_transitions(size_t &size) const;
	std::string substitute_action(const LALR::Rule &rule);
	template <typename T>
	std::string serialize_array(const std::vector<T> &array) const;
};


#endif
