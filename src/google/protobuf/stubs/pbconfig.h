// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef GOOGLE_PROTOBUF_STUBS_PBCONFIG_H__
#define GOOGLE_PROTOBUF_STUBS_PBCONFIG_H__

#define GOOGLE_PROTOBUF_HAVE_HASH_MAP 1
#define GOOGLE_PROTOBUF_HAVE_HASH_SET 1

// Use C++11 unordered_{map|set} if available.
#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X)
# define GOOGLE_PROTOBUF_HASH_NAMESPACE std
# define GOOGLE_PROTOBUF_HASH_MAP_H <unordered_map>
# define GOOGLE_PROTOBUF_HASH_MAP_CLASS unordered_map
# define GOOGLE_PROTOBUF_HASH_SET_H <unordered_set>
# define GOOGLE_PROTOBUF_HASH_SET_CLASS unordered_set

// Version checks for gcc and clang.
#elif defined(__GNUC__)
// Special case clang. As of clang-3.6.1, it still defines __GNU__ related
// macros to be compatbile with gcc-4.2, which does not support tr1. If we only
// look at the __GNUC__ version, <backward/hash_{map|set}> will be chosen, which
// will produce massive deprecation warnings if the tr1 package is actually
// available. Therefore, we use the clang extension __has_include to explicitly
// check the tr1 package availability. If __has_include is not supported in the
// clang version, we fallback to check __GNUC__ version.
# if defined(__clang__) && defined(__has_include)
#  if __has_include(<tr1/unordered_map>) && \
      __has_include(<tr1/unordered_set>)
#    define GOOGLE_PROTOBUF_HAS_TR1
#  endif
# endif
// For GCC 4.3+, use tr1::unordered_map/set; otherwise, follow the
// instructions from:
// https://gcc.gnu.org/onlinedocs/libstdc++/manual/backwards.html
# if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3) || \
     defined(GOOGLE_PROTOBUF_HAS_TR1)
#  undef GOOGLE_PROTOBUF_HAS_TR1
#  define GOOGLE_PROTOBUF_HASH_NAMESPACE std::tr1
#  define GOOGLE_PROTOBUF_HASH_MAP_H <tr1/unordered_map>
#  define GOOGLE_PROTOBUF_HASH_MAP_CLASS unordered_map
#  define GOOGLE_PROTOBUF_HASH_SET_H <tr1/unordered_set>
#  define GOOGLE_PROTOBUF_HASH_SET_CLASS unordered_set
# elif __GNUC__ >= 3
#  define GOOGLE_PROTOBUF_HASH_MAP_H <backward/hash_map>
#  define GOOGLE_PROTOBUF_HASH_MAP_CLASS hash_map
#  define GOOGLE_PROTOBUF_HASH_SET_H <backward/hash_set>
#  define GOOGLE_PROTOBUF_HASH_SET_CLASS hash_set
#  if __GNUC__ == 3 && __GNUC_MINOR__ == 0
#   define GOOGLE_PROTOBUF_HASH_NAMESPACE std       // GCC 3.0
#  else
#   define GOOGLE_PROTOBUF_HASH_NAMESPACE __gnu_cxx // GCC 3.1 and later
#  endif
# else
#  define GOOGLE_PROTOBUF_HASH_NAMESPACE
#  define GOOGLE_PROTOBUF_HASH_MAP_H <hash_map>
#  define GOOGLE_PROTOBUF_HASH_MAP_CLASS hash_map
#  define GOOGLE_PROTOBUF_HASH_SET_H <hash_set>
#  define GOOGLE_PROTOBUF_HASH_SET_CLASS hash_set
# endif

// Version checks for MSC.
// Apparently Microsoft decided to move hash_map *back* to the std namespace in
// MSVC 2010:
// http://blogs.msdn.com/vcblog/archive/2009/05/25/stl-breaking-changes-in-visual-studio-2010-beta-1.aspx
// And.. they are moved back to stdext in MSVC 2013 (haven't checked 2012). That
// said, use unordered_map for MSVC 2010 and beyond is our safest bet.
#elif defined(_MSC_VER)
# if _MSC_VER >= 1600  // Since Visual Studio 2010
#  define GOOGLE_PROTOBUF_HASH_NAMESPACE std
#  define GOOGLE_PROTOBUF_HASH_MAP_H <unordered_map>
#  define GOOGLE_PROTOBUF_HASH_MAP_CLASS unordered_map
#  define GOOGLE_PROTOBUF_HASH_SET_H <unordered_set>
#  define GOOGLE_PROTOBUF_HASH_SET_CLASS unordered_set
#  define GOOGLE_PROTOBUF_HASH_COMPARE std::hash_compare
# elif _MSC_VER >= 1500  // Since Visual Studio 2008
#  define GOOGLE_PROTOBUF_HASH_NAMESPACE std::tr1
#  define GOOGLE_PROTOBUF_HASH_MAP_H <unordered_map>
#  define GOOGLE_PROTOBUF_HASH_MAP_CLASS unordered_map
#  define GOOGLE_PROTOBUF_HASH_SET_H <unordered_set>
#  define GOOGLE_PROTOBUF_HASH_SET_CLASS unordered_set
#  define GOOGLE_PROTOBUF_HASH_COMPARE stdext::hash_compare
# elif _MSC_VER >= 1310
#  define GOOGLE_PROTOBUF_HASH_NAMESPACE stdext
#  define GOOGLE_PROTOBUF_HASH_MAP_H <hash_map>
#  define GOOGLE_PROTOBUF_HASH_MAP_CLASS hash_map
#  define GOOGLE_PROTOBUF_HASH_SET_H <hash_set>
#  define GOOGLE_PROTOBUF_HASH_SET_CLASS hash_set
#  define GOOGLE_PROTOBUF_HASH_COMPARE stdext::hash_compare
# else
#  define GOOGLE_PROTOBUF_HASH_NAMESPACE std
#  define GOOGLE_PROTOBUF_HASH_MAP_H <hash_map>
#  define GOOGLE_PROTOBUF_HASH_MAP_CLASS hash_map
#  define GOOGLE_PROTOBUF_HASH_SET_H <hash_set>
#  define GOOGLE_PROTOBUF_HASH_SET_CLASS hash_set
#  define GOOGLE_PROTOBUF_HASH_COMPARE stdext::hash_compare
# endif

// **ADD NEW COMPILERS SUPPORT HERE.**
// For other compilers, undefine the macro and fallback to use std::map, in
// google/protobuf/stubs/hash.h
#else
# undef GOOGLE_PROTOBUF_HAVE_HASH_MAP
# undef GOOGLE_PROTOBUF_HAVE_HASH_SET
#endif

#endif // GOOGLE_PROTOBUF_STUBS_PBCONFIG_H__
