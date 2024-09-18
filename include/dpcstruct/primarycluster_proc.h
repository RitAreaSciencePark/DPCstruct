#pragma once
#include <dpcstruct/types/Alignment.h>
#include <vector>

std::vector<int> process_by_query(const std::vector<Alignment>& alignments, int numThreads);
