#include <fstream>
#include "Scanner.hpp"
#include "Serializer.hpp"
#include "serializers/c.hpp"
#include "serializers/b.hpp"
#include "serializers/cpp.hpp"
#include "serializers/zig.hpp"
#include "Generator.hpp"
#include "LALR.hpp"
#include "Parser.hpp"
#include "CommandLine.hpp"
#ifndef SKELETONS_PATHS
# define SKELETONS_PATHS "./"
#endif

CommandLine commandLine;

int main(int argc, char **argv)
{
	commandLine = CommandLine(argc, argv);

    try {
        Scanner scanner(commandLine.input_file);

        Parser parser(scanner);
        if (parser.parse())
            return 1;

        LALR lalr = parser.get_LALR();
        try {
            lalr.build(parser.get_config().start);
        } catch (std::exception &e) {
            std::cerr << "error: " << e.what() << std::endl;
        }

        if (commandLine.write_description) {
            std::ofstream output(commandLine.root + commandLine.file_prefix + ".output");
            lalr.print(output);
        }

		if (commandLine.language == "c" )
			CSerializer(parser.get_config(), lalr).generate();
		else if (commandLine.language == "b" )
			BSerializer(parser.get_config(), lalr).generate();
		else if (commandLine.language == "zig" )
			ZigSerializer(parser.get_config(), lalr).generate();
		else if (commandLine.language == "c++" )
			CppSerializer(parser.get_config(), lalr).generate();
    }
    catch (std::exception &e)
    {
        std::cerr << "Fatal: " << e.what() << std::endl;
    }
}