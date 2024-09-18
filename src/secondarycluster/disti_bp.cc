// Author: DPCfam authors @ Institute for Research and Technologies (Area Science Park) & SISSA.
// Package: DPCfam pipeline
// SPDX-License-Identifier:  BSD-2-Clause
// Description: c++ program to compute distance matrix between primary clusters

// This code uses the moodycamel::ConcurrentQueue ( https://github.com/cameron314/concurrentqueue ) which is freely available under the Simplified BSD Licence

// #include <chrono>
#include <cstring> // memcpy
#include <fstream>
#include <iostream>
#include <map>
#include <math.h>
#include <memory>
#include <unistd.h> // getopt
#include <vector>

#include <pthread.h>

#include <dpcfam/datatypes.h>
#include <dpcfam/normalization.h>
#include <dpcfam/concurrentqueue.h>


#define LOCAL_BUFFER_SIZE 10000
#define PRODUCER_THREADS 3
#define CONSUMER_THREADS 20
#define MAX_qID 20000

#ifdef DIAGONAL
#define COUNT_FACTOR 2
#else
#define COUNT_FACTOR 1
#endif


using namespace moodycamel;

//-------------------------------------------------------------------------
// User defined types
//-------------------------------------------------------------------------

typedef std::map<uint32_t, std::map<uint32_t, Ratio> >  map2_t;

//-------------------------------------------------------------------------
// Global variables
//-------------------------------------------------------------------------

char * bufferA{NULL}, * bufferB{NULL};
uint64_t totalLinesA{0}, totalLinesB{0};

// global flag to terminate consumers
std::atomic<int> doneProducers(0);

// producer rank
uint32_t pRankCount{0};
pthread_mutex_t pRankLock = PTHREAD_MUTEX_INITIALIZER;

// consumer rank
uint32_t cRankCount{0};
pthread_mutex_t cRankLock = PTHREAD_MUTEX_INITIALIZER;

// array of map2_t for consumers
std::vector<map2_t> vec_maps(CONSUMER_THREADS);

// qIDs_partition array for a balanced load among threads. You need (n-1) points to generate n partitions.
std::array <uint64_t, CONSUMER_THREADS-1> qIDs_partition; 

//-------------------------------------------------------------------------
// ALIGNMENTS DISTANCE on the SEARCH
//-------------------------------------------------------------------------
double dist(const SmallCA * i, const SmallCA * j){
    uint16_t hi, lo;
    double inte, uni;
    uint16_t istart, iend, jstart, jend;
    istart = i->sstart; iend = i->send; jstart = j->sstart; jend = j->send;
    //calculate intersection
    inte=0.0;
    hi=std::min(iend,jend);
    lo=std::max(istart,jstart);
    if(hi>lo) inte=hi-lo;
    //calculate union
    hi=std::max(iend,jend);
    lo=std::min(istart,jstart);
    uni=hi-lo;
    return (uni-inte)/uni;
}

//---------------------------------------------------------------------------------------------------------
// Thread methods
//---------------------------------------------------------------------------------------------------------

void balanced_partition(std::array <uint64_t, CONSUMER_THREADS - 1> & array)
// calculate the qIDs to equally distribute the pairs qID1-qID2 in each thread.
// It is an analytical expression based on the probability of finding a pair qID1-qID2, where qID1<qID2 always 
{
    for (int i = 1; i < CONSUMER_THREADS; ++i)
    {
        array[i-1] = MAX_qID*(    1 - sqrt( 1 - i*((MAX_qID-1.)/(MAX_qID*CONSUMER_THREADS)) )  );
    }
    return;
}

// void files_partition(uint64_t (&partIndices)[PRODUCER_THREADS+1][2])
void files_partition(std::array<std::array<uint64_t, 2>, PRODUCER_THREADS + 1> & partIndices, SmallCA* bufferA, SmallCA* bufferB, int rank)
// finds the indices of the partitions for bufferA and bufferB
{

    // Index 0: bufferA and Index 1: bufferB
    SmallCA * alignment[2] = {bufferA, bufferB};
    
    uint64_t partSize[2] = {totalLinesA/PRODUCER_THREADS, totalLinesB/PRODUCER_THREADS};
    uint64_t partReminder[2] = {totalLinesA%PRODUCER_THREADS, totalLinesB%PRODUCER_THREADS};
    
    // uniform partition A
    partIndices[0][0] = 0;
    partIndices[0][1] = 0;
    for (uint32_t i = 1; i <=  PRODUCER_THREADS; ++i)
    {
        for (uint32_t f = 0; f < 2; ++f)
        {
            partIndices[i][f] = partSize[f] + partIndices[i-1][f];
            if (i <= partReminder[f]) {partIndices[i][f] +=1;}      
        }
    }

    // shift to match sID
    for (uint32_t i = 1; i <  PRODUCER_THREADS; ++i)
    {
        // we use bufferA for picking the reference sID
        auto sValue = (alignment[0] + partIndices[i][0])->sID;
        
        for (uint32_t f = 0; f < 2; ++f)
        {
            auto start = std::lower_bound(alignment[f] + partIndices[i-1][f], alignment[f] + partIndices[i+1][f], sValue, compare_sID<SmallCA>());
            partIndices[i][f] -= (alignment[f] + partIndices[i][f]) - start; 
        }
        
    }

    return;
}


