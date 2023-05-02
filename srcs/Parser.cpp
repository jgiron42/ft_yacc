#include "Parser.hpp"
#include <iostream>
#include <regex>

Parser::Parser(std::list<Scanner::token> l) : tokens(l), config({}) {}

bool Parser::parse() {
	this->begin();
	try {
		this->parse_def();
		this->parse_rules();
		this->parse_tail();
	} catch (UnexpectedToken & e) {
		std::cerr << e.what() << std::endl;
		return true;
	}
	return do_diagnostic();
}

void Parser::parse_def() {
	while (!accept(Scanner::token::MARK))
	{
		if (accept(Scanner::token::START))
		{
			auto t = expect(Scanner::token::IDENTIFIER);
			if (lalr.is_terminal(t.value))
				this->add_error(t.file, t.line, "the start symbol" + t.value + " cannot be declared to be a token", SyntaxError::Error);
			this->config.start = t.value;
		}
		else if (auto u = accept(Scanner::token::UNION))
		{
			this->config.stack_type = "union " + expect(Scanner::token::ACTION).value;
			this->config.union_enabled = true;
		}
		else if (auto c = accept(Scanner::token::HEADER_CODE))
			this->config.header_code.append(c->value);
		else if (auto rword = accept({Scanner::token::TOKEN, Scanner::token::RIGHT, Scanner::token::LEFT, Scanner::token::NONASSOC}))
		{
			static int precedence = 0;
			++precedence;
			LALR::Token::Associativity associativity;
			switch (rword->type)
			{
				case Scanner::token::TOKEN:
					associativity = LALR::Token::Associativity::DEFAULT;
					break;
				case Scanner::token::RIGHT:
					associativity = LALR::Token::Associativity::RIGHT;
					break;
				case Scanner::token::LEFT:
					associativity = LALR::Token::Associativity::LEFT;
					break;
				case Scanner::token::NONASSOC:
					associativity = LALR::Token::Associativity::NONASSOC;
					break;
			}
			auto type = get_tag();
			std::optional<Scanner::token> name_tok = expect(Scanner::token::IDENTIFIER);
			do {
				int value;
				if (auto v = accept(Scanner::token::NUMBER))
					value = std::stoi(v->value);
				else
					value = this->lalr.generate_token_value();
				if (this->config.start == name_tok->value)
					this->add_error(name_tok->file, name_tok->line, "the start symbol " + name_tok->value + " is a token", SyntaxError::Error);
				try {
					this->lalr.add_token(name_tok->value, {true, value, precedence, associativity});
				} catch (std::exception &e) {
					this->add_error(name_tok->file, name_tok->line, e.what(), SyntaxError::Error);
				}
				if (type)
					this->config.token_types[name_tok->value] = type.value();
			} while ((name_tok = accept(Scanner::token::IDENTIFIER)).has_value());
		}
		else if (accept({Scanner::token::TYPE}))
		{
			auto type = get_tag(true);
			std::optional<Scanner::token> name_tok = expect(Scanner::token::IDENTIFIER);
			do this->config.token_types[name_tok->value] = type.value();
			while ((name_tok = accept(Scanner::token::IDENTIFIER)));
		}
		else
			unexpected();
	}
}

void Parser::parse_rules() {
	if (auto tok = accept({Scanner::token::MARK, Scanner::token::END}))
		this->add_error(tok->file, tok->line, "no grammar has been specified", SyntaxError::Error);
	while (!accept({Scanner::token::MARK, Scanner::token::END}))
	{
		auto symbol = expect(Scanner::token::C_IDENTIFIER);
		if (config.start.empty())
			config.start = symbol.value;
		if (lalr.is_terminal(symbol.value))
			this->add_error(symbol.file, symbol.line, "a token appears on the left side of a production", SyntaxError::Error);
		this->lalr.add_token(symbol.value, {false, -1, LALR::Token::DEFAULT}); // add the token to the list of token
		this->lalr.add_token_mapping(symbol.value);
		expect(static_cast<Scanner::token::token_type>(':'));
		do {
			LALR::Rule r;
			r.symbol = symbol.value;
			int line = this->peak().line;
			std::string file = this->peak().file;
			try {
				while (auto component = accept({Scanner::token::IDENTIFIER, Scanner::token::ACTION}))
					if (component->type == Scanner::token::IDENTIFIER)
						r.syntax.push_back(component->value);
					else if (is_of_type({Scanner::token::IDENTIFIER, Scanner::token::ACTION}))
						r.syntax.push_back(get_action_rule(component->value, r.syntax));
					else
						r.action = component->value;
				if (accept(Scanner::token::PREC))
				{
					auto t = expect(Scanner::token::IDENTIFIER);
					if (!lalr.is_terminal(t.value))
						this->add_error(t.file, t.line, "nonterminal " + t.value + " after %prec", SyntaxError::Error);
					r.precedence = t.value;
				}
				if (auto component = accept(Scanner::token::ACTION))
				{
					if (!r.action.empty())
						r.syntax.push_back(get_action_rule(r.action, r.syntax));
					r.action = component->value;
				}
				r.action = substitute_action(r.symbol, r.syntax, r.action);
				this->lalr.add_rule(LALR::Rule(r));
			} catch (std::exception &e) {
				this->add_error(file, line, e.what(), SyntaxError::Error);
			}
		} while (accept(static_cast<Scanner::token::token_type>('|')));
		expect(static_cast<Scanner::token::token_type>(';'));
	}
}

