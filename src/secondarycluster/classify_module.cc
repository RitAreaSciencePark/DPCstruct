#include <iostream>
#include <string>
#include <filesystem>

#include <dpcstruct/secondarycluster.h>
#include <dpcstruct/secondarycluster/classify_proc.h>
#include <commandparser/CommandParser.h>


void sc_classify_module(int argc, char** argv) {
    std::vector<Option> options = {
        {'i', "INPUT", "list of space-separated files"},
        {'o', "OUTPUT", "output file containing the classified primary clusters"},
        {'t', "THREADS", "number of threads (TBI)", false}
    };

    std::string optstring = "o:t:";
    std::string program_desc = "Classify primary clusters into secondary clusters or metaclusters.";

    OptionParser dist_parser(options, optstring, program_desc);
    auto parsed_options = dist_parser.parse(argc, argv);

    std::string filenames = parsed_options["i"];
    std::vector<std::string> filesList = OptionParser::split_filenames(filenames);
    // check if the input files are valid
    for (const auto& filename : filesList) {
        if (!std::filesystem::exists(filename)) {
            std::cerr << "Error: File " << filename << " does not exist.\n";
            return;
        }
    }
    std::string outputFile = parsed_options["o"];

    int numThreads = 1;  // Default value
    if (parsed_options.count("t")) {
        numThreads = std::stoi(parsed_options["t"]);
    }

    run_classification(filesList, outputFile, numThreads);
}