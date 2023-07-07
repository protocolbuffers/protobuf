#pragma once


#ifdef _MSC_VER

#include <intrin.h>

class uint128_t {
public:
    uint64_t lo, hi;
    uint128_t(int v) { lo = v; hi = 0; }
    uint128_t(uint32_t v) { lo = v; hi = 0; }
    uint128_t(uint64_t v) { lo = v; hi = 0; }
    uint128_t(uint64_t p_hi, uint64_t p_lo) { lo = p_lo; hi = p_hi; }

    inline uint128_t &operator+=(const uint64_t &other) {
        lo += other;
        hi += (lo < other);
        return *this;
    }
    inline uint128_t &operator+=(const uint128_t &other) {
        lo += other.lo;
        hi += (lo < other.lo);
        hi += other.hi;
        return *this;
    }
    inline uint128_t &operator&=(const uint128_t &other) {
        lo &= other.lo;
        hi &= other.hi;
        return *this;
    }
    inline uint128_t &operator|=(const uint128_t &other) {
        lo |= other.lo;
        hi |= other.hi;
        return *this;
    }
    inline uint128_t &operator^=(const uint128_t &other) {
        lo ^= other.lo;
        hi ^= other.hi;
        return *this;
    }
    inline uint128_t &operator>>=(int amount) {
        if (amount == 64) {
            lo = hi;
            hi = 0;
            return *this;
        }
        if (amount >= 64)
        {
            lo = hi >> (amount - 64);
            hi = 0;
            return *this;
        }
        lo = (lo >> amount) | (hi << (64 - amount));
        hi = hi >> amount;
        return *this;
    }
    inline uint128_t &operator<<=(int amount) {
        if (amount == 64) {
            hi = lo;
            lo = 0;
            return *this;
        }
        if (amount >= 64) {
            hi = lo << (amount - 64);
            lo = 0;
            return *this;
        }
        hi = (hi << amount) | (lo << (64 - amount));
        lo = lo << amount;
        return *this;
    }

    operator unsigned long long() const {
        return lo;
    }
};

inline bool operator==(const uint128_t &lhs, const int &rhs) { return lhs.lo == rhs && lhs.hi == 0; }
inline bool operator!=(const uint128_t &lhs, const int &rhs) { return !(lhs == rhs); }

#else

#include <x86intrin.h>

typedef unsigned __int128 uint128_t;

#endif