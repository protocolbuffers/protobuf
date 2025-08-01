// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/importer.h"

#include <memory>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest.h>
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"

namespace google {
namespace protobuf {
namespace compiler {

namespace {

bool FileExists(const std::string& path) {
  return File::Exists(path);
}

class MockErrorCollector : public MultiFileErrorCollector {
 public:
  MockErrorCollector() = default;
  ~MockErrorCollector() override = default;

  std::string text_;
  std::string warning_text_;

  // implements ErrorCollector ---------------------------------------
  void RecordError(absl::string_view filename, int line, int column,
                   absl::string_view message) override {
    absl::SubstituteAndAppend(&text_, "$0:$1:$2: $3\n", filename, line, column,
                              message);
  }

  void RecordWarning(absl::string_view filename, int line, int column,
                     absl::string_view message) override {
    absl::SubstituteAndAppend(&warning_text_, "$0:$1:$2: $3\n", filename, line,
                              column, message);
  }
};

// -------------------------------------------------------------------

// A dummy implementation of SourceTree backed by a simple map.
class MockSourceTree : public SourceTree {
 public:
  MockSourceTree() = default;
  ~MockSourceTree() override = default;

  void AddFile(absl::string_view name, const char* contents) {
    files_[name] = contents;
  }

  // implements SourceTree -------------------------------------------
  io::ZeroCopyInputStream* Open(absl::string_view filename) override {
    auto it = files_.find(filename);
    if (it == files_.end()) return nullptr;
    return new io::ArrayInputStream(it->second,
                                    static_cast<int>(strlen(it->second)));
  }

  std::string GetLastErrorMessage() override { return "File not found."; }

 private:
  absl::flat_hash_map<std::string, const char*> files_;
};

// ===================================================================

class ImporterTest : public testing::Test {
 protected:
  ImporterTest() : importer_(&source_tree_, &error_collector_) {}

  void AddFile(const std::string& filename, const char* text) {
    source_tree_.AddFile(filename, text);
  }

  // Return the collected error text
  std::string warning() const { return error_collector_.warning_text_; }

