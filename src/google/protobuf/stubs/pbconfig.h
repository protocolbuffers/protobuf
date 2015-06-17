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
# define GOOGLE_PROTOBUF_HAS_CXX11_HASH

// For XCode >= 4.6:  the compiler is clang with libc++.
// For earlier XCode version: the compiler is gcc-4.2.1 with libstdc++.
// libc++ provides <unordered_map> and friends even in non C++11 mode,
// and it does not provide the tr1 library. Therefore the following macro
// checks against this special case.
// Note that we should not test the __APPLE_CC__ version number or the
// __clang__ macro, since the new compiler can still use -stdlib=libstdc++, in
// which case <unordered_map> is not compilable without -std=c++11
#elif defined(__APPLE_CC__)
# if defined(_LIBCPP_VERSION)
#  define GOOGLE_PROTOBUF_HAS_CXX11_HASH
# elif __GNUC__ >= 4
#  define GOOGLE_PROTOBUF_HAS_TR1
# else
// Not tested for gcc < 4... These setting can compile under 4.2.1 though.
#  define GOOGLE_PROTOBUF_HASH_NAMESPACE __gnu_cxx
#  define GOOGLE_PROTOBUF_HASH_MAP_H <ext/hash_map>
#  define GOOGLE_PROTOBUF_HASH_MAP_CLASS hash_map
#  define GOOGLE_PROTOBUF_HASH_SET_H <ext/hash_set>
#  define GOOGLE_PROTOBUF_HASH_SET_CLASS hash_set
# endif

// Version checks for gcc.
#elif defined(__GNUC__)
// For GCC 4.x+, use tr1::unordered_map/set; otherwise, follow the
// instructions from:
// https://gcc.gnu.org/onlinedocs/libstdc++/manual/backwards.html
# if __GNUC__ >= 4
#  define GOOGLE_PROTOBUF_HAS_TR1
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
#  define GOOGLE_PROTOBUF_HAS_CXX11_HASH
#  define GOOGLE_PROTOBUF_HASH_COMPARE std::hash_compare
# elif _MSC_VER >= 1500  // Since Visual Studio 2008
#  define GOOGLE_PROTOBUF_HAS_TR1
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

#if defined(GOOGLE_PROTOBUF_HAS_CXX11_HASH)
# define GOOGLE_PROTOBUF_HASH_NAMESPACE std
# define GOOGLE_PROTOBUF_HASH_MAP_H <unordered_map>
# define GOOGLE_PROTOBUF_HASH_MAP_CLASS unordered_map
# define GOOGLE_PROTOBUF_HASH_SET_H <unordered_set>
# define GOOGLE_PROTOBUF_HASH_SET_CLASS unordered_set
#elif defined(GOOGLE_PROTOBUF_HAS_TR1)
# define GOOGLE_PROTOBUF_HASH_NAMESPACE std::tr1
# define GOOGLE_PROTOBUF_HASH_MAP_H <tr1/unordered_map>
# define GOOGLE_PROTOBUF_HASH_MAP_CLASS unordered_map
# define GOOGLE_PROTOBUF_HASH_SET_H <tr1/unordered_set>
# define GOOGLE_PROTOBUF_HASH_SET_CLASS unordered_set
#endif

#undef GOOGLE_PROTOBUF_HAS_CXX11_HASH
#undef GOOGLE_PROTOBUF_HAS_TR1

#endif // GOOGLE_PROTOBUF_STUBS_PBCONFIG_H__
