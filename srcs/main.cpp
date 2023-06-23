#include <fstream>
#include "Scanner.hpp"
#include "Serializer.hpp"
#include "Generator.hpp"
#include "LALR.hpp"
#include "Parser.hpp"
#include "CommandLine.hpp"
#ifndef SKELETONS_PATHS
# define SKELETONS_PATHS "./"
#endif


int main(int argc, char **argv)
{
	CommandLine commandLine(argc, argv);

    try {
        Scanner scanner(commandLine.input_file);
        scanner.scan();

        Parser parser(scanner.get_tokens());
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

        Serializer serializer(parser.get_config(), commandLine, lalr);
		serializer.build();
		if (commandLine.language == "c" ) {
			serializer.get_generator().generate(SKELETONS_PATHS "/c.skl",
												commandLine.root + commandLine.file_prefix + ".tab.c");
			if (commandLine.write_header)
				serializer.get_generator().generate(SKELETONS_PATHS "/h.skl",
													commandLine.root + commandLine.file_prefix + ".tab.h");
		}
		else if (commandLine.language == "c++" ) {
			serializer.get_generator().set("HEADER_NAME", commandLine.file_prefix + ".tab.hpp");
			serializer.get_generator().set("DEF_NAME", commandLine.file_prefix + ".def.hpp");
			serializer.get_generator().generate(SKELETONS_PATHS "/cpp.skl",
												commandLine.root + commandLine.file_prefix + ".tab.cpp");
			serializer.get_generator().generate(SKELETONS_PATHS "/hpp.skl",
													commandLine.root + commandLine.file_prefix + ".tab.hpp");
			serializer.get_generator().generate(SKELETONS_PATHS "/def.skl",
													commandLine.root + commandLine.file_prefix + ".def.hpp");
		}
    }
    catch (std::exception &e)
    {
        std::cerr << "Fatal: " << e.what() << std::endl;
    }
}