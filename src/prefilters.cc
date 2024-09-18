#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>
#include <iomanip>
#include <numeric>
#include <filesystem>

#include <commandparser/CommandParser.h>
#include <dpcstruct/types/Alignment.h>
#include <dpcstruct/fileparser/AlnsFileParser.h>

namespace fs = std::filesystem;

struct BufferDescriptor{
    uint64_t size;
    uint64_t offset;
};

typedef std::unordered_map<std::string, BufferDescriptor> DescriptorMap;

void load_plddt_files(const std::vector<std::string>& filenames, std::vector<char>& buffer) {

    // Calculate the total size of all files
    std::streampos totalSize = 0;
    for (const std::string& filename : filenames) {
        std::ifstream file(filename, std::ios::binary);
        if (file) {
            file.seekg(0, std::ios::end);
            totalSize += file.tellg();
            file.close();
        }
        else {
            std::cerr << "Failed to open file: " << filename << std::endl;
        }
    }

    // Allocate the buffer with the total size
    buffer.resize(totalSize);

    // Load the files into the buffer
    std::streampos currentPosition = 0;
    for (const std::string& filename : filenames) {
        std::ifstream file(filename, std::ios::binary);
        if (file) {
            file.seekg(0, std::ios::end);
            std::streampos fileSize = file.tellg();
            file.seekg(0, std::ios::beg);

            // Read the file directly into the buffer
            file.read(buffer.data() + currentPosition, fileSize);
            currentPosition += fileSize;

            file.close();
        }
        else {
            std::cerr << "Failed to open file: " << filename << std::endl;
        }
    }

    return;
}

void load_plddt_descriptors(const std::vector<std::string>& filenames, DescriptorMap& descriptor) {
    uint64_t offset{0};
    for (const std::string& filename : filenames) {
        std::ifstream file(filename);
        if (file) {
            std::string line;
            while (std::getline(file, line)) {
                std::stringstream ss(line);
                std::string name;
                uint64_t size;
                if (ss >> name >> size) {
                    descriptor[name] = {size, offset};
                    offset += size;
                }
            }
        file.close();
        }
        else {
            std::cerr << "Failed to open file: " << filename << std::endl;
        }
    }
    return;
}


// void load_alignments_old(const std::string filename, std::vector<Alignment>& aligns) {
    
//     std::ifstream file (filename);
//     // reserve 4 GB of memory for the alignments
    
//     if (file) {
//         std::streamsize fileSize = file.tellg();
//         file.seekg(0, std::ios::beg);
//         // Estimated average line length (in bytes)
//         const size_t averageLineLength = 70; 
//         const size_t numLines = fileSize / averageLineLength;
//         aligns.reserve(numLines);

//         // read filec content
//         std::string line;
//         uint32_t queryID, queryStart, queryEnd, queryLength;
//         uint32_t searchID, searchStart, searchEnd, searchLength;
//         uint32_t alnLength, bits;
//         double pident, evalue;
//         double tmScore, lddt;
//         while (std::getline(file,line)){
//             std::stringstream ss(line);
//             if (ss >> queryID >> searchID >> queryStart >> queryEnd >> searchStart >> searchEnd >> 
//                     queryLength >> searchLength >> alnLength >> pident >> evalue >> bits) {
//                 aligns.emplace_back(queryID, searchID, queryStart, queryEnd, searchStart, searchEnd, 
//                                         queryLength, searchLength, alnLength, pident, evalue, bits, tmScore, lddt);
//             }
//         }
//     }
//     else {
//         std::cerr << "Failed to open file: " << filename << std::endl;
//     }
    
//     return;
// }

// load file where each line is store in an std::vector<std::string>
void load_idxname_map(const std::string& filename, std::vector<std::string>& idxToName) {
    std::ifstream file(filename);
    idxToName.push_back("None"); // indexes start from 1
    
    if (file) {
        std::string line;
        std::string name;
        while (std::getline(file, line)) {
            size_t lastDelimiterPos = line.find_last_of(' ');
            if (lastDelimiterPos == std::string::npos) { // handles 1 or 3 cols dictionaries
                lastDelimiterPos = 0;
            }
            name = line.substr(lastDelimiterPos + 1);
            size_t foundPos = name.find("-model");
            if (foundPos != std::string::npos) {
                name = name.substr(0, foundPos);
            }
            
            idxToName.push_back(name);
        }
        file.close();
    } else {
        std::cerr << "Failed to open file: " << filename << std::endl;
    }

    return;
}