  MockErrorCollector error_collector_;
  MockSourceTree source_tree_;
  Importer importer_;
};

TEST_F(ImporterTest, Import) {
  // Test normal importing.
  AddFile("foo.proto",
          "syntax = \"proto2\";\n"
          "message Foo {}\n");

  const FileDescriptor* file = importer_.Import("foo.proto");
  EXPECT_EQ("", error_collector_.text_);
  ASSERT_TRUE(file != nullptr);

  ASSERT_EQ(1, file->message_type_count());
  EXPECT_EQ("Foo", file->message_type(0)->name());

  // Importing again should return same object.
  EXPECT_EQ(file, importer_.Import("foo.proto"));
}

TEST_F(ImporterTest, ImportNested) {
  // Test that importing a file which imports another file works.
  AddFile("foo.proto",
          "syntax = \"proto2\";\n"
          "import \"bar.proto\";\n"
          "message Foo {\n"
          "  optional Bar bar = 1;\n"
          "}\n");
  AddFile("bar.proto",
          "syntax = \"proto2\";\n"
          "message Bar {}\n");

  // Note that both files are actually parsed by the first call to Import()
  // here, since foo.proto imports bar.proto.  The second call just returns
  // the same ProtoFile for bar.proto which was constructed while importing
  // foo.proto.  We test that this is the case below by checking that bar
  // is among foo's dependencies (by pointer).
  const FileDescriptor* foo = importer_.Import("foo.proto");
  const FileDescriptor* bar = importer_.Import("bar.proto");
  EXPECT_EQ("", error_collector_.text_);
  ASSERT_TRUE(foo != nullptr);
  ASSERT_TRUE(bar != nullptr);

  // Check that foo's dependency is the same object as bar.
  ASSERT_EQ(1, foo->dependency_count());
  EXPECT_EQ(bar, foo->dependency(0));

  // Check that foo properly cross-links bar.
  ASSERT_EQ(1, foo->message_type_count());
  ASSERT_EQ(1, bar->message_type_count());
  ASSERT_EQ(1, foo->message_type(0)->field_count());
  ASSERT_EQ(FieldDescriptor::TYPE_MESSAGE,
            foo->message_type(0)->field(0)->type());
  EXPECT_EQ(bar->message_type(0),
            foo->message_type(0)->field(0)->message_type());
}

TEST_F(ImporterTest, FileNotFound) {
  // Error:  Parsing a file that doesn't exist.
  EXPECT_TRUE(importer_.Import("foo.proto") == nullptr);
  EXPECT_EQ("foo.proto:-1:0: File not found.\n", error_collector_.text_);
}

TEST_F(ImporterTest, ImportNotFound) {
  // Error:  Importing a file that doesn't exist.
  AddFile("foo.proto",
          "syntax = \"proto2\";\n"
          "import \"bar.proto\";\n");

  EXPECT_TRUE(importer_.Import("foo.proto") == nullptr);
  EXPECT_EQ(
      "bar.proto:-1:0: File not found.\n"
      "foo.proto:1:0: Import \"bar.proto\" was not found or had errors.\n",
      error_collector_.text_);
}

TEST_F(ImporterTest, RecursiveImport) {
  // Error:  Recursive import.
  AddFile("recursive1.proto",
          "syntax = \"proto2\";\n"
          "\n"
          "import \"recursive2.proto\";\n");
  AddFile("recursive2.proto",
          "syntax = \"proto2\";\n"
          "import \"recursive1.proto\";\n");

  EXPECT_TRUE(importer_.Import("recursive1.proto") == nullptr);
  EXPECT_EQ(
      "recursive1.proto:2:0: File recursively imports itself: "
      "recursive1.proto "
      "-> recursive2.proto -> recursive1.proto\n"
      "recursive2.proto:1:0: Import \"recursive1.proto\" was not found "
      "or had errors.\n"
      "recursive1.proto:2:0: Import \"recursive2.proto\" was not found "
      "or had errors.\n",
      error_collector_.text_);
}

TEST_F(ImporterTest, RecursiveImportSelf) {
  // Error:  Recursive import.
  AddFile("recursive.proto",
          "syntax = \"proto2\";\n"
          "\n"
          "import \"recursive.proto\";\n");

  EXPECT_TRUE(importer_.Import("recursive.proto") == nullptr);
  EXPECT_EQ(
      "recursive.proto:2:0: File recursively imports itself: "
      "recursive.proto -> recursive.proto\n",
      error_collector_.text_);
}

TEST_F(ImporterTest, LiteRuntimeImport) {
  // Error:  Recursive import.
  AddFile("bar.proto",
          "syntax = \"proto2\";\n"
          "option optimize_for = LITE_RUNTIME;\n");
  AddFile("foo.proto",
          "syntax = \"proto2\";\n"
          "import \"bar.proto\";\n");

  EXPECT_TRUE(importer_.Import("foo.proto") == nullptr);
  EXPECT_EQ(
      "foo.proto:1:0: Files that do not use optimize_for = LITE_RUNTIME "
      "cannot import files which do use this option.  This file is not "
      "lite, but it imports \"bar.proto\" which is.\n",
      error_collector_.text_);
}


// ===================================================================

class DiskSourceTreeTest : public testing::Test {
 protected:
  void SetUp() override {
    dirnames_.push_back(
        absl::StrCat(TestTempDir(), "/test_proto2_import_path_1"));
    dirnames_.push_back(
        absl::StrCat(TestTempDir(), "/test_proto2_import_path_2"));

    for (int i = 0; i < dirnames_.size(); i++) {
      if (FileExists(dirnames_[i])) {
        File::DeleteRecursively(dirnames_[i], NULL, NULL);
      }
      ABSL_CHECK_OK(File::CreateDir(dirnames_[i], 0777));
    }
  }

  void TearDown() override {
    for (int i = 0; i < dirnames_.size(); i++) {
      if (FileExists(dirnames_[i])) {
        File::DeleteRecursively(dirnames_[i], NULL, NULL);
      }
    }
  }

