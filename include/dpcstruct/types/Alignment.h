#pragma once
#include <cstdint>

struct Alignment {
    uint32_t queryID, searchID, queryStart, queryEnd, searchStart, searchEnd;
    uint32_t queryLength, searchLength, alnLength, bits;
    double pident, evalue;
    double tmScore, lddt;

    Alignment(uint32_t qID, uint32_t sID, uint32_t qStart, uint32_t qEnd, uint32_t sStart, uint32_t sEnd,
                uint32_t qLength, uint32_t sLength, uint32_t alnLen, double pid, double eval, uint32_t bscore,
                double tScore, double lScore)
        : queryID(qID), searchID(sID), queryStart(qStart), queryEnd(qEnd), searchStart(sStart), searchEnd(sEnd),
            queryLength(qLength), searchLength(sLength), alnLength(alnLen), pident(pid), evalue(eval), bits(bscore),
            tmScore(tScore), lddt(lScore) {}
};
