// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string.h>

// On x86-64 Linux with glibc, we link against the 2.2.5 version of memcpy so
// that we avoid depending on the 2.14 version of the symbol. This way,
// distributions that are using pre-2.14 versions of glibc can successfully use
// the gem we distribute
// (https://github.com/protocolbuffers/protobuf/issues/2783).
//
// This wrapper is enabled by passing the linker flags -Wl,-wrap,memcpy in
// extconf.rb.
#ifdef __linux__
#if defined(__x86_64__) && defined(__GNU_LIBRARY__)
__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");
void *__wrap_memcpy(void *dest, const void *src, size_t n) {
  return memcpy(dest, src, n);
}
#else
void *__wrap_memcpy(void *dest, const void *src, size_t n) {
  return memmove(dest, src, n);
}
#endif
#endif
