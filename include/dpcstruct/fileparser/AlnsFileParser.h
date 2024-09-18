#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <omp.h>  // Include OpenMP header

#include <dpcstruct/types/Alignment.h>
#include <memorymapped/MemoryMapped.h>

class AlnsFileParser {
public:
    AlnsFileParser(const std::string& filename);
    void loadAlignments(std::vector<Alignment>& aligns, uint64_t skipRows = 0);

private:
    std::string filename;
    MemoryMapped data;

    void parseLine(const char* lineStart, const char* lineEnd, std::vector<Alignment>& localAligns);
};
