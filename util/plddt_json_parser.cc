#include "rapidjson/document.h" // uses header from https://github.com/Tencent/rapidjson/
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cmath>

using namespace rapidjson;

int main(int argc, char** argv) {

    std::string plddtsJSONDir = argv[1];
    std::string suffix = argv[2];
    std::string outDir = argv[3];

    // check if input or output directory does not exist
    if (!std::filesystem::exists(plddtsJSONDir) || !std::filesystem::exists(outDir)){
        std::cerr << "***Error: Input or output directory does not exist." << std::endl;
        return 1;
    }

    // iterate over files in directory
    std::string dictFilename = "plddts_desc_" + suffix + ".txt";
    std::string plddtsFilename = "plddts_" + suffix + ".bin";
    std::vector<std::string> af2Names;
    std::vector<uint64_t> af2Sizes;
    std::vector<uint8_t> plddts;

    for (const auto & entry : std::filesystem::directory_iterator(plddtsJSONDir)){

        // basename of entry.path()
        std::string basename = entry.path().filename().string();
        
        // clean basename
        basename.erase(basename.find("-F1-confidence_v4.json"), std::string::npos);
        basename.erase(0, 3);

        // 1. Read JSON data from file
        std::ifstream inputFile(entry.path());
        std::string jsonData((std::istreambuf_iterator<char>(inputFile)),
                            std::istreambuf_iterator<char>());

        // 2. Parse JSON data into DOM
        Document document;
        document.Parse(jsonData.c_str());

        // 3. Access and extract the field into an array of doubles
        if (!document.HasParseError()) {
            // Example: Extract the "data" field into an array of doubles
            const Value& dataArray = document["confidenceScore"];
            if (dataArray.IsArray()) {
                for (SizeType i = 0; i < dataArray.Size(); i++) {
                    if (dataArray[i].IsNumber()) {
                        double value = dataArray[i].GetDouble();
                        uint8_t roundedNumber = static_cast<uint8_t>(std::floor(value));
                        plddts.push_back(roundedNumber);
                    }
                }
            }
            af2Names.push_back(basename);
            af2Sizes.push_back(dataArray.Size());
        } else {
            std::cerr << "***Error parsing JSON file." << std::endl;
        }
    }
    

    std::cout << "Number of files processed: " << af2Names.size() << std::endl;

    // output vector content to binary file
    std::ofstream plddtsBinFile;
    plddtsBinFile.open(outDir + "/" + plddtsFilename, std::ios::binary);
    plddtsBinFile.write(reinterpret_cast<const char*>(plddts.data()), plddts.size()*sizeof(uint8_t)); 
    plddtsBinFile.close();

    std::ofstream plddtsDictFile;
    plddtsDictFile.open(outDir + "/" + dictFilename);
    for (size_t i = 0; i < af2Names.size(); i++)
    {
        plddtsDictFile << af2Names[i] << " " << af2Sizes[i] << std::endl;
    }
    plddtsDictFile.close();


    // read binary file
    // std::ifstream infile;
    // infile.open("plddts.bin", std::ios::binary);
    // std::vector<uint8_t> plddts2;
    // infile.seekg(0, std::ios::end);
    // size_t size = infile.tellg();
    // infile.seekg(0, std::ios::beg);
    // plddts2.resize(size);
    // infile.read(reinterpret_cast<char*>(plddts2.data()), size);
    // infile.close();

    return 0;
}