// Copyright 2026 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

// Benchmarks for UTF-8 validation over the three text types that exercise its
// distinct paths: pure ASCII, latin1 text (short ASCII runs between two-byte
// codepoints), and dense CJK (non-ASCII from the first byte).

#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "utf8_validity.h"

namespace {

// Everything is seeded deterministically per (corpus, length, index), so every
// run and every A/B comparison measures identical bytes.
std::mt19937 Rng(uint32_t corpus, uint32_t length, uint32_t index) {
  std::seed_seq seq{corpus, length, index};
  return std::mt19937(seq);
}

void AppendCodepoint(std::string& s, uint32_t cp) {
  if (cp < 0x80) {
    s.push_back(static_cast<char>(cp));
  } else if (cp < 0x800) {
    s.push_back(static_cast<char>(0xC0 | (cp >> 6)));
    s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else {
    s.push_back(static_cast<char>(0xE0 | (cp >> 12)));
    s.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  }
}

std::string MakeAscii(size_t bytes, std::mt19937& rng) {
  std::string s;
  s.reserve(bytes);
  while (s.size() < bytes) {
    AppendCodepoint(s, 0x20 + rng() % 0x5F);
  }
  return s;
}

// 90% ASCII with two-byte codepoints mixed in. One non-ASCII codepoint is
// guaranteed by converting a draw rather than adding one, so a short string
// cannot silently degenerate into an ASCII benchmark.
std::string MakeLatin(size_t bytes, std::mt19937& rng) {
  std::string s;
  s.reserve(bytes + 1);
  bool has_non_ascii = false;
  while (s.size() < bytes) {
    if (rng() % 10 == 0 || (!has_non_ascii && s.size() + 2 >= bytes)) {
      AppendCodepoint(s, 0xC0 + rng() % 0x40);
      has_non_ascii = true;
    } else {
      AppendCodepoint(s, 0x20 + rng() % 0x5F);
    }
  }
  return s;
}

std::string MakeCjk(size_t bytes, std::mt19937& rng) {
  std::string s;
  s.reserve(bytes + 2);
  while (s.size() < bytes) {
    AppendCodepoint(s, 0x4E00 + rng() % 0x5000);
  }
  return s;
}

using Maker = std::string (*)(size_t, std::mt19937&);
constexpr Maker kMakers[] = {MakeAscii, MakeLatin, MakeCjk};

// A small corpus per configuration, rotated through the timing loop, so the
// result is not the story of one particular draw or one trained branch
// pattern.
constexpr size_t kCorpusSize = 8;

std::vector<std::string> MakeCorpus(uint32_t corpus, size_t bytes) {
  std::vector<std::string> inputs;
  inputs.reserve(kCorpusSize);
  for (uint32_t i = 0; i < kCorpusSize; ++i) {
    std::mt19937 rng = Rng(corpus, static_cast<uint32_t>(bytes), i);
    inputs.push_back(kMakers[corpus](bytes, rng));
  }
  return inputs;
}

void BM_IsStructurallyValid(benchmark::State& state, uint32_t corpus,
                            size_t bytes_per_input) {
  const std::vector<std::string> inputs = MakeCorpus(corpus, bytes_per_input);
  size_t i = 0;
  int64_t bytes = 0;
  for (auto _ : state) {
    const std::string& s = inputs[i++ % kCorpusSize];
    bytes += static_cast<int64_t>(s.size());
    benchmark::DoNotOptimize(utf8_range::IsStructurallyValid(s));
  }
  state.SetBytesProcessed(bytes);
}

void BM_SpanStructurallyValid(benchmark::State& state, uint32_t corpus,
                              size_t bytes_per_input) {
  const std::vector<std::string> inputs = MakeCorpus(corpus, bytes_per_input);
  size_t i = 0;
  int64_t bytes = 0;
  for (auto _ : state) {
    const std::string& s = inputs[i++ % kCorpusSize];
    bytes += static_cast<int64_t>(s.size());
    benchmark::DoNotOptimize(utf8_range::SpanStructurallyValid(s));
  }
  state.SetBytesProcessed(bytes);
}

constexpr const char* kCorpusNames[] = {"ascii", "latin", "cjk"};

int RegisterAll() {
  for (uint32_t corpus = 0; corpus < 3; ++corpus) {
    for (size_t bytes : {4, 8, 16, 32, 64, 512, 4096}) {
      const std::string suffix =
          std::string("/") + kCorpusNames[corpus] + "/" + std::to_string(bytes);
      benchmark::RegisterBenchmark(("BM_IsStructurallyValid" + suffix).c_str(),
                                   [corpus, bytes](benchmark::State& state) {
                                     BM_IsStructurallyValid(state, corpus,
                                                            bytes);
                                   });
      benchmark::RegisterBenchmark(
          ("BM_SpanStructurallyValid" + suffix).c_str(),
          [corpus, bytes](benchmark::State& state) {
            BM_SpanStructurallyValid(state, corpus, bytes);
          });
    }
  }
  return 0;
}

const int kRegistered = RegisterAll();

}  // namespace