  void AddFile(const std::string& filename, const char* contents) {
    ABSL_CHECK_OK(File::SetContents(filename, contents, true));
  }

  void AddSubdir(const std::string& dirname) {
    ABSL_CHECK_OK(File::CreateDir(dirname, 0777));
  }

  void ExpectFileContents(const std::string& filename,
                          const char* expected_contents) {
    std::unique_ptr<io::ZeroCopyInputStream> input(source_tree_.Open(filename));

    ASSERT_FALSE(input == nullptr);

    // Read all the data from the file.
    std::string file_contents;
    const void* data;
    int size;
    while (input->Next(&data, &size)) {
      file_contents.append(reinterpret_cast<const char*>(data), size);
    }

    EXPECT_EQ(expected_contents, file_contents);
  }

  void ExpectCannotOpenFile(const std::string& filename,
                            const std::string& error_message) {
    std::unique_ptr<io::ZeroCopyInputStream> input(source_tree_.Open(filename));
    EXPECT_TRUE(input == nullptr);
    EXPECT_EQ(error_message, source_tree_.GetLastErrorMessage());
  }

  DiskSourceTree source_tree_;

  // Paths of two on-disk directories to use during the test.
  std::vector<std::string> dirnames_;
};

TEST_F(DiskSourceTreeTest, MapRoot) {
  // Test opening a file in a directory that is mapped to the root of the
  // source tree.
  AddFile(absl::StrCat(dirnames_[0], "/foo"), "Hello World!");
  source_tree_.MapPath("", dirnames_[0]);

  ExpectFileContents("foo", "Hello World!");
  ExpectCannotOpenFile("bar", "File not found.");
}

TEST_F(DiskSourceTreeTest, MapDirectory) {
  // Test opening a file in a directory that is mapped to somewhere other
  // than the root of the source tree.

  AddFile(absl::StrCat(dirnames_[0], "/foo"), "Hello World!");
  source_tree_.MapPath("baz", dirnames_[0]);

  ExpectFileContents("baz/foo", "Hello World!");
  ExpectCannotOpenFile("baz/bar", "File not found.");
  ExpectCannotOpenFile("foo", "File not found.");
  ExpectCannotOpenFile("bar", "File not found.");

  // Non-canonical file names should not work.
  ExpectCannotOpenFile("baz//foo",
                       "Backslashes, consecutive slashes, \".\", or \"..\" are "
                       "not allowed in the virtual path");
  ExpectCannotOpenFile("baz/../baz/foo",
                       "Backslashes, consecutive slashes, \".\", or \"..\" are "
                       "not allowed in the virtual path");
  ExpectCannotOpenFile("baz/./foo",
                       "Backslashes, consecutive slashes, \".\", or \"..\" are "
                       "not allowed in the virtual path");
  ExpectCannotOpenFile("baz/foo/", "File not found.");
}

TEST_F(DiskSourceTreeTest, NoParent) {
  // Test that we cannot open files in a parent of a mapped directory.

  AddFile(absl::StrCat(dirnames_[0], "/foo"), "Hello World!");
  AddSubdir(absl::StrCat(dirnames_[0], "/bar"));
  AddFile(absl::StrCat(dirnames_[0], "/bar/baz"), "Blah.");
  source_tree_.MapPath("", absl::StrCat(dirnames_[0], "/bar"));

  ExpectFileContents("baz", "Blah.");
  ExpectCannotOpenFile("../foo",
                       "Backslashes, consecutive slashes, \".\", or \"..\" are "
                       "not allowed in the virtual path");
  ExpectCannotOpenFile("../bar/baz",
                       "Backslashes, consecutive slashes, \".\", or \"..\" are "
                       "not allowed in the virtual path");
}

TEST_F(DiskSourceTreeTest, MapFile) {
  // Test opening a file that is mapped directly into the source tree.

  AddFile(absl::StrCat(dirnames_[0], "/foo"), "Hello World!");
  source_tree_.MapPath("foo", absl::StrCat(dirnames_[0], "/foo"));

  ExpectFileContents("foo", "Hello World!");
  ExpectCannotOpenFile("bar", "File not found.");
}

TEST_F(DiskSourceTreeTest, SearchMultipleDirectories) {
  // Test mapping and searching multiple directories.

  AddFile(absl::StrCat(dirnames_[0], "/foo"), "Hello World!");
  AddFile(absl::StrCat(dirnames_[1], "/foo"), "This file should be hidden.");
  AddFile(absl::StrCat(dirnames_[1], "/bar"), "Goodbye World!");
  source_tree_.MapPath("", dirnames_[0]);
  source_tree_.MapPath("", dirnames_[1]);

  ExpectFileContents("foo", "Hello World!");
  ExpectFileContents("bar", "Goodbye World!");
  ExpectCannotOpenFile("baz", "File not found.");
}

TEST_F(DiskSourceTreeTest, OrderingTrumpsSpecificity) {
  // Test that directories are always searched in order, even when a latter
  // directory is more-specific than a former one.

  // Create the "bar" directory so we can put a file in it.
  ABSL_CHECK_OK(File::CreateDir(absl::StrCat(dirnames_[0], "/bar"),
                                0777));

  // Add files and map paths.
  AddFile(absl::StrCat(dirnames_[0], "/bar/foo"), "Hello World!");
  AddFile(absl::StrCat(dirnames_[1], "/foo"), "This file should be hidden.");
  source_tree_.MapPath("", dirnames_[0]);
  source_tree_.MapPath("bar", dirnames_[1]);

  // Check.
  ExpectFileContents("bar/foo", "Hello World!");
}

TEST_F(DiskSourceTreeTest, DiskFileToVirtualFile) {
  // Test DiskFileToVirtualFile.

  AddFile(absl::StrCat(dirnames_[0], "/foo"), "Hello World!");
  AddFile(absl::StrCat(dirnames_[1], "/foo"), "This file should be hidden.");
  source_tree_.MapPath("bar", dirnames_[0]);
  source_tree_.MapPath("bar", dirnames_[1]);

  std::string virtual_file;
  std::string shadowing_disk_file;

  EXPECT_EQ(DiskSourceTree::NO_MAPPING,
            source_tree_.DiskFileToVirtualFile("/foo", &virtual_file,
                                               &shadowing_disk_file));

  EXPECT_EQ(DiskSourceTree::SHADOWED, source_tree_.DiskFileToVirtualFile(
                                          absl::StrCat(dirnames_[1], "/foo"),
                                          &virtual_file, &shadowing_disk_file));
  EXPECT_EQ("bar/foo", virtual_file);
  EXPECT_EQ(absl::StrCat(dirnames_[0], "/foo"), shadowing_disk_file);

  EXPECT_EQ(
      DiskSourceTree::CANNOT_OPEN,
      source_tree_.DiskFileToVirtualFile(absl::StrCat(dirnames_[1], "/baz"),
                                         &virtual_file, &shadowing_disk_file));
  EXPECT_EQ("bar/baz", virtual_file);

  EXPECT_EQ(DiskSourceTree::SUCCESS, source_tree_.DiskFileToVirtualFile(
                                         absl::StrCat(dirnames_[0], "/foo"),
                                         &virtual_file, &shadowing_disk_file));
  EXPECT_EQ("bar/foo", virtual_file);
}

TEST_F(DiskSourceTreeTest, DiskFileToVirtualFileCanonicalization) {
  // Test handling of "..", ".", etc. in DiskFileToVirtualFile().

  source_tree_.MapPath("dir1", "..");
  source_tree_.MapPath("dir2", "../../foo");
  source_tree_.MapPath("dir3", "./foo/bar/.");
  source_tree_.MapPath("dir4", ".");
  source_tree_.MapPath("", "/qux");
  source_tree_.MapPath("dir5", "/quux/");

  std::string virtual_file;
  std::string shadowing_disk_file;

  // "../.." should not be considered to be under "..".
  EXPECT_EQ(DiskSourceTree::NO_MAPPING,
            source_tree_.DiskFileToVirtualFile("../../baz", &virtual_file,
                                               &shadowing_disk_file));

  // "/foo" is not mapped (it should not be misinterpreted as being under ".").
  EXPECT_EQ(DiskSourceTree::NO_MAPPING,
            source_tree_.DiskFileToVirtualFile("/foo", &virtual_file,
                                               &shadowing_disk_file));

#ifdef WIN32
  // "C:\foo" is not mapped (it should not be misinterpreted as being under
  // ".").
  EXPECT_EQ(DiskSourceTree::NO_MAPPING,
            source_tree_.DiskFileToVirtualFile("C:\\foo", &virtual_file,
                                               &shadowing_disk_file));
#endif  // WIN32

  // But "../baz" should be.
  EXPECT_EQ(DiskSourceTree::CANNOT_OPEN,
            source_tree_.DiskFileToVirtualFile("../baz", &virtual_file,
                                               &shadowing_disk_file));
  EXPECT_EQ("dir1/baz", virtual_file);

  // "../../foo/baz" is under "../../foo".
  EXPECT_EQ(DiskSourceTree::CANNOT_OPEN,
            source_tree_.DiskFileToVirtualFile("../../foo/baz", &virtual_file,
                                               &shadowing_disk_file));
  EXPECT_EQ("dir2/baz", virtual_file);

  // "foo/./bar/baz" is under "./foo/bar/.".
  EXPECT_EQ(DiskSourceTree::CANNOT_OPEN,
            source_tree_.DiskFileToVirtualFile("foo/bar/baz", &virtual_file,
                                               &shadowing_disk_file));
  EXPECT_EQ("dir3/baz", virtual_file);

  // "bar" is under ".".
  EXPECT_EQ(DiskSourceTree::CANNOT_OPEN,
            source_tree_.DiskFileToVirtualFile("bar", &virtual_file,
                                               &shadowing_disk_file));
  EXPECT_EQ("dir4/bar", virtual_file);

  // "/qux/baz" is under "/qux".
  EXPECT_EQ(DiskSourceTree::CANNOT_OPEN,
            source_tree_.DiskFileToVirtualFile("/qux/baz", &virtual_file,
                                               &shadowing_disk_file));
  EXPECT_EQ("baz", virtual_file);

  // "/quux/bar" is under "/quux".
  EXPECT_EQ(DiskSourceTree::CANNOT_OPEN,
            source_tree_.DiskFileToVirtualFile("/quux/bar", &virtual_file,
                                               &shadowing_disk_file));
  EXPECT_EQ("dir5/bar", virtual_file);
}

TEST_F(DiskSourceTreeTest, VirtualFileToDiskFile) {
  // Test VirtualFileToDiskFile.

  AddFile(absl::StrCat(dirnames_[0], "/foo"), "Hello World!");
  AddFile(absl::StrCat(dirnames_[1], "/foo"), "This file should be hidden.");
  AddFile(absl::StrCat(dirnames_[1], "/quux"),
          "This file should not be hidden.");
  source_tree_.MapPath("bar", dirnames_[0]);
  source_tree_.MapPath("bar", dirnames_[1]);

  // Existent files, shadowed and non-shadowed case.
  std::string disk_file;
  EXPECT_TRUE(source_tree_.VirtualFileToDiskFile("bar/foo", &disk_file));
  EXPECT_EQ(absl::StrCat(dirnames_[0], "/foo"), disk_file);
  EXPECT_TRUE(source_tree_.VirtualFileToDiskFile("bar/quux", &disk_file));
  EXPECT_EQ(absl::StrCat(dirnames_[1], "/quux"), disk_file);

  // Nonexistent file in existent directory and vice versa.
  std::string not_touched = "not touched";
  EXPECT_FALSE(source_tree_.VirtualFileToDiskFile("bar/baz", &not_touched));
  EXPECT_EQ("not touched", not_touched);
  EXPECT_FALSE(source_tree_.VirtualFileToDiskFile("baz/foo", &not_touched));
  EXPECT_EQ("not touched", not_touched);

  // Accept NULL as output parameter.
  EXPECT_TRUE(source_tree_.VirtualFileToDiskFile("bar/foo", nullptr));
  EXPECT_FALSE(source_tree_.VirtualFileToDiskFile("baz/foo", nullptr));
}

class SourceTreeDescriptorDatabaseTest : public testing::Test {
 protected:
  void SetUp() override {
    source_tree_.AddFile("foo.proto", R"(
      edition = "2023";
      package proto2_unittest;
      message Foo {
        extensions 1 to 10;
        extensions 20 to max;
      }
    )");
  }

