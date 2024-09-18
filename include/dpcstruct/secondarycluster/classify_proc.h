#pragma once
#include <string>
#include <vector>

// The run_classification function, which performs the actual classification
void run_classification(const std::vector<std::string> filesList, const std::string& outputFile, int numThreads);