#include "Generator.hpp"

void Generator::set(std::string name, std::string definition) {
	this->definitions[name] = definition;
}

void Generator::generate(std::string skeleton, std::string dst) {
	std::ofstream out;
	std::ifstream in;
	out.open(dst);
	if (!out.is_open())
		throw std::runtime_error("Can't open " + dst + ": " + strerror(errno));
	in.open(skeleton);
	if (!in.is_open())
		throw std::runtime_error("Can't open " + skeleton + ": " + strerror(errno));
	static std::regex r(R"(%%\[([a-zA-Z_][a-zA-Z0-9_]*)]%%)", std::regex::extended);

	std::string s;
	while (std::getline(in, s))
	{
		std::smatch mr;
		while (std::regex_search(s, mr, r))
		{
			out << s.substr(0, mr.position());
			if (this->definitions.contains(mr[1].str()))
				out << this->definitions[mr[1]];
			s = s.substr(mr.position() + mr.length());
		}
		out << s << '\n';
	}
}