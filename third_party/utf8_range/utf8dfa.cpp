// --------------------------------------------------------------------------------
// 
// 
// https://bjoern.hoehrmann.de/utf-8/decoder/dfa/
//
//

// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

// --------------------------------------------------------------------------------
// This code has been heavily modified for optimizations
// - DFA changed to keep in rejected state 
// - split the tables to remove an addition of 256
// - divide and conquer : code stitching to benefit from OOO
// - ....
// --------------------------------------------------------------------------------

#include <stdint.h>
#include <string.h>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include "utf8.h"

#define UTF8_ACCEPT 0
#define UTF8_REJECT_BIT 7
#define UTF8_REJECT (1 << UTF8_REJECT_BIT)

#define R UTF8_REJECT      // reject state

static const uint8_t utf8d[] = {
    // The first part of the table maps bytes to character classes that
    // to reduce the size of the transition table and create bitmasks.
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
     7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
     8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8 };

static const uint8_t utf8s[] = {
    // The second part is a transition table that maps a combination
    // of a state of the automaton and a character class to a state.
    0,R,24,36,60,96,84,R,R,R,48,72,           // 0 ... 107 (state machine)
    R,R,R,R,R,R,R,R,R,R,R,R,
    R,0,R,R,R,R,R,0,R, 0,R,R,
    R,24,R,R,R,R,R,24,R,24,R,R,
    R,R,R,R,R,R,R,24,R,R,R,R,
    R,24,R,R,R,R,R,R,R,24,R,R,
    R,R,R,R,R,R,R,36,R,36,R,R,
    R,36,R,R,R,R,R,36,R,36,R,R,
    R,36,R,R,R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,R,R,           // 108 ... 127 (padding)
    R,R,R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,R,R,R,R    // R ... 139 (rejected states)
};

#undef R

#define FILTER_ASCII7 \
do {  while (length && (*s & 0x80) == 0) { length--; s++; }   \
   while (length && (*(s + length - 1) & 0x80) == 0) { length--; }  } while (0)


bool u_utf8_2dfa(const uint8_t *s, uint32_t length) {
    FILTER_ASCII7;
    // cut the input buffer in 2 parts
    const uint8_t *s1_end = s + length;
    const uint8_t *s1_start = utf8MidBoundary(s, s1_end, 2);
    const uint8_t *s0_start = s;
    const uint8_t *s0_end = s1_start;
    uint8_t byte0, byte1;
    uint64_t state0, state1;

    // the input buffer is now cut in 2 parts, 
    // s1_start ... s1_end   (largest buffer)
    // s2_start ... s2_end   (smallest buffer)

    // now iterate over both buffers in parallel, take advantage of OOO execution 
    state0 = UTF8_ACCEPT;
    state1 = UTF8_ACCEPT;

    // iterate over the smaller half-buffer
    while (s1_start != s1_end) {
        byte0 = *s0_start++;
        byte1 = *s1_start++;
        byte0 = utf8d[byte0];
        byte1 = utf8d[byte1];
        state0 = utf8s[state0 + byte0];
        state1 = utf8s[state1 + byte1];
    }

    // finish the largest half-buffer
    while (s0_start != s0_end) {
        byte0 = *s0_start++;
        byte0 = utf8d[byte0];
        state0 = utf8s[state0 + byte0];
    }

    uint64_t fail = state0 | state1;
    fail |= fail ? UTF8_REJECT : 0;
    return (~fail) & UTF8_REJECT;
}

bool u_utf8_5dfa(const uint8_t *s, uint32_t length) {
    FILTER_ASCII7;
    // split the input buffer into 5 parts
    const uint8_t *s4_end = s + length;
    const uint8_t *s4_start = utf8MidBoundary(s, s4_end, 5);
    const uint8_t *s3_end = s4_start;
    const uint8_t *s2_start = utf8MidBoundary(s, s3_end, 2);
    const uint8_t *s1_start = utf8MidBoundary(s, s2_start, 2);
    const uint8_t *s3_start = utf8MidBoundary(s2_start, s3_end, 2);
    const uint8_t *s1_end = s2_start;
    const uint8_t *s0_start = s;
    const uint8_t *s0_end = s1_start;
    const uint8_t *s2_end = s3_start;

    // get the length of the smallest buffer
    uint32_t len0 = (uint32_t)(s0_end - s0_start);
    uint32_t len1 = (uint32_t)(s1_end - s1_start);
    len0 = len0 < len1 ? len0 : len1;
    uint32_t len2 = (uint32_t)(s2_end - s2_start);
    uint32_t len3 = (uint32_t)(s3_end - s3_start);
    len2 = len2 < len3 ? len2 : len3;
    uint32_t len4 = (uint32_t)(s4_end - s4_start);
    uint32_t len = len0 < len2 ? len0 : len2;
    len = len < len4 ? len : len4;

    // the input buffer is now cut in 2 parts, 
    // s1_start ... s1_end   (largest buffer)
    // s2_start ... s2_end   (smallest buffer)

    // now iterate over all buffers in parallel, take advantage of OOO execution 
    uint32_t byte0, byte1, byte2, byte3, byte4;
    uint64_t state0, state1, state2, state3, state4;

    state0 = UTF8_ACCEPT;
    state1 = UTF8_ACCEPT;
    state2 = UTF8_ACCEPT;
    state3 = UTF8_ACCEPT;
    state4 = UTF8_ACCEPT;

    while (len--) {
        byte0 = *s0_start++;
        byte1 = *s1_start++;
        byte2 = *s2_start++;
        byte3 = *s3_start++;
        byte4 = *s4_start++;
        byte0 = utf8d[byte0];
        byte1 = utf8d[byte1];
        byte2 = utf8d[byte2];
        byte3 = utf8d[byte3];
        byte4 = utf8d[byte4];
        state0 = utf8s[state0 + byte0];
        state1 = utf8s[state1 + byte1];
        state2 = utf8s[state2 + byte2];
        state3 = utf8s[state3 + byte3];
        state4 = utf8s[state4 + byte4];
    }

    // finish the larger buffers
    while (s4_start != s4_end) {
        byte4 = *s4_start++;
        byte4 = utf8d[byte4];
        state4 = utf8s[state4 + byte4];
    }
    while (s3_start != s3_end) {
        byte3 = *s3_start++;
        byte3 = utf8d[byte3];
        state3 = utf8s[state3 + byte3];
    }
    while (s2_start != s2_end) {
        byte2 = *s2_start++;
        byte2 = utf8d[byte2];
        state2 = utf8s[state2 + byte2];
    }
    while (s1_start != s1_end) {
        byte1 = *s1_start++;
        byte1 = utf8d[byte1];
        state1 = utf8s[state1 + byte1];
    }
    while (s0_start != s0_end) {
        byte0 = *s0_start++;
        byte0 = utf8d[byte0];
        state0 = utf8s[state0 + byte0];
    }

    uint64_t fail = state0 | state1 | state2 | state3 | state4;
    fail |= fail ? UTF8_REJECT : 0;
    return (~fail) & UTF8_REJECT;
}