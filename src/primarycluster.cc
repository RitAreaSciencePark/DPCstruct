// Author: DPCfam authors @ Institute for Research and Technologies (Area Science Park) & SISSA.
// Package: DPCfam pipeline
// SPDX-License-Identifier:  BSD-2-Clause
// Description: C++ program to perform primary clustering of local pairwise alignments on a single query.
// USAGE:  ./a.out  input_FOLDER  output_FOLDER dpar gpar
// input is a BLAST output obtained from:
// blastp  -query $myquery -db $mydb -evalue 0.1 -max_target_seqs 200000 -out $myout -outfmt "6 qseqid sseqid qstart qend sstart send qlen slen length pident evalue score"


#include <filesystem>
#include <iostream>
#include <fstream>
#include <omp.h>
#include <span>

#include <dpcstruct/fileparser/AlnsFileParser.h>
#include <dpcstruct/primarycluster_core.h>
#include <dpcstruct/primarycluster_proc.h>
#include <dpcstruct/types/Alignment.h>
#include <dpcstruct/types/PrimaryCluster.h>
#include <dpcstruct/distance.h>
#include <dpcstruct/sort.h>

#include <commandparser/CommandParser.h>

void compute_cluster_size(SmallPC* clusterAlign, const uint64_t length) {
    auto end = clusterAlign + length;

    for (auto low = clusterAlign; low != end; ) {
        uint32_t val = low->qID;

        // Find range of equal qID using std::equal_range
        auto range = std::equal_range(low, end, val, compare_qID());

        // Compute size (difference between range.first and range.second)
        auto count = range.second - range.first;

        // Update the qSize for all elements in the range
        for (auto p = range.first; p != range.second; ++p) {
            p->qSize = count;
        }

        // Move the pointer to the end of the current cluster
        low = range.second;
    }
}



int main(int argc, char** argv) {

	// parse command line
    std::vector<Option> options = {
        {'i', "INPUT", "input filename"},
        {'o', "OUTPUT", "output filename"},
        {'t', "THREADS", "number of threads", false}
    };

    std::string optstring = "i:o:t:";
    std::string program_desc = "Identifies primary clusters given a set of query proteins.";

    OptionParser parser(options, optstring, program_desc);

    auto parsed_options = parser.parse(argc, argv);

	std::string inPath = parsed_options["i"];
	std::string outPath = parsed_options["o"];

    // if parsed_options["t"] is not provided, default to system threads
    int numThreads = parsed_options.count("t") ? std::stoi(parsed_options["t"]) : omp_get_max_threads();
    
	// error check
	if (!std::filesystem::exists(inPath)) {
		std::cerr << "Input file does not exist: " << inPath << std::endl;
        return 1;
	}
	if (std::filesystem::exists(outPath)) {
		std::cerr << "Output file already exists: " << outPath << std::endl;
        return 1;
    }

	std::cout << "Input filename: " << inPath << std::endl;
	std::cout << "Output filename: " << outPath << std::endl;
    std::cout << "Number of threads: " << numThreads << std::endl;

	std::vector<Alignment> allAlignments;
    AlnsFileParser alnsParser(inPath);
    alnsParser.loadAlignments(allAlignments);

	std::cout << "Number of alignments: " << allAlignments.size() << std::endl; 

    std::vector<int> labels = process_by_query(allAlignments, numThreads);

    // Build primary clusters
    std::vector<SmallPC> clusterAlns;
    clusterAlns.reserve(allAlignments.size());

    for (uint64_t i = 0; i < allAlignments.size(); ++i) {
        const Alignment& aln = allAlignments[i];
        int label = labels[i];

        if (label < 0) { continue;}

        clusterAlns.emplace_back(
            aln.queryID * 100 + label,   
            0,                           // placeholder. TODO: remove this field
            aln.searchID,                
            static_cast<uint16_t>(aln.searchStart),  
            static_cast<uint16_t>(aln.searchEnd)     
        );
    }    

    // sort wrt qID+center
    radix_sort(reinterpret_cast<unsigned char*>(clusterAlns.data()), clusterAlns.size(), 16, 4, 0);

    // compute size of each primary cluster: qSize
    compute_cluster_size(clusterAlns.data(), clusterAlns.size());

    // sort back wrt sID
    radix_sort(reinterpret_cast<unsigned char*>(clusterAlns.data()), clusterAlns.size(), 16, 4, 8);

    // Write the clusterAlns vector as binary
    std::ofstream outFile(outPath, std::ios::binary);
    if (!outFile) {
    throw std::runtime_error("Failed to open output file: " + outPath);
    }

    std::cout << "Number of alignments clustered: " << clusterAlns.size() << std::endl;
    std::cout << "Writing to file: " << outPath << std::endl;
    outFile.write(reinterpret_cast<const char*>(clusterAlns.data()), clusterAlns.size() * sizeof(SmallPC));
    outFile.close();

	return 0;
}

 	