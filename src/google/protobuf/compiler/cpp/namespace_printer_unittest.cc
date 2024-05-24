#include "google/protobuf/compiler/cpp/namespace_printer.h"

#include <string>

#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

namespace {

class NamespacePrinterTest : public testing::Test {
 protected:
  io::ZeroCopyOutputStream* output() {
    ABSL_CHECK(stream_.has_value());
    return &*stream_;
  }
  absl::string_view written() {
    stream_.reset();
    return out_;
  }

  std::string out_;
  absl::optional<io::StringOutputStream> stream_{&out_};
};

TEST_F(NamespacePrinterTest, Basic) {
  {
    io::Printer printer(output(), '$');

    const NamespacePrinter namespace_printer(&printer, {"A", "B", "E"});

    EXPECT_FALSE(printer.failed());
  }

  EXPECT_EQ(written(),
            "namespace A {\n"
            "namespace B {\n"
            "namespace E {\n"
            "\n"
            "}  // namespace A\n"
            "}  // namespace B\n"
            "}  // namespace E\n");
}

TEST_F(NamespacePrinterTest, DifferentDelim) {
  {
    io::Printer printer(output(), '\0');

    const NamespacePrinter namespace_printer(&printer, {"A", "B", "E"});

    EXPECT_FALSE(printer.failed());
  }

  EXPECT_EQ(written(),
            "namespace A {\n"
            "namespace B {\n"
            "namespace E {\n"
            "\n"
            "}  // namespace A\n"
            "}  // namespace B\n"
            "}  // namespace E\n");
}

}  // namespace

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
