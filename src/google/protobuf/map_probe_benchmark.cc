// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "absl/random/random.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/map.h"

namespace google::protobuf::internal {
struct MapBenchmarkPeer {
  template <typename T>
  static double LoadFactor(const T& map) {
    return static_cast<double>(map.size()) /
           static_cast<double>(map.num_buckets_);
  }

  template <typename T>
  static double GetMeanProbeLength(const T& map) {
    double total_probe_cost = 0;
    for (map_index_t b = 0; b < map.num_buckets_; ++b) {
      if (map.TableEntryIsList(b)) {
        auto* node = internal::TableEntryToNode(map.table_[b]);
        size_t cost = 0;
        while (node != nullptr) {
          total_probe_cost += static_cast<double>(cost);
          cost++;
          node = node->next;
        }
      } else if (map.TableEntryIsTree(b)) {
        // Overhead factor to account for more costly binary search.
        constexpr double kTreeOverhead = 2.0;
        size_t tree_size = TableEntryToTree(map.table_[b])->size();
        total_probe_cost += kTreeOverhead * static_cast<double>(tree_size) *
                            std::log2(tree_size);
      }
    }
    return total_probe_cost / map.size();
  }

  template <typename T>
  static double GetPercentTree(const T& map) {
    size_t total_tree_size = 0;
    for (map_index_t b = 0; b < map.num_buckets_; ++b) {
      if (map.TableEntryIsTree(b)) {
        total_tree_size += TableEntryToTree(map.table_[b])->size();
      }
    }
    return static_cast<double>(total_tree_size) /
           static_cast<double>(map.size());
  }
};
}  // namespace protobuf
}  // namespace google::internal

namespace {

using Peer = google::protobuf::internal::MapBenchmarkPeer;

absl::BitGen& GlobalBitGen() {
  static auto* value = new absl::BitGen;
  return *value;
}

template <class T>
using Table = google::protobuf::Map<T, int>;

struct LoadSizes {
  size_t min_load;
  size_t max_load;
};

LoadSizes GetMinMaxLoadSizes() {
  static const auto sizes = [] {
    Table<int> t;

    // First, fill enough to have a good distribution.
    constexpr size_t kMinSize = 10000;
    while (t.size() < kMinSize) t[static_cast<int>(t.size())];

    const auto reach_min_load_factor = [&] {
      const double lf = Peer::LoadFactor(t);
      while (lf <= Peer::LoadFactor(t)) t[static_cast<int>(t.size())];
    };

    // Then, insert until we reach min load factor.
    reach_min_load_factor();
    const size_t min_load_size = t.size();

    // Keep going until we hit min load factor again, then go back one.
    t[static_cast<int>(t.size())];
    reach_min_load_factor();

    return LoadSizes{min_load_size, t.size() - 1};
  }();
  return sizes;
}

struct Ratios {
  double min_load;
  double avg_load;
  double max_load;
  double percent_tree;
};

template <class ElemFn>
Ratios CollectMeanProbeLengths() {
  const auto min_max_sizes = GetMinMaxLoadSizes();

  ElemFn elem;
  using Key = decltype(elem());
  Table<Key> t;

  Ratios result;
  while (t.size() < min_max_sizes.min_load) t[elem()];
  result.min_load = Peer::GetMeanProbeLength(t);

  while (t.size() < (min_max_sizes.min_load + min_max_sizes.max_load) / 2)
    t[elem()];
  result.avg_load = Peer::GetMeanProbeLength(t);

  while (t.size() < min_max_sizes.max_load) t[elem()];
  result.max_load = Peer::GetMeanProbeLength(t);
  result.percent_tree = Peer::GetPercentTree(t);

  return result;
}

constexpr char kStringFormat[] = "/path/to/file/name-%07d-of-9999999.txt";

template <bool small>
struct String {
  std::string value;
  static std::string Make(uint32_t v) {
    return {small ? absl::StrCat(v) : absl::StrFormat(kStringFormat, v)};
  }
};

template <class T>
struct Sequential {
  T operator()() const { return current++; }
  mutable T current{};
};

template <bool small>
struct Sequential<String<small>> {
  std::string operator()() const { return String<small>::Make(current++); }
  mutable uint32_t current = 0;
};

template <class T, int percent_skip>
struct AlmostSequential {
  mutable Sequential<T> current;

