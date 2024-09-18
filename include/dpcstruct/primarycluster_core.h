#pragma once

#include <dpcstruct/types/Alignment.h>
#include <vector>
#include <span>

std::vector<size_t> get_nonredundant_indices(std::span<const Alignment> alignments, 
                                            double distanceThreshold);


std::vector<double> calculate_density(std::span<const Alignment> alignments, 
                                    const std::vector<size_t>& validIndices,
                                    double dpar=0.2);

std::vector<double> calculate_delta(std::span<const Alignment> alignments, 
                                    const std::vector<size_t>& validIndices, 
                                    const std::vector<double>& rho);

std::vector<size_t> pick_peaks(const std::vector<double>& rho, 
                                const std::vector<double>& delta, 
                                double rho_threshold = 10.0, 
                                double delta_threshold = 0.4, 
                                size_t max_peaks = 10);

std::vector<int> assign_labels(std::span<const Alignment> alignments,
                                const std::vector<size_t>& validIndices, 
                                const std::vector<size_t>& peaks,
                                double dpar=0.2);

std::vector<int> cluster_alignments(std::span<const Alignment> alignments, double dpar=0.2);
