
#if ((defined(__ARM_NEON) && defined(__aarch64__)) || defined(__SSE4_1__)) && !defined(TRUFFLERUBY)
int utf8_range2(const unsigned char* data, int len);
#else
int utf8_naive(const unsigned char* data, int len);
static inline int utf8_range2(const unsigned char* data, int len) {
  return utf8_naive(data, len);
}
#endif