  auto operator()() const -> decltype(current()) {
    while (absl::Uniform(GlobalBitGen(), 0.0, 1.0) <= percent_skip / 100.)
      current();
    return current();
  }
};

struct Uniform {
  template <typename T>
  T operator()(T) const {
    return absl::Uniform<T>(absl::IntervalClosed, GlobalBitGen(), T{0}, ~T{0});
  }
};

struct Gaussian {
  template <typename T>
  T operator()(T) const {
    double d;
    do {
      d = absl::Gaussian<double>(GlobalBitGen(), 1e6, 1e4);
    } while (d <= 0 || d > std::numeric_limits<T>::max() / 2);
    return static_cast<T>(d);
  }
};

struct Zipf {
  template <typename T>
  T operator()(T) const {
    return absl::Zipf<T>(GlobalBitGen(), std::numeric_limits<T>::max(), 1.6);
  }
};

template <class T, class Dist>
struct Random {
  T operator()() const { return Dist{}(T{}); }
};

template <class Dist, bool small>
struct Random<String<small>, Dist> {
  std::string operator()() const {
    return String<small>::Make(Random<uint32_t, Dist>{}());
  }
};

template <typename>
std::string Name();

std::string Name(uint64_t*) { return "u64"; }

template <bool small>
std::string Name(String<small>*) {
  return small ? "StrS" : "StrL";
}

template <class T>
std::string Name(Sequential<T>*) {
  return "Sequential";
}

template <class T, int P>
std::string Name(AlmostSequential<T, P>*) {
  return absl::StrCat("AlmostSeq_", P);
}

template <class T>
std::string Name(Random<T, Uniform>*) {
  return "UnifRand";
}

template <class T>
std::string Name(Random<T, Gaussian>*) {
  return "GausRand";
}

template <class T>
std::string Name(Random<T, Zipf>*) {
  return "ZipfRand";
}

template <typename T>
std::string Name() {
  return Name(static_cast<T*>(nullptr));
}

struct Result {
  std::string name;
  std::string dist_name;
  Ratios ratios;
};

template <typename T, typename Dist>
void RunForTypeAndDistribution(std::vector<Result>& results) {
  results.push_back({Name<T>(), Name<Dist>(), CollectMeanProbeLengths<Dist>()});
}

template <class T>
void RunForType(std::vector<Result>& results) {
  RunForTypeAndDistribution<T, Sequential<T>>(results);
  RunForTypeAndDistribution<T, AlmostSequential<T, 20>>(results);
  RunForTypeAndDistribution<T, AlmostSequential<T, 50>>(results);
  RunForTypeAndDistribution<T, Random<T, Uniform>>(results);
  RunForTypeAndDistribution<T, Random<T, Gaussian>>(results);
  RunForTypeAndDistribution<T, Random<T, Zipf>>(results);
}

}  // namespace

int main(int argc, char** argv) {
  std::vector<Result> results;
  RunForType<uint64_t>(results);
  RunForType<String<true>>(results);
  RunForType<String<false>>(results);

  absl::PrintF("{\n");
  absl::PrintF("  \"benchmarks\": [\n");
  absl::string_view comma;
  for (const auto& result : results) {
    auto print = [&](absl::string_view stat, double Ratios::*val) {
      std::string name =
          absl::StrCat(result.name, "/", result.dist_name, "/", stat);
      absl::PrintF("    %s{\n", comma);
      absl::PrintF("      \"cpu_time\": 0,\n");
      absl::PrintF("      \"real_time\": 0,\n");
      absl::PrintF("      \"allocs_per_iter\": %f,\n", result.ratios.*val);

      absl::PrintF("      \"iterations\": 1,\n");
      absl::PrintF("      \"name\": \"%s\",\n", name);
      absl::PrintF("      \"time_unit\": \"ns\"\n");
      absl::PrintF("    }\n");
      comma = ",";
    };
    print("min", &Ratios::min_load);
    print("avg", &Ratios::avg_load);
    print("max", &Ratios::max_load);
    print("tree_percent", &Ratios::percent_tree);
  }
  absl::PrintF("  ],\n");
  absl::PrintF("  \"context\": {\n");
  absl::PrintF("  }\n");
  absl::PrintF("}\n");

  return 0;
}
