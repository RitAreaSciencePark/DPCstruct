#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <dpcstruct/types/Alignment.h>
#include <dpcstruct/primarycluster_core.h>


// Test cases
TEST_CASE("Test primary cluster pick_peaks", "[pick_peaks]") {

    SECTION("Empty rho and delta vectors") {
        std::vector<double> rho;
        std::vector<double> delta;

        auto peaks = pick_peaks(rho, delta);

        REQUIRE(peaks.empty()); 
    }

    SECTION("Single element below threshold") {
        std::vector<double> rho = {5.0};  // Below rho_threshold of 10
        std::vector<double> delta = {0.2};  // Below delta_threshold of 0.4

        auto peaks = pick_peaks(rho, delta);

        REQUIRE(peaks.empty());  
    }

    SECTION("Single element above threshold") {
        std::vector<double> rho = {12.0};  // Above rho_threshold of 10
        std::vector<double> delta = {0.5};  // Above delta_threshold of 0.4

        auto peaks = pick_peaks(rho, delta);

        REQUIRE(peaks.size() == 1);  
        REQUIRE(peaks[0] == 0);  
    }

    SECTION("Multiple elements with no peaks") {
        std::vector<double> rho = {5.0, 8.0, 6.0};  
        std::vector<double> delta = {0.3, 0.2, 0.1};  

        auto peaks = pick_peaks(rho, delta);

        REQUIRE(peaks.empty());  
    }

    SECTION("Multiple elements with all peaks") {
        std::vector<double> rho = {12.0, 15.0, 11.0};  
        std::vector<double> delta = {0.6, 0.8, 0.5};  

        auto peaks = pick_peaks(rho, delta);

        REQUIRE(peaks.size() == 3);  
        REQUIRE(peaks == std::vector<size_t>{0, 1, 2}); 
    }

    SECTION("More peaks than max_peaks") {
        std::vector<double> rho = {12.0, 15.0, 11.0, 14.0, 10.5, 13.0};  
        std::vector<double> delta = {0.6, 0.8, 0.5, 0.7, 0.45, 0.65};  

        auto peaks = pick_peaks(rho, delta, 10.0, 0.4, 3);  // Set max_peaks to 3

        REQUIRE(peaks.size() == 3);

        REQUIRE(peaks == std::vector<size_t>{1, 3, 5}); 
    }

    SECTION("Threshold edge cases") {
        std::vector<double> rho = {10.0, 10.1, 9.9};
        std::vector<double> delta = {0.4, 0.41, 0.39};

        auto peaks = pick_peaks(rho, delta);

        REQUIRE(peaks.size() == 1);
        REQUIRE(peaks[0] == 1); 
    }

    SECTION("Test exact number of max_peaks") {
        std::vector<double> rho = {12.0, 15.0, 11.0, 14.0, 10.5, 13.0};  
        std::vector<double> delta = {0.6, 0.8, 0.5, 0.7, 0.45, 0.65};  

        auto peaks = pick_peaks(rho, delta, 10.0, 0.4, 6);  // Set max_peaks to 6 (the exact number of peaks)

        REQUIRE(peaks.size() == 6);  
    }
}
