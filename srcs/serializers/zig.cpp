#include "zig.hpp"
#include <regex>

ZigSerializer::ZigSerializer(Parser::Config &c, LALR &l) : config(c), lalr(l) {}

void ZigSerializer::generate() {
	this->build();
	this->generator.generate(SKELETONS_PATHS "/zig.skl",
			commandLine.root + commandLine.file_prefix + ".tab.zig");
}

void ZigSerializer::build() {

	std::string token_defs;
	token_defs += "enum(usize) {\n";
	for (auto &token: lalr.get_token_definitions())
		token_defs += token.first + " = " + std::to_string(token.second) + ",\n";
	token_defs += "}";
	generator.set("TOKEN_DEFS", token_defs);

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

	generator.set("YYSTYPE", this->serialize_yystype());

	generator.set("TAIL_CODE", config.tail_code);
}

std::string ZigSerializer::serialize_rules() {
	std::string ret;
	auto rules = this->lalr.get_rules();
	for (size_t i = 0; i < rules.size(); i++)
		ret += std::to_string(i) + "=> {\n" + this->substitute_action(rules[i]) + "\n},\n";
	return ret;
}

std::string ZigSerializer::substitute_action(const LALR::Rule &rule) {
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
				access = "self.yylval";
			else if (rule.symbol[0] == '@')
				access = "self.YY_STACK_ELEMENT(" + std::to_string(offset.value() - (int)rule.real_syntax.size()) + ")";
			else
				access = "self.YY_STACK_ELEMENT(" + std::to_string(offset.value()) + ")";
			if (config.union_enabled || config.variant_enabled || commandLine.language == "zig")
			{
				std::string type;
				if (mr[2].matched)
				{
					static int type_id = 0;
					const auto ret = this->types.insert({mr[2].str(), type_id++});
						type = "yy_anonymous_type_" + std::to_string(ret.first->second);
				}
				else if (mr[3].str()[0] == '$' && this->config.token_types.contains(rule.symbol))
					type = this->config.token_types.find(rule.symbol)->second;
				else if (int n = atoi(mr[3].str().c_str()); mr[3].str()[0] != '$' && n > 0 && n <= rule.real_syntax.size() &&
															this->config.token_types.contains(rule.real_syntax[n - 1]))
					type = this->config.token_types.find(rule.real_syntax[n - 1])->second;
				else
					throw std::runtime_error(mr.str() + " of `" + rule.symbol + "` has no declared type");
				access = "(" + access + ").value.@\"" + type + "\"";
			}
			ret.append(access);
			i += mr[0].str().size() - 1;
		}
		else
			ret.push_back(rule.action[i]);
	}
	return ret;
}

std::string ZigSerializer::serialize_yystype() const {
//	if (this->config.variant_enabled)
//		return this->serialize_variant();
	if (this->config.union_enabled)
		return "union " + this->config.stack_type;
	return "Token";
}

std::string ZigSerializer::serialize_zig_union() const {
	std::string ret = "struct {\nvalue : extern union {\n";
	for (auto &t : this->lalr.get_token_definitions()) {
		const auto it = this->config.token_types.find(t.first);
		if (it != this->config.token_types.end()) {
			ret += t.first + ": " + it->second + ",\n";
		}
//		else {
//			ret += t.first + ",\n";
//		}
	}
	for (auto &t : this->types) {
		ret += "yy_anonymous_type_" + std::to_string(t.second) + ": " + t.first + ",\n";
	}
	ret += "},\n}";
	return ret;
}

std::string ZigSerializer::serialize_transitions(size_t &size) const {
	auto table = this->lalr.get_table();
	size = table.front().size();
	std::string ret;
	ret += "[_][" + std::to_string(size) + "]isize{\n";
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
std::string ZigSerializer::serialize_array(const std::vector<T> &array) const {
	std::string ret;
	ret += "[_]isize{\n";
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