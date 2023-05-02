#include <fstream>
#include "Scanner.hpp"
#include "Generator.hpp"
#include "LALR.hpp"
#include "Parser.hpp"
#include "CommandLine.hpp"

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
            std::ofstream output(commandLine.file_prefix + ".output");
            lalr.print(output);
        }

        Generator generator(parser.get_config(), commandLine, lalr);
        std::ofstream c(commandLine.file_prefix + ".tab.c");
        generator.generate_c(c);

        if (commandLine.write_header) {
            std::ofstream h(commandLine.file_prefix + ".tab.h");
            generator.generate_h(h);
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "Fatal: " << e.what() << std::endl;
    }
}