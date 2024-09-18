#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cassert>
#include <unordered_map>

#include <commandparser/CommandParser.h>
#include <dpcstruct/types/PrimaryCluster.h>



template<typename T>
int load_file(std::string filename, std::vector<T> & vector) {
    std::ifstream inFile (filename, std::ifstream::binary);
    
    size_t bytes{0};
    size_t totalEntries{0};

    if (inFile) {
        inFile.seekg (0, inFile.end);
        bytes = inFile.tellg();
        inFile.seekg (0, inFile.beg);
        totalEntries = bytes/sizeof(T);

        vector.resize(totalEntries);

        // read data as a block into vector:
        inFile.read(reinterpret_cast<char*>(vector.data()), bytes); // or &buf[0] for C++98
    }
    else {
        throw std::ios_base::failure( "File " + filename + " could not be read." );  
    }
    inFile.close();

    return 0;
}

//-------------------------------------------------------------------------
// ALIGNMENTS DISTANCE on the SEARCH
//-------------------------------------------------------------------------
double dist(const SequenceLabel & i, const SequenceLabel & j) {
    uint16_t hi, lo;
    double inte, uni;
    uint16_t istart, iend, jstart, jend;
    istart = i.sstart; iend = i.send; jstart = j.sstart; jend = j.send;
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
// < 0.2

std::vector<SequenceLabel> filtering(std::vector<SequenceLabel> & sequences)
{
    
    std::vector<SequenceLabel> sequencesFiltered;
    sequencesFiltered.reserve(sequences.size()/10);
    // std::vector<double> distances;
    // distances.reserve(sequences.size()/10);

    auto end = sequences.cend();
    auto low = sequences.cbegin();

    while (low != end)
    {
        // find last occurrence with same label
        uint32_t labelref = low->label;
        auto high_l = std::upper_bound(low, end, labelref, compare_label());
    
        while (low != high_l)
        {   
            std::vector<SequenceLabel> sameSearchBuff;
            sameSearchBuff.reserve(100000);
            uint32_t sIDref = low->sID;

            // find last occurrence
            auto high = std::upper_bound(low, high_l, sIDref, compare_sID<SequenceLabel>());

            assert((high-1)->label == low->label);
            assert((high-1)->sID == low->sID);

            // compute the difference
            auto count = high - low;
            // std::cout << "count: " << count << '\n';
            // std::cout << "sIDref: " << sIDref << '\n';


            // if only one 
            if(count == 1){low = low + count; continue;}
            


            // choose sequences which appear more than one
            // and that overlaps with itself.
            // (e.g. 12314 12 41 && 12314 10 39 [sID ss se])
            for (auto itr_i = low; itr_i < high; ++itr_i)
            {
                for (auto itr_j = low; itr_j < high; ++itr_j)
                {
                    if(itr_i == itr_j){continue;}

                    if (dist(*itr_i, *itr_j)<0.2)
                    {
                        sameSearchBuff.emplace_back(*itr_i);
                        break;
                    }
                }
            }


            if (sameSearchBuff.size()==0){low = low + count; continue;}

            // calculate average sequence among overlapped seq
            SequenceLabel avgSequence;
            avgSequence.sID = sIDref;
            size_t sstartAcc{0}; // avoid avgSequence overflow
            size_t sendAcc{0};
            for (const auto & seq: sameSearchBuff)
            {
                sstartAcc += seq.sstart;
                sendAcc += seq.send;
            }

            avgSequence.sstart = sstartAcc / sameSearchBuff.size();
            avgSequence.send = sendAcc / sameSearchBuff.size();


            // find the sequence which best represents the average sequence (dist metric) 
            double minDist = 1;
            SequenceLabel exponentSequence;
            for (auto & seq: sameSearchBuff)
            {
                auto distToAvg = dist(avgSequence, seq);
                // std::cout << "distToAvg: " << distToAvg << '\n';
                if (minDist >= distToAvg)
                {
                    exponentSequence = seq;
                    minDist = distToAvg;
                }
            }

            sequencesFiltered.push_back(exponentSequence); 
            // distances.push_back(minDist);

            // std::cout << "exponentSequence: ";
            // printSL(exponentSequence);

            // move to next element in vector (not immediate next)
            low = low + count;

        }

    }

    return sequencesFiltered;
}



std::vector<std::pair<SequenceLabel, double>> filtering_dist(std::vector<SequenceLabel> & sequences)
{
    
    std::vector<std::pair<SequenceLabel, double>> sequencesFiltered;
    sequencesFiltered.reserve(sequences.size()/10);

    auto end = sequences.cend();
    auto low = sequences.cbegin();

    while (low != end)
    {
        // find last occurrence with same label
        uint32_t labelref = low->label;
        auto high_l = std::upper_bound(low, end, labelref, compare_label());
    
        while (low != high_l)
        {   
            std::vector<SequenceLabel> sameSearchBuff;
            sameSearchBuff.reserve(100000);
            uint32_t sIDref = low->sID;

            // find last occurrence
            auto high = std::upper_bound(low, high_l, sIDref, compare_sID<SequenceLabel>());

            // assert((high-1)->label == low->label);
            // assert((high-1)->sID == low->sID);

            // compute the difference
            auto count = high - low;
            
            if(count == 1){low = low + count; continue;}
            


            // choose sequences which appear more than one
            // and that overlaps with itself.
            // (e.g. 12314 12 41 && 12314 10 39 [sID ss se])
            for (auto itr_i = low; itr_i < high; ++itr_i)
            {
                for (auto itr_j = low; itr_j < high; ++itr_j)
                {
                    if(itr_i == itr_j){continue;}

                    if (dist(*itr_i, *itr_j)<0.2)
                    {
                        sameSearchBuff.emplace_back(*itr_i);
                        break;
                    }
                }
            }


            if (sameSearchBuff.size()==0){low = low + count; continue;}

            // calculate average sequence among overlapped seq
            SequenceLabel avgSequence;
            avgSequence.sID = sIDref;
            size_t sstartAcc{0}; // avoid avgSequence overflow
            size_t sendAcc{0};
            for (const auto & seq: sameSearchBuff)
            {
                sstartAcc += seq.sstart;
                sendAcc += seq.send;
            }

            avgSequence.sstart = sstartAcc / sameSearchBuff.size();
            avgSequence.send = sendAcc / sameSearchBuff.size();

            // std::cout << "avg sequence: ";
            // printSL(avgSequence);

            // find the sequence which best represents the average sequence (dist metric) 
            double minDist = 1;
            SequenceLabel exponentSequence;
            for (auto & seq: sameSearchBuff)
            {
                auto distToAvg = dist(avgSequence, seq);
                // std::cout << "distToAvg: " << distToAvg << '\n';
                if (minDist >= distToAvg)
                {
                    exponentSequence = seq;
                    minDist = distToAvg;
                }
            }



            sequencesFiltered.push_back(std::make_pair(exponentSequence,minDist)); 
            // distances.push_back(minDist);

            // std::cout << "exponentSequence: ";
            // printSL(exponentSequence);

            // move to next element in vector (not immediate next)
            low = low + count;

        }

    }

    return sequencesFiltered;
}

int main(int argc, char* argv[]) {

    std::unordered_map<uint32_t, std::string> dict;
    std::vector<SequenceLabel> sequences;
    // std::vector<SequenceLabel> sequencesFiltered;
    std::vector<std::pair<SequenceLabel, double>> sequencesFiltered;

    // Define program options
    std::vector<Option> options = {
        {'i', "INPUT", "input filename"},
        {'o', "OUTPUT", "output filename",false},
    };

    // Define the option string and program description
    std::string optstring = "i:o:";
    std::string program_desc = "filters the results produced by traceback.cc.";

    // Create an OptionParser instance
    OptionParser parser(options, optstring, program_desc);

    // Parse options
    auto parsed_args = parser.parse(argc, argv);
    std::string inFilename = parsed_args["i"];
    std::string outFilename = parsed_args["o"];


    // if output filename is not provided, create one
    if (! parsed_args.count("o")){
        std::string base_inFilename = inFilename.substr(inFilename.find_last_of("/\\") + 1);
        std::string::size_type const p(base_inFilename.find_last_of('.'));
        outFilename = base_inFilename.substr(0, p) + "_filtered.txt";
    }

    std::cout << "Input file: " << inFilename << std::endl;
    std::cout << "Output file: " << outFilename << std::endl;

    load_file(inFilename, sequences);

    // TODO: check if sequences sorted wrt 1st sID and 2nd MClabel

    // std::cout << "Filtering!" << std::endl;
    sequencesFiltered = filtering_dist(sequences);


    std::ofstream outFile(outFilename);
    for (size_t i = 0; i < sequencesFiltered.size(); ++i)
    {
        outFile <<  sequencesFiltered[i].first.sID << " " << sequencesFiltered[i].first.sstart << " " 
        << sequencesFiltered[i].first.send <<  " " << sequencesFiltered[i].first.label << '\n';   

        // std::cout << ' ' << sequencesFiltered[i].second << '\n';
    }

    outFile.close();

    return 0;
}
