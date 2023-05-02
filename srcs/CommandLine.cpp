#include "CommandLine.hpp"
#include <unistd.h>
#include <iostream>

CommandLine::CommandLine(int argc, char **argv) :
	input_file(""),
	file_prefix("y"),
	sym_prefix("yy"),
	write_header(false),
	write_description(false),
	debug_defines(false)
{
	while (1)
		switch (getopt(argc, argv, "-dltvb:p:"))
		{
			case 'd':
				this->write_header = true;
				break;
			case 'l':
				break;
			case 't':
				this->debug_defines = true;
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