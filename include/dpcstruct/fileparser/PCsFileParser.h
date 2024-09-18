#pragma once

#include <vector>
#include <dpcstruct/types/PrimaryCluster.h>
#include <dpcstruct/fileparser/PCsFileParser.h>
#include <memorymapped/MemoryMapped.h>

class PCsFileParser {
public:
    // Constructor initializes the filename
    explicit PCsFileParser(const std::string& filename);
    
    // Destructor to clean up the allocated buffer
    ~PCsFileParser();

    // Function to load primary clusters into the buffer
    void loadPCs();

    // Access to the buffer
    SmallPC* getData() const { return reinterpret_cast<SmallPC*>(data); }

    // Access to the total number of lines
    uint64_t getTotalLines() const { return totalLines; }

private:
    std::string filename;
    char* data;      // Pointer to the buffer holding raw binary data
    uint64_t totalLines;  // Total number of loaded lines
    size_t dataSize;  // Size of the buffer

    // Helper function to read binary data from file
    void readBinaryData(size_t elementSize);

};