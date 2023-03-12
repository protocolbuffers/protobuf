// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/io/printer.h"

#include <ostream>
#include <string>
#include <tuple>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"

namespace google {
namespace protobuf {
namespace io {
namespace {
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::IsEmpty;
using ::testing::MatchesRegex;

class PrinterTest : public testing::Test {
 protected:
  ZeroCopyOutputStream* output() {
    ABSL_CHECK(stream_.has_value());
    return &*stream_;
  }
  absl::string_view written() {
    stream_.reset();
    return out_;
  }

  std::string out_;
  absl::optional<StringOutputStream> stream_{&out_};
};

TEST_F(PrinterTest, EmptyPrinter) {
  Printer printer(output(), '\0');
  EXPECT_FALSE(printer.failed());
}

TEST_F(PrinterTest, BasicPrinting) {
  {
    Printer printer(output(), '\0');

    printer.Print("Hello World!");
    printer.Print("  This is the same line.\n");
    printer.Print("But this is a new one.\nAnd this is another one.");
    EXPECT_FALSE(printer.failed());
  }

  EXPECT_EQ(written(),
            "Hello World!  This is the same line.\n"
            "But this is a new one.\n"
            "And this is another one.");
}

TEST_F(PrinterTest, WriteRaw) {
  {
    absl::string_view string_obj = "From an object\n";
    Printer printer(output(), '$');
    printer.WriteRaw("Hello World!", 12);
    printer.PrintRaw("  This is the same line.\n");
    printer.PrintRaw("But this is a new one.\nAnd this is another one.");
    printer.WriteRaw("\n", 1);
    printer.PrintRaw(string_obj);
    EXPECT_FALSE(printer.failed());
  }

  EXPECT_EQ(written(),
            "Hello World!  This is the same line.\n"
            "But this is a new one.\n"
            "And this is another one."
            "\n"
            "From an object\n");
}

TEST_F(PrinterTest, VariableSubstitution) {
  {
    Printer printer(output(), '$');

    absl::flat_hash_map<std::string, std::string> vars{
        {"foo", "World"},
        {"bar", "$foo$"},
        {"abcdefg", "1234"},
    };

    printer.Print(vars, "Hello $foo$!\nbar = $bar$\n");
    printer.PrintRaw("RawBit\n");
    printer.Print(vars, "$abcdefg$\nA literal dollar sign:  $$");

    vars["foo"] = "blah";
    printer.Print(vars, "\nNow foo = $foo$.");

    EXPECT_FALSE(printer.failed());
  }

  EXPECT_EQ(written(),
            "Hello World!\n"
            "bar = $foo$\n"
            "RawBit\n"
            "1234\n"
            "A literal dollar sign:  $\n"
            "Now foo = blah.");
}

TEST_F(PrinterTest, InlineVariableSubstitution) {
  {
    Printer printer(output(), '$');
    printer.Print("Hello $foo$!\n", "foo", "World");
    printer.PrintRaw("RawBit\n");
    printer.Print("$foo$ $bar$\n", "foo", "one", "bar", "two");
    EXPECT_FALSE(printer.failed());
  }

  EXPECT_EQ(written(),
            "Hello World!\n"
            "RawBit\n"
            "one two\n");
}

// FakeDescriptorFile defines only those members that Printer uses to write out
// annotations.
struct FakeDescriptorFile {
  const std::string& name() const { return filename; }
  std::string filename;
};

// FakeDescriptor defines only those members that Printer uses to write out
// annotations.
struct FakeDescriptor {
  const FakeDescriptorFile* file() const { return &fake_file; }
  void GetLocationPath(std::vector<int>* output) const { *output = path; }

  FakeDescriptorFile fake_file;
  std::vector<int> path;
};

class FakeAnnotationCollector : public AnnotationCollector {
 public:
  ~FakeAnnotationCollector() override = default;

  // Records that the bytes in file_path beginning with begin_offset and ending
  // before end_offset are associated with the SourceCodeInfo-style path.
  void AddAnnotation(size_t begin_offset, size_t end_offset,
                     const std::string& file_path,
                     const std::vector<int>& path) override {
    annotations_.emplace_back(
        Record{begin_offset, end_offset, file_path, path});
  }

  void AddAnnotation(size_t begin_offset, size_t end_offset,
                     const std::string& file_path, const std::vector<int>& path,
                     absl::optional<Semantic> semantic) override {
    annotations_.emplace_back(
        Record{begin_offset, end_offset, file_path, path, semantic});
  }