std::string Parser::substitute_action(const std::string & symbol, std::vector<std::string> syntax, const std::string &action) {
	static std::regex comments_and_strings("(/\\*([^*]|\\*[^/])*\\*/|\"(\\\\([^\n]|[0-7]{1,3}|x[[:xdigit:]]{1,2})|[^\n\"\\\\])*\")", std::regex_constants::extended);
	static std::regex pseudo_var("\\$(<([[:alpha:]_][[:alnum:]_]*)>)?(\\$|-?[[:digit:]]+)", std::regex_constants::extended);
	std::string ret;
	for (int i = 0; i < action.size(); i++)
	{
		std::match_results<std::string::const_iterator> mr;
		std::string s2(action.substr(i));
		if (std::regex_search(s2, mr, comments_and_strings, std::regex_constants::match_continuous)) {
			ret.append(mr[0]);
			i += mr[0].str().size();
		}
		if (std::regex_search(s2, mr, pseudo_var, std::regex_constants::match_continuous))
		{
			std::string type;
			if (mr[3].str()[0] == '$')
				ret.append("yylval");
			else if (symbol[0] == '@')
				ret.append("YY_STACK_ELEMENT(" + std::to_string(syntax.size()) + "-" + mr[3].str() + ")");
			else
				ret.append("YY_STACK_ELEMENT(" + mr[3].str() + ")");
			if (config.union_enabled)
			{
				if (mr[2].matched)
					ret.append("." + mr[2].str());
				else if (mr[3].str()[0] == '$' && this->config.token_types.contains(symbol))
					ret.append("." + this->config.token_types[symbol]);
				else if (int n = atoi(mr[3].str().c_str()); mr[3].str()[0] != '$' && n > 0 && n <= syntax.size() &&
															this->config.token_types.contains(syntax[n - 1]))
					ret.append("." + this->config.token_types[syntax[n - 1]]);
				else
					throw std::runtime_error(mr.str() + " of `" + symbol + "` has no declared type");
			}
			i += mr[0].str().size() - 1;
		}
		else
			ret.push_back(action[i]);
	}
	return ret;
}

void Parser::parse_tail() {
	if (auto tail = accept(Scanner::token::TAIL))
		this->config.tail_code = tail->value;
}

LALR Parser::get_LALR()
{
	return this->lalr;
}

Parser::Config &Parser::get_config() {
	return this->config;
}

bool Parser::do_diagnostic() const {
	bool error = false;
	for (auto &s : this->diagnostic)
	{
		std::cout << std::string(s) << std::endl;
		error |= s.severity == SyntaxError::Error;
	}
	return error;
}

std::optional<std::string> Parser::get_tag(bool expected)
{
	if (expected)
		expect(static_cast<Scanner::token::token_type>('<'));
	else if (!accept(static_cast<Scanner::token::token_type>('<')))
		return {};
	std::string ret = expect(Scanner::token::IDENTIFIER).value;
	expect(static_cast<Scanner::token::token_type>('>'));
	return ret;
}

std::string Parser::get_action_rule(const std::string &action, std::vector<std::string> syntax) {
	std::string sym = lalr.generate_symbol_name();
	this->lalr.add_rule({sym, {}, "", substitute_action(sym, syntax, action)});
	return sym;
}

Scanner::token Parser::peak() {return *current;}

bool Parser::is_of_type(std::initializer_list<Scanner::token::token_type> &&list) {
	for (auto &t : list)
		if (t == current->type)
			return true;
	return false;
}

bool Parser::is_of_type(Scanner::token::token_type t) {
	return is_of_type({t});
}

std::optional<Scanner::token> Parser::accept(std::initializer_list<Scanner::token::token_type> &&list) {
	if (is_of_type(std::move(list)))
	{
			auto ret = *current;
			this->advance();
			return ret;
	}
	return std::optional<Scanner::token>();
}

std::optional<Scanner::token> Parser::accept(Scanner::token::token_type t) {return this->accept({t});}

Scanner::token Parser::expect(Scanner::token::token_type t) {
	auto tmp = this->accept(t);
	if (tmp.has_value())
		return tmp.value();
	unexpected(Scanner::token::get_type_string(t));
}

void Parser::unexpected(const std::string expected) {
	if (!expected.empty())
		throw UnexpectedToken(*current, expected);
	else
		throw UnexpectedToken(*current);
}

void Parser::advance() {
//	std::cout << current->value << std::endl;
	current++;
}

void Parser::begin() {
	this->current = this->tokens.begin();
}