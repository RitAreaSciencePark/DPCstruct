#include <algorithm>
#include <limits>
#include <numeric>
#include <vector>

#include <dpcstruct/primarycluster_core.h>
#include <dpcstruct/distance.h>


std::vector<size_t> get_nonredundant_indices(std::span<const Alignment> alignments, double distanceThreshold) {
    // Get the sorted positions based on searchID
    std::vector<size_t> sortedIndices(alignments.size());

    // Initialize indices
    std::iota(sortedIndices.begin(), sortedIndices.end(), 0);

    // Sort indices based on searchID in the alignments
    std::sort(sortedIndices.begin(), sortedIndices.end(),
              [&alignments](size_t i_1, size_t i_2) {
                  return alignments[i_1].searchID < alignments[i_2].searchID;
              });

    // Remove redundant alignments based on distance
    std::vector<bool> valid(alignments.size(), true);

    for (size_t i = 0; i < sortedIndices.size(); ++i) {
        if (!valid[sortedIndices[i]]) continue;  

        for (size_t j = i + 1; j < sortedIndices.size(); ++j) {
            // If searchID is different, stop checking further as they are sorted by searchID
            if (alignments[sortedIndices[i]].searchID != alignments[sortedIndices[j]].searchID) {
                break;  
            }

            // Check the distance between alignments
            double dist = distance(alignments[sortedIndices[i]], alignments[sortedIndices[j]]);


            // TODO: sholuld remove that with bigger evalue
            if (dist < distanceThreshold) {
                valid[sortedIndices[j]] = false;  
            }
        }
    }

    // Collect and return the valid indices
    std::vector<size_t> validIndices;
    for (size_t i = 0; i < sortedIndices.size(); ++i) {
        if (valid[sortedIndices[i]]) {
            validIndices.push_back(sortedIndices[i]);  // Add only the valid ones
        }
    }

    return validIndices;
}

std::vector<double> calculate_density(const std::span<const Alignment> alignments, 
                                    const std::vector<size_t>& validIndices, 
                                    double dpar) {
    std::vector<double> rho(validIndices.size(), 1.0);

    // Initialize rho with random seed
    for (size_t i = 0; i < validIndices.size(); ++i) {
        auto i_og = validIndices[i];
        const Alignment& aln = alignments[i_og];
        int seed = aln.searchID + aln.queryStart + aln.queryEnd - aln.searchStart * aln.searchEnd;
        srand(seed); 
        rho[i] += 0.1 * ((double)rand() / RAND_MAX); // Add a small random value to rho
    }

    // Pairwise distance calculation to update rho
    for (size_t i = 0; i < validIndices.size(); ++i) {
        auto i_og = validIndices[i];
        for (size_t j = i + 1; j < validIndices.size(); ++j) {
            auto j_og = validIndices[j];
            double d = distance(alignments[i_og], alignments[j_og]);
            if (d < dpar) {
                rho[i] += 1;
                rho[j] += 1;
            }
        }
    }

    return rho;
}

std::vector<double> calculate_delta(std::span<const Alignment> alignments, 
                                    const std::vector<size_t>& validIndices,
                                    const std::vector<double>& rho) {
    std::vector<double> delta(validIndices.size(), 1000.0);  // max delta is large initially

    std::vector<size_t> sortedIndices(validIndices.size());
    std::iota(sortedIndices.begin(), sortedIndices.end(), 0);

    // Sort sortedIndices by the corresponding rho values in descending order
    std::sort(sortedIndices.begin(), sortedIndices.end(), [&](size_t a, size_t b) {
        return rho[a] > rho[b];  // Sort in descending order by rho
    });

    // Calculate delta for each alignment
    for (size_t i = 0; i < sortedIndices.size(); ++i) {
        // Get the original index
        auto i_og = validIndices[sortedIndices[i]];

        // Check only elements with higher rho
        for (size_t j = 0; j < i; ++j) {

            auto j_og = validIndices[sortedIndices[j]];

            // Calculate distance
            double dist = distance(alignments[i_og], alignments[j_og]);
            // Add small randomness to avoid ties
            dist += 0.00001 * ((double)rand() / RAND_MAX);

            // Update delta if we found a smaller distance
            if (dist < delta[sortedIndices[i]]) {
                delta[sortedIndices[i]] = dist;
            }
        }
    }

    return delta;
}

