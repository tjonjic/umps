/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// Fun bit twiddling stuff
// See "Hacker's Delight", Henry S. Warren, Jr.

#ifndef BASE_BIT_TRICKS_H
#define BASE_BIT_TRICKS_H

#include "base/basic_types.h"

inline uint32_t FloorP2(uint32_t x)
{
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return x - (x >> 1);
}

inline uint32_t CeilingP2(uint32_t x)
{
    x -= 1;
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return x + 1;
}

#endif // BASE_BIT_TRICKS_H
