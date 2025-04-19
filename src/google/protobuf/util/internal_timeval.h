// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Sets up the definition of struct timeval. This may result in the inclusion of
// windows.h, so it should be avoided inside of other headers; instead, just use
// `struct timeval` explicitly in definitions.

#ifndef GOOGLE_PROTOBUF_UTIL_INTERNAL_TIMEVAL_H__
#define GOOGLE_PROTOBUF_UTIL_INTERNAL_TIMEVAL_H__

#ifdef _MSC_VER
#ifdef _XBOX_ONE
struct timeval {
  int64_t tv_sec;  /* seconds */
  int64_t tv_usec; /* and microseconds */
};
#else
#include <winsock2.h>
#endif  // _XBOX_ONE
#else
#include <sys/time.h>
#endif

#endif  // GOOGLE_PROTOBUF_UTIL_INTERNAL_TIMEVAL_H__