std::vector<size_t> pick_peaks(const std::vector<double>& rho, 
                               const std::vector<double>& delta, 
                               double rho_threshold, 
                               double delta_threshold, 
                               size_t max_peaks) {

    std::vector<size_t> peaks;

    // Identify initial peak peaks based on rho and delta thresholds
    for (size_t i = 0; i < rho.size(); ++i) {
        if (rho[i] > rho_threshold && delta[i] > delta_threshold) {
            peaks.push_back(i);
        }
    }

    // If no peaks, return an empty list
    if (peaks.empty()) {
        return peaks;
    }

    // If peaks exceed max_peaks, prune based on rho * delta (gamma)
    if (peaks.size() > max_peaks) {
        // Calculate gamma (rho * delta) for each center
        std::vector<double> peaks_gamma(peaks.size());
        for (size_t i = 0; i < peaks.size(); ++i) {
            peaks_gamma[i] = rho[peaks[i]] * delta[peaks[i]];
        }

        // Get indices sorted by descending gamma
        std::vector<size_t> sorted_idxs_gamma(peaks.size());
        std::iota(sorted_idxs_gamma.begin(), sorted_idxs_gamma.end(), 0);

        std::sort(sorted_idxs_gamma.begin(), sorted_idxs_gamma.end(), [&](size_t i, size_t j) {
            return peaks_gamma[i] > peaks_gamma[j];  // Sort by gamma in descending order
        });

        // Remove peaks beyond the first max_peaks (prune the lowest gamma values)
        std::vector<size_t> top_peaks;
        for (size_t i = 0; i < max_peaks; ++i) {
            top_peaks.push_back(peaks[sorted_idxs_gamma[i]]);
        }
        peaks = top_peaks;
    }

    return peaks;
}


std::vector<int> assign_labels(std::span<const Alignment> alignments, 
                               const std::vector<size_t>& validIndices, 
                               const std::vector<size_t>& peaks,
                               double dpar) {
    // Initialize labels with -1 (unassigned)
    std::vector<int> labels(validIndices.size(), -1);

    // Assign each peak to its own cluster
    for (size_t i = 0; i < peaks.size(); ++i) {
        labels[peaks[i]] = static_cast<int>(i);  // Assign peak index as label
    }

    // Assign the rest of the elements to the closest peak
    for (size_t i = 0; i < validIndices.size(); ++i) {
        if (labels[i] != -1) continue;  // Skip if already assigned

        // Find the closest peak
        double minDist = std::numeric_limits<double>::max();
        int closestPeak = -1;

        for (size_t j = 0; j < peaks.size(); ++j) {
            double dist = distance(alignments[validIndices[i]], alignments[validIndices[peaks[j]]]);
            if (dist >= dpar) continue; 
            if (dist < minDist) {
                minDist = dist;
                closestPeak = j;
            }
        }

        labels[i] = closestPeak;  // Assign closest peak's cluster label
    }

    return labels;
}


std::vector<int> cluster_alignments(std::span<const Alignment> alignments, double dpar) {
    // std::cout << "Processing queryID: " << alignments[start].queryID << " from index " << start << " to " << end-1 << std::endl;

    // Core
    std::vector<size_t> validIndices = get_nonredundant_indices(alignments, 0.2); // TODO: use ranges to avoid validIndices

    std::vector<double> rho = calculate_density(alignments, validIndices, dpar);

    std::vector<double> delta = calculate_delta(alignments, validIndices, rho);

    std::vector<size_t> peaks = pick_peaks(rho, delta);

    std::vector<int> labels = assign_labels(alignments, validIndices, peaks, 0.2);

    // print labels only if assigned
    // for (size_t i = 0; i < validIndices.size(); ++i) {
    //     auto i_all = validIndices[i];
    //     if (labels[i] != -1) {
    //         std::cout << "Query: " << alignments[i_all].queryID << " Search: " << alignments[i_all].searchID << " assigned to cluster " << labels[i] << std::endl;
    //     }
    // }
    
    // labels for all alignments, not only valid ones
    std::vector<int> labels_all(alignments.size(), -1);
    for (size_t i = 0; i < validIndices.size(); ++i) {
        labels_all[validIndices[i]] = labels[i];
    }
    
    return labels_all;

}