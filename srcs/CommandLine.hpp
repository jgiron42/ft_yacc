#ifndef FT_YACC_POC_COMMANDLINE_HPP
#define FT_YACC_POC_COMMANDLINE_HPP
#include <string>

struct CommandLine {
	std::string input_file;
	std::string root;
	std::string file_prefix;
	std::string sym_prefix;
	bool		write_header;
	bool		write_description;
	bool		debug_defines;
	std::string language;
	CommandLine(int argc, char **argv);
	CommandLine();
	CommandLine(const CommandLine &) = default;
	CommandLine &operator=(const CommandLine &) = default;
};

extern CommandLine commandLine;

#endif