void *producer(void *qs) 
{

    ConcurrentQueue<MatchedPair> *queues = (ConcurrentQueue<MatchedPair>*) qs;

    // define own rank
    pthread_mutex_lock(&pRankLock);
    int rank = pRankCount;
    ++pRankCount;
    pthread_mutex_unlock(&pRankLock);

    // find the partition of file A and file B to analize according to rank
    std::array<std::array<uint64_t, 2>, PRODUCER_THREADS + 1> partIndices;
    files_partition(partIndices, (SmallCA*) bufferA, (SmallCA*) bufferB, rank);

    uint64_t linesA = partIndices[rank+1][0] - partIndices[rank][0];
    uint64_t linesB = partIndices[rank+1][1] - partIndices[rank][1];
        
    // define internal buffers
    uint64_t localBufferSize {LOCAL_BUFFER_SIZE}; 
    uint64_t localBufferIndex[CONSUMER_THREADS] = {0};
    MatchedPair **localBuffer = new MatchedPair* [CONSUMER_THREADS];
    for(uint32_t i = 0; i < CONSUMER_THREADS; ++i) 
    {
        localBuffer[i] = new MatchedPair[LOCAL_BUFFER_SIZE]{};
    }
   
    // pointer to correspondant partition of fileA
    SmallCA * alA = (SmallCA *) bufferA  + partIndices[rank][0];  
    // pointer to correspondant partition of fileB
    SmallCA * alB = (SmallCA *) bufferB  + partIndices[rank][1];
    
    
    uint64_t posA{0}, posB{0};
    uint32_t s0{0};

    while(posA<linesA && posB<linesB)
    {
        while(posB<linesB && alB->sID<alA->sID)
        {
            ++alB;
            ++posB;
        }
        while(posA<linesA && alB->sID>alA->sID)
        {
            ++alA;
            ++posA;
        }

        if(alB->sID == alA->sID) 
        {
            s0 = alA->sID;
            const SmallCA * init_subA = alA;
            const SmallCA * init_subB = alB;
            
            while(posA<linesA && alA->sID == s0 ) //cerca tutti gli altri che seguono con la stessa sID (sono sortati wrt sID apposta..) ---- A
            {
                ++alA;
                ++posA;
            }
            // std::cout << icount << std::endl;

            while(posB<linesB && alB->sID == s0 ) //cerca tutti gli altri che seguono con la stessa sID (sono sortati wrt sID apposta..) ---- B
            {
                ++alB;
                ++posB;
            }            
            
            //compute distances on selected alignments with the same sID
            for (auto pA = init_subA; pA < alA; ++pA)
            {
                for (auto pB = init_subB; pB < alB; ++pB)
                {   

                    // check if match
                    if( dist(pA,pB) <= 0.2 ) 
                    {

                        auto qID1 = std::min(pA->qID, pB->qID);
                        auto qID2 = std::max(pA->qID, pB->qID);
                        auto norm = std::min(pA->qSize, pB->qSize);
                        // auto norm = 1.;
                

                        #ifdef DIAGONAL
                        if (qID1 == qID2) continue;
                        #endif

                        auto pair = MatchedPair( qID1, qID2, COUNT_FACTOR*norm );
            
                        // decide to which queue the pair goes
                        int tidx = CONSUMER_THREADS - 1;
                        for (int i = 0; i < CONSUMER_THREADS-1; ++i)
                        {
                            if(qID1 < qIDs_partition[i])
                            {
                                tidx = i;
                                break;
                            }
                        }
                        
                        localBuffer[tidx][localBufferIndex[tidx]] = pair;
                        
                        ++localBufferIndex[tidx];
                        if (localBufferIndex[tidx] >= localBufferSize)
                        {
                            queues[tidx].enqueue_bulk(localBuffer[tidx], localBufferSize);
                            localBufferIndex[tidx]=0;
                        }
                        
                    }                    
                }
            }            
        }   
    }

    // Fill the remaining local buffer with zero elements (MatchedPair())
    for (int i = 0; i < CONSUMER_THREADS; ++i)
    {
        for (int j = localBufferIndex[i]; j < LOCAL_BUFFER_SIZE; ++j)
        {
            localBuffer[i][j]=MatchedPair();
        }
        // enqueue the last local buffer
        queues[i].enqueue_bulk(localBuffer[i], LOCAL_BUFFER_SIZE);
    }

    for(uint32_t i = 0; i < CONSUMER_THREADS; ++i) 
    {
        delete [] localBuffer[i];
    }
    delete [] localBuffer;
    
    doneProducers.fetch_add(1, std::memory_order_release);
    pthread_exit(NULL);

}

