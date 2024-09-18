#pragma once

#include <cstdint>

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
