#include "CommandLine.hpp"
#include <unistd.h>
#include <iostream>
#include <algorithm>

CommandLine::CommandLine() {}

CommandLine::CommandLine(int argc, char **argv) :
	input_file(""),
	file_prefix("y"),
	sym_prefix("yy"),
	write_header(false),
	write_description(false),
	debug_defines(false),
	language("c")
{
	while (1)
		switch (getopt(argc, argv, "-dltvb:p:r:x:+"))
		{
			case 'd':
				this->write_header = true;
				break;
			case 'l':
				break;
			case 't':
				this->debug_defines = true;
				break;
			case '+':
				this->language = "c++";
				break;
			case 'x':
				this->language = std::string(optarg);
				std::transform(this->language.begin(), this->language.end(), this->language.begin(),
							   [](unsigned char c){ return std::tolower(c); });
				break;
			case 'v':
				this->write_description = true;
				break;
			case 'b':
				this->file_prefix = optarg;
				break;
			case 'p':
				this->sym_prefix = optarg;
				break;
			case 'r':
				this->root = std::string(optarg) + '/';
				break;
			case 1:
				this->input_file = optarg;
				break;
			case '?':
				exit(1);
			case -1:
				if (this->input_file.empty())
				{
					std::cerr << "Missing operand" << std::endl;
					exit(1);
				}
				return;
		}
}
