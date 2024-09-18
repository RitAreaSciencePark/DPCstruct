#include <iostream>
#include <omp.h>
#include <vector>
#include <span>

#include <dpcstruct/primarycluster_core.h>
#include <dpcstruct/primarycluster_proc.h>

std::vector<int> process_by_query(const std::vector<Alignment>& alignments, int numThreads) {

    // Set number of threads
    omp_set_num_threads(numThreads);

    if (alignments.empty()) {
        throw std::invalid_argument("The alignments vector is empty");
    }

    size_t numAlns = alignments.size();
    
    std::vector<std::pair<size_t, size_t>> chunks;  

    // Identify ranges where queryID is the same
    size_t start = 0;
    for (size_t i = 1; i <= numAlns; ++i) {
        if (i == numAlns || alignments[i].queryID != alignments[start].queryID) {
            // Found a chunk from start to i-1
            chunks.push_back({start, i});
            start = i;
        }
    }

    // define labels vector
    std::vector<int> labels(alignments.size(), -1);

    // Parallel processing of each chunk
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < chunks.size(); ++i) {
        size_t start = chunks[i].first;
        size_t end = chunks[i].second;

        // Process alignments for the current chunk
        std::span<const Alignment> perQueryAlns(alignments.data() + start, end - start);
        std::vector<int> perQueryLabels = cluster_alignments(perQueryAlns, 0.2);
        
        std::copy(perQueryLabels.begin(), perQueryLabels.end(), labels.begin() + start);        
    }

    return labels;
}