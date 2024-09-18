#pragma once

#include <iostream>
#include <cstdint>

struct SmallPC {
    uint32_t qID; // qID*100 + center
    uint32_t qSize;
    uint32_t sID;
    uint16_t sstart;
    uint16_t send;
    
    SmallPC(uint32_t q, uint32_t qs, uint32_t s, uint16_t ss, uint16_t se): qID(q), qSize(qs), sID(s), sstart(ss), send(se){}
    SmallPC(): qID(0), qSize(0),sID(0), sstart(0), send (0) {}

    friend std::ostream& operator<<(std::ostream& os, const SmallPC& pc) {
        os << pc.qID << " " << pc.qSize << " " << pc.sID << " " << pc.sstart << " " << pc.send;
        return os;
    }
};

struct SequenceLabel {
    uint32_t sID;
    uint16_t sstart;
    uint16_t send;
    int label;
    
    SequenceLabel(uint32_t s, uint16_t ss, uint16_t se, int l): sID(s), sstart(ss), send(se), label(l) {}
    SequenceLabel(): sID(0), sstart(0), send (0), label(-9) {}

};


struct compare_qID {
    bool operator() (const SmallPC & left, const SmallPC & right){
        return left.qID < right.qID;
    }
    bool operator() (const SmallPC & left, uint32_t right){
        return left.qID < right;
    }
    bool operator() (uint32_t left, const SmallPC & right){   
        return left < right.qID;
    }
};

template<class T> 
struct compare_sID {
    bool operator() (const T & left, const T & right){
        return left.sID < right.sID;
    }
    bool operator() (const T & left, uint32_t right){
        return left.sID < right;
    }
    bool operator() (uint32_t left, const T & right){   
        return left < right.sID;
    }
};


struct compare_label {
    bool operator() (const SequenceLabel & left, const SequenceLabel & right) {
        return left.label < right.label;
    }
    bool operator() (const SequenceLabel & left, int right) {
        return left.label < right;
    }
    bool operator() (int left, const SequenceLabel & right) {   
        return left < right.label;
    }
};
