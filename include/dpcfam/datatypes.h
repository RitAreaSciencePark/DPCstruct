
// Author: Federico Barone @ Institute for Research and Technologies (Area Science Park) & SISSA.
// Package: DPCfam pipeline
// Description: data dtypes used in some c++ programs

// SPDX-License-Identifier:  BSD-2-Clause

#ifndef DTYPES
#define DTYPES

#include <iostream>

struct Segment {	
  // SEARCH and QUERY ids (in numbers, no txts!)
  int sid;
  int qid;	
  //  alignment START and END on the QUERY
  int qstart;
  int qend;
  //  alignment START and END on the SEARCH
  int sstart;
  int send;
  //total alignment length
  int len;
  //roughscore
  int score;
  // rho and delta for DPC  
  double rho, delta;
  // evalue 
  double evalue;
  // is this alignment a cluster center?
  int isc;
} ;

struct MatchedPair
{
    uint32_t ID1;
    uint32_t ID2;
    uint32_t normFactor;

    MatchedPair(): ID1{0}, ID2{0}, normFactor{0}{};
    MatchedPair(uint32_t id1, uint32_t id2, uint32_t n): ID1(id1), ID2(id2), normFactor(n) {}
};

struct NormalizedPair
{
    uint32_t ID1;
    uint32_t ID2;
    double distance;

    NormalizedPair()=default;
    NormalizedPair(uint32_t id1, uint32_t id2, double d):  ID1(id1), ID2(id2), distance(d) {}
};

struct Ratio
{
    uint32_t num;
    uint32_t denom;
    
    Ratio()=default;
    Ratio(uint32_t n, uint32_t d):  num(n), denom(d) {}

    inline double as_double() const {return (double)num/denom;}
};

// Small Clustered Alignment structure (only essential data...)
struct SmallCA
{
    uint32_t qID; // qID*100 + center
    uint32_t qSize;
    uint32_t sID;
    uint16_t sstart;
    uint16_t send;
    
    SmallCA(uint32_t q, uint32_t qs, uint32_t s, uint16_t ss, uint16_t se): qID(q), qSize(qs), sID(s), sstart(ss), send(se){}
    SmallCA(): qID(0), qSize(0),sID(0), sstart(0), send (0) {}

};


// needed for std::upper_bound in compute_cluster_size
struct compare_qID
{
    bool operator() (const SmallCA & left, const SmallCA & right)
    {
        return left.qID < right.qID;
    }
    bool operator() (const SmallCA & left, uint32_t right)
    {
        return left.qID < right;
    }
    bool operator() (uint32_t left, const SmallCA & right)
    {   
        return left < right.qID;
    }
};



// print a ClusteredAlignment TO STDOUT
inline void printSCA( const SmallCA & clusterAlign) 
{
    std::cout << clusterAlign.qID << " " << clusterAlign.qSize << " " << clusterAlign.sID << " " << clusterAlign.sstart << " " << clusterAlign.send << std::endl;   
}


struct SequenceLabel
{
    uint32_t sID;
    uint16_t sstart;
    uint16_t send;
    int label;
    
    SequenceLabel(uint32_t s, uint16_t ss, uint16_t se, int l): sID(s), sstart(ss), send(se), label(l) {}
    SequenceLabel(): sID(0), sstart(0), send (0), label(-9) {}

};

struct compare_label
{
    bool operator() (const SequenceLabel & left, const SequenceLabel & right)
    {
        return left.label < right.label;
    }
    bool operator() (const SequenceLabel & left, int right)
    {
        return left.label < right;
    }
    bool operator() (int left, const SequenceLabel & right)
    {   
        return left < right.label;
    }
};


// print a ClusteredAlignment TO STDOUT
inline void printSL( const SequenceLabel & sl) 
{
    std::cout <<  sl.sID << " " << sl.sstart << " " << sl.send <<  " " << sl.label << std::endl;   
}


template<class T> 
struct compare_sID // used by auxiliary kmerge_binary
{
    bool operator() (const T & left, const T & right)
    {
        return left.sID < right.sID;
    }
    bool operator() (const T & left, uint32_t right)
    {
        return left.sID < right;
    }
    bool operator() (uint32_t left, const T & right)
    {   
        return left < right.sID;
    }
};


#endif //DTYPES


