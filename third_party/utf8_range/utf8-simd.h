#pragma once

bool u_utf8_2dfa(const uint8_t *buf, uint32_t len);
bool u_utf8_5dfa(const uint8_t *buf, uint32_t len);

#ifdef __AVX512F__
bool u_utf8_d512(const uint8_t *buf, uint32_t len);
#endif

#ifdef __AVX2__
bool u_utf8_d256(const uint8_t *buf, uint32_t len);
#endif