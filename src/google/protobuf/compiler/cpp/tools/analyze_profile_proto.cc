// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/cpp/tools/analyze_profile_proto.h"

#include <sys/types.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "google/protobuf/testing/fileenums.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/path.h"
#include "file/util/fileyielder.h"
#include "google/protobuf/compiler/access_info_map.h"
#include "google/protobuf/compiler/split_map.h"
#include "google/protobuf/compiler/profile_bootstrap.pb.h"
#include "google/protobuf/descriptor.pb.h"
#include "absl/log/absl_log.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/cord.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "google/protobuf/compiler/cpp/cpp_access_info_parse_helper.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"
#include "third_party/re2/re2.h"
#include "thread/threadpool.h"
#include "google/protobuf/stubs/status_macros.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace tools {

namespace {

enum PDProtoScale { kInvalid, kNever, kRarely, kDefault, kLikely };

struct PDProtoAnalysis {
  PDProtoScale presence = PDProtoScale::kDefault;
  PDProtoScale usage = PDProtoScale::kDefault;
  uint64_t presence_count = 0;
  uint64_t usage_count = 0;
  float presence_probability = 0.0;
  std::optional<AccessInfoMap::ElementStats> element_stats;
};

std::ostream& operator<<(std::ostream& s, PDProtoScale scale) {
  switch (scale) {
    case PDProtoScale::kInvalid:
      return s << "INVALID";
    case PDProtoScale::kNever:
      return s << "NEVER";
    case PDProtoScale::kRarely:
      return s << "RARELY";
    case PDProtoScale::kDefault:
      return s << "DEFAULT";
    case PDProtoScale::kLikely:
      return s << "LIKELY";
  }
  return s;
}

enum PDProtoOptimization { kNone, kUnverifiedLazy, kLazy, kInline, kSplit };

std::ostream& operator<<(std::ostream& s, PDProtoOptimization optimization) {
  switch (optimization) {
    case PDProtoOptimization::kNone:
      return s << "NONE";
    case PDProtoOptimization::kLazy:
      return s << "LAZY";
    case PDProtoOptimization::kUnverifiedLazy:
      return s << "UNVERIFIED_LAZY";
    case PDProtoOptimization::kInline:
      return s << "INLINE";
    case PDProtoOptimization::kSplit:
      return s << "SPLIT";
  }
  return s;
}

class PDProtoAnalyzer {
 public:
  explicit PDProtoAnalyzer(const AccessInfo& access_info)
      : info_map_(access_info) {
    info_map_.SetAccessInfoParseHelper(
        std::make_unique<cpp::CppAccessInfoParseHelper>());
    options_.access_info_map = &info_map_;
    scc_analyzer_ = std::make_unique<cpp::MessageSCCAnalyzer>(options_);
  }

  void SetFile(const FileDescriptor* file) {
    if (current_file_ != file) {
      split_map_ = cpp::CreateSplitMap(file, options_);
      options_.split_map = &split_map_;
      current_file_ = file;
    }
  }

  bool HasProfile(const Descriptor* descriptor) const {
    return info_map_.HasProfile(descriptor);
  }

  PDProtoAnalysis AnalyzeField(const FieldDescriptor* field) {
    PDProtoAnalysis analysis;

    if (!info_map_.InProfile(field)) {
      return analysis;
    }

    analysis.presence_probability = GetPresenceProbability(field);

    if (IsLikelyPresent(field)) {
      analysis.presence = PDProtoScale::kLikely;
    } else if (IsRarelyPresent(field)) {
      analysis.presence = PDProtoScale::kRarely;
    }
    analysis.presence_count =
        info_map_.AccessCount(field, AccessInfoMap::kReadWrite);

    if (!info_map_.HasUsage(field)) {
      analysis.usage = PDProtoScale ::kInvalid;
    } else {
      analysis.usage_count =
          info_map_.AccessCount(field, AccessInfoMap::kReadWriteOther);
      if (analysis.usage_count <= info_map_.GetUnlikelyUsedThreshold()) {
        analysis.usage = PDProtoScale::kRarely;
      }
    }

    analysis.element_stats = info_map_.RepeatedElementStats(field);

    return analysis;
  }

  PDProtoOptimization OptimizeField(const FieldDescriptor* field) {
    if (IsFieldInlined(field, options_)) {
      return PDProtoOptimization::kInline;
    }
    if (IsLazy(field, options_, scc_analyzer_.get())) {
      if (IsLazilyVerifiedLazy(field, options_)) {
        return PDProtoOptimization::kUnverifiedLazy;
      }
      return PDProtoOptimization::kLazy;
    }

    if (cpp::ShouldSplit(field, options_)) {
      return PDProtoOptimization::kSplit;
    }

    return PDProtoOptimization::kNone;
  }

