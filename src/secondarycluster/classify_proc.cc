#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <numeric>

#include <dpcstruct/secondarycluster/classify_proc.h>
#include <dpcstruct/types/ProducerConsumer.h>


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

// TODO: add overflow error check
template <typename T>
std::vector<uint32_t> sort_indexes(const std::vector<T> &v) {

    // initialize original index locations
    std::vector<uint32_t> idx(v.size());
    std::iota(idx.begin(), idx.end(), 0);

    std::stable_sort(idx.begin(), idx.end(),
       [&v](T i1, T i2) {return v[i1] > v[i2];});

    return idx;
}


 // Find all entries which have a top half value of gamma and minDistance == 1
template<typename densityC, typename MinDistC>
std::vector<uint32_t> find_peaks(densityC const& density, MinDistC const& minDistance) {
    std::vector<uint32_t> peaksIdx;
    // starts from 1 because id=0 is the null node
    for (uint32_t i = 0; i < minDistance.size(); ++i) {
        if (density[i] > 1 && minDistance[i] >= 0.99) {
            peaksIdx.emplace_back(i);
        }
    }

    if (peaksIdx.size() == 0) throw std::runtime_error("No peaks found!");
    return peaksIdx;
}


class utriag_matrix {
    size_t dim;
    std::vector<double> buffer;
    public:
    utriag_matrix(size_t d): dim(d), buffer( ( (d*(d-1))/2 ) ,0 ){}
    
    size_t size() {return dim;}

    double& at(size_t i, size_t j) {
        if(i==j){ throw std::invalid_argument( "at(i,j) with i == j is not a valid element of utriag_matrix." );}
        if (i>j){std::swap(i,j);}
        return buffer[dim*i - (i*(i+1))/2 - i + j - 1];
    }
};

uint32_t find(uint32_t x, std::vector<uint32_t> & labels) {
    // find root label
    uint32_t y = x;
    while (labels[y]!=y)
        y = labels[y];

    while (labels[x] != x) {
        uint32_t z = labels[x];
        labels[x] = y;
        x = z;
    }
    return y;
}


void merge(uint32_t l1, uint32_t l2, std::vector<uint32_t> & labels) {
    labels[find(l2, labels)] = find(l1, labels);
    return;
}

