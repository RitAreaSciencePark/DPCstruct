#pragma once

// LSB radix sort of record with respect to specified key
void radix_sort(unsigned char * pData, uint64_t count, uint32_t record_size, 
                uint32_t key_size, uint32_t key_offset = 0);