#pragma once

#include "stdafx.h"

#define divRoundUp(a, b) (((a) / (b)) + ((a) % (b)))

inline size_t roundToPowerOf2(size_t number)
{
    DWORD highestBit;
    BitScanReverse64(&highestBit, number);
    size_t ans = size_t(1) << highestBit;
    ans <<= size_t(number != ans);
    return ans;
}
