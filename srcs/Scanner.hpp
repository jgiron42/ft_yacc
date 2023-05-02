#ifndef FT_YACC_POC_SCANNER_HPP
#define FT_YACC_POC_SCANNER_HPP
#include <functional>
#include <iostream>
#include <string>
#include <list>
#include <deque>
#include <variant>

class Scanner {
public:
	struct token {
		enum token_type {
			END = 0,
			IDENTIFIER = 256,
			C_IDENTIFIER,
			NUMBER,
			TYPE,
			LEFT,
			RIGHT,
			NONASSOC,
			TOKEN,
			MARK,
			START,
			UNION,
			PREC,
			LCURL,
			RCURL,
			HEADER_CODE,
			TAIL,
			ACTION
		};
		int 		type;
		std::string value;
		std::string file;
		int			line;
		explicit operator std::string() const
		{
			return this->get_type_string(this->type);
		}
		static std::string get_type_string(int type)
		{
			if (type == 0)
				return "END";
			if (isprint(type))
				return "`" + std::string (1, type) + "`";
			if (type < 256)
				return std::to_string(type);
			return (std::string []){
					"IDENTIFIER",
					"C_IDENTIFIER",
					"NUMBER",
					"TYPE",
					"LEFT",
					"RIGHT",
					"NONASSOC",
					"TOKEN",
					"MARK",
					"START",
					"UNION",
					"PREC",
					"LCURL",
					"RCURL",
					"HEADER_CODE",
					"TAIL",
					"ACTION"
			}[type - 256];
		}
	};
private:
	std::string						filename;
	std::list<token>				tokens;
public:
	Scanner(const std::string &filename);
	void				scan();
	std::list<token>	get_tokens();
};


#endif