  uint64_t UnlikelyUsedThreshold() const {
    return info_map_.GetUnlikelyUsedThreshold();
  }

 private:
  bool IsLikelyPresent(const FieldDescriptor* field) const {
    // This threshold was arbitrarily chosen based on a few macrobenchmark
    // results.
    constexpr double kHotRatio = 0.90;

    return (info_map_.IsHot(field, AccessInfoMap::kRead, kHotRatio) ||
            info_map_.IsHot(field, AccessInfoMap::kWrite, kHotRatio));
  }

  bool IsRarelyPresent(const FieldDescriptor* field) const {
    // This threshold was arbitrarily chosen based on a few macrobenchmark
    // results. Since most cold fields have zero presence count, PDProto
    // optimization hasn't been sensitive to the threshold.
    constexpr double kColdRatio = 0.005;

    return info_map_.IsCold(field, AccessInfoMap::kRead, kColdRatio) &&
           info_map_.IsCold(field, AccessInfoMap::kWrite, kColdRatio);
  }

  float GetPresenceProbability(const FieldDescriptor* field) {
    // Since message count is max(#parse, #serialization), return the max of
    // access ratio of both parse and serialization.
    return std::max(info_map_.AccessRatio(field, AccessInfoMap::kWrite),
                    info_map_.AccessRatio(field, AccessInfoMap::kRead));
  }

