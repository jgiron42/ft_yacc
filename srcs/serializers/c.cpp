#include "c.hpp"
#include <regex>

CSerializer::CSerializer(Parser::Config &c, LALR &l) : config(c), lalr(l) {}

void CSerializer::generate() {
	this->build();

	this->generator.generate(SKELETONS_PATHS "/c.skl",
			commandLine.root + commandLine.file_prefix + ".tab.c");
	if (commandLine.write_header)
		this->generator.generate(SKELETONS_PATHS "/h.skl",
			commandLine.root + commandLine.file_prefix + ".tab.h");
}

void CSerializer::build() {
	std::string defines;
	if (commandLine.debug_defines)
		defines +=  "#define YYDEBUG 1\n";
	generator.set("DEFINES", defines);

	generator.set("YYSTYPE", this->serialize_yystype());

	generator.set("SYM_PREFIX", commandLine.sym_prefix);

	std::string token_defs;
	for (auto &token: lalr.get_token_definitions())
		token_defs += token.first + " = " + std::to_string(token.second) + ",\n";

	generator.set("TOKEN_DEFS", token_defs);

	generator.set("SYMBOL_DEFS", serialize_sym_def());

	generator.set("HEADER_CODE", this->config.header_code);

	generator.set("ACCEPT_STATE", std::to_string(this->lalr.get_accept()));

	generator.set("MAP_TOKEN_TABLE", this->serialize_array(this->lalr.get_token_map()));

	generator.set("MAP_TOKEN_SIZE", std::to_string(this->lalr.get_token_map().size()));

	generator.set("REDUCE_TABLE", this->serialize_array(this->lalr.get_reduce_table()));

	generator.set("REDUCE_TOKENS_TABLE", this->serialize_array(this->lalr.get_reduce_tokens()));

	size_t transition_table_size;
	generator.set("TRANSITION_TABLE", this->serialize_transitions(transition_table_size));
	generator.set("TRANSITION_TABLE_SIZE", std::to_string(transition_table_size));

	generator.set("RULES", this->serialize_rules());

	generator.set("TAIL_CODE", config.tail_code);
}

std::string CSerializer::serialize_sym_def() const {
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

std::string CSerializer::serialize_rules() {
	std::string ret;
	auto rules = this->lalr.get_rules();
	for (size_t i = 0; i < rules.size(); i++)
		ret += "case " + std::to_string(i) + ":\n"
			+ this->substitute_action(rules[i]) + "goto yy_end_of_semantic_action;\n";
	return ret;
}


std::string CSerializer::substitute_action(const LALR::Rule &rule) {
	static std::regex comments_and_strings("(/\\*([^*]|\\*[^/])*\\*/|\"(\\\\([^\n]|[0-7]{1,3}|x[[:xdigit:]]{1,2})|[^\n\"\\\\])*\")", std::regex_constants::extended);
	static std::regex pseudo_var("\\$(<([^>]+)>)?(\\$|-?[[:digit:]]+)", std::regex_constants::extended);
	std::string ret;
	for (int i = 0; i < rule.action.size(); i++)
	{
		std::match_results<std::string::const_iterator> mr;
		std::string s2(rule.action.substr(i));
		if (std::regex_search(s2, mr, comments_and_strings, std::regex_constants::match_continuous)) {
			ret.append(mr[0]);
			i += mr[0].str().size();
		}
		if (std::regex_search(s2, mr, pseudo_var, std::regex_constants::match_continuous))
		{
			std::string type;
			std::string access;
			std::optional<int>	offset = mr[3].str()[0] == '$' ? std::nullopt : std::optional<int>(atoi(mr[3].str().c_str()));
			if (offset == std::nullopt)
				access = "yylval";
			else if (rule.symbol[0] == '@')
				access = "YY_STACK_ELEMENT(" + std::to_string(offset.value() - rule.real_syntax.size()) + ")";
			else
				access = "YY_STACK_ELEMENT(" + std::to_string(offset.value()) + ")";
			if (commandLine.language == "zig")
				access = "self." + access;
			if (config.union_enabled || config.variant_enabled || commandLine.language == "zig")
			{
				std::string type;
				if (mr[2].matched)
				{
					static int type_id = 0;
					// todo
					const auto ret = this->types.insert({mr[2].str(), type_id++});
					type = mr[2].str();
				}
				else if (mr[3].str()[0] == '$' && this->config.token_types.contains(rule.symbol))
					type = this->config.token_types.find(rule.symbol)->second;
				else if (int n = atoi(mr[3].str().c_str()); mr[3].str()[0] != '$' && n > 0 && n <= rule.real_syntax.size() &&
															this->config.token_types.contains(rule.real_syntax[n - 1]))
					type = this->config.token_types.find(rule.real_syntax[n - 1])->second;
				else
					throw std::runtime_error(mr.str() + " of `" + rule.symbol + "` has no declared type");
				access = "YY_ACCESS_TYPE(" + access + ", " + type + ")";
			}
			ret.append(access);
			i += mr[0].str().size() - 1;
		}
		else
			ret.push_back(rule.action[i]);
	}
	return ret;
}

std::string CSerializer::serialize_yystype() const {
	if (this->config.union_enabled)
		return "union " + this->config.stack_type;
	else
		return "int";
}

std::string CSerializer::serialize_transitions(size_t &size) const {
	auto table = this->lalr.get_table();
	size = table.front().size();
	std::string ret;
	ret += "{\n";
	bool is_first = true;
	for (auto &s : table)
	{
		if (!is_first)
			ret += ", \n";
		ret += serialize_array(s);
		is_first = false;
	}
	ret += '}';
	return ret;
}

template<typename T>
std::string CSerializer::serialize_array(const std::vector<T> &array) const {
	std::string ret;
	ret += "{";
	bool is_first = true;
	for (const auto &e : array)
	{
		if (!is_first)
			ret += ", \n";
		ret += std::to_string(e);
		is_first = false;
	}
	ret += "}";
	return ret;
}
