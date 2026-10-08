// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/protobuf_test.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>
#include <tuple>
#include <vector>

#include "absl/log/absl_log.h"
#include "absl/numeric/int128.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"

namespace google::protobuf::internal {
namespace {

// We use a TEST_P only as a way to inject a callback into GoogleTest so that we
// can register our own tests at the right time, which is when GoogleTest itself
// is registering the TEST_P tests.
class ProtobufDelayedRegistration : public ::testing::TestWithParam<int> {};
TEST_P(ProtobufDelayedRegistration, Unused) {}
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ProtobufDelayedRegistration);
INSTANTIATE_TEST_SUITE_P(
    Unused, ProtobufDelayedRegistration, ([] {
      static bool init [[maybe_unused]] =
          (::google::protobuf::internal::PartialTestRegistry::Get().RegisterAll(), true);
      return ::testing::ValuesIn(std::vector<int>{});
    })());

}  // namespace

std::string PartialTestRegistry::PerFixture::PerInstantiation::ValueName(
    size_t i) const {
  std::string res;
  for (const auto& [l, dim] : values_) {
    if (!res.empty()) absl::StrAppend(&res, "/");
    absl::StrAppend(&res, dim.names[i % dim.names.size()]);
    i /= dim.names.size();
  }
  return res;
}

std::string PartialTestRegistry::PerFixture::PerInstantiation::TypeName()
    const {
  std::string res;
  const auto get_id = [&](size_t id) {
    for (auto pci : instantiation_key) {
      if (pci.counter == id) return pci.value;
    }
    return size_t{};
  };
  for (const auto& [l, dim] : types_) {
    if (!res.empty()) absl::StrAppend(&res, "/");
    absl::StrAppend(&res, dim.names[get_id(l)]);
  }
  return res;
}

template <typename Map, typename F>
static auto Sort(const Map& map, F func) {
  std::vector<const typename Map::mapped_type*> out;
  for (auto& [k, v] : map) out.push_back(&v);
  std::sort(out.begin(), out.end(),
            [&](auto* a, auto* b) { return func(*a) < func(*b); });
  return out;
}

void PartialTestRegistry::RegisterAll() const {
  for (const auto* fixture : Sort(fixtures_, [](auto& f) {
         return std::tuple<absl::string_view, int, absl::string_view,
                           absl::string_view>(f.file, f.line, f.test_suite_name,
                                              f.test_name);
       })) {
    for (const auto* pi :
         Sort(fixture->per_instantiation_,
              [](auto& pi) -> auto& { return pi.instantiation_key; })) {
      absl::uint128 num_tests = 1;
      for (const auto& [l, dim] : pi->values_) {
        num_tests *= dim.names.size();
        if (num_tests > std::numeric_limits<size_t>::max()) {
          ABSL_LOG(FATAL) << "Too many combinations.";
        }
      }
      for (size_t i = 0; i < num_tests; ++i) {
        pi->do_register(i);
      }
    }
  }
}

}  // namespace protobuf
}  // namespace google::internal