void *consumer(void *qs) 
{
    pthread_mutex_lock(&cRankLock);    
    int rank = cRankCount;
    ++cRankCount;
    pthread_mutex_unlock(&cRankLock);
    
    // Access queue corresponding to the rank of the consumer
    ConcurrentQueue<MatchedPair> * queue = ((ConcurrentQueue<MatchedPair>*) qs) + rank;

    // MatchedPair pairs[LOCAL_BUFFER_SIZE];
    MatchedPair *pairs = new MatchedPair [LOCAL_BUFFER_SIZE]();
    std::pair<std::map<uint32_t,Ratio>::iterator,bool> ret;

    bool itemsLeft;
    do {
        // It's important to fence (if the producers have finished) *before* dequeueing
        itemsLeft = doneProducers.load(std::memory_order_acquire) != PRODUCER_THREADS;
        while (queue->try_dequeue_bulk(pairs, LOCAL_BUFFER_SIZE)) 
        {
            itemsLeft = true;
            // consume dequeue data
            for (int i = 0; i < LOCAL_BUFFER_SIZE; ++i)
            {
                Ratio countNorm(1,pairs[i].normFactor);

                ret = vec_maps[rank][ pairs[i].ID1 ].insert(std::pair<uint32_t, Ratio>(pairs[i].ID2, countNorm) );
                if (ret.second==false)
                {
                    ret.first->second.num+=1;
                }
            }
        }
    } while (itemsLeft);
    

    delete[] pairs;
    // terminate thread
    pthread_exit(NULL);

}


calculate_block_distance(const std::string& inputFile, const std::string& outputFile, int producers, int consumers) {
    // Open both files containing clustered alignments
    std::ifstream infileA (input1, std::ifstream::binary);
    std::ifstream infileB (input2, std::ifstream::binary);
    unsigned long int bytesA{0}, bytesB{0};
    if (infileA)
    {
        infileA.seekg (0, infileA.end);
        bytesA = infileA.tellg();
        infileA.seekg (0, infileA.beg);
        bufferA = new char [bytesA];
        totalLinesA = bytesA/sizeof(SmallCA);

        std::cout << "Reading " << bytesA << " characters... ";
        // read data as a block:
        infileA.read (bufferA,bytesA);
        std::cout << "all characters read successfully from " << input1 << "\n";
    }
    else
    {
      std::cout << "error: only " << infileA.gcount() << " could be read \n";
      return 1;
    }
    infileA.close();
    
    if (infileB)
    {
        infileB.seekg (0, infileB.end);
        bytesB = infileB.tellg();
        infileB.seekg (0, infileB.beg);
        bufferB = new char [bytesB];
        totalLinesB = bytesB/sizeof(SmallCA);

        std::cout << "Reading " << bytesB << " characters... ";
        // read data as a block:
        infileB.read (bufferB,bytesB);
        std::cout << "all characters read successfully from " << input2 << "\n";
    }
    else
    {
      std::cout << "error: only " << infileB.gcount() << " could be read \n";
      return 1;
    }
    infileB.close();

    
    // Initialize queues
    std::array<ConcurrentQueue<MatchedPair>, CONSUMER_THREADS> queues;
    // for (auto & q: queues)
        // q = ConcurrentQueue<MatchedPair>(5000000);

    // Define qIDs balanced partition per thread
    balanced_partition(qIDs_partition);
    

    // auto t1_processing = std::chrono::high_resolution_clock::now();   
    
    // Producer thread
    pthread_t producerThreads[PRODUCER_THREADS];
    for (int i = 0; i < PRODUCER_THREADS; ++i)
    {
        // Create attributes
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&producerThreads[i], &attr, producer, &queues);
    }

    // Consumer threads
    pthread_t consumerThreads[CONSUMER_THREADS];
    for (int i = 0; i < CONSUMER_THREADS; ++i)
    {
        // Create attributes
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&consumerThreads[i], &attr, consumer, &queues);
    } 

    /* Producer-consumer running */

    // Join threads when finished
    for (int i = 0; i < PRODUCER_THREADS; ++i)
    {
        pthread_join(producerThreads[i], NULL);    
    }

    // std::cout << "PRODUCERS DONE!" << '\n';

    // auto t_producer = std::chrono::high_resolution_clock::now();   
    for (int i = 0; i < CONSUMER_THREADS; ++i)
    {
        pthread_join(consumerThreads[i], NULL);    
    }



    // PRINT MAP
    std::cout << "Writing to " << output << "... ";
    auto outfile = std::fstream(output, std::ios::out | std::ios::binary);
    NormalizedPair outLine;

    for (int tidx = 0; tidx < CONSUMER_THREADS; ++tidx)
    {
        for (auto itr_out = vec_maps[tidx].cbegin(); itr_out != vec_maps[tidx].cend(); ++itr_out) { 
            if (itr_out->first == 0) {continue;}
            for (auto itr_in = itr_out->second.cbegin(); itr_in != itr_out->second.cend(); ++itr_in)
            {        
                auto distance = 1.- itr_in->second.as_double();
                outLine.ID1 = itr_out->first; 
                outLine.ID2 = itr_in->first;
                outLine.distance = distance;  
                outfile.write((char*)&outLine, sizeof(NormalizedPair));
            }   
        } 
    }

    outfile.close();


    delete[] bufferA;
    delete[] bufferB;
     
    return 0;
}






