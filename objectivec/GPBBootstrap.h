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

// The Objective C runtime has complete enough info that most protos don’t end
// up using this, so leaving it on is no cost or very little cost.  If you
// happen to see it causing bloat, this is the way to disable it. If you do
// need to disable it, try only disabling it for Release builds as having
// full TextFormat can be useful for debugging.
#ifndef GPBOBJC_SKIP_MESSAGE_TEXTFORMAT_EXTRAS
#define GPBOBJC_SKIP_MESSAGE_TEXTFORMAT_EXTRAS 0
#endif

// Most uses of protocol buffers don't need field options, by default the
// static data will be compiled out, define this to 1 to include it. The only
// time you need this is if you are doing introspection of the protocol buffers.
#ifndef GPBOBJC_INCLUDE_FIELD_OPTIONS
#define GPBOBJC_INCLUDE_FIELD_OPTIONS 0
#endif

// Used in the generated code to give sizes to enums. int32_t was chosen based
// on the fact that Protocol Buffers enums are limited to this range.
#if !__has_feature(objc_fixed_enum)
 #error All supported Xcode versions should support objc_fixed_enum.
#endif
// If the headers are imported into Objective-C++, we can run into an issue
// where the defintion of NS_ENUM (really CF_ENUM) changes based on the C++
// standard that is in effect.  If it isn't C++11 or higher, the definition
// doesn't allow us to forward declare. We work around this one case by
// providing a local definition. The default case has to use NS_ENUM for the
// magic that is Swift bridging of enums.
#if (__cplusplus && __cplusplus < 201103L)
 #define GPB_ENUM(X) enum X : int32_t X; enum X : int32_t
#else
 #define GPB_ENUM(X) NS_ENUM(int32_t, X)
#endif
// GPB_ENUM_FWD_DECLARE is used for forward declaring enums, ex:
//   GPB_ENUM_FWD_DECLARE(Foo_Enum)
//   @property (nonatomic) Foo_Enum value;
#define GPB_ENUM_FWD_DECLARE(X) enum X : int32_t

// Based upon CF_INLINE. Forces inlining in release.
#if !defined(DEBUG)
#define GPB_INLINE static __inline__ __attribute__((always_inline))
#else
#define GPB_INLINE static __inline__
#endif

// For use in public headers that might need to deal with ARC.
#ifndef GPB_UNSAFE_UNRETAINED
#if __has_feature(objc_arc)
#define GPB_UNSAFE_UNRETAINED __unsafe_unretained
#else
#define GPB_UNSAFE_UNRETAINED
#endif
#endif

// If property name starts with init we need to annotate it to get past ARC.
// http://stackoverflow.com/questions/18723226/how-do-i-annotate-an-objective-c-property-with-an-objc-method-family/18723227#18723227
#define GPB_METHOD_FAMILY_NONE __attribute__((objc_method_family(none)))

// The protoc-gen-objc version which works with the current version of the
// generated Objective C sources.  In general we don't want to change the
// runtime interfaces (or this version) as it means everything has to be
// regenerated.
#define GOOGLE_PROTOBUF_OBJC_GEN_VERSION 30000
