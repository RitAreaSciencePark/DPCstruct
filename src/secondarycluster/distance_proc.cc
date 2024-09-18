#include <map>
#include <pthread.h>
#include <math.h>
#include <cstdint>
#include <fstream>


#include <dpcstruct/secondarycluster/distance_proc.h>
#include <dpcstruct/fileparser/PCsFileParser.h>
#include <dpcstruct/distance.h>
#include <moodycamel/concurrentqueue.h>

using namespace moodycamel;

typedef std::map<uint32_t, std::map<uint32_t, Ratio> >  map2_t;

#define LOCAL_BUFFER_SIZE 10000 // Buffer size for each producer


void balanced_partition(std::vector<uint64_t>& partitionArray, SmallPC* buffer, uint64_t totalLines, int numConsumers) {
    // Find the maximum searchID in the buffer
    uint64_t maxSearchID = 0;
    for (uint64_t i = 0; i < totalLines; ++i) {
        if (buffer[i].sID > maxSearchID) {
            maxSearchID = buffer[i].sID;
        }
    }

    // Calculate the balanced partitions based on maxSearchID
    for (int i = 1; i < numConsumers; ++i) {
        partitionArray[i - 1] = maxSearchID * (1 - sqrt(1 - i * ((maxSearchID - 1.0) / (maxSearchID * numConsumers))));
    }

    return;
}

void files_partition(std::vector<std::array<uint64_t, 2>>& partIndices, 
                     SmallPC* bufferA, 
                     SmallPC* bufferB, 
                     uint64_t totalLinesA, 
                     uint64_t totalLinesB, 
                     int numProducers) {

    // Index 0: bufferA and Index 1: bufferB
    SmallPC * alignment[2] = {bufferA, bufferB};
    
    uint64_t partSize[2] = {totalLinesA/numProducers, totalLinesB/numProducers};
    uint64_t partReminder[2] = {totalLinesA%numProducers, totalLinesB%numProducers};
    
    // uniform partition A
    partIndices[0][0] = 0;
    partIndices[0][1] = 0;
    for (uint32_t i = 1; i <=  numProducers; ++i)
    {
        for (uint32_t f = 0; f < 2; ++f)
        {
            partIndices[i][f] = partSize[f] + partIndices[i-1][f];
            if (i <= partReminder[f]) {partIndices[i][f] +=1;}      
        }
    }

    // shift to match sID
    for (uint32_t i = 1; i <  numProducers; ++i)
    {
        // we use bufferA for picking the reference sID
        auto sValue = (alignment[0] + partIndices[i][0])->sID;
        
        for (uint32_t f = 0; f < 2; ++f)
        {
            auto start = std::lower_bound(alignment[f] + partIndices[i-1][f], alignment[f] + partIndices[i+1][f], sValue, compare_sID<SmallPC>());
            partIndices[i][f] -= (alignment[f] + partIndices[i][f]) - start; 
        }
        
    }

    return;
}


