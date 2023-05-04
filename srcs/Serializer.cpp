#include "Serializer.hpp"
#include <regex>

Serializer::Serializer(Parser::Config &c, CommandLine &cl, LALR &l) : config(c), commandLine(cl), lalr(l) {}

Generator &Serializer::get_generator() {
	return this->generator;
}

void Serializer::build() {
	std::string defines;
	if (commandLine.debug_defines)
		defines +=  "#define YYDEBUG 1\n";
	if (config.variant_enabled)
		defines +=  "#define YY_COMPLETE_SYMBOL 1\n";
	if (config.use_cpp_lex)
	{
		defines += "#define YY_USE_CPP_LEX 1\n";
		if (config.lex_header.empty())
			generator.set("LEX_HEADER", "lex.yy.hpp");
		else
			generator.set("LEX_HEADER", this->config.lex_header);
	}
	generator.set("DEFINES", defines);


	generator.set("YYSTYPE", this->serialize_yystype());

	generator.set("SYM_PREFIX", this->commandLine.sym_prefix);

	std::string token_defs;
	for (auto &token : lalr.get_token_definitions())
		token_defs += token.first + " = " + std::to_string(token.second) + ",\n";
	generator.set("TOKEN_DEFS", token_defs);

	generator.set("SYMBOL_DEFS", serialize_sym_def());

	generator.set("HEADER_CODE", this->config.header_code);

	generator.set("ACCEPT_STATE", std::to_string(this->lalr.get_accept()));

	generator.set("MAP_TOKEN_TABLE", this->serialize_array(this->lalr.get_token_map()));

	generator.set("REDUCE_TABLE", this->serialize_array(this->lalr.get_reduce_table()));

	generator.set("REDUCE_TOKENS_TABLE", this->serialize_array(this->lalr.get_reduce_tokens()));


	size_t transition_table_size;
	generator.set("TRANSITION_TABLE", this->serialize_transitions(transition_table_size));
	generator.set("TRANSITION_TABLE_SIZE", std::to_string(transition_table_size));

	generator.set("RULES", this->serialize_rules());

	generator.set("TAIL_CODE", config.tail_code);
}

std::string Serializer::serialize_sym_def() const {
	std::string ret;
	if (commandLine.sym_prefix != "yy") {
		auto syms =
				{
						"_state_t",
						"_stack_type",
						"_stack",
						"_stack_size",
						"_stack_cap",
						"lval",
						"_lookahead",
						"_rule",
						"parse",
						"internal_yyparse",
						"_yacc_transitions",
						"_terminals_limit",
						"_reduce_tokens",
						"_reduce_table",
						"_map_token",
						"_reduce",
						"_shift",
						"_stack_top",
						"_stack_pop",
						"_stack_push",
						"lex",
						"in"
				};
		for (std::string sym: syms)
			ret += "#define yy" + sym + " " + commandLine.sym_prefix + sym + '\n';
	}
	return ret;
}


std::string Serializer::serialize_rules() const {
	std::string ret;
	auto action = this->lalr.get_actions();
	for (size_t i = 0; i < action.size(); i++)
		ret += "case " + std::to_string(i) + ":\n"
			+ action[i] + "break;\n";

	return ret;
}

std::string Serializer::serialize_yystype() const {
	if (this->config.variant_enabled)
		return this->serialize_variant();
	else if (this->config.union_enabled)
		return this->config.stack_type;
	return "int";
}

std::string Serializer::serialize_variant() const {
	std::string ret = "std::monostate";
	std::set<std::string> types = this->config.types;
	for (auto &p : this->config.token_types)
		types.insert(p.second);
	for (auto &t : types)
		ret += ", " + t;
	return "std::variant<" + ret + ">";
}

std::string Serializer::serialize_transitions(size_t &size) const {
	auto table = this->lalr.get_table();
	size = table.front().size();
	std::string ret;
	ret += "{\n";
	for (auto &s : table)
		ret += serialize_array(s) + ", \n";
	ret += '}';
	return ret;
}

template<typename T>
std::string Serializer::serialize_array(const std::vector<T> &array) const {
	std::string ret = "{";
	for (const auto &e : array)
		ret += std::to_string(e) + ", ";
	ret += "}";
	return ret;
}
