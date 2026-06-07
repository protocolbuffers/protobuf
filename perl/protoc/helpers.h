// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_PERL_HELPERS_H__
#define GOOGLE_PROTOBUF_COMPILER_PERL_HELPERS_H__

#include <string>
#include <vector>

#ifdef _WIN32
#include <string_view>
namespace absl {
using string_view = std::string_view;
}  // namespace absl
#elif defined(GOOGLE3)
#include "absl/strings/string_view.h"
#else
#include "absl/strings/string_view.h"
#endif

namespace google {
namespace protobuf {
namespace compiler {
namespace perl {

// Custom string helpers to avoid Abseil library dependencies
bool ConsumePrefix(absl::string_view* s, absl::string_view prefix);
std::vector<absl::string_view> SplitString(absl::string_view s, char delim);

// Converts a string to CamelCase, with custom mappings for known words.
std::string ToCamelCase(absl::string_view s);

// Converts a CamelCase string to snake_case.
std::string ToSnakeCase(absl::string_view s);

// Extracts the default host for standard Google Cloud APIs based on the package
// prefix.
std::string GetDefaultHost(absl::string_view proto_pkg);

// Capitalizes each segment of a package name (e.g. foo.bar -> Foo.Bar), with
// custom mappings.
std::string CapitalizePackage(absl::string_view s);

}  // namespace perl
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_PERL_HELPERS_H__