  cpp::Options options_;
  AccessInfoMap info_map_;
  SplitMap split_map_;
  std::unique_ptr<cpp::MessageSCCAnalyzer> scc_analyzer_;
  const FileDescriptor* current_file_ = nullptr;
};

size_t GetLongestName(const DescriptorPool& pool, absl::string_view name,
                      size_t min_length) {
  size_t pos = name.length();
  while (pos > min_length) {
    if (name[--pos] == '_') {
      if (pool.FindMessageTypeByName(name.substr(0, pos))) {
        return pos;
      }
    }
  }
  return 0;
}

const Descriptor* FindMessageTypeByCppName(const DescriptorPool& pool,
                                           absl::string_view name) {
  std::string s = absl::StrReplaceAll(name, {{"::", "."}});
  const Descriptor* descriptor = pool.FindMessageTypeByName(s);
  if (descriptor) return descriptor;

  size_t min_length = 1;
  while (size_t pos = GetLongestName(pool, s, min_length)) {
    s[pos] = '.';
    descriptor = pool.FindMessageTypeByName(s);
    if (descriptor) return descriptor;
    min_length = pos + 1;
  }

  if (ABSL_VLOG_IS_ON(1)) {
    ABSL_LOG(WARNING) << "Unknown C++ message name '" << name << "'";
  }
  return nullptr;
}

std::string TypeName(const FieldDescriptor* descriptor) {
  if (descriptor == nullptr) return "UNKNOWN";
  std::string s;
  switch (descriptor->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      s = "int32";
      break;
    case FieldDescriptor::CPPTYPE_INT64:
      s = "int64";
      break;
    case FieldDescriptor::CPPTYPE_UINT32:
      s = "uint32";
      break;
    case FieldDescriptor::CPPTYPE_UINT64:
      s = "uint64";
      break;
    case FieldDescriptor::CPPTYPE_DOUBLE:
      s = "double";
      break;
    case FieldDescriptor::CPPTYPE_FLOAT:
      s = "float";
      break;
    case FieldDescriptor::CPPTYPE_BOOL:
      s = "bool";
      break;
    case FieldDescriptor::CPPTYPE_ENUM:
      s = "enum";
      break;
    case FieldDescriptor::CPPTYPE_STRING:
      s = "string";
      break;
    case FieldDescriptor::CPPTYPE_MESSAGE:
      s = descriptor->message_type()->name();
      break;
    default:
      s = "UNKNOWN";
      break;
  }
  if (descriptor->is_repeated()) s += "[]";
  return s;
}

absl::StatusOr<AccessInfo> AccessInfoFromFile(absl::string_view profile) {
  absl::Cord cord;
  absl::Status status = GetContents(profile, &cord, true);
  if (!status.ok()) {
    return status;
  }

  AccessInfo access_info_proto;
  if (!access_info_proto.ParseFromString(cord)) {
    return absl::DataLossError("Failed to parse AccessInfo");
  }

  return access_info_proto;
}

std::vector<const MessageAccessInfo*> SortMessages(
    const AccessInfo& access_info) {
  std::vector<const MessageAccessInfo*> sorted;
  for (const MessageAccessInfo& info : access_info.message()) {
    sorted.push_back(&info);
  }
  std::sort(sorted.begin(), sorted.end(),
            [](const MessageAccessInfo* lhs, const MessageAccessInfo* rhs) {
              return lhs->name() < rhs->name();
            });
  return sorted;
}

struct Stats {
  uint64_t singular_total_pcount = 0;
  uint64_t repeated_total_pcount = 0;
  uint64_t singular_lazy_pcount = 0;
  uint64_t singular_lazy_0usage_pcount = 0;
  uint64_t repeated_lazy_pcount = 0;
  uint64_t singular_lazy_num = 0;
  uint64_t singular_lazy_0usage_num = 0;
  uint64_t repeated_lazy_num = 0;
  uint64_t max_pcount = 0;
  uint64_t max_ucount = 0;
  // Element count stats, if the field is repeated. Otherwise, the all-zeros
  // default value is used.
  AccessInfoMap::ElementStats repeated_elem_stats;
};

void Aggregate(const FieldDescriptor* field, const PDProtoAnalysis& analysis,
               const PDProtoOptimization& optimized, Stats& stats) {
  if (stats.max_pcount < analysis.presence_count) {
    stats.max_pcount = analysis.presence_count;
  }
  if (stats.max_ucount < analysis.usage_count) {
    stats.max_ucount = analysis.usage_count;
  }
  if (field->type() == FieldDescriptor::TYPE_MESSAGE) {
    if (field->is_repeated()) {
      stats.repeated_total_pcount += analysis.presence_count;
      if (analysis.element_stats.has_value()) {
        stats.repeated_elem_stats = *analysis.element_stats;
      }
    } else {
      stats.singular_total_pcount += analysis.presence_count;
    }
  }
  if (optimized == kLazy) {
    if (field->is_repeated()) {
      stats.repeated_lazy_num++;
      stats.repeated_lazy_pcount += analysis.presence_count;
    } else {
      stats.singular_lazy_num++;
      stats.singular_lazy_pcount += analysis.presence_count;
      if (analysis.usage_count == 0) {
        stats.singular_lazy_0usage_num++;
        stats.singular_lazy_0usage_pcount += analysis.presence_count;
      }
    }
  }
  if (field->is_repeated() && analysis.element_stats.has_value()) {
    stats.repeated_elem_stats += *analysis.element_stats;
  }
}

void Aggregate(const Stats& in, Stats& out) {
  out.singular_total_pcount += in.singular_total_pcount;
  out.repeated_total_pcount += in.repeated_total_pcount;
  out.singular_lazy_num += in.singular_lazy_num;
  out.singular_lazy_0usage_num += in.singular_lazy_0usage_num;
  out.repeated_lazy_num += in.repeated_lazy_num;
  out.singular_lazy_pcount += in.singular_lazy_pcount;
  out.singular_lazy_0usage_pcount += in.singular_lazy_0usage_pcount;
  out.repeated_lazy_pcount += in.repeated_lazy_pcount;
  out.max_pcount = std::max(out.max_pcount, in.max_pcount);
  out.max_ucount = std::max(out.max_ucount, in.max_ucount);
  out.repeated_elem_stats += in.repeated_elem_stats;
}

std::ostream& operator<<(std::ostream& s, Stats stats) {
  s << "========" << std::endl
    << "singular_lazy_num=" << stats.singular_lazy_num << std::endl
    << "singular_lazy_0usage_num=" << stats.singular_lazy_0usage_num
    << std::endl
    << "repeated_lazy_num=" << stats.repeated_lazy_num << std::endl
    << "singular_total_pcount=" << stats.singular_total_pcount << std::endl
    << "repeated_total_pcount=" << stats.repeated_total_pcount << std::endl
    << "singular_lazy_pcount=" << stats.singular_lazy_pcount << std::endl
    << "singular_lazy_0usage_pcount=" << stats.singular_lazy_0usage_pcount
    << std::endl
    << "repeated_lazy_pcount=" << stats.repeated_lazy_pcount << std::endl
    << "max_pcount=" << stats.max_pcount << std::endl
    << "max_ucount=" << stats.max_ucount << std::endl
    << "repeated_lazy_num/singular_lazy_num="
    << static_cast<double>(stats.repeated_lazy_num) /
           static_cast<double>(stats.singular_lazy_num)
    << std::endl
    << "repeated_lazy_pcount/singular_lazy_pcount="
    << static_cast<double>(stats.repeated_lazy_pcount) /
           static_cast<double>(stats.singular_lazy_pcount)
    << std::endl
    << "singular_lazy_pcount/singular_total_pcount="
    << static_cast<double>(stats.singular_lazy_pcount) /
           static_cast<double>(stats.singular_total_pcount)
    << std::endl
    << "singular_lazy_0usage_pcount/singular_total_pcount="
    << static_cast<double>(stats.singular_lazy_0usage_pcount) /
           static_cast<double>(stats.singular_total_pcount)
    << std::endl
    << "repeated_lazy_pcount/repeated_total_pcount="
    << static_cast<double>(stats.repeated_lazy_pcount) /
           static_cast<double>(stats.repeated_total_pcount)
    << std::endl
    << "repeated_num_elements_histogram=["
    << absl::StrJoin(stats.repeated_elem_stats.histogram, ", ") << "]"
    << std::endl
    << "repeated_num_elements_mean=" << stats.repeated_elem_stats.mean
    << std::endl
    << "repeated_num_elements_stdev=" << stats.repeated_elem_stats.stddev
    << std::endl;
  return s;
}

}  // namespace

