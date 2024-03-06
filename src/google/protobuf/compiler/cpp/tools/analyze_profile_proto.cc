// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/cpp/tools/analyze_profile_proto.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
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
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/cpp_access_info_parse_helper.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"
#include "third_party/re2/re2.h"

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

  ABSL_LOG(WARNING) << "Unknown c++ message name '" << name << "'";
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
  if (!access_info_proto.ParseFromCord(cord)) {
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
      if (const Descriptor* descriptor =
              FindMessageTypeByCppName(pool, message->name())) {
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
                if (analysis.presence != PDProtoScale::kDefault) {
                  stream << " " << analysis.presence << "_PRESENT";
                }
                if (analysis.usage != PDProtoScale::kDefault) {
                  stream << " " << analysis.usage << "_USED("
                         << analysis.usage_count << ")";
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

absl::Status AnalyzeAndAggregateProfileProtosToText(
    std::ostream& stream, absl::string_view root,
    const AnalyzeProfileProtoOptions& options) {
  FileYielder yielder;
  int errors = 0;
  yielder.Start({file::JoinPath(root, "*")}, file::MATCH_DEFAULT,
                /*recursively_expand=*/true, &errors);
  if (errors > 0) {
    return absl::InternalError(absl::StrCat("Failed to traverse path: ", root));
  }
  Stats merged;
  for (; !yielder.Done(); yielder.Next()) {
    const std::string& full_path = yielder.FullPathName();
    if (!absl::EndsWith(full_path, "proto.profile")) {
      continue;
    }
    stream << full_path << std::endl;
    auto stats = *AnalyzeProfileProto(stream, full_path, options);
    Aggregate(stats, merged);
  }
  stream << merged;
  return absl::OkStatus();
}

}  // namespace tools
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
