#include <cstdint>
#include <cstring> // memcpy
#include <utility> // swap

#include <dpcstruct/sort.h>

void radix_sort(unsigned char * pData, uint64_t count, uint32_t record_size, 
                uint32_t key_size, uint32_t key_offset){
                                
    typedef uint8_t k_t [key_size];
    typedef uint8_t record_t [record_size];

    // index matrix
    uint64_t mIndex[key_size][256] = {0};            
    unsigned char * pTemp = new unsigned char [count*sizeof(record_t)];
    unsigned char * pDst, * pSrc;
    uint64_t i,j,m,n;
    k_t u;
    record_t v;

    // generate histograms
    for(i = 0; i < count; i++)
    {         
        std::memcpy(&u, (pData + record_size*i + key_offset), sizeof(u));
        
        for(j = 0; j < key_size; j++)
        {
            mIndex[j][(size_t)(u[j])]++;
            // mIndex[j][(size_t)(u & 0xff)]++;
            // u >>= 8;
        }       
    }
    // convert to indices 
    for(j = 0; j < key_size; j++)
    {
        n = 0;
        for(i = 0; i < 256; i++)
        {
            m = mIndex[j][i];
            mIndex[j][i] = n;
            n += m;
        }       
    }

    // radix sort
    pDst = pTemp;                       
    pSrc = pData;
    for(j = 0; j < key_size; j++)
    {
        for(i = 0; i < count; i++)
        {
            std::memcpy(&u, (pSrc + record_size*i + key_offset), sizeof(u));
            std::memcpy(&v, (pSrc + record_size*i), sizeof(v));
            m = (size_t)(u[j]);
            // m = (size_t)(u >> (j<<3)) & 0xff;
            std::memcpy(pDst + record_size*mIndex[j][m]++, &v, sizeof(v));
        }
        std::swap(pSrc, pDst);
        
    }
    delete[] pTemp;
    return;
}