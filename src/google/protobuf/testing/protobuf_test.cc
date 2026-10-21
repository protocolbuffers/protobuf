// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/testing/protobuf_test.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_log.h"
#include "absl/numeric/int128.h"
#include "absl/strings/ascii.h"
#include "absl/strings/charset.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "absl/synchronization/mutex.h"
#include "absl/types/optional.h"

namespace google {
namespace protobuf {
namespace internal {
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

// Strips leading namespace, class, or member access qualifiers (`ns::`, `obj.`,
// `ptr->`, `&Class::`) from each element in `names`.
//
// By default, strips the entire leading qualifier from each element (e.g.
// `{"ns1::kFoo", "ns2::kBar"}` -> `{"kFoo", "kBar"}`). If multiple elements
// would strip down to the same unqualified suffix (e.g.
// `{"ns::Sub1::kFoo", "ns::Sub2::kFoo"}`), only strips the common qualifier
// prefix shared among those colliding elements (`"ns::"`), preserving the
// distinguishing qualifiers (`{"Sub1::kFoo", "Sub2::kFoo"}`).
void StripRedundantQualifiers(std::vector<std::string>& names) {
  static constexpr absl::CharSet kIdentStart =
      absl::CharSet::AsciiAlphabet() | absl::CharSet::Char('_');
  static constexpr absl::CharSet kIdentCont =
      absl::CharSet::AsciiAlphanumerics() | absl::CharSet::Char('_');
  static constexpr absl::CharSet kQualChars =
      kIdentCont | absl::CharSet(":.>-&");

  const auto find_last_sep = [](absl::string_view s) {
    const auto find = [&](absl::string_view sep) -> size_t {
      size_t pos = s.rfind(sep);
      return pos == s.npos ? 0 : pos + sep.size();
    };
    return std::max({find("::"), find("->"), find(".")});
  };

  std::vector<absl::string_view> stripped(names.size());
  absl::flat_hash_map<absl::string_view, absl::string_view> common_by_stripped;
  for (size_t i = 0; i < names.size(); ++i) {
    absl::string_view s = names[i];
    size_t qual_len = 0;
    while (qual_len < s.size() && kQualChars.contains(s[qual_len])) ++qual_len;
    size_t m = find_last_sep(s.substr(0, qual_len));
    if (m >= qual_len || !kIdentStart.contains(s[m])) m = 0;
    stripped[i] = s.substr(m);
    auto [it, inserted] =
        common_by_stripped.try_emplace(stripped[i], s.substr(0, m));
    if (!inserted) {
      it->second = absl::FindLongestCommonPrefix(it->second, s.substr(0, m));
    }
  }
  // Compute prefix lengths before mutating `names`, since `common_by_stripped`
  // keys and values are string_views pointing into `names`.
  std::vector<size_t> prefix_sizes(names.size());
  for (size_t i = 0; i < names.size(); ++i) {
    prefix_sizes[i] = find_last_sep(common_by_stripped[stripped[i]]);
  }
  for (size_t i = 0; i < names.size(); ++i) {
    names[i].erase(0, prefix_sizes[i]);
  }
}

}  // namespace

void PartialTestRegistry::SanitizeNames(
    std::vector<std::string>& names,
    absl::optional<absl::string_view> name_prefix, bool is_bool) {
  static constexpr absl::CharSet kIdentCont =
      absl::CharSet::AsciiAlphanumerics() | absl::CharSet::Char('_');

  StripRedundantQualifiers(names);

  for (std::string& name : names) {
    if (is_bool && name_prefix.has_value() && !name_prefix->empty() &&
        (name == "true" || name == "false")) {
      if (name == "true") {
        name = std::string(*name_prefix);
      } else {
        name =
            absl::StrCat(absl::ascii_isupper((*name_prefix)[0]) ? "No" : "no_",
                         *name_prefix);
      }
    } else if (name_prefix.has_value()) {
      name = absl::StrCat(*name_prefix, "_", name);
    }

    const auto replace = [&](absl::string_view needle, absl::string_view rep) {
      while (true) {
        if (auto pos = absl::string_view(name).find(needle); pos != name.npos) {
          name.replace(pos, needle.size(), rep.data(), rep.size());
        } else {
          break;
        }
      }
    };

    // Let's keep "pointer" in the name.
    replace("*", "Ptr");

    for (char& c : name) {
      if (!kIdentCont.contains(c)) {
        c = '_';
      }
    }

    // Collapse duplicate _
    replace("__", "_");

    // Remove leading _
    while (name[0] == '_') name.erase(0, 1);
    // Remove trailing _
    while (!name.empty() && name.back() == '_') name.pop_back();

    // Don't have empty names.
    if (name.empty()) name = "_";
  }

  DedupNames(names);
}

void PartialTestRegistry::DedupNames(std::vector<std::string>& names) {
  absl::flat_hash_set<std::string> dupes;
  for (auto& s : names) {
    if (dupes.insert(s).second) continue;
    for (int i = 2;; ++i) {
      std::string dedup =
          s == "_" ? absl::StrCat("_", i) : absl::StrCat(s, "_", i);
      if (dupes.insert(dedup).second) {
        s = std::move(dedup);
        break;
      }
    }
  }
}

size_t PartialTestRegistry::GetValueIndex(CounterT id, const void*,
                                          Location loc) {
  if (current_test_ == nullptr) {
    ADD_FAILURE_AT(loc.file, loc.line) << "Must use PB_TEST or PB_TEST_F";
    return 0;
  }

  {
    absl::MutexLock lock(used_values_mu_);
    if (!used_values_.insert(id).second) {
      ADD_FAILURE_AT(loc.file, loc.line)
          << "Attempted to call PB_TEST_CHOOSE_VALUE(...) twice in a single "
             "test run. Running it twice will not give a new value, so the "
             "system prevents that to avoid accidental misuse.";
    }
  }

  size_t index = current_test_index_;
  for (const auto& [i, dim] : current_test_->dimensions) {
    if (dim.kind != DimensionKind::kValue) continue;
    if (i == id) {
      return index % dim.names.size();
    } else {
      index /= dim.names.size();
    }
  }

  ADD_FAILURE_AT(loc.file, loc.line)
      << "PB_TEST_CHOOSE_VALUE(...) called from a class that is neither the "
         "current PB_TEST nor its direct fixture base class.";
  return 0;
}

void PartialTestRegistry::UnsetCurrentTest() {
  absl::MutexLock lock(used_values_mu_);
  if (!::testing::Test::HasFatalFailure() && !::testing::Test::IsSkipped()) {
    for (const auto& [id, dim] : current_test_->dimensions) {
      if (dim.kind == DimensionKind::kValue && !used_values_.contains(id)) {
        const Location loc = current_fixture_->locations.at(id);
        ADD_FAILURE_AT(loc.file, loc.line)
            << "PB_TEST_CHOOSE_VALUE(...) dimension was registered for this "
               "test but never evaluated during the test run.";
      }
    }
  }
  current_fixture_ = nullptr;
  current_test_ = nullptr;
  current_test_index_ = ~size_t{};
  used_values_.clear();
}

absl::optional<std::vector<std::string>> PartialTestRegistry::ConsumeList(
    absl::string_view& literal, char open, char close,
    bool allow_angle_brackets) {
  literal = absl::StripAsciiWhitespace(literal);
  if (!absl::ConsumePrefix(&literal, absl::string_view(&open, 1))) {
    return absl::nullopt;
  }
  std::vector<std::string> res;
  std::vector<char> to_close = {close};
  size_t elem_start = 0;
  for (size_t i = 0; i < literal.size(); ++i) {
    const char c = literal[i];
    if (c == '"' || c == '\'') {
      for (++i; i < literal.size() && literal[i] != c; ++i) {
        if (literal[i] == '\\') ++i;
      }
      if (i >= literal.size()) break;
    } else if (c == '{') {
      to_close.push_back('}');
    } else if (c == '(') {
      to_close.push_back(')');
    } else if (c == '[') {
      to_close.push_back(']');
    } else if (c == '<' && allow_angle_brackets &&
               (to_close.size() == 1 || to_close.back() == '>')) {
      to_close.push_back('>');
    } else if (c == '}' || c == ')' || c == ']') {
      if (to_close.back() != c) break;
      to_close.pop_back();
    } else if (c == '>' && allow_angle_brackets && to_close.back() == '>') {
      to_close.pop_back();
    }
    if (to_close.empty() || (c == ',' && to_close.size() == 1)) {
      absl::string_view elem = absl::StripAsciiWhitespace(
          literal.substr(elem_start, i - elem_start));
      if (absl::ConsumePrefix(&elem, "static_cast<")) {
        if (size_t pos = elem.find(">("); pos != elem.npos) {
          elem = elem.substr(pos + 1);
        }
      }
      if (!elem.empty()) res.emplace_back(elem);
      elem_start = i + 1;
      if (to_close.empty()) {
        literal = literal.substr(elem_start);
        return res;
      }
    }
  }
  return absl::nullopt;
}

absl::optional<std::vector<std::string>>
PartialTestRegistry::ConsumeExpressionList(absl::string_view& literal,
                                           size_t expected_size) {
  absl::string_view copy = literal;
  if (auto res = ConsumeList(copy, '(', ')', /*allow_angle_brackets=*/false);
      res.has_value() && res->size() == expected_size) {
    literal = copy;
    return res;
  }
  copy = literal;
  if (auto res = ConsumeList(copy, '(', ')', /*allow_angle_brackets=*/true);
      res.has_value() && res->size() == expected_size) {
    literal = copy;
    return res;
  }
  return absl::nullopt;
}

std::vector<std::string> PartialTestRegistry::ParseNamesFromLiteral(
    absl::string_view literal, size_t size) {
  absl::optional<std::string> name_prefix;
  absl::optional<std::vector<std::string>> parsed =
      ConsumeExpressionList(literal, size);
  if (!parsed.has_value()) {
    size_t comma = literal.find(',');
    if (comma != literal.npos) literal = literal.substr(comma);
  }
  literal = absl::StripAsciiWhitespace(literal);
  if (absl::ConsumePrefix(&literal, ",")) {
    literal = absl::StripAsciiWhitespace(literal);
    if (absl::ConsumePrefix(&literal, "\"")) {
      size_t end = literal.find('"');
      if (end != literal.npos) {
        name_prefix = std::string(literal.substr(0, end));
        literal = absl::StripAsciiWhitespace(literal.substr(end + 1));
        absl::ConsumePrefix(&literal, ",");
      }
    }
    if (auto explicit_names = ConsumeList(literal, '{', '}')) {
      parsed = *std::move(explicit_names);
    }
  }

  std::vector<std::string> names;
  if (parsed.has_value() && parsed->size() == size) {
    names = *std::move(parsed);
  } else {
    names.reserve(size);
    for (size_t i = 0; i < size; ++i) {
      names.push_back(absl::StrCat(i));
    }
  }

  SanitizeNames(names, name_prefix);
  return names;
}

PartialTestRegistry::PerFixture::PerInstantiation::TestNames
PartialTestRegistry::PerFixture::PerInstantiation::GetNames(
    absl::string_view base_test_name, size_t i) const {
  TestNames out{std::string(base_test_name), "", ""};
  const auto get_id = [&](CounterT id) {
    for (auto pci : instantiation_key) {
      if (pci.counter == id) return pci.value;
    }
    return size_t{};
  };
  for (const auto& [l, dim] : dimensions) {
    absl::string_view name;
    std::string* target = nullptr;
    switch (dim.kind) {
      case DimensionKind::kType:
        name = dim.names[get_id(l)];
        target = &out.type_name;
        break;
      case DimensionKind::kConstexprValue:
        name = dim.names[get_id(l)];
        target = &out.value_name;
        break;
      case DimensionKind::kValue:
        name = dim.names[i % dim.names.size()];
        i /= dim.names.size();
        target = &out.value_name;
        break;
    }
    absl::StrAppend(&out.test_name, "/", name);
    if (!target->empty()) absl::StrAppend(target, "/");
    absl::StrAppend(target, name);
  }
  return out;
}

template <typename Map, typename F>
static auto Sort(const Map& map, F func) {
  std::vector<const typename Map::mapped_type*> out;
  for (auto& [k, v] : map) out.push_back(&v);
  std::sort(out.begin(), out.end(),
            [&](auto* a, auto* b) { return func(*a) < func(*b); });
  return out;
}

void PartialTestRegistry::RegisterAll() {
  // Push all the parent dimensions to their children.
  for (auto& [k, fixture] : fixtures_) {
    auto it = fixtures_.find(fixture.parent_key);
    if (it == fixtures_.end()) continue;

    fixture.locations.insert(it->second.locations.begin(),
                             it->second.locations.end());
    auto& parent_dimensions =
        it->second.per_instantiation.begin()->second.dimensions;

    for (auto& [ik, inst] : fixture.per_instantiation) {
      inst.dimensions.insert(parent_dimensions.begin(),
                             parent_dimensions.end());
    }
  }

  // Compile-time instantiation type tags (`Indices`) omit index 0 so the base
  // instantiation (`void()`) can execute index 0 of each compile-time dimension
  // without re-instantiating the test body. Fill in `{id, 0}` for any
  // compile-time dimension not present in `instantiation_key` so that
  // lexicographical sorting compares indices at matching dimension counters
  // rather than comparing an earlier dimension's non-zero counter against a
  // later dimension's counter.
  for (auto& [k, fixture] : fixtures_) {
    if (fixture.parent_key == nullptr) continue;
    for (auto& [ik, inst] : fixture.per_instantiation) {
      for (const auto& [id, dim] : inst.dimensions) {
        if (dim.kind == DimensionKind::kValue) continue;
        auto it = std::lower_bound(inst.instantiation_key.begin(),
                                   inst.instantiation_key.end(),
                                   PerCounterIndex{id, 0});
        if (it == inst.instantiation_key.end() || it->counter != id) {
          inst.instantiation_key.insert(it, PerCounterIndex{id, 0});
        }
      }
    }
  }

  for (const auto* fixture : Sort(fixtures_, [](auto& f) {
         return std::tuple(f.file, f.line, f.test_suite_name, f.test_name);
       })) {
    if (fixture->parent_key == nullptr) {
      // This is a parent, so skip
      continue;
    }
    for (const auto* pi :
         Sort(fixture->per_instantiation,
              [](auto& pi) -> auto& { return pi.instantiation_key; })) {
      absl::uint128 num_tests = 1;
      for (const auto& [l, dim] : pi->dimensions) {
        if (dim.kind != DimensionKind::kValue) continue;
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

}  // namespace internal
}  // namespace protobuf
}  // namespace google