  MockSourceTree source_tree_;
};

TEST_F(SourceTreeDescriptorDatabaseTest, ExtensionDeclarations) {
  source_tree_.AddFile("extension_declarations1.txtpb", R"pb(
    declaration {
      number: 1
      full_name: ".proto2_unittest.foo_extension1"
      type: ".proto2_unittest.Message1"
    }
  )pb");
  source_tree_.AddFile("extension_declarations2.txtpb", R"pb(
    declaration {
      number: 30
      full_name: ".proto2_unittest.foo_extension2"
      type: ".proto2_unittest.Message2"
    }
  )pb");
  SourceTreeDescriptorDatabase database(&source_tree_);
  database.AddExtensionDeclarationsFile("foo.proto", "Foo",
                                        "extension_declarations1.txtpb");
  database.AddExtensionDeclarationsFile("foo.proto", "Foo",
                                        "extension_declarations2.txtpb");

  FileDescriptorProto file_proto;
  ASSERT_TRUE(database.FindFileByName("foo.proto", &file_proto));
  ASSERT_EQ(file_proto.message_type_size(), 1);
  const DescriptorProto& descriptor = file_proto.message_type(0);
  ASSERT_EQ(descriptor.extension_range_size(), 2);

  // First extension range
  {
    const DescriptorProto::ExtensionRange& range =
        descriptor.extension_range(0);
    ASSERT_EQ(range.options().declaration_size(), 1);
    const ExtensionRangeOptions::Declaration& declaration =
        range.options().declaration(0);
    EXPECT_EQ(declaration.number(), 1);
    EXPECT_EQ(declaration.full_name(), ".proto2_unittest.foo_extension1");
    EXPECT_EQ(declaration.type(), ".proto2_unittest.Message1");
  }

  // Second extension range
  {
    const DescriptorProto::ExtensionRange& range =
        descriptor.extension_range(1);
    ASSERT_EQ(range.options().declaration_size(), 1);
    const ExtensionRangeOptions::Declaration& declaration =
        range.options().declaration(0);
    EXPECT_EQ(declaration.number(), 30);
    EXPECT_EQ(declaration.full_name(), ".proto2_unittest.foo_extension2");
    EXPECT_EQ(declaration.type(), ".proto2_unittest.Message2");
  }
}

TEST_F(SourceTreeDescriptorDatabaseTest, ExtensionDeclarationsMissing) {
  SourceTreeDescriptorDatabase database(&source_tree_);
  database.AddExtensionDeclarationsFile("foo.proto", "Foo",
                                        "extension_declarations.txtpb");

  // The descriptor database should read the .proto file successfully even if
  // the .txtpb file was not present.
  FileDescriptorProto file_proto;
  EXPECT_TRUE(database.FindFileByName("foo.proto", &file_proto));
  ASSERT_EQ(file_proto.message_type_size(), 1);
  const DescriptorProto& descriptor = file_proto.message_type(0);
  ASSERT_EQ(descriptor.extension_range_size(), 2);
  EXPECT_EQ(descriptor.extension_range(0).options().declaration_size(), 0);
  EXPECT_EQ(descriptor.extension_range(1).options().declaration_size(), 0);
}

TEST_F(SourceTreeDescriptorDatabaseTest, ExtensionDeclarationsSyntaxError) {
  source_tree_.AddFile("extension_declarations.txtpb", R"pb(
    invalid {}
  )pb");
  SourceTreeDescriptorDatabase database(&source_tree_);
  MockErrorCollector error_collector;
  database.RecordErrorsTo(&error_collector);
  database.AddExtensionDeclarationsFile("foo.proto", "Foo",
                                        "extension_declarations.txtpb");

  FileDescriptorProto file_proto;
  EXPECT_FALSE(database.FindFileByName("foo.proto", &file_proto));
  EXPECT_EQ(
      error_collector.text_,
      "extension_declarations.txtpb:1:12: Message type "
      "\"google.protobuf.ExtensionRangeOptions\" has no field named \"invalid\".\n");
}

TEST_F(SourceTreeDescriptorDatabaseTest, ExtensionDeclarationsMessageNotFound) {
  source_tree_.AddFile("extension_declarations.txtpb", R"pb(
    declaration {
      number: 1
      full_name: ".proto2_unittest.foo_extension"
      type: ".proto2_unittest.Foo"
    }
  )pb");
  SourceTreeDescriptorDatabase database(&source_tree_);
  MockErrorCollector error_collector;
  database.RecordErrorsTo(&error_collector);
  database.AddExtensionDeclarationsFile("foo.proto", "Bar",
                                        "extension_declarations.txtpb");

  FileDescriptorProto file_proto;
  EXPECT_FALSE(database.FindFileByName("foo.proto", &file_proto));
  EXPECT_EQ(error_collector.text_,
            "extension_declarations.txtpb:1:1: Message Bar not found in "
            "foo.proto.\n");
}

TEST_F(SourceTreeDescriptorDatabaseTest, ExtensionDeclarationsRangeNotFound) {
  source_tree_.AddFile("extension_declarations.txtpb", R"pb(
    declaration {
      number: 11
      full_name: ".proto2_unittest.foo_extension"
      type: ".proto2_unittest.Foo"
    }
  )pb");
  SourceTreeDescriptorDatabase database(&source_tree_);
  MockErrorCollector error_collector;
  database.RecordErrorsTo(&error_collector);
  database.AddExtensionDeclarationsFile("foo.proto", "Foo",
                                        "extension_declarations.txtpb");

  FileDescriptorProto file_proto;
  EXPECT_FALSE(database.FindFileByName("foo.proto", &file_proto));
  EXPECT_EQ(error_collector.text_,
            "extension_declarations.txtpb:1:1: No extension range found for "
            "number 11 in message Foo.\n");
}

namespace {

class FakeDescriptorPoolErrorCollector : public DescriptorPool::ErrorCollector {
 public:
  void RecordError(absl::string_view filename, absl::string_view element_name,
                   const Message* descriptor, ErrorLocation location,
                   absl::string_view message) override {
    absl::SubstituteAndAppend(&text_, "$0: $1: $2\n", filename, element_name,
                              message);
  }

