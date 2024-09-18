#include <sstream>
#include <dpcstruct/types/Alignment.h>
#include <dpcstruct/fileparser/AlnsFileParser.h>

AlnsFileParser::AlnsFileParser(const std::string& filename)
    : filename(filename), data(filename, MemoryMapped::WholeFile, MemoryMapped::SequentialScan) {
    if (!data.isValid()) {
        throw std::runtime_error("Failed to map file: " + filename);
    }
}

void AlnsFileParser::loadAlignments(std::vector<Alignment>& aligns, uint64_t skipRows) {
    const char* buffer = reinterpret_cast<const char*>(data.getData());
    uint64_t dataSize = data.size();

    if (dataSize == 0) {
        throw std::runtime_error("File is empty: " + filename);
    }

    int numThreads = omp_get_max_threads();
    std::vector<std::vector<Alignment>> threadAlignments(numThreads);

    // Skip the first `skipRows` lines
    uint64_t pos = 0;
    uint64_t skipped = 0;
    while (skipped < skipRows && pos < dataSize) {
        if (buffer[pos] == '\n') {
            skipped++;
        }
        pos++;
    }

    // Parallel processing of the remaining file
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        uint64_t chunkSize = (dataSize - pos) / numThreads;
        uint64_t start = pos + tid * chunkSize;
        uint64_t end = (tid == numThreads - 1) ? dataSize : start + chunkSize;

        // Adjust the start position to the beginning of the next line
        if (tid != 0) {
            while (start < end && buffer[start] != '\n') {
                ++start;
            }
            if (start < end) {
                ++start;  // Move past the newline character
            }
        }

        // Process the chunk
        std::vector<Alignment>& localAligns = threadAlignments[tid];
        uint64_t localPos = start;
        while (localPos < end) {
            const char* lineStart = buffer + localPos;
            const char* lineEnd = lineStart;

            while (lineEnd < buffer + dataSize && *lineEnd != '\n') {
                ++lineEnd;
            }

            parseLine(lineStart, lineEnd, localAligns);

            localPos = lineEnd - buffer + 1;  // Move to the start of the next line
        }
    }

    // Combine results from all threads
    for (int i = 0; i < numThreads; ++i) {
        aligns.insert(aligns.end(), threadAlignments[i].begin(), threadAlignments[i].end());
    }
}


void AlnsFileParser::parseLine(const char* lineStart, const char* lineEnd, std::vector<Alignment>& localAligns) {
    uint32_t queryID, queryStart, queryEnd, queryLength;
    uint32_t searchID, searchStart, searchEnd, searchLength;
    uint32_t alnLength, bits;
    double pident, evalue;
    double tmScore, lddt;

    std::stringstream ss(std::string(lineStart, lineEnd - lineStart));

    if (ss >> queryID >> searchID >> queryStart >> queryEnd >> searchStart >> searchEnd >>
        queryLength >> searchLength >> alnLength >> pident >> evalue >> bits >> tmScore >> lddt) {
        localAligns.emplace_back(queryID, searchID, queryStart, queryEnd, searchStart, searchEnd,
                                 queryLength, searchLength, alnLength, pident, evalue, bits, tmScore, lddt);
    }
}