  void AddAnnotationNew(Annotation& a) override {
    GeneratedCodeInfo::Annotation annotation;
    annotation.ParseFromString(a.second);

    Record r{a.first.first, a.first.second, annotation.source_file(), {}};
    for (int i : annotation.path()) {
      r.path.push_back(i);
    }

    annotations_.emplace_back(r);
  }

  struct Record {
    size_t start = 0;
    size_t end = 0;
    std::string file_path;
    std::vector<int> path;
    absl::optional<Semantic> semantic;

    friend std::ostream& operator<<(std::ostream& out, const Record& record) {
      return out << "Record{" << record.start << ", " << record.end << ", \""
                 << record.file_path << "\", ["
                 << absl::StrJoin(record.path, ", ") << "], "
                 << record.semantic.value_or(kNone) << "}";
    }
  };

  absl::Span<const Record> Get() const { return annotations_; }

 private:
  std::vector<Record> annotations_;
};

template <typename Start, typename End, typename FilePath, typename Path,
          typename Semantic = absl::optional<AnnotationCollector::Semantic>>
testing::Matcher<FakeAnnotationCollector::Record> Annotation(
    Start start, End end, FilePath file_path, Path path,
    Semantic semantic = absl::nullopt) {
  return AllOf(
      Field("start", &FakeAnnotationCollector::Record::start, start),
      Field("end", &FakeAnnotationCollector::Record::end, end),
      Field("file_path", &FakeAnnotationCollector::Record::file_path,
            file_path),
      Field("path", &FakeAnnotationCollector::Record::path, path),
      Field("semantic", &FakeAnnotationCollector::Record::semantic, semantic));
}

TEST_F(PrinterTest, AnnotateMap) {
  FakeAnnotationCollector collector;
  {
    Printer printer(output(), '$', &collector);
    absl::flat_hash_map<std::string, std::string> vars = {{"foo", "3"},
                                                          {"bar", "5"}};
    printer.Print(vars, "012$foo$4$bar$\n");

    FakeDescriptor descriptor_1{{"path_1"}, {33}};
    FakeDescriptor descriptor_2{{"path_2"}, {11, 22}};
    printer.Annotate("foo", "foo", &descriptor_1);
    printer.Annotate("bar", "bar", &descriptor_2);
  }

  EXPECT_EQ(written(), "012345\n");
  EXPECT_THAT(collector.Get(),
              ElementsAre(Annotation(3, 4, "path_1", ElementsAre(33)),
                          Annotation(5, 6, "path_2", ElementsAre(11, 22))));
}

TEST_F(PrinterTest, AnnotateInline) {
  FakeAnnotationCollector collector;
  {
    Printer printer(output(), '$', &collector);
    printer.Print("012$foo$4$bar$\n", "foo", "3", "bar", "5");

    FakeDescriptor descriptor_1{{"path_1"}, {33}};
    FakeDescriptor descriptor_2{{"path_2"}, {11, 22}};
    printer.Annotate("foo", "foo", &descriptor_1);
    printer.Annotate("bar", "bar", &descriptor_2);
  }

  EXPECT_EQ(written(), "012345\n");
  EXPECT_THAT(collector.Get(),
              ElementsAre(Annotation(3, 4, "path_1", ElementsAre(33)),
                          Annotation(5, 6, "path_2", ElementsAre(11, 22))));
}

TEST_F(PrinterTest, AnnotateRange) {
  FakeAnnotationCollector collector;
  {
    Printer printer(output(), '$', &collector);
    printer.Print("012$foo$4$bar$\n", "foo", "3", "bar", "5");

    FakeDescriptor descriptor{{"path"}, {33}};
    printer.Annotate("foo", "bar", &descriptor);
  }

  EXPECT_EQ(written(), "012345\n");
  EXPECT_THAT(collector.Get(),
              ElementsAre(Annotation(3, 6, "path", ElementsAre(33))));
}

TEST_F(PrinterTest, AnnotateEmptyRange) {
  FakeAnnotationCollector collector;
  {
    Printer printer(output(), '$', &collector);
    printer.Print("012$foo$4$baz$$bam$$bar$\n", "foo", "3", "bar", "5", "baz",
                  "", "bam", "");

    FakeDescriptor descriptor{{"path"}, {33}};
    printer.Annotate("baz", "bam", &descriptor);
  }

  EXPECT_EQ(written(), "012345\n");
  EXPECT_THAT(collector.Get(),
              ElementsAre(Annotation(5, 5, "path", ElementsAre(33))));
}

TEST_F(PrinterTest, AnnotateDespiteUnrelatedMultipleUses) {
  FakeAnnotationCollector collector;
  {
    Printer printer(output(), '$', &collector);
    printer.Print("012$foo$4$foo$$bar$\n", "foo", "3", "bar", "5");

    FakeDescriptor descriptor{{"path"}, {33}};
    printer.Annotate("bar", "bar", &descriptor);
  }

  EXPECT_EQ(written(), "0123435\n");
  EXPECT_THAT(collector.Get(),
              ElementsAre(Annotation(6, 7, "path", ElementsAre(33))));
}

TEST_F(PrinterTest, AnnotateIndent) {
  FakeAnnotationCollector collector;
  {
    Printer printer(output(), '$', &collector);
    printer.Print("0\n");
    printer.Indent();

    printer.Print("$foo$", "foo", "4");
    FakeDescriptor descriptor1{{"path"}, {44}};
    printer.Annotate("foo", &descriptor1);

    printer.Print(",\n");
    printer.Print("$bar$", "bar", "9");
    FakeDescriptor descriptor2{{"path"}, {99}};
    printer.Annotate("bar", &descriptor2);

    printer.Print("\n${$$D$$}$\n", "{", "", "}", "", "D", "d");
    FakeDescriptor descriptor3{{"path"}, {1313}};
    printer.Annotate("{", "}", &descriptor3);

    printer.Outdent();
    printer.Print("\n");
  }

  EXPECT_EQ(written(), "0\n  4,\n  9\n  d\n\n");
  EXPECT_THAT(collector.Get(),
              ElementsAre(Annotation(4, 5, "path", ElementsAre(44)),
                          Annotation(9, 10, "path", ElementsAre(99)),
                          Annotation(13, 14, "path", ElementsAre(1313))));
}

TEST_F(PrinterTest, AnnotateIndentNewline) {
  FakeAnnotationCollector collector;
  {
    Printer printer(output(), '$', &collector);
    printer.Indent();

    printer.Print("$A$$N$$B$C\n", "A", "", "N", "\nz", "B", "");
    FakeDescriptor descriptor{{"path"}, {0}};
    printer.Annotate("A", "B", &descriptor);

    printer.Outdent();
    printer.Print("\n");
  }
  EXPECT_EQ(written(), "\nz  C\n\n");

  EXPECT_THAT(collector.Get(),
              ElementsAre(Annotation(0, 4, "path", ElementsAre(0))));
}

TEST_F(PrinterTest, Indenting) {
  {
    Printer printer(output(), '$');
    absl::flat_hash_map<std::string, std::string> vars = {{"newline", "\n"}};

    printer.Print("This is not indented.\n");
    printer.Indent();
    printer.Print("This is indented\nAnd so is this\n");
    printer.Outdent();
    printer.Print("But this is not.");
    printer.Indent();
    printer.Print(
        "  And this is still the same line.\n"
        "But this is indented.\n");
    printer.PrintRaw("RawBit has indent at start\n");
    printer.PrintRaw("but not after a raw newline\n");
    printer.Print(vars,
                  "Note that a newline in a variable will break "
                  "indenting, as we see$newline$here.\n");
    printer.Indent();
    printer.Print("And this");
    printer.Outdent();
    printer.Outdent();
    printer.Print(" is double-indented\nBack to normal.");

    EXPECT_FALSE(printer.failed());
  }

  EXPECT_EQ(
      written(),
      "This is not indented.\n"
      "  This is indented\n"
      "  And so is this\n"
      "But this is not.  And this is still the same line.\n"
      "  But this is indented.\n"
      "  RawBit has indent at start\n"
      "but not after a raw newline\n"
      "Note that a newline in a variable will break indenting, as we see\n"
      "here.\n"
      "    And this is double-indented\n"
      "Back to normal.");
}

TEST_F(PrinterTest, WriteFailurePartial) {
  std::string buffer(17, '\xaa');
  ArrayOutputStream output(&buffer[0], buffer.size());
  Printer printer(&output, '$');

  // Print 16 bytes to almost fill the buffer (should not fail).
  printer.Print("0123456789abcdef");
  EXPECT_FALSE(printer.failed());

  // Try to print 2 chars. Only one fits.
  printer.Print("<>");
  EXPECT_TRUE(printer.failed());

  // Anything else should fail too.
  printer.Print(" ");
  EXPECT_TRUE(printer.failed());
  printer.Print("blah");
  EXPECT_TRUE(printer.failed());

  // Buffer should contain the first 17 bytes written.
  EXPECT_EQ(buffer, "0123456789abcdef<");
}

TEST_F(PrinterTest, WriteFailureExact) {
  std::string buffer(16, '\xaa');
  ArrayOutputStream output(&buffer[0], buffer.size());
  Printer printer(&output, '$');

  // Print 16 bytes to fill the buffer exactly (should not fail).
  printer.Print("0123456789abcdef");
  EXPECT_FALSE(printer.failed());

  // Try to print one more byte (should fail).
  printer.Print(" ");
  EXPECT_TRUE(printer.failed());

  // Should not crash
  printer.Print("blah");
  EXPECT_TRUE(printer.failed());

  // Buffer should contain the first 16 bytes written.
  EXPECT_EQ(buffer, "0123456789abcdef");
}

TEST_F(PrinterTest, FormatInternalDirectSub) {
  {
    Printer printer(output(), '$');
    printer.FormatInternal({"arg1", "arg2"}, {}, "$1$ $2$");
  }
  EXPECT_EQ(written(), "arg1 arg2");
}

TEST_F(PrinterTest, FormatInternalSubWithSpacesLeft) {
  {
    Printer printer(output(), '$');
    printer.FormatInternal({}, {{"foo", "bar"}, {"baz", "bla"}, {"empty", ""}},
                           "$foo$$ baz$$ empty$");
  }
  EXPECT_EQ(written(), "bar bla");
}

TEST_F(PrinterTest, FormatInternalSubWithSpacesRight) {
  {
    Printer printer(output(), '$');
    printer.FormatInternal({}, {{"foo", "bar"}, {"baz", "bla"}, {"empty", ""}},
                           "$empty $$foo $$baz$");
  }
  EXPECT_EQ(written(), "bar bla");
}

TEST_F(PrinterTest, FormatInternalSubMixed) {
  {
    Printer printer(output(), '$');
    printer.FormatInternal({"arg1", "arg2"},
                           {{"foo", "bar"}, {"baz", "bla"}, {"empty", ""}},
                           "$empty $$1$ $foo $$2$ $baz$");
  }
  EXPECT_EQ(written(), "arg1 bar arg2 bla");
}

TEST_F(PrinterTest, FormatInternalIndent) {
  {
    Printer printer(output(), '$');
    printer.Indent();
    printer.FormatInternal({"arg1", "arg2"},
                           {{"foo", "bar"}, {"baz", "bla"}, {"empty", ""}},
                           "$empty $\n\n$1$ $foo $$2$\n$baz$");
    printer.Outdent();
  }
  EXPECT_EQ(written(), "\n\n  arg1 bar arg2\n  bla");
}

TEST_F(PrinterTest, FormatInternalAnnotations) {
  FakeAnnotationCollector collector;
  {
    Printer printer(output(), '$', &collector);

    printer.Indent();
    GeneratedCodeInfo::Annotation annotation;
    annotation.set_source_file("file.proto");
    annotation.add_path(33);

    printer.FormatInternal({annotation.SerializeAsString(), "arg1", "arg2"},
                           {{"foo", "bar"}, {"baz", "bla"}, {"empty", ""}},
                           "$empty $\n\n${1$$2$$}$ $3$\n$baz$");
    printer.Outdent();
  }

  EXPECT_EQ(written(), "\n\n  arg1 arg2\n  bla");
  EXPECT_THAT(collector.Get(),
              ElementsAre(Annotation(4, 8, "file.proto", ElementsAre(33))));
}

TEST_F(PrinterTest, Emit) {
  {
    Printer printer(output());
    printer.Emit(R"cc(
      class Foo {
        int x, y, z;
      };
    )cc");
    printer.Emit(R"java(
      public final class Bar {
        Bar() {}
      }
    )java");
  }

  EXPECT_EQ(written(),
            "class Foo {\n"
            "  int x, y, z;\n"
            "};\n"
            "public final class Bar {\n"
            "  Bar() {}\n"
            "}\n");
}

TEST_F(PrinterTest, EmitWithSubs) {
  {
    Printer printer(output());
    printer.Emit(
        {{"class", "Foo"}, {"f1", "x"}, {"f2", "y"}, {"f3", "z"}, {"init", 42}},
        R"cc(
          class $class$ {
            int $f1$, $f2$, $f3$ = $init$;
          };
        )cc");
  }

  EXPECT_EQ(written(),
            "class Foo {\n"
            "  int x, y, z = 42;\n"
            "};\n");
}

TEST_F(PrinterTest, EmitComments) {
  {
    Printer printer(output());
    printer.Emit(R"cc(
      // Yes.
      //~ No.
    )cc");
    printer.Emit("//~ Not a raw string.");
  }

  EXPECT_EQ(written(), "// Yes.\n//~ Not a raw string.");
}

TEST_F(PrinterTest, EmitWithVars) {
  {
    Printer printer(output());
    auto v = printer.WithVars({
        {"class", "Foo"},
        {"f1", "x"},
        {"f2", "y"},
        {"f3", "z"},
        {"init", 42},
    });
    printer.Emit(R"cc(
      class $class$ {
        int $f1$, $f2$, $f3$ = $init$;
      };
    )cc");
  }

  EXPECT_EQ(written(),
            "class Foo {\n"
            "  int x, y, z = 42;\n"
            "};\n");
}

TEST_F(PrinterTest, EmitConsumeAfter) {
  {
    Printer printer(output());
    printer.Emit(
        {
            {"class", "Foo"},
            Printer::Sub{"var", "int x;"}.WithSuffix(";"),
        },
        R"cc(
          class $class$ {
            $var$;
          };
        )cc");
  }

  EXPECT_EQ(written(),
            "class Foo {\n"
            "  int x;\n"
            "};\n");
}

TEST_F(PrinterTest, EmitWithSpacedVars) {
  {
    Printer printer(output());
    auto v = printer.WithVars({
        {"is_final", "final"},
        {"isnt_final", ""},
        {"class", "Foo"},
    });
    printer.Emit(R"java(
      public $is_final $class $class$ {
        // Stuff.
      }
    )java");
    printer.Emit(R"java(
      public $isnt_final $class $class$ {
        // Stuff.
      }
    )java");
  }

  EXPECT_EQ(written(),
            "public final class Foo {\n"
            "  // Stuff.\n"
            "}\n"
            "public class Foo {\n"
            "  // Stuff.\n"
            "}\n");
}

TEST_F(PrinterTest, EmitWithIndent) {
  {
    Printer printer(output());
    auto v = printer.WithIndent();
    printer.Emit({{"f1", "x"}, {"f2", "y"}, {"f3", "z"}}, R"cc(
      class Foo {
        int $f1$, $f2$, $f3$;
      };
    )cc");
  }

  EXPECT_EQ(written(),
            "  class Foo {\n"
            "    int x, y, z;\n"
            "  };\n");
}


TEST_F(PrinterTest, EmitSameNameAnnotation) {
  FakeAnnotationCollector collector;
  {
    Printer printer(output(), '$', &collector);
    FakeDescriptor descriptor{{"file.proto"}, {33}};
    auto v = printer.WithVars({{"class", "Foo"}});
    auto a = printer.WithAnnotations({{"class", &descriptor}});

    printer.Emit({{"f1", "x"}, {"f2", "y"}, {"f3", "z"}}, R"cc(
      class $class$ {
        int $f1$, $f2$, $f3$;
      };
    )cc");
  }

  EXPECT_EQ(written(),
            "class Foo {\n"
            "  int x, y, z;\n"
            "};\n");

  EXPECT_THAT(collector.Get(),
              ElementsAre(Annotation(6, 9, "file.proto", ElementsAre(33))));
}

TEST_F(PrinterTest, EmitSameNameAnnotationWithSemantic) {
  FakeAnnotationCollector collector;
  {
    Printer printer(output(), '$', &collector);
    FakeDescriptor descriptor{{"file.proto"}, {33}};
    auto v = printer.WithVars({{"class", "Foo"}});
    auto a = printer.WithAnnotations(
        {{"class", {&descriptor, AnnotationCollector::kSet}}});

    printer.Emit({{"f1", "x"}, {"f2", "y"}, {"f3", "z"}}, R"cc(
      class $class$ {
        int $f1$, $f2$, $f3$;
      };
    )cc");
  }

  EXPECT_EQ(written(),
            "class Foo {\n"
            "  int x, y, z;\n"
            "};\n");

  EXPECT_THAT(collector.Get(),
              ElementsAre(Annotation(6, 9, "file.proto", ElementsAre(33),
                                     AnnotationCollector::kSet)));
}

TEST_F(PrinterTest, EmitSameNameAnnotationFileNameOnly) {
  FakeAnnotationCollector collector;
  {
    Printer printer(output(), '$', &collector);
    auto v = printer.WithVars({{"class", "Foo"}});
    auto a = printer.WithAnnotations({{"class", "file.proto"}});

    printer.Emit({{"f1", "x"}, {"f2", "y"}, {"f3", "z"}}, R"cc(
      class $class$ {
        int $f1$, $f2$, $f3$;
      };
    )cc");
  }

  EXPECT_EQ(written(),
            "class Foo {\n"
            "  int x, y, z;\n"
            "};\n");

  EXPECT_THAT(collector.Get(),
              ElementsAre(Annotation(6, 9, "file.proto", IsEmpty())));
}

TEST_F(PrinterTest, EmitThreeArgWithVars) {
  FakeAnnotationCollector collector;
  {
    Printer printer(output(), '$', &collector);
    auto v = printer.WithVars({
        Printer::Sub("class", "Foo").AnnotatedAs("file.proto"),
    });

    printer.Emit({{"f1", "x"}, {"f2", "y"}, {"f3", "z"}}, R"cc(
      class $class$ {
        int $f1$, $f2$, $f3$;
      };
    )cc");
  }

  EXPECT_EQ(written(),
            "class Foo {\n"
            "  int x, y, z;\n"
            "};\n");

  EXPECT_THAT(collector.Get(),
              ElementsAre(Annotation(6, 9, "file.proto", IsEmpty())));
}

TEST_F(PrinterTest, EmitRangeAnnotation) {
  FakeAnnotationCollector collector;
  {
    Printer printer(output(), '$', &collector);
    FakeDescriptor descriptor1{{"file1.proto"}, {33}};
    FakeDescriptor descriptor2{{"file2.proto"}, {11, 22}};
    auto v = printer.WithVars({{"class", "Foo"}});
    auto a = printer.WithAnnotations({
        {"message", &descriptor1},
        {"field", &descriptor2},
    });

    printer.Emit({{"f1", "x"}, {"f2", "y"}, {"f3", "z"}}, R"cc(
      $_start$message$ class $class$ {
        $_start$field$ int $f1$, $f2$, $f3$;
        $_end$field$
      };
      $_end$message$
    )cc");
  }

  EXPECT_EQ(written(),
            "class Foo {\n"
            "  int x, y, z;\n"
            "};\n");

  EXPECT_THAT(
      collector.Get(),
      ElementsAre(Annotation(14, 27, "file2.proto", ElementsAre(11, 22)),
                  Annotation(0, 30, "file1.proto", ElementsAre(33))));
}

TEST_F(PrinterTest, EmitCallbacks) {
  {
    Printer printer(output());
    printer.Emit(
        {
            {"class", "Foo"},
            {"method", "bar"},
            {"methods",
             [&] {
               printer.Emit(R"cc(
                 int $method$() { return 42; }
               )cc");
             }},
            {"fields",
             [&] {
               printer.Emit(R"cc(
                 int $method$_;
               )cc");
             }},
        },
        R"cc(
          class $class$ {
           public:
            $methods$;

           private:
            $fields$;
          };
        )cc");
  }

  EXPECT_EQ(written(),
            "class Foo {\n"
            " public:\n"
            "  int bar() { return 42; }\n"
            "\n"
            " private:\n"
            "  int bar_;\n"
            "};\n");
}

TEST_F(PrinterTest, PreserveNewlinesThroughEmits) {
  {
    Printer printer(output());
    const std::vector<std::string> insertion_lines = {"// line 1", "// line 2"};
    printer.Emit(
        {
            {"insert_lines",
             [&] {
               for (const auto& line : insertion_lines) {
                 printer.Emit({{"line", line}}, R"cc(
                   $line$
                 )cc");
               }
             }},
        },
        R"cc(
          // one
          // two

          $insert_lines$;

          // three
          // four
        )cc");
  }
  EXPECT_EQ(written(),
            "// one\n"
            "// two\n"
            "\n"
            "// line 1\n"
            "// line 2\n"
            "\n"
            "// three\n"
            "// four\n");
}

}  // namespace
}  // namespace io
}  // namespace protobuf
}  // namespace google
