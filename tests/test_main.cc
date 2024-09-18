#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>


TEST_CASE("Simple test case", "[example]") {
    REQUIRE(1 + 1 == 2);
    REQUIRE(2 * 2 == 4);
}