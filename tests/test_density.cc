#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <dpcstruct/primarycluster_core.h>
#include <dpcstruct/types/Alignment.h>

// Test the density calculation logic
TEST_CASE("Test primary cluster density calculation", "[rho]") {
    // Mock alignments
    std::vector<Alignment> alignments = {
        {1, 101, 50, 100, 100, 150, 100, 200, 40, 50, 0.95, 15, 0.8, 0.9}, // overlaps with 1,2 
        {1, 101, 55, 105, 105, 155, 100, 200, 50, 60, 0.92, 16, 0.7, 0.85}, // overlaps with 0
        {1, 102, 45, 97, 100, 160, 100, 200, 50, 60, 0.88, 17, 0.6, 0.8}, // overlaps with 0
        {1, 103, 30, 120, 200, 250, 100, 200, 40, 50, 0.9, 14, 0.75, 0.95}, // overlaps with 4,5
        {1, 104, 30, 120, 110, 160, 100, 200, 40, 50, 0.85, 18, 0.5, 0.75}, // overlaps with 3,5
        {1, 105, 20, 110, 150, 200, 100, 200, 40, 50, 0.82, 19, 0.4, 0.7}, // overlaps with 3,4
        {1, 106, 80, 90, 150, 200, 100, 200, 40, 50, 0.82, 19, 0.4, 0.7}, // overlaps with 6 - ignored
        {1, 107, 10, 150, 150, 200, 100, 200, 40, 50, 0.82, 19, 0.4, 0.7}, // overlaps with 7
    };

    // 0.1 = 90/100
    // 0.2 = 80/100
    // 0.3 = 70/100
    // Set the distance threshold
    double dpar = 0.2;

    // Expected valid indices after filtering (mock implementation, so assume only 6 not valid)
    std::vector<size_t> validIndices = {0, 1, 2, 3, 4, 5, 7};

    // Calculate the density based on the mock alignments
    auto rho = calculate_density(alignments, validIndices, dpar);

    // Validate the calculated densities
    REQUIRE(rho[0] == Catch::Approx(3.0).epsilon(0.1));  
    REQUIRE(rho[1] == Catch::Approx(2.0).epsilon(0.1));
    REQUIRE(rho[2] == Catch::Approx(2.0).epsilon(0.1));
    REQUIRE(rho[3] == Catch::Approx(3.0).epsilon(0.1));
    REQUIRE(rho[4] == Catch::Approx(3.0).epsilon(0.1));
    REQUIRE(rho[5] == Catch::Approx(3.0).epsilon(0.1));
    REQUIRE(rho[6] == Catch::Approx(1.0).epsilon(0.1)); 
    // REQUIRE(rho[7] == Catch::Approx(1.0).epsilon(0.1)); 
}