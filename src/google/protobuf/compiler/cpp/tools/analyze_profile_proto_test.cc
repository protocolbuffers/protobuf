#include "google/protobuf/compiler/cpp/tools/analyze_profile_proto.h"

#include <sstream>
#include <string>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/path.h"
#include "google/protobuf/compiler/profile.pb.h"
#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "google/protobuf/compiler/cpp/tools/analyze_profile_proto_test.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/test_textproto.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace tools {
namespace {

std::string AnalyzeToText(const AccessInfo& info,
                          AnalyzeProfileProtoOptions options) {
  std::string filename = file::JoinPath(TestTempDir(), "profile.proto");
  ABSL_CHECK_OK(file::SetBinaryProto(filename, info, true));
  std::stringstream str;
  ABSL_CHECK_OK(AnalyzeProfileProtoToText(str, filename, options));
  return str.str();
}

TEST(AnalyzeProfileProtoTest, EmptyProfileToText) {
  AccessInfo info;
  AnalyzeProfileProtoOptions options;
  options.pool = DescriptorPool::generated_pool();
  EXPECT_STREQ(AnalyzeToText(info, options).c_str(),
               "Unlikely Used Threshold = 0\n"
               "See http://go/pdlazy for more information\n"
               "-----------------------------------------\n");
}

TEST(AnalyzeProfileProtoTest, UnlikelyStringPresence) {
  AccessInfo info = ParseTextOrDie(R"pb(
    language: "cpp"
    message {
      name: "google::protobuf::compiler::tools::AnalyzeThis"
      count: 100
      field { name: "optional_string" getters_count: 1 }
    }
  )pb");
  AnalyzeProfileProtoOptions options;
  options.print_unused_threshold = false;
  options.pool = DescriptorPool::generated_pool();
  EXPECT_STREQ(AnalyzeToText(info, options).c_str(), "");
}

TEST(AnalyzeProfileProtoTest, LikelyStringPresence) {
  AccessInfo info = ParseTextOrDie(R"pb(
    language: "cpp"
    message {
      name: "google::protobuf::compiler::tools::AnalyzeThis"
      count: 100
      field { name: "optional_string" getters_count: 99 }
    }
  )pb");
  AnalyzeProfileProtoOptions options;
  options.print_unused_threshold = false;
  options.pool = DescriptorPool::generated_pool();
  EXPECT_STREQ(AnalyzeToText(info, options).c_str(),
               "Message google::protobuf::compiler::tools::AnalyzeThis\n"
               "  string optional_string: INLINE\n");
}

TEST(AnalyzeProfileProtoTest, ChildLikelyPresentAndUsed) {
  // Note that the logic pics a 50th percentile threshold which we need to
  // exceed, making testing slightly awkward
  AccessInfo info = ParseTextOrDie(R"pb(
    language: "cpp"
    message {
      name: "google::protobuf::compiler::tools::AnalyzeThis"
      count: 100
      field { name: "id" getters_count: 1000 configs_count: 100 }
      field { name: "optional_string" getters_count: 100 configs_count: 100 }
      field { name: "optional_child" getters_count: 100 configs_count: 102 }
      field { name: "repeated_string" getters_count: 100 configs_count: 1 }
      field { name: "repeated_child" getters_count: 100 configs_count: 1 }
      field { name: "nested" getters_count: 0 configs_count: 1 }
    }
  )pb");
  AnalyzeProfileProtoOptions options;
  options.print_unused_threshold = false;
  options.pool = DescriptorPool::generated_pool();
  EXPECT_STREQ(AnalyzeToText(info, options).c_str(),
               "Message google::protobuf::compiler::tools::AnalyzeThis\n"
               "  string optional_string: INLINE\n");
}

TEST(AnalyzeProfileProtoTest, UnlikelyPresent) {
  AccessInfo info = ParseTextOrDie(R"pb(
    language: "cpp"
    message {
      name: "google::protobuf::compiler::tools::AnalyzeThis"
      count: 100
      field { name: "id" getters_count: 0 }
      field { name: "optional_string" getters_count: 0 }
      field { name: "optional_child" getters_count: 100 }
      field { name: "repeated_string" getters_count: 0 }
      field { name: "repeated_child" getters_count: 0 }
      field { name: "nested" getters_count: 0 }
    }
  )pb");
  AnalyzeProfileProtoOptions options;
  options.print_unused_threshold = false;
  options.pool = DescriptorPool::generated_pool();
  EXPECT_STREQ(AnalyzeToText(info, options).c_str(),
               "Message google::protobuf::compiler::tools::AnalyzeThis\n"
               "  int32 id: SPLIT\n"
               "  string optional_string: SPLIT\n"
               "  string[] repeated_string: SPLIT\n"
               "  AnalyzeChild[] repeated_child: SPLIT\n"
               "  Nested nested: SPLIT\n");
}

TEST(AnalyzeProfileProtoTest, ChildLikelyPresentAndRarelyUsed) {
  // Note that the logic pics a 50th percentile threshold which we need to
  // exceed, making testing slightly awkward
  AccessInfo info = ParseTextOrDie(R"pb(
    language: "cpp"
    message {
      name: "google::protobuf::compiler::tools::AnalyzeThis"
      count: 100
      field { name: "id" getters_count: 1 configs_count: 100 }
      field { name: "optional_string" getters_count: 1 configs_count: 100 }
      field { name: "optional_child" getters_count: 100 configs_count: 1 }
      field { name: "repeated_string" getters_count: 100 configs_count: 100 }
      field { name: "repeated_child" getters_count: 100 configs_count: 100 }
      field { name: "nested" getters_count: 1 configs_count: 100 }
    }
  )pb");
  AnalyzeProfileProtoOptions options;
  options.print_unused_threshold = false;
  options.pool = DescriptorPool::generated_pool();
  EXPECT_STREQ(AnalyzeToText(info, options).c_str(),
               "Message google::protobuf::compiler::tools::AnalyzeThis\n"
               "  AnalyzeChild optional_child: LAZY\n");
}

TEST(AnalyzeProfileProtoTest, NestedCppNameMatchedToPoolName) {
  AccessInfo info = ParseTextOrDie(R"pb(
    language: "cpp"
    message {
      name: "google::protobuf::compiler::tools::AnalyzeThis_Nested"
      count: 100
      field { name: "id" getters_count: 100 }
      field { name: "optional_string" getters_count: 100 }
    }
  )pb");
  AnalyzeProfileProtoOptions options;
  options.print_unused_threshold = false;
  options.pool = DescriptorPool::generated_pool();
  EXPECT_STREQ(AnalyzeToText(info, options).c_str(),
               "Message google::protobuf::compiler::tools::AnalyzeThis::Nested\n"
               "  string optional_string: INLINE\n");
}

TEST(AnalyzeProfileProtoTest, PrintStatistics) {
  AccessInfo info = ParseTextOrDie(R"pb(
    language: "cpp"
    message {
      name: "google::protobuf::compiler::tools::AnalyzeThis"
      count: 100
      field { name: "id" getters_count: 1 configs_count: 100 }
      field { name: "optional_string" getters_count: 1 configs_count: 100 }
      field { name: "optional_child" getters_count: 100 configs_count: 1 }
      field {
        name: "repeated_string"
        getters_count: 100
        configs_count: 100
        num_elements_histogram: [ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 ]
        mean_num_elements: 66.1
        stdev_num_elements: 1.5
      }
      field { name: "repeated_child" getters_count: 100 configs_count: 100 }
      field { name: "nested" getters_count: 1 configs_count: 100 }
    }
  )pb");
  AnalyzeProfileProtoOptions options;
  options.print_unused_threshold = false;
  options.print_optimized = false;
  options.print_analysis = true;
  options.pool = DescriptorPool::generated_pool();
  EXPECT_STREQ(AnalyzeToText(info, options).c_str(),
               R"(Message google::protobuf::compiler::tools::AnalyzeThis
  int32 id: RARELY_USED(100)
  string optional_string: RARELY_USED(100)
  string[] repeated_string: LIKELY_PRESENT(100.00%) RARELY_USED(100) NUM_ELEMS_HISTO[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11] NUM_ELEMS_MEAN=66.1 NUM_ELEMS_STDDEV=1.5
  AnalyzeChild optional_child: LIKELY_PRESENT(100.00%) RARELY_USED(1) LAZY
  AnalyzeChild[] repeated_child: LIKELY_PRESENT(100.00%) RARELY_USED(100)
  Nested nested: RARELY_USED(100)
========
singular_lazy_num=1
singular_lazy_0usage_num=0
repeated_lazy_num=0
singular_total_pcount=101
repeated_total_pcount=100
singular_lazy_pcount=100
singular_lazy_0usage_pcount=0
repeated_lazy_pcount=0
max_pcount=100
max_ucount=100
repeated_lazy_num/singular_lazy_num=0
repeated_lazy_pcount/singular_lazy_pcount=0
singular_lazy_pcount/singular_total_pcount=0.990099
singular_lazy_0usage_pcount/singular_total_pcount=0
repeated_lazy_pcount/repeated_total_pcount=0
repeated_num_elements_histogram=[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]
repeated_num_elements_mean=66.1
repeated_num_elements_stdev=1.5
)");
}

TEST(AnalyzeProfileProtoTest, PrintStatisticsAll) {
  AccessInfo info = ParseTextOrDie(R"pb(
    language: "cpp"
    message {
      name: "google::protobuf::compiler::tools::AnalyzeThis"
      count: 100
      field { name: "id" getters_count: 1 configs_count: 100 }
      field { name: "optional_string" getters_count: 1 configs_count: 100 }
      field { name: "optional_child" getters_count: 100 configs_count: 1 }
      field { name: "repeated_string" getters_count: 100 configs_count: 100 }
      field { name: "repeated_child" getters_count: 100 configs_count: 100 }
      field { name: "nested" getters_count: 1 configs_count: 100 }
    }
  )pb");
  AnalyzeProfileProtoOptions options;
  options.print_unused_threshold = false;
  options.print_optimized = false;
  options.print_analysis = true;
  options.print_analysis_all = true;
  options.pool = DescriptorPool::generated_pool();
  EXPECT_STREQ(AnalyzeToText(info, options).c_str(),
               R"(Message google::protobuf::compiler::tools::AnalyzeThis
  int32 id: DEFAULT_PRESENT(1.00%) RARELY_USED(100)
  string optional_string: DEFAULT_PRESENT(1.00%) RARELY_USED(100)
  string[] repeated_string: LIKELY_PRESENT(100.00%) RARELY_USED(100)
  AnalyzeChild optional_child: LIKELY_PRESENT(100.00%) RARELY_USED(1) LAZY
  AnalyzeChild[] repeated_child: LIKELY_PRESENT(100.00%) RARELY_USED(100)
  Nested nested: DEFAULT_PRESENT(1.00%) RARELY_USED(100)
========
singular_lazy_num=1
singular_lazy_0usage_num=0
repeated_lazy_num=0
singular_total_pcount=101
repeated_total_pcount=100
singular_lazy_pcount=100
singular_lazy_0usage_pcount=0
repeated_lazy_pcount=0
max_pcount=100
max_ucount=100
repeated_lazy_num/singular_lazy_num=0
repeated_lazy_pcount/singular_lazy_pcount=0
singular_lazy_pcount/singular_total_pcount=0.990099
singular_lazy_0usage_pcount/singular_total_pcount=0
repeated_lazy_pcount/repeated_total_pcount=0
repeated_num_elements_histogram=[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
repeated_num_elements_mean=0
repeated_num_elements_stdev=0
)");
}

}  // namespace
}  // namespace tools
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
