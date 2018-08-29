#pragma once
#include <vector>
#define MAX_VOL_DIMENSION 4096
//no more than 16Gb of data.
#define MAX_NUM_VOX 16000000000
class Volume {
public:
    std::vector<uint8_t> data;
    size_t size[3];
    uint8_t & operator()(size_t x, size_t y, size_t z);
    uint8_t operator()(size_t x, size_t y, size_t z)const ;
    bool allocate(size_t sx, size_t sy, size_t sz);
};