static absl::StatusOr<Stats> AnalyzeProfileProto(
    std::ostream& stream, absl::string_view proto_profile,
    const AnalyzeProfileProtoOptions& options) {
  if (options.pool == nullptr) {
    return absl::InvalidArgumentError("pool must not be null");
  }
  const DescriptorPool& pool = *options.pool;
  RE2 regex(options.message_filter.empty() ? ".*" : options.message_filter);
  if (!regex.ok()) {
    return absl::InvalidArgumentError("Invalid regular expression");
  }

  absl::StatusOr<AccessInfo> access_info = AccessInfoFromFile(proto_profile);
  if (!access_info.ok()) {
    return access_info.status();
  }
  PDProtoAnalyzer analyzer(*access_info);

  if (options.print_unused_threshold) {
    stream << "Unlikely Used Threshold = " << analyzer.UnlikelyUsedThreshold()
           << "\n"
           << "See http://go/pdlazy for more information\n"
           << "-----------------------------------------\n";
  }

  Stats stats;
  for (const MessageAccessInfo* message : SortMessages(*access_info)) {
    if (RE2::PartialMatch(message->name(), regex)) {
      const Descriptor* descriptor =
          FindMessageTypeByCppName(pool, message->name());

      if (descriptor == nullptr) continue;

      analyzer.SetFile(descriptor->file());
      if (analyzer.HasProfile(descriptor)) {
        bool message_header = false;
        for (int i = 0; i < descriptor->field_count(); ++i) {
          const FieldDescriptor* field = descriptor->field(i);
          PDProtoAnalysis analysis = analyzer.AnalyzeField(field);
          PDProtoOptimization optimized = analyzer.OptimizeField(field);
          Aggregate(field, analysis, optimized, stats);
          if (options.print_all_fields || options.print_analysis ||
              (options.print_optimized &&
               (optimized != PDProtoOptimization::kNone))) {
            if (!message_header) {
              message_header = true;
              stream << "Message "
                     << absl::StrReplaceAll(descriptor->full_name(),
                                            {{".", "::"}})
                     << "\n";
            }
            stream << "  " << TypeName(field) << " " << field->name() << ":";

            if (options.print_analysis) {
              if (analysis.presence != PDProtoScale::kDefault ||
                  options.print_analysis_all) {
                stream << " " << analysis.presence << "_PRESENT"
                       << absl::StrFormat("(%.2f%%)",
                                          analysis.presence_probability * 100);
              }
              if (analysis.usage != PDProtoScale::kDefault) {
                stream << " " << analysis.usage << "_USED("
                       << analysis.usage_count << ")";
              }
              if (analysis.element_stats.has_value()) {
                stream << " NUM_ELEMS_HISTO["
                       << absl::StrJoin(analysis.element_stats->histogram, ", ")
                       << "]"
                       << " NUM_ELEMS_MEAN=" << analysis.element_stats->mean
                       << " NUM_ELEMS_STDDEV="
                       << analysis.element_stats->stddev;
              }
            }
            if (optimized != PDProtoOptimization::kNone) {
              stream << " " << optimized;
            }
            stream << "\n";
          }
        }
      }
    }
  }
  if (options.print_analysis) {
    stream << stats;
  }
  return stats;
}

