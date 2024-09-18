#include <fstream>
#include <iostream>
#include <dpcstruct/fileparser/PCsFileParser.h>

PCsFileParser::PCsFileParser(const std::string& filename) 
    : filename(filename), data(nullptr), totalLines(0), dataSize(0) {}

// Destructor to clean up the buffer
PCsFileParser::~PCsFileParser() {
    delete[] data;  // Safely delete the buffer when the object is destroyed
}


// wrapper function to load primary clusters. In the future it will load different sources
void PCsFileParser::loadPCs() {    
    readBinaryData(sizeof(SmallPC));
}

void PCsFileParser::readBinaryData(size_t elementSize) {
    std::ifstream infile(filename, std::ifstream::binary);

    if (!infile) {
        throw std::runtime_error("Error: Unable to open file " + filename);
    }

    // Move to the end of the file to determine its size
    infile.seekg(0, infile.end);
    dataSize = infile.tellg();
    infile.seekg(0, infile.beg);

    // Allocate memory for the buffer
    data = new char[dataSize];
    totalLines = dataSize / elementSize;

    if (totalLines == 0) {
        throw std::runtime_error("Error: File " + filename + " contains no valid entries.");
    }

    // Read the file contents into the buffer
    infile.read(data, dataSize);

    if (!infile) {
        delete[] data;
        data = nullptr;
        throw std::runtime_error("Error reading binary file: " + filename);
    }

    infile.close();
}
