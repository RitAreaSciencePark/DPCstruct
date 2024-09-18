#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include <cstring>
#include <stdexcept>
#include <fstream>

#include <commandparser/CommandParser.h>
#include <dpcstruct/sort.h>
#include <dpcstruct/types/PrimaryCluster.h>
#include <dpcstruct/fileparser/PCsFileParser.h>


// Loads binary file into std::vector<T>
int load_labels(std::string filename, std::unordered_map<int32_t, int64_t> & labels)
{
    std::ifstream inFile (filename);
    std::string line;
	
    int32_t cID;
    int32_t density;
    double minDistance;
    int64_t label;
    

	// test file open   
	if (inFile) 
	{        
	    // read the elements in the file into an unordered map
	    while(std::getline(inFile, line))
	    {
            // Create a stringstream of the current line
            std::stringstream ss(line);
            ss >> cID >> density >> minDistance >> label;
            labels[cID] = label;
	    }
	}
    else 
    {
        throw std::system_error(errno, std::system_category(), "failed to open "+ filename);
    }

   
    inFile.close();

    return 0;
}

// Assigns a metacluster label to each alignment (search sequence) inside a primary cluster
std::vector<SequenceLabel> assign_search_label(SmallPC * clusterAlign,  std::unordered_map<int32_t, int64_t> &labels, const uint64_t length)
{
    // Note that as implemented, all values have a label even if they are not present in the dmatII
    auto end = clusterAlign + length;
    auto low = clusterAlign;

    std::vector<SequenceLabel> searchLabel;
    searchLabel.reserve(length);

    while (low != end)
    {
        uint32_t cID = low->qID;

        // find last occurrence
        auto high = std::upper_bound(low, end, cID, compare_qID());

        // compute the difference
        auto count = high - low;

        // add cluster size to the list
        for (auto p = low; p < high; ++p)
        {
            // skip non assigned cIDs
            auto itLabel = labels.find(cID);
            if (itLabel == labels.end() || itLabel->second < 0) {continue;}

            searchLabel.emplace_back(SequenceLabel(p->sID, p->sstart, p->send, labels[cID])    );
        }

        // move to next element in vector (not immediate next)
        low = low + count;
    }


    return searchLabel;
}


int main(int argc, char *argv[]) {
    try {

        // Label vector: cluster label for each node
        std::unordered_map<int32_t, int64_t> labels;
        std::vector<SequenceLabel> searchLabel;

        // Argument parser options
        std::vector<Option> options = {
            {'i',"INPUT", "input files containing all primary cluster domains", true},
            {'l', "MC_LABELS", "file containing a metacluster label for each primary cluster", true},
            {'o', "OUTDIR", "output path (optional, default is ./)", false},
            {'n', "NUM_OUTPUT", "estimated number of output files (optional)", false}
        };

        std::string optstring = "l:o:n:";
        std::string program_desc = "Assign a metacluster label to each domain inside a primary cluster.";

        // Initialize the command parser
        OptionParser parser(options, optstring, program_desc);
        auto parsed_options = parser.parse(argc, argv);

        // Parse the arguments
        std::string filenames = parsed_options["i"];  // Required option
        std::string labelFilename = parsed_options["l"];  // Required option
        std::string outputPath = parsed_options.count("o") ? parsed_options["o"] : "./"; 
        int num_output_files = parsed_options.count("n") ? std::stoi(parsed_options["n"]) : 1;

        // Collect input filenames
        std::vector<std::string> filesList = OptionParser::split_filenames(filenames);
        // check if the input files are valid
        for (const auto& filename : filesList) {
            if (!std::filesystem::exists(filename)) {
                throw std::invalid_argument("Error: File " + filename + " does not exist.");
            }
        }
        if (filesList.empty()) {
            throw std::invalid_argument("Error: missing input file(s).");
        }
        
        if (!std::filesystem::exists(outputPath)) {
            throw std::invalid_argument("Error: Output path (" + outputPath + ") does not exist.");
        }

        // Ensure outputPath ends with '/'
        if (!outputPath.empty() && outputPath.back() != '/') {
            outputPath += '/';
        }

        if (!std::filesystem::exists(labelFilename)) {
            throw std::invalid_argument("Error: MC labels file (" + labelFilename + ") does not exist.");
        }

        // std output files
        std::cout << "Output path: " << outputPath << std::endl;
        std::cout << "MC labels file: " << labelFilename << std::endl;
        
        // Processing..
        
        // Vector to hold parsers
        std::vector<PCsFileParser> fileParsers;
        uint64_t totalLines = 0;

        for (const auto& filename : filesList) {
            fileParsers.emplace_back(filename);
            fileParsers.back().loadPCs();  // Load the primary clusters
            totalLines += fileParsers.back().getTotalLines();  // Accumulate total lines
        }

        std::cout << "totalLines: " << totalLines << std::endl;

        // Allocate buffer for all SmallPC entries
        SmallPC* bufferPC = nullptr;
        try {
            bufferPC = new SmallPC[totalLines];
        } catch (const std::bad_alloc& e) {
            std::cerr << "Memory Allocation failed. Try reducing the number of files to process!\n" << e.what() << std::endl;
            throw;
        }

        // Populate the buffer
        auto fileBuffer = bufferPC;
        for (const auto& parser : fileParsers) {
            std::memcpy(fileBuffer, parser.getData(), parser.getTotalLines() * sizeof(SmallPC));
            fileBuffer += parser.getTotalLines();  // Move the pointer for the next block
        }


        load_labels(labelFilename, labels);

        radix_sort(reinterpret_cast<unsigned char*>(bufferPC), totalLines, 16, 4, 0);
        if (!std::is_sorted(bufferPC, bufferPC + totalLines, compare_qID())) {
            throw std::runtime_error("Radix sort failed! Check if qIDs are 32-bit unsigned integers.");
        }

        searchLabel = assign_search_label(bufferPC, labels, totalLines);

        // Clean up
        delete[] bufferPC;
        
        // sort wrt to sID
        radix_sort((unsigned char*) searchLabel.data(), searchLabel.size(), 12, 4, 0);
        // sort wrt label
        radix_sort((unsigned char*) searchLabel.data(), searchLabel.size(), 12, 4, 8);
        // TODO: implement radix_sort for std::vector

        // Output
        size_t avgLines = searchLabel.size() / num_output_files;
        auto startIt = searchLabel.begin();
        auto endIt = startIt;

        for (int i = 1; i <= num_output_files; ++i) {
            if (endIt == searchLabel.end()) {
                num_output_files = i - 1; 
                break;
            }

            std::string outFilename = outputPath + "sequence-labels_" + std::to_string(i) + ".bin";
            std::ofstream outFile(outFilename, std::ofstream::binary);

            // Move endIt to the approximate avgLines position
            endIt = startIt + avgLines;

            if (endIt < searchLabel.end()) {
                int current_label = endIt->label;
                // Move endIt forward while elements have the same label
                while (endIt != searchLabel.end() && endIt->label == current_label) {
                    ++endIt;
                }
            }

            // Write the chunk to file
            outFile.write(reinterpret_cast<const char*>(&(*startIt)), (endIt - startIt) * sizeof(SequenceLabel));
            
            // Move to the next chunk
            startIt = endIt;
            outFile.close();
        }

        std::cout << "Total output files: " << num_output_files << "\n";
        std::cout << "Processing complete." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;  // Return success exit code
}