absl::Status AnalyzeProfileProtoToText(
    std::ostream& stream, absl::string_view proto_profile,
    const AnalyzeProfileProtoOptions& options) {
  return AnalyzeProfileProto(stream, proto_profile, options).status();
}

namespace {

absl::StatusOr<std::vector<std::string>> FindProtoProfileFiles(
    absl::string_view root) {
  std::vector<std::string> paths;
  FileYielder yielder;
  int errors = 0;
  yielder.Start({file::JoinPath(root, "*")}, file::MATCH_DEFAULT,
                /*recursively_expand=*/true, &errors);
  if (errors > 0) {
    return absl::InternalError(absl::StrCat("Failed to traverse path: ", root));
  }
  for (; !yielder.Done(); yielder.Next()) {
    const std::string& full_path = yielder.FullPathName();
    if (absl::EndsWith(full_path, "proto.profile")) {
      paths.push_back(full_path);
    }
  }
  return paths;
}

struct ParallelRunResults {
  size_t num_done = 0;
  size_t num_succeeded = 0;
  size_t num_failed = 0;
  absl::Status status = absl::OkStatus();
};

ParallelRunResults RunInParallel(
    size_t num_runs, size_t num_workers,
    const std::function<std::string(size_t i)>& get_run_id,
    const std::function<absl::Status(size_t i)>& do_work) {
  absl::Mutex mu;
  ParallelRunResults results;
  {
    ThreadPool threads{static_cast<int>(std::min(num_runs, num_workers))};
    threads.StartWorkers();
    for (size_t i = 0; i < num_runs; ++i) {
      threads.Schedule([i, num_runs, get_run_id, do_work, &results, &mu]() {
        // Asynchronous section.
        const auto run_id = get_run_id(i);
        ABSL_LOG(INFO) << "STARTING: " << run_id << " ...";
        const absl::Time start = absl::Now();
        const auto status = do_work(i);
        const absl::Duration duration = absl::Now() - start;

        // Synchronous section.
        absl::MutexLock lock(&mu);
        ++results.num_done;
        ++(status.ok() ? results.num_succeeded : results.num_failed);
        results.status.Update(status);
        ABSL_LOG(INFO) << "FINISHED " << results.num_done << " OF " << num_runs
                       << " (" << (status.ok() ? "SUCCESS IN " : "FAILURE IN ")
                       << duration << "): " << run_id;
      });
    }
  }  // Threads joins here.

  ABSL_LOG(INFO) << "TOTAL SUCCEEDED: " << results.num_succeeded << " OF "
                 << num_runs;
  ABSL_LOG(INFO) << "TOTAL FAILED: " << results.num_failed << " OF "
                 << num_runs;

  return results;
}

}  // namespace

absl::Status AnalyzeAndAggregateProfileProtosToText(
    std::ostream& stream, absl::string_view root,
    const AnalyzeProfileProtoOptions& options) {
  // Find files.
  ASSIGN_OR_RETURN(const auto paths, FindProtoProfileFiles(root));

  // Process files in parallel.
  absl::Mutex mu;
  std::vector<std::stringstream> substreams{paths.size()};
  Stats merged_stats;
  const auto results = RunInParallel(
      /*num_runs=*/paths.size(),
      /*num_workers=*/static_cast<size_t>(options.parallelism),
      /*get_run_id=*/
      [&paths, &root](size_t i) {
        return std::string{absl::StripPrefix(paths[i], root)};
      },
      /*do_work=*/
      [&paths, &substreams, &stream, &merged_stats, &options, &mu](size_t i) {
        const auto& path = paths[i];
        auto& substream = substreams[i];
        // Asynchronous section.
        substream << "PROFILE " << path << ":\n";
        ASSIGN_OR_RETURN(const auto stats,
                         AnalyzeProfileProto(substream, path, options));

        // Synchronous section.
        absl::MutexLock lock(&mu);
        Aggregate(stats, merged_stats);
        if (!options.sort_output_by_file_name) {
          stream << substream.str() << std::endl;
          substream.clear();
        }
        return absl::OkStatus();
      });

  // Print the results, unless already printed asynchronously.
  if (options.sort_output_by_file_name) {
    for (auto& substream : substreams) {
      stream << substream.str() << std::endl;
    }
  }
  stream << merged_stats;

  return results.status;
}

}  // namespace tools
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