std::vector<char> get_plddt(const std::string& name, const std::vector<char>& buffer, const DescriptorMap& descriptors) {
    std::vector<char> plddt;
    plddt.reserve(1000);
    BufferDescriptor desc;

    try { desc=descriptors.at(name);}
    catch (std::out_of_range& e) { return {}; }

    for (uint64_t i = 0; i < desc.size; i++) {
        plddt.push_back(buffer[desc.offset + i]);
    }
    return plddt;
}

void write_alignments(const std::string& filename, const std::vector<Alignment>& aligns) {
    std::ofstream file(filename);
    if (file) {
        for (const Alignment& align : aligns) {
            file << align.queryID << " " << align.searchID << " " << align.queryStart << " " << align.queryEnd << " "
                 << align.searchStart << " " << align.searchEnd << " " << align.queryLength << " " << align.searchLength << " "
                  << align.alnLength << " " << align.pident << " " << align.evalue << " " << align.bits << " " << align.tmScore << " " << align.lddt << std::endl;
        }
        file.close();
    }
    else {
        std::cerr << "Failed to open file: " << filename << std::endl;
    }
    return;
}

int main(int argc, char* argv[]) {

    // Define program options
    std::vector<Option> options = {
        {'i', "ALIGNMENTS", "path to alignments file"},
        {'o', "OUTPUT", "output directory"},
        {'p', "PLDDTS", "path to PLDDTs directory"},
        {'m', "PROTS-LOOKUP", "protein lookup file"},
    };

    // Define the option string and program description
    std::string optstring = "o:p:m:";
    std::string program_desc = "Filters alignments based on quality metrics.";

    // Create an OptionParser instance
    OptionParser parser(options, optstring, program_desc);

    // Parse options
    auto parsed_args = parser.parse(argc, argv);
    const std::string alignPath = parsed_args["i"];
    const std::string alignFilteredPath = parsed_args["o"];
    const std::string protsMapPath = parsed_args["m"];
    const std::string plddtsDir = parsed_args["p"];

    if (!fs::is_regular_file(alignPath)) {
        std::cerr << "Error: Alignment file does not exist: " << alignPath << std::endl;
        return 1;
    }

    if (!fs::is_regular_file(protsMapPath)) {
        std::cerr << "Error: Proteins mapping file does not exist: " << protsMapPath << std::endl;
        return 1;
    }

    // check if plddtsDir exists
    if (!fs::exists(plddtsDir)){
        std::cerr << "PLDDTs directory does not exist: " << plddtsDir << std::endl;
        return 1;
    }

    // check if output file exists. exit with error if it does
    if (fs::exists(alignFilteredPath)){
        std::cerr << "Output file already exists: " << alignFilteredPath << std::endl;
        return 1;
    }

    // Filters
    double plddtThres=60.;
    double tmThres=0.4;
    double lddtThres=0.4;
    double gapsThres=0.2;
    
    std::vector<std::string> plddtPaths;
    std::vector<std::string> descPaths;

    // grab plddt and descriptor file protsMapPath
    for (const auto & entry : std::filesystem::directory_iterator(plddtsDir)){
        if (entry.is_regular_file()) {
            std::string filepath = entry.path().string();
            
            if (filepath.ends_with(".bin")) {
                plddtPaths.push_back(filepath);
            } else if (filepath.ends_with(".txt")) {
                descPaths.push_back(filepath);
            }
        }   
    }

    std::cout << "Loading plddt files... " << std::flush;
    std::vector<char> plddtsBuffer;
    load_plddt_files(plddtPaths, plddtsBuffer);
    std::cout << "Done" << std::endl;

    std::cout << "Loading plddt file descriptors... " << std::flush;
    DescriptorMap plddtsDescriptor;
    load_plddt_descriptors(descPaths, plddtsDescriptor);
    std::cout << "Done" << std::endl;

    std::cout << "Loading alignments... " << std::flush;
    std::vector<Alignment> aligns;
    AlnsFileParser alnsParser(alignPath);
    alnsParser.loadAlignments(aligns,1);
    std::cout << "Done" << std::endl;

    std::cout << "Loading idx to name map... " << std::flush;
    std::vector<std::string> idxToName;
    load_idxname_map(protsMapPath, idxToName);
    std::cout << "Done" << std::endl;
    
    std::cout << "Filtering alignments... " << std::flush;
    // define a buffer for the filtered alignments
    std::vector<Alignment> alignsFiltered;
    alignsFiltered.reserve(aligns.size());
    
    uint32_t queryIDPrev = 0;
    std::vector<char> queryPLDDTs;
    for (int i = 0; i < aligns.size(); i++) {

        uint32_t queryAlignLength = aligns[i].queryEnd - aligns[i].queryStart + 1;
        uint32_t searchAlignLength = aligns[i].searchEnd - aligns[i].searchStart + 1;
        // struct quality filters
        int queryGaps = aligns[i].alnLength - queryAlignLength;
        int searchGaps = aligns[i].alnLength - searchAlignLength;

        double queryGapsRatio = (double)queryGaps / aligns[i].alnLength;
        double searchGapsRatio = (double)searchGaps / aligns[i].alnLength;

        // check they are both positives
        if (queryGaps < 0 || searchGaps < 0) {
            std::cerr << "Gaps are negative: " << queryGaps << " " << searchGaps << std::endl;
            std::cerr << "alnLength: " << aligns[i].alnLength << std::endl;
            std::cerr << "queryLength: " << aligns[i].queryLength << std::endl;
            std::cerr << "searchLength: " << aligns[i].searchLength << std::endl;
            return 1;
        }

        // if any of the gaps ratio is above the threshold, skip
        if (queryGapsRatio >= gapsThres || searchGapsRatio >= gapsThres) {
            continue;
        }


        // PLDDT filters
        auto queryName = idxToName[aligns[i].queryID];
        auto queryStart = aligns[i].queryStart;
        auto queryEnd = aligns[i].queryEnd;

        if (aligns[i].queryID != queryIDPrev) {
            queryPLDDTs.clear();
            queryPLDDTs = get_plddt(queryName, plddtsBuffer, plddtsDescriptor);
        }
        queryIDPrev = aligns[i].queryID;

        if (queryPLDDTs.empty()){
            std::cerr << "Failed to get plddt for: " << queryName << std::endl;
            return 1;
        }

        double queryPlddtSum = std::accumulate(queryPLDDTs.begin() + queryStart-1, queryPLDDTs.begin()+queryEnd, 0.0);
        double queryPlddtMean = queryPlddtSum / (queryEnd - queryStart + 1);

        if (queryPlddtMean >= plddtThres){
            auto searchName = idxToName[aligns[i].searchID];
            auto searchStart = aligns[i].searchStart;
            auto searchEnd = aligns[i].searchEnd;
            
            auto searchPLDDTs = get_plddt(searchName, plddtsBuffer, plddtsDescriptor);
            if (searchPLDDTs.empty()){
                std::cerr << "Failed to get plddt for: " << searchName << std::endl;
                continue;
            }  
    
            double searchPlddtSum = std::accumulate(searchPLDDTs.begin()+searchStart-1, searchPLDDTs.begin()+searchEnd, 0.0);
            double searchPlddtMean = searchPlddtSum / (searchEnd - searchStart + 1);

            // std::cout << "i: " << i << std::endl;
            // std::cout << aligns[i].searchID << " " << searchName << " " << searchStart << " " << searchEnd << " " << searchPlddtMean << std::endl;
            // std::cout << aligns[i].queryID << " " << queryName << " " << queryStart << " " << queryEnd << " " << queryPlddtMean << std::endl; 
            if (searchPlddtMean>=plddtThres){
                alignsFiltered.push_back(aligns[i]);
            }            
        }

    }

    std::cout << "Done" << std::endl;

    std::cout << "Number of alignments: " << aligns.size() << std::endl;    
    std::cout << "Number of alignments after filters: " << alignsFiltered.size() << std::endl;    

    // output alignsFiltered in a text file
    std::cout << "Writing filtered alignments... " << std::flush;
    write_alignments(alignFilteredPath, alignsFiltered);
    std::cout << "Done" << std::endl;
        
    return 0;
}
