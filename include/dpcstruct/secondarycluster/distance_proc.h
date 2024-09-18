#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <atomic>
#include <mutex>
#include <map>

#include <dpcstruct/types/ProducerConsumer.h>
#include <dpcstruct/types/PrimaryCluster.h>
#include <moodycamel/concurrentqueue.h>

using namespace moodycamel;


typedef std::map<uint32_t, std::map<uint32_t, Ratio> >  map2_t;

struct ThreadData {
    int numConsumers;
    int numProducers;

    char* bufferA;
    char* bufferB;
    uint64_t totalLinesA;
    uint64_t totalLinesB;

    std::atomic<int> doneProducers;
    uint32_t producerRankCount;
    uint32_t consumerRankCount;

    std::vector<map2_t> vec_maps;    // Vector of maps for storing results, one per consumer
    std::vector<ConcurrentQueue<MatchedPair>> queues;  // Queues for communication between producers and consumers

    std::vector<uint64_t> qIDs_partition;  // Balanced partition of qIDs for `numConsumers-1`
    std::vector<std::array<uint64_t, 2>> partIndices;  // Partition indices for the files

    double countFactor; 

    std::mutex rankMutex;  // mutex to control rank counters

    
    // Constructor
    ThreadData(int numProducers, int numConsumers)
        : numProducers(numProducers), numConsumers(numConsumers), bufferA(nullptr), bufferB(nullptr), 
          totalLinesA(0), totalLinesB(0), doneProducers(0), producerRankCount(0), consumerRankCount(0),
          vec_maps(numConsumers), queues(numConsumers), qIDs_partition(numConsumers - 1, 0), partIndices(numProducers + 1), 
          countFactor(1.0) {}
};
void balanced_partition(std::vector<uint64_t>& partitionArray, SmallPC* buffer, uint64_t totalLines, int numConsumers);
void calculate_distance_matrix(const std::string& inputFileA, const std::string& inputFileB, const std::string& outputFile, int numProducers, int numConsumers);
void calculate_block_distance(const std::string& inputFile, const std::string& inputFileB, const std::string& outputFile, int numProducers, int numConsumers);
