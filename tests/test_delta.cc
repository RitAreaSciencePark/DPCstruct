#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <vector>
#include <span>
#include <iostream>

#include <dpcstruct/primarycluster_core.h>
#include <dpcstruct/distance.h>

// Test case for calculating delta
TEST_CASE("Test primary cluster delta calculation with sorted density", "[delta]") {
    // Prepare sample data (alignments and rho values)
    std::vector<Alignment> alignments = {
        {1, 102, 100, 199, 110, 160, 0, 0, 0, 0, 0, 0, 0, 0},  
        {1, 101, 100, 149, 100, 150, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 103, 100, 189, 120, 170, 0, 0, 0, 0, 0, 0, 0, 0},   
        {1, 666, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},         
        {1, 104, 200, 279, 120, 170, 0, 0, 0, 0, 0, 0, 0, 0},   
        {1, 105, 200, 299, 120, 170, 0, 0, 0, 0, 0, 0, 0, 0},   
    };

    // Sample rho values corresponding to the above alignments
    std::vector<double> rho = {2.0, 1.5, 1.0, 3.0, 2.5};  // Assigning rho values

    // Valid indices pointing to alignments
    std::vector<size_t> validIndices = {0, 1, 2, 4, 5};  // All indices are valid in this test

    // Call calculate_delta function
    std::vector<double> delta = calculate_delta(alignments, validIndices, rho);

    // Assertions: Check that delta values are computed correctly
    REQUIRE(delta.size() == 5);  // We expect 5 delta values
    REQUIRE(delta[0] == Catch::Approx(1.0).epsilon(0.001)); 
    REQUIRE(delta[1] == Catch::Approx(0.5).epsilon(0.001));  
    REQUIRE(delta[2] == Catch::Approx(0.1).epsilon(0.001));  
    REQUIRE(delta[3] == Catch::Approx(1000).epsilon(0.001));  
    REQUIRE(delta[4] == Catch::Approx(0.2).epsilon(0.001));  
}