  absl::string_view text() { return text_; }

 private:
  std::string text_;
};

}  // namespace

TEST_F(SourceTreeDescriptorDatabaseTest,
       ExtensionDeclarationsCrossingRangeBoundary) {
  // All extensions in the .txtpb file need to be part of the same extension
  // range. SourceTreeDescriptorDatabase does not enforce this, but
  // DescriptorPool does.
  source_tree_.AddFile("extension_declarations.txtpb", R"pb(
    declaration {
      number: 10
      full_name: ".proto2_unittest.foo_extension1"
      type: ".proto2_unittest.Foo1"
    }
    declaration {
      number: 20
      full_name: ".proto2_unittest.foo_extension2"
      type: ".proto2_unittest.Foo2"
    }
  )pb");
  SourceTreeDescriptorDatabase database(&source_tree_);
  database.AddExtensionDeclarationsFile("foo.proto", "Foo",
                                        "extension_declarations.txtpb");

  FileDescriptorProto file_proto;
  EXPECT_TRUE(database.FindFileByName("foo.proto", &file_proto));

  DescriptorPool pool;
  FakeDescriptorPoolErrorCollector error_collector;
  EXPECT_FALSE(pool.BuildFileCollectingErrors(file_proto, &error_collector));
  EXPECT_EQ(error_collector.text(),
            "foo.proto: proto2_unittest.Foo: Extension declaration number 20 "
            "is not in the extension range.\n");
}

}  // namespace

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