void run_classification(const std::vector<std::string> filesList, const std::string& outFilename, int numThreads){
    // Placeholder for the actual classification code
    // This function will be implemented in the future
    // For now, it just prints a message
    // std::cout << "Running classification with " << numThreads << " threads" << std::endl;
    std::cout << "Input files: ";
    for (const auto& file : filesList) {
        std::cout << file << " ";
    }
    std::cout << std::endl;
    std::cout << "Output file: " << outFilename << std::endl;

    std::vector<uint32_t> density;
    std::vector<uint32_t> densitySortedIdx;
    std::vector<double> minDistance;
    std::vector<uint32_t> peaksIdx;    // vector containing the peaks ids
    std::vector<NormalizedPair> pcDistanceMat;  // Sparce distance Matrix
    typedef std::map<int, uint32_t> CounterMap;
    CounterMap mcCounts;    // population of each metacluster - needed for mc merging
    std::vector<int64_t> label; // cluster label for each node
    std::vector<double> distToPeak; // initialized to a number bigger than maximum distance between points

    // Create cID dictionary
    std::unordered_set<int32_t> cID_set;
    std::unordered_map<uint32_t, uint32_t> fwd; // from og cIDs={15,32,56} to ascending cIDs={0,1,2}
    std::vector<uint32_t> rev; // from ascending cIDs to og cIDs
    for (auto filename: filesList)
    {
        load_file(filename, pcDistanceMat);
        for (auto & entry: pcDistanceMat)
        {
            cID_set.insert(entry.ID1);
            cID_set.insert(entry.ID2);
        }
    }

    auto num_clusters = cID_set.size();

    rev.reserve(num_clusters);
    for (auto const& value : cID_set)
        rev.emplace_back(value);
        
    cID_set.clear();

    
    std::sort(rev.begin(),rev.end()); // not necessary but orders the output

    // populate the fwd map
    for (uint32_t i = 0; i < rev.size(); i++) {
        fwd[rev[i]] = i;
    }


    // Initialize vectors

    density.assign(num_clusters, 1); 
    minDistance.assign(num_clusters, 1);
    label.assign(num_clusters, -9);
    distToPeak.assign(num_clusters, 10000.); // initialized to a number bigger than maximum distance between points (>=1.0)


    // Calculate density
    std::cout << "Calculating density... ";


    // loop threw input files
    for (auto filename: filesList)
    {

        load_file(filename, pcDistanceMat);

        for (auto & entry: pcDistanceMat)
        {
            // density calculated with cut-off = 0.9
            if (entry.distance < 0.9)
            {	
                auto nID1 = fwd[entry.ID1];
                auto nID2 = fwd[entry.ID2];
                density[nID1] += 1;
                density[nID2] += 1;
            }
        }
    }
        std::cout << "Finished! \n";


    // PC indexes sorted in descending order wrt density 
    // (auxiliary array used later to label the points)

    densitySortedIdx = sort_indexes(density);


    // Calculate min distance to higher density points

    std::cout << "Calculating minimum distance... ";

    // manually assign maximum minDistance to point with highest density
    minDistance[densitySortedIdx[0]] = 20; 
        	        
    // loop threw input files
    for (auto filename: filesList)
    {

	load_file(filename,pcDistanceMat);

        for (const auto & entry: pcDistanceMat)
        {

            auto nID1 = fwd[entry.ID1];
            auto nID2 = fwd[entry.ID2];
	        // don't link elements with same density
            if ( density[nID1] == density[nID2] ) continue;

            auto minDensityID = std::min(nID1, nID2, 
            [&density](uint32_t i1,uint32_t i2){return ( density[i1] < density[i2]);});
            
            // Update minDistance (intial value is 1) 
            if (entry.distance < minDistance[minDensityID])
            {
                minDistance[minDensityID] = entry.distance;    
            }
            
        }
    }

    std::cout << "Finished! \n";
    

    // Find peaks 

      std::cout << "Finding peaks... ";
    // peaksIdx = find_peaks(gamma, minDistance);
    peaksIdx = find_peaks(density, minDistance);
    std::cout << peaksIdx.size() << " peaks found \n";
    
    // Debug output
    // TODO: use filesystem.h instad (std=c++17)
    size_t stripIdx = outFilename.find_last_of("/\\");
    std::string baseDir = "";
    if (stripIdx+1 != 0)
    {
        baseDir = outFilename.substr(0, stripIdx) + "/";
    }
    auto peaksFilename = baseDir + "sc_peaks_idx.txt";
    std::ofstream outPeaksIdx(peaksFilename);
    for (size_t i = 0; i < peaksIdx.size(); ++i)
    {
        outPeaksIdx << rev[peaksIdx[i]] << '\n';
    }
    outPeaksIdx.close();


    // Label the points

    std::cout << "Assigning labels... ";

    // ASSIGN CONSIDERING MINIMUM DISTANCE TO PEAKS
    
    // label the peaks: labels go from 0 to peaksIdx.size()-1
    for (uint32_t i = 0; i < peaksIdx.size(); ++i) {
        label[peaksIdx[i]] = i;
    }
            
    // loop threw all datapoints and compare distance to peaks
    for (auto filename: filesList) {

        load_file(filename,pcDistanceMat);

        for (auto & entry: pcDistanceMat) {
            auto nID1 = fwd[entry.ID1];
            auto nID2 = fwd[entry.ID2];
            // don't assign points far away from peak
            if (entry.distance > 0.9 ) continue;
            
            // check if cIDs are peaks (lower bound uses binary search which is faster than find)
            auto itr1 = std::lower_bound(peaksIdx.begin(), peaksIdx.end(), nID1);
            if (*itr1 != nID1) itr1 = peaksIdx.end();
            auto itr2 = std::lower_bound(peaksIdx.begin(), peaksIdx.end(), nID2);
            if (*itr2 != nID2) itr2 = peaksIdx.end();


            // if ID1 is a peak
            if( itr1 != peaksIdx.end()) {   

                // if ID2 is also peak then we don't do anything
                if ( itr2 != peaksIdx.end() ) continue;

                if ( entry.distance < distToPeak[nID2] ) {
                    distToPeak[nID2] = entry.distance;
                    label[nID2] = itr1 - peaksIdx.begin();
                }
                else if( distToPeak[nID2] == entry.distance ) {
                    if (density[ peaksIdx[label[nID2]] ] < density[nID1]) {
                        distToPeak[nID2] = entry.distance;
                        label[nID2] =  itr1 - peaksIdx.begin();                        
                    }
                }


                else continue;
            }

            // if ID2 is a peak
            if( itr2 != peaksIdx.end() ) {

                if( distToPeak[nID1] > entry.distance) {
                    distToPeak[nID1] = entry.distance;
                    label[nID1] = itr2 - peaksIdx.begin();
                }
                else if(distToPeak[nID1] == entry.distance) {
                    if (density[ peaksIdx[label[nID1]] ] < density[nID2]) {
                        distToPeak[nID1] = entry.distance;
                        label[nID1] = itr2 - peaksIdx.begin();
                    }
                }

                else continue;  

            } 
        }

    }

    std::cout << "Finished! \n";


    // Merge metaclusters

    std::cout << "Merging metaclusters... " << std::endl;
    
    // Count metacluster population
    for (uint32_t i = 0; i < label.size(); ++i) {
        mcCounts[label[i]]++;
    }

    // metaclusters distance matrix: we only store upper triangular part
    utriag_matrix mcDistanceMat(peaksIdx.size());
    // array with the MC labels used for labeling the PC
    std::vector<uint32_t> labels(peaksIdx.size());
    std::iota(labels.begin(), labels.end(), 0);
    // TODO: move up these definitions with all the vector definitions


    // loop threw all datapoints and add distance to corresponding mc pair
    for (auto filename: filesList) {

        load_file(filename,pcDistanceMat);

        for (auto & entry: pcDistanceMat)
        {
            auto nID1 = fwd[entry.ID1];
            auto nID2 = fwd[entry.ID2];
            if (label[nID1] < 0 || label[nID2] < 0) continue;
            if (label[nID1] == label[nID2]) continue;
            mcDistanceMat.at(label[nID1], label[nID2]) += (1. - entry.distance); 
            // we use the complement of the distance because our dist_Icl doesn't have 1. entries.
            // the trick is:  Pcomp = 1-sum(dist(ID1,ID2))/n and Pc = 1-Pcomp
        }

    }
 


    // normalize metacluster distance
    for (uint32_t i = 0; i < mcDistanceMat.size(); ++i) {
        for (uint32_t j = i+1; j < mcDistanceMat.size(); ++j) {

            mcDistanceMat.at(i,j) = 1. - ((mcDistanceMat.at(i,j)) / (mcCounts[i]*mcCounts[j]));

            if (mcDistanceMat.at(i,j)<0.9) {
                if (labels[i] == labels[j]) continue;
                // merge both clusters
                merge(i,j,labels);
             }
        }
    }


    std::cout << "Finished distance matrix normalization" << std::endl;

    // repaint labels
    for (uint32_t i = 0; i < label.size(); ++i) {
        if (label[i] < 0) continue;
        label[i] = find(label[i],labels);
    }

    std::cout << "Finished! \n";

    
    // Output

    std::ofstream outFile(outFilename);

    for (size_t i = 0; i < label.size(); ++i)
    {
        outFile << rev[i] << '\t' << density[i] << '\t' << minDistance[i] << '\t' << label[i] << '\n';
    }

    outFile.close();

    return;

}