void *producer(void *td) 
{

    ThreadData* threadData = static_cast<ThreadData*>(td);

    // Define own rank
    threadData->rankMutex.lock();
    int rank = threadData->producerRankCount;
    ++threadData->producerRankCount;
    threadData->rankMutex.unlock();

    uint64_t linesA = threadData->partIndices[rank+1][0] - threadData->partIndices[rank][0];
    uint64_t linesB = threadData->partIndices[rank+1][1] - threadData->partIndices[rank][1];

    // define internal buffers
    uint64_t localBufferSize {LOCAL_BUFFER_SIZE}; 
    uint64_t localBufferIndex[threadData->numConsumers] = {0};
    MatchedPair **localBuffer = new MatchedPair* [threadData->numConsumers];
    for(uint32_t i = 0; i < threadData->numConsumers; ++i) 
    {
        localBuffer[i] = new MatchedPair[LOCAL_BUFFER_SIZE]{};
    }
   
    // pointer to correspondant partition of buffers
    SmallPC * alA = (SmallPC *) threadData->bufferA + threadData->partIndices[rank][0];  
    SmallPC * alB = (SmallPC *) threadData->bufferB + threadData->partIndices[rank][1];
    
    
    uint64_t posA{0}, posB{0};
    uint32_t s0{0};

    while(posA < linesA && posB < linesB) {
        while(posB < linesB && alB->sID < alA->sID) {
            ++alB;
            ++posB;
        }
        while(posA < linesA && alB->sID > alA->sID) {
            ++alA;
            ++posA;
        }

        if(alB->sID == alA->sID) {
            s0 = alA->sID;
            const SmallPC * init_subA = alA;
            const SmallPC * init_subB = alB;
            
            while(posA < linesA && alA->sID == s0 ) {
                ++alA;
                ++posA;
            }

            while(posB<linesB && alB->sID == s0 ) {
                ++alB;
                ++posB;
            }            
            
            //compute distances on selected alignments with the same sID
            for (auto pA = init_subA; pA < alA; ++pA) {
                for (auto pB = init_subB; pB < alB; ++pB) {   

                    // check if match
                    if( distance(pA,pB) <= 0.2 ) 
                    {

                        auto qID1 = std::min(pA->qID, pB->qID);
                        auto qID2 = std::max(pA->qID, pB->qID);
                        auto norm = std::min(pA->qSize, pB->qSize);
                        // auto norm = 1.;
                
                        if (qID1 == qID2) continue;

                        auto pair = MatchedPair( qID1, qID2, threadData->countFactor*norm );
            
                        // decide to which queue the pair goes
                        int tidx = threadData->numConsumers - 1;
                        for (int i = 0; i < threadData->numConsumers - 1; ++i) {
                            if (qID1 < threadData->qIDs_partition[i]) {
                                tidx = i;
                                break;
                            }
                        }
                        
                        localBuffer[tidx][localBufferIndex[tidx]] = pair;
                        ++localBufferIndex[tidx];
                        if (localBufferIndex[tidx] >= localBufferSize) {
                            threadData->queues[tidx].enqueue_bulk(localBuffer[tidx], localBufferSize);
                            localBufferIndex[tidx] = 0;
                        }
                        
                    }                    
                }
            }            
        }   
    }

    // Fill remaining local buffer with zero elements (MatchedPair())
    for (int i = 0; i < threadData->numConsumers; ++i) {
        for (int j = localBufferIndex[i]; j < localBufferSize; ++j) {
            localBuffer[i][j] = MatchedPair();
        }
        // Enqueue the last local buffer
        threadData->queues[i].enqueue_bulk(localBuffer[i], localBufferSize);
    }

    // Clean up local buffer
    for (uint32_t i = 0; i < threadData->numConsumers; ++i) {
        delete[] localBuffer[i];
    }
    delete[] localBuffer;
    
    threadData->doneProducers.fetch_add(1, std::memory_order_release);
    pthread_exit(NULL);
}


void *consumer(void *td) 
{
        
    ThreadData* threadData = static_cast<ThreadData*>(td);
    threadData->rankMutex.lock();
    int rank = threadData->consumerRankCount;
    ++threadData->consumerRankCount;
    threadData->rankMutex.unlock();

    // Access the queue corresponding to the consumer rank
    ConcurrentQueue<MatchedPair>* queue = &(threadData->queues[rank]);

    // Allocate local buffer for dequeued items
    MatchedPair* pairs = new MatchedPair[LOCAL_BUFFER_SIZE]();
    std::pair<std::map<uint32_t, Ratio>::iterator, bool> ret;

    bool itemsLeft;
    do {
        // Fence to check if producers are still active
        itemsLeft = threadData->doneProducers.load(std::memory_order_acquire) != threadData->numProducers;
        while (queue->try_dequeue_bulk(pairs, LOCAL_BUFFER_SIZE)) 
        {
            itemsLeft = true;
            // Consume dequeued data
            for (int i = 0; i < LOCAL_BUFFER_SIZE; ++i)
            {
                Ratio countNorm(1, pairs[i].normFactor);

                ret = threadData->vec_maps[rank][pairs[i].ID1].insert(std::make_pair(pairs[i].ID2, countNorm));
                if (ret.second == false)
                {
                    ret.first->second.num += 1;
                }
            }
        }
    } while (itemsLeft);

    // Clean up allocated memory
    delete[] pairs;
    
    // Terminate thread
    pthread_exit(NULL);
}




