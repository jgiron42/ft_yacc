#include "Parser.hpp"
#include "CommandLine.hpp"
#include <iostream>
#include <regex>

Parser::Parser(Scanner &scanner) : scanner(scanner), config({}) {}

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
			this->config.stack_type = expect(Scanner::token::ACTION).value;
			this->config.union_enabled = true;
		}
		else if (accept(Scanner::token::VARIANT))
			this->config.variant_enabled = true;
		else if (accept(Scanner::token::USE_CPP_LEX))
		{
			this->config.use_cpp_lex = true;
			if (auto filename = accept(Scanner::token::FILENAME))
				this->config.lex_header = filename->value;
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
			auto type = accept(Scanner::token::TAG);
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
					this->config.token_types[name_tok->value] = type->value;
			} while ((name_tok = accept(Scanner::token::IDENTIFIER)).has_value());
		}
		else if (accept({Scanner::token::TYPE}))
		{
			auto type = expect(Scanner::token::TAG);
			std::optional<Scanner::token> name_tok = expect(Scanner::token::IDENTIFIER);
			do this->config.token_types[name_tok->value] = type.value;
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

		if (auto type = accept(Scanner::token::TAG)) {
			this->config.token_types[symbol.value] = type->value;
		}

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
					{
						try {
							r.syntax.push_back(get_action_rule(component->value, r.syntax));
						} catch (std::exception &e) {
							this->add_error(file, line, e.what(), SyntaxError::Error);
						}
					}
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
				r.real_syntax = r.syntax;
				this->lalr.add_rule(LALR::Rule(r));
			} catch (std::exception &e) {
				this->add_error(file, line, e.what(), SyntaxError::Error);
			}
		} while (accept(static_cast<Scanner::token::token_type>('|')));
		expect(static_cast<Scanner::token::token_type>(';'));
	}
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
	this->lalr.add_token(sym, {false, -1, LALR::Token::DEFAULT}); // add the token to the list of token
	this->lalr.add_rule({sym, {}, syntax, "", action});
	return sym;
}

Scanner::token Parser::peak() {return current;}

bool Parser::is_of_type(std::initializer_list<Scanner::token::token_type> &&list) {
	for (auto &t : list)
		if (t == current.type)
			return true;
	return false;
}

bool Parser::is_of_type(Scanner::token::token_type t) {
	return is_of_type({t});
}

std::optional<Scanner::token> Parser::accept(std::initializer_list<Scanner::token::token_type> &&list) {
	if (is_of_type(std::move(list)))
	{
			auto ret = current;
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
		throw UnexpectedToken(current, expected);
	else
		throw UnexpectedToken(current);
}

void Parser::advance() {
//	std::cout << current->value << std::endl;
	current = scanner.next();
}

void Parser::begin() {
//	this->current = this->tokens.begin();
	this->current = scanner.next();
}