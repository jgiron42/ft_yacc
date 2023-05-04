//
// Created by citron on 5/2/23.
//

#ifndef FT_LEX_GENERATOR_HPP
#define FT_LEX_GENERATOR_HPP
#include <map>
#include <fstream>
#include <string>
#include <regex>

class Generator {
private:
	std::map<std::string, std::string> definitions;
public:
	void	set(std::string, std::string);
	void	generate(std::string skeleton, std::string dst);
};


#endif //FT_LEX_GENERATOR_HPP
