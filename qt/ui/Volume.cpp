#include "Volume.hpp"

uint8_t & Volume::operator()(size_t x, size_t y, size_t z)
{ 
    return data[x*size[1] * size[2] + y * size[2] + z];
}

uint8_t Volume::operator()(size_t x, size_t y, size_t z) const
{
    return data[x*size[1] * size[2] + y * size[2] + z];
}

bool Volume::allocate(size_t sx, size_t sy, size_t sz)
{
    if (sx > MAX_VOL_DIMENSION || sy > MAX_VOL_DIMENSION || sz > MAX_VOL_DIMENSION) {
        return false;
    }
    size_t nVox = sx * sy*sz;
    if (nVox > MAX_NUM_VOX || nVox == 0) {
        return false;
    }
    data.resize(nVox);
    size[0] = sx;
    size[1] = sy;
    size[2] = sz;
    return true;
}

bool checkSize(size_t sx, size_t sy, size_t sz) {
    if (sx > MAX_VOL_DIMENSION || sy > MAX_VOL_DIMENSION || sz > MAX_VOL_DIMENSION) {
        return false;
    }
    size_t nVox = sx * sy*sz;
    if (nVox > MAX_NUM_VOX || nVox == 0) {
        return false;
    }
    return true;
}
