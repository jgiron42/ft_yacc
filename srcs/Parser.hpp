#ifndef FT_YACC_POC_PARSER_HPP
#define FT_YACC_POC_PARSER_HPP
#include "Scanner.hpp"
#include <optional>
#include <map>
#include <stdexcept>
#include "LALR.hpp"
#include "SyntaxError.hpp"

class Parser {
public:
	struct Config {
		struct token {
			enum Associativity {
				DEFAULT,
				RIGHT,
				LEFT,
				NONASSOC
			}							associativity;
			int							value;
		};
		std::map<std::string, std::string>	token_types;
		std::string							start;
		bool								union_enabled;
		std::string 						stack_type;
		bool 								variant_enabled;
		std::string							header_code;
		std::string							tail_code;
		bool 								use_cpp_lex;
		std::string							lex_header;
	};
	class UnexpectedToken : public std::runtime_error {
	public:
		UnexpectedToken(const Scanner::token &t) : std::runtime_error(t.file + ":" + std::to_string(t.line) + ": Unexpected token: " + std::string(t)) {}
		UnexpectedToken(const Scanner::token &t, std::string expected) : std::runtime_error(t.file + ":" + std::to_string(t.line) + ": Unexpected token: \"" + t.value + "\", expected: " + expected) {}
	};
	class InvalidSyntax : public std::exception {};
	/*
	 * rule: (produced token, list of token, position)
	 */
private:
	Config				config;

	LALR								lalr;
	Scanner								&scanner;
	Scanner::token						current;
	std::list<SyntaxError>				diagnostic;
public:
	Parser(Scanner &);
	bool	parse();
	Config	&get_config();
	LALR	get_LALR();
private:
	bool	do_diagnostic() const;
	void	parse_def();
	void	parse_rules();
	void	parse_tail();
	std::optional<std::string>	get_tag(bool expected = false);
	std::string get_action_rule(const std::string &, std::vector<std::string> syntax);
	std::string					substitute_action(const std::string &, std::vector<std::string>, const std::string &);
	template <typename ...Args>
	void						add_error(Args... args)
	{
		this->diagnostic.emplace_back(args...);
	}

	Scanner::token					peak();
	bool							is_of_type(Scanner::token::token_type);
	bool							is_of_type(std::initializer_list<Scanner::token::token_type>&&);
	std::optional<Scanner::token>	accept(Scanner::token::token_type);
	std::optional<Scanner::token>	accept(std::initializer_list<Scanner::token::token_type>&&);
	Scanner::token					expect(Scanner::token::token_type);
	Scanner::token					expect(std::initializer_list<Scanner::token::token_type>&&);
	[[noreturn]] void				unexpected(const std::string expected = "");
	bool 							is_at_end();
	void							advance();
	void							begin();

};


#endif //FT_YACC_POC_PARSER_HPP