void calculate_block_distance(const std::string& inputFileA, 
                        const std::string& inputFileB,
                        const std::string& outputFile,
                        int numProducers,
                        int numConsumers) {  

    try {

        std::cout << "Calculating distance matrix between primary clusters..." << std::endl;
        std::cout << "Input file A: " << inputFileA << std::endl;
        std::cout << "Input file B: " << inputFileB << std::endl;
        std::cout << "Output file: " << outputFile << std::endl;
        std::cout << "Producers: " << numProducers << std::endl;
        std::cout << "Consumers: " << numConsumers << std::endl;

        // Open both files containing clustered alignments
        PCsFileParser loaderA(inputFileA);
        PCsFileParser loaderB(inputFileB);

        // Load the primary clusters
        loaderA.loadPCs();
        loaderB.loadPCs();

        // Access the loaded data
        SmallPC* pcsBufferA = loaderA.getData();
        SmallPC* pcsBufferB = loaderB.getData();
        uint64_t totalLinesA = loaderA.getTotalLines();
        uint64_t totalLinesB = loaderB.getTotalLines();

        // Now pcsBufferA and pcsBufferB point to the loaded data
        std::cout << "Total lines from file A: " << totalLinesA << std::endl;
        std::cout << "Total lines from file B: " << totalLinesB << std::endl;

        // Data for threads
        ThreadData threadData(numProducers, numConsumers);  // Constructor handles initialization
        threadData.bufferA = reinterpret_cast<char*>(pcsBufferA);
        threadData.bufferB = reinterpret_cast<char*>(pcsBufferB);
        threadData.totalLinesA = totalLinesA;
        threadData.totalLinesB = totalLinesB;

        // if filenames are the same, countFactor is 2
        if (inputFileA == inputFileB) {
            threadData.countFactor = 2.0;
        }

        // Define qIDs balanced partition per thread (based on only one file)
        balanced_partition(threadData.qIDs_partition, pcsBufferA, totalLinesA, numConsumers);

        // Partition files
        std::vector<std::array<uint64_t, 2>> partIndices(numProducers + 1);
        files_partition(threadData.partIndices, pcsBufferA, pcsBufferB, totalLinesA, totalLinesB, numProducers);

        // Create producer threads
        std::vector<pthread_t> producerThreads(numProducers);
        for (int i = 0; i < numProducers; ++i) {
            pthread_create(&producerThreads[i], nullptr, producer,(void*)&threadData);
        }
        
        pthread_t consumerThreads[numConsumers];
        for (int i = 0; i < numConsumers; ++i) {
            pthread_create(&consumerThreads[i], NULL, consumer, (void*)&threadData);
        }

        // /* Producer-consumer running */

        // // Join producer threads
        for (int i = 0; i < numProducers; ++i) {
            pthread_join(producerThreads[i], nullptr);
        }

        // Join consumer threads
        for (int i = 0; i < numConsumers; ++i) {
            pthread_join(consumerThreads[i], nullptr);
        }

        // PRINT MAP
        std::cout << "Writing to " << outputFile << "... ";
        std::fstream outfile(outputFile, std::ios::out | std::ios::binary);
        NormalizedPair outLine;

        for (int tidx = 0; tidx < threadData.numConsumers; ++tidx)
        {
            for (auto itr_out = threadData.vec_maps[tidx].cbegin(); itr_out != threadData.vec_maps[tidx].cend(); ++itr_out) 
            { 
                if (itr_out->first == 0) continue;  // Skip entries with key == 0

                for (auto itr_in = itr_out->second.cbegin(); itr_in != itr_out->second.cend(); ++itr_in)
                {        
                    auto distance = 1.0 - itr_in->second.as_double();
                    outLine.ID1 = itr_out->first; 
                    outLine.ID2 = itr_in->first;
                    outLine.distance = distance;

                    outfile.write((char*)&outLine, sizeof(NormalizedPair));  // Write to binary output file
                }   
            } 
        }

        // Close the output file
        outfile.close();
        std::cout << "Done writing" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        throw;
    }
}

// Function that calculates distances between primary clusters (dummy implementation)
void calculate_distance_matrix(const std::string& inputFileA, 
                            const std::string& inputFileB,
                            const std::string& outputFile,
                            int producers,
                            int consumers) {    
    // TODO: Handle whole distance matrix calculation here
    // For now its just a placeholder. It handles only one block
    calculate_block_distance(inputFileA, inputFileB, outputFile, producers, consumers);
    
}



