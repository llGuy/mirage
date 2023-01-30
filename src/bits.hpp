#pragma once

#include "types.hpp"

inline u32 pop_count(u32 bits) {
#ifndef __GNUC__
    return __popcnt(bits);
#else
    return __builtin_popcount(bits);
#endif
}
