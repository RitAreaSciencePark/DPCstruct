#include <iostream>
#include <dpcstruct/secondarycluster.h>
#include <commandparser/CommandParser.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: \n";
        std::cerr << "\t" << argv[0] << " MODE OPTIONS \n";
        std::cerr << "Modes:\n";
        std::cerr << "\tdistance   Calculate distance matrix between primary clusters.\n\n";
        std::cerr << "\tclassify   Classify secondary clustering based on distance matrix.\n";
        return 1;
    }

    std::string mode = argv[1];

    // Merge argv[0] and argv[1] into a single argument
    std::string new_argv0 = std::string(argv[0]) + " " + mode;
    argv[0] = const_cast<char*>(new_argv0.c_str());  // Update argv[0]

    // Shift the arguments by removing argv[1]
    for (int i = 1; i < argc - 1; ++i) {
        argv[i] = argv[i + 1];
    }
    argc--;

    if (mode == "distance") {
        sc_distance_module(argc, argv);  
    } else if (mode == "classify") {
        std::cout << "Running secondary clustering...\n";
        sc_classify_module(argc, argv);
    } else {
        std::cerr << "Invalid mode. Use 'distance' or 'classify'.\n";
        return 1;
    }

    return 0;
}

