#include <dpcstruct/distance.h>

double distance(const Alignment& i, const Alignment& j) {
    int istart = i.queryStart, iend = i.queryEnd;
    int jstart = j.queryStart, jend = j.queryEnd;

    int interStart = std::max(istart, jstart);
    int interEnd = std::min(iend, jend);
    double intersection = std::max(0, interEnd - interStart + 1);

    int unionStart = std::min(istart, jstart);
    int unionEnd = std::max(iend, jend);
    double unionLength = unionEnd - unionStart + 1;

    return (unionLength - intersection) / unionLength;
}


double distance(const SmallPC* i, const SmallPC* j) {
    int istart = i->sstart, iend = i->send;
    int jstart = j->sstart, jend = j->send;

    int interStart = std::max(istart, jstart);
    int interEnd = std::min(iend, jend);
    double intersection = std::max(0, interEnd - interStart + 1);

    int unionStart = std::min(istart, jstart);
    int unionEnd = std::max(iend, jend);
    double unionLength = unionEnd - unionStart + 1;

    return (unionLength - intersection) / unionLength;
}
