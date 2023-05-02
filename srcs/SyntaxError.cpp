#include "SyntaxError.hpp"

SyntaxError::SyntaxError(std::string f, int l, std::string d, SyntaxError::Severity s) : file(f), line(l), description(d), severity(s) {}


SyntaxError::operator std::string() const{
	std::string ret;
	static std::map<Severity, std::string> severity_string =
			{
					{Notice, "Notice"},
					{Warning, "Warning"},
					{Error, "Error"},
			};
	ret += file + ":";
	if (line > 0)
		ret += std::to_string(line) + ": ";

	ret += severity_string[severity] + ": ";

	ret += description;
	return ret;
}