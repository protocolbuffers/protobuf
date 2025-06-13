#include "google/protobuf/compiler/cpp/ifndef_guard.h"

#include <optional>
#include <string>

#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

namespace {

class IfnDefGuardTest : public testing::Test {
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
  std::optional<io::StringOutputStream> stream_{&out_};
};

TEST_F(IfnDefGuardTest, Basic) {
  {
    io::Printer printer(output(), '$');

    const IfdefGuardPrinter ifdef_guard(&printer, "A/B-E.alpha");

    EXPECT_FALSE(printer.failed());
  }

  EXPECT_EQ(written(),
            "#ifndef A_B_E_ALPHA_\n"
            "#define A_B_E_ALPHA_\n"
            "\n"
            "\n"
            "#endif  // A_B_E_ALPHA_\n");
}

TEST_F(IfnDefGuardTest, DifferentDelim) {
  {
    io::Printer printer(output(), '\0');

    const IfdefGuardPrinter ifdef_guard(&printer, "A/B/E/alpha");

    EXPECT_FALSE(printer.failed());
  }

  EXPECT_EQ(written(),
            "#ifndef A_B_E_ALPHA_\n"
            "#define A_B_E_ALPHA_\n"
            "\n"
            "\n"
            "#endif  // A_B_E_ALPHA_\n");
}

TEST_F(IfnDefGuardTest, DifferentSubstitutionFunction) {
  {
    io::Printer printer(output(), '$');

    const IfdefGuardPrinter ifdef_guard(
        &printer, "A/B/E/alpha", [](absl::string_view) { return "FOO_BAR_"; });

    EXPECT_FALSE(printer.failed());
  }

  EXPECT_EQ(written(),
            "#ifndef FOO_BAR_\n"
            "#define FOO_BAR_\n"
            "\n"
            "\n"
            "#endif  // FOO_BAR_\n");
}

}  // namespace

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
