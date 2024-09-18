#include <dpcstruct/secondarycluster.h>
#include <dpcstruct/secondarycluster/distance_proc.h>
#include <commandparser/CommandParser.h>

// Function to handle the arguments and run distance calculation
void sc_distance_module(int argc, char** argv) {
    std::vector<Option> options = {
        {'i', "INPUTA", "input file A"},
        {'j', "INPUTB", "input file B"},
        {'o', "OUTPUT", "output file for the distance matrix"},
        {'p', "PRODUCERS", "producer threads"},
        {'c', "CONSUMERS", "consumer threads"}
    };

    std::string optstring = "i:j:o:p:c:";
    std::string program_desc = "Calculate distances between primary clusters.";

    OptionParser dist_parser(options, optstring, program_desc);
    auto parsed_options = dist_parser.parse(argc, argv);

    std::string inputFileA = parsed_options["i"];
    std::string inputFileB = parsed_options["j"];
    std::string outputFile = parsed_options["o"];
    int producers = std::stoi(parsed_options["p"]);
    int consumers = std::stoi(parsed_options["c"]);

    if (consumers < 2) {
        std::cerr << "Number of consumers should be greater than 1.\n";
        return;
    }

    calculate_distance_matrix(inputFileA, inputFileB, outputFile, producers, consumers);
}
