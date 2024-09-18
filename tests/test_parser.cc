#include "optionparser.h"

int main(int argc, char* argv[]) 
{
    // Define program options, including a positional argument
    std::vector<Option> options = {
        {'o', "OUTPUT_DIR", "Output directory"},
        {'v', "", "Enable verbose mode"},
        {'d', "LEVEL", "Set debug level"},
        {'x', "INPUT_FILES", "Input files"} // Positional argument with description
    };

    // Define the option string and program description
    std::string optstring = "o:vd:"; // h - help, o - output, v - verbose, d - debug level
    std::string program_desc = "This program does something useful with options.";

    // Create an OptionParser instance
    OptionParser parser(options, optstring, program_desc);

    // Parse options and positional arguments
    auto parsed_args = parser.parse(argc, argv);

    // Accessing the output directory using the string key "o"
    const std::string outDir = parsed_args.count("o") ? parsed_args["o"] : "";

    if (outDir.empty()) {
        std::cerr << "Error: Output directory is required." << std::endl;
        return 1;
    }

    // Handle other parsed options and positional arguments
    if (parsed_args.count("v")) {
        std::cout << "Verbose mode enabled" << std::endl;
    }
    if (parsed_args.count("d")) {
        std::cout << "Debug level set to: " << parsed_args["d"] << std::endl;
    }
    if (parsed_args.count("x")) { // Accessing positional arguments using the key "x"
        std::cout << "Positional arguments (input files): " << parsed_args["x"] << std::endl;
    }

    // Your main program logic here

    return 0;
}
