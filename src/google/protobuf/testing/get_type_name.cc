// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/testing/get_type_name.h"

#include <cstddef>
#include <string>
#include <vector>

#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"

namespace google {
namespace protobuf {
namespace internal {
namespace {

// Uses a known `probe` instantiation (whose argument is `target`) to determine
// the compiler-specific prefix and suffix in `RawPrettyFunction` output.
absl::optional<absl::string_view> ExtractRaw(
    absl::optional<absl::string_view> raw,
    absl::optional<absl::string_view> probe, absl::string_view target) {
  if (!raw.has_value() || !probe.has_value()) return absl::nullopt;
  size_t prefix = probe->find(target);
  if (prefix == absl::string_view::npos) return absl::nullopt;
  size_t suffix = probe->size() - prefix - target.size();
  // On MSVC, `__FUNCSIG__` precedes `TypeIdentity` with `"struct "` (7 chars),
  // whereas template arguments may begin with `"class "` (6 chars). Exclude the
  // tag keyword from `prefix` so `CleanTypeName` can strip whichever tag is
  // present.
  if (absl::EndsWith(probe->substr(0, prefix), "struct ")) {
    prefix -= 7;
  }
  raw->remove_prefix(prefix);
  raw->remove_suffix(suffix);
  return *raw;
}

absl::optional<std::string> CleanTypeName(absl::string_view str) {
  // Reject names that still contain template arguments we couldn't decompose
  // (e.g. non-type template parameters or nested types inside templates).
  if (absl::StrContains(str, '<')) return absl::nullopt;
  // Strip MSVC tag keywords, anonymous namespace markers, and standard library
  // inline namespaces.
  return absl::StrReplaceAll(absl::StripAsciiWhitespace(str),
                             {{"class ", ""},
                              {"struct ", ""},
                              {"enum ", ""},
                              {"union ", ""},
                              {"(anonymous namespace)::", ""},
                              {"{anonymous}::", ""},
                              {"`anonymous namespace'::", ""},
                              {"`anonymous-namespace'::", ""},
                              {"std::__u::", "std::"},
                              {"std::__1::", "std::"},
                              {"std::__cxx11::", "std::"}});
}

}  // namespace

absl::optional<std::string> CleanRawTypeName(
    absl::optional<absl::string_view> raw) {
  absl::optional<absl::string_view> extracted =
      ExtractRaw(raw, RawPrettyFunction::Get<double>(), "double");
  if (!extracted.has_value()) return absl::nullopt;
  return CleanTypeName(*extracted);
}

absl::optional<std::string> FormatTemplateTypeName(
    absl::optional<absl::string_view> raw,
    absl::Span<const absl::optional<std::string>> args) {
  absl::optional<absl::string_view> extracted =
      ExtractRaw(raw, RawPrettyFunction::Get<TypeIdentity>(),
                 "google::protobuf::internal::TypeIdentity");
  if (!extracted.has_value()) return absl::nullopt;
  absl::optional<std::string> base = CleanTypeName(*extracted);
  if (!base.has_value()) return absl::nullopt;

  std::vector<absl::string_view> unwrapped_args;
  unwrapped_args.reserve(args.size());
  for (const auto& arg : args) {
    if (!arg.has_value()) return absl::nullopt;
    unwrapped_args.push_back(*arg);
  }
  return absl::StrCat(*base, "<", absl::StrJoin(unwrapped_args, ", "), ">");
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
