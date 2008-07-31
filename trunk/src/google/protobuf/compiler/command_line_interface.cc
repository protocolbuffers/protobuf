// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef _MSC_VER
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <errno.h>
#include <iostream>
#include <ctype.h>

#include <google/protobuf/compiler/command_line_interface.h>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/strutil.h>


namespace google {
namespace protobuf {
namespace compiler {

#if defined(_WIN32)
#define mkdir(name, mode) mkdir(name)
#ifndef W_OK
#define W_OK 02  // not defined by MSVC for whatever reason
#endif
#ifndef F_OK
#define F_OK 00  // not defined by MSVC for whatever reason
#endif
#endif

#ifndef O_BINARY
#ifdef _O_BINARY
#define O_BINARY _O_BINARY
#else
#define O_BINARY 0     // If this isn't defined, the platform doesn't need it.
#endif
#endif

namespace {
#if defined(_WIN32) && !defined(__CYGWIN__)
static const char* kPathSeparator = ";";
#else
static const char* kPathSeparator = ":";
#endif

// Returns true if the text looks like a Windows-style absolute path, starting
// with a drive letter.  Example:  "C:\foo".
static bool IsWindowsAbsolutePath(const string& text) {
#if defined(_WIN32) || defined(__CYGWIN__)
  return text.size() >= 3 && text[1] == ':' &&
         isalpha(text[0]) &&
         (text[2] == '/' || text[2] == '\\') &&
         text.find_last_of(':') == 1;
#else
  return false;
#endif
}

}  // namespace

// A MultiFileErrorCollector that prints errors to stderr.
class CommandLineInterface::ErrorPrinter : public MultiFileErrorCollector {
 public:
  ErrorPrinter() {}
  ~ErrorPrinter() {}

  // implements MultiFileErrorCollector ------------------------------
  void AddError(const string& filename, int line, int column,
                const string& message) {
    // Users typically expect 1-based line/column numbers, so we add 1
    // to each here.
    cerr << filename;
    if (line != -1) {
      cerr << ":" << (line + 1) << ":" << (column + 1);
    }
    cerr << ": " << message << endl;
  }
};

// -------------------------------------------------------------------

// An OutputDirectory implementation that writes to disk.
class CommandLineInterface::DiskOutputDirectory : public OutputDirectory {
 public:
  DiskOutputDirectory(const string& root);
  ~DiskOutputDirectory();

  bool VerifyExistence();

  inline bool had_error() { return had_error_; }
  inline void set_had_error(bool value) { had_error_ = value; }

  // implements OutputDirectory --------------------------------------
  io::ZeroCopyOutputStream* Open(const string& filename);

 private:
  string root_;
  bool had_error_;
};

// A FileOutputStream that checks for errors in the destructor and reports
// them.  We extend FileOutputStream via wrapping rather than inheritance
// for two reasons:
// 1) Implementation inheritance is evil.
// 2) We need to close the file descriptor *after* the FileOutputStream's
//    destructor is run to make sure it flushes the file contents.
class CommandLineInterface::ErrorReportingFileOutput
    : public io::ZeroCopyOutputStream {
 public:
  ErrorReportingFileOutput(int file_descriptor,
                           const string& filename,
                           DiskOutputDirectory* directory);
  ~ErrorReportingFileOutput();

  // implements ZeroCopyOutputStream ---------------------------------
  bool Next(void** data, int* size) { return file_stream_->Next(data, size); }
  void BackUp(int count)            {        file_stream_->BackUp(count);    }
  int64 ByteCount() const           { return file_stream_->ByteCount();      }

 private:
  scoped_ptr<io::FileOutputStream> file_stream_;
  int file_descriptor_;
  string filename_;
  DiskOutputDirectory* directory_;
};

// -------------------------------------------------------------------

CommandLineInterface::DiskOutputDirectory::DiskOutputDirectory(
    const string& root)
  : root_(root), had_error_(false) {
  // Add a '/' to the end if it doesn't already have one.  But don't add a
  // '/' to an empty string since this probably means the current directory.
  if (!root_.empty() && root[root_.size() - 1] != '/') {
    root_ += '/';
  }
}

CommandLineInterface::DiskOutputDirectory::~DiskOutputDirectory() {
}

bool CommandLineInterface::DiskOutputDirectory::VerifyExistence() {
  if (!root_.empty()) {
    // Make sure the directory exists.  If it isn't a directory, this will fail
    // because we added a '/' to the end of the name in the constructor.
    if (access(root_.c_str(), W_OK) == -1) {
      cerr << root_ << ": " << strerror(errno) << endl;
      return false;
    }
  }

  return true;
}

io::ZeroCopyOutputStream* CommandLineInterface::DiskOutputDirectory::Open(
    const string& filename) {
  // Recursively create parent directories to the output file.
  vector<string> parts;
  SplitStringUsing(filename, "/", &parts);
  string path_so_far = root_;
  for (int i = 0; i < parts.size() - 1; i++) {
    path_so_far += parts[i];
    if (mkdir(path_so_far.c_str(), 0777) != 0) {
      if (errno != EEXIST) {
        cerr << filename << ": while trying to create directory "
             << path_so_far << ": " << strerror(errno) << endl;
        had_error_ = true;
        // Return a dummy stream.
        return new io::ArrayOutputStream(NULL, 0);
      }
    }
    path_so_far += '/';
  }

  // Create the output file.
  int file_descriptor;
  do {
    file_descriptor =
      open((root_ + filename).c_str(),
           O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
  } while (file_descriptor < 0 && errno == EINTR);

  if (file_descriptor < 0) {
    // Failed to open.
    cerr << filename << ": " << strerror(errno) << endl;
    had_error_ = true;
    // Return a dummy stream.
    return new io::ArrayOutputStream(NULL, 0);
  }

  return new ErrorReportingFileOutput(file_descriptor, filename, this);
}

CommandLineInterface::ErrorReportingFileOutput::ErrorReportingFileOutput(
    int file_descriptor,
    const string& filename,
    DiskOutputDirectory* directory)
  : file_stream_(new io::FileOutputStream(file_descriptor)),
    file_descriptor_(file_descriptor),
    filename_(filename),
    directory_(directory) {}

CommandLineInterface::ErrorReportingFileOutput::~ErrorReportingFileOutput() {
  // Check if we had any errors while writing.
  if (file_stream_->GetErrno() != 0) {
    cerr << filename_ << ": " << strerror(file_stream_->GetErrno()) << endl;
    directory_->set_had_error(true);
  }

  // Close the file stream.
  if (!file_stream_->Close()) {
    cerr << filename_ << ": " << strerror(file_stream_->GetErrno()) << endl;
    directory_->set_had_error(true);
  }
}

// ===================================================================

CommandLineInterface::CommandLineInterface()
  : disallow_services_(false),
    inputs_are_proto_path_relative_(false) {}
CommandLineInterface::~CommandLineInterface() {}

void CommandLineInterface::RegisterGenerator(const string& flag_name,
                                             CodeGenerator* generator,
                                             const string& help_text) {
  GeneratorInfo info;
  info.generator = generator;
  info.help_text = help_text;
  generators_[flag_name] = info;
}

int CommandLineInterface::Run(int argc, const char* const argv[]) {
  Clear();
  if (!ParseArguments(argc, argv)) return -1;

  // Set up the source tree.
  DiskSourceTree source_tree;
  for (int i = 0; i < proto_path_.size(); i++) {
    source_tree.MapPath(proto_path_[i].first, proto_path_[i].second);
  }

  // Map input files to virtual paths if necessary.
  if (!inputs_are_proto_path_relative_) {
    if (!MakeInputsBeProtoPathRelative(&source_tree)) {
      return -1;
    }
  }

  // Allocate the Importer.
  ErrorPrinter error_collector;
  DescriptorPool pool;
  Importer importer(&source_tree, &error_collector);

  // Parse each file and generate output.
  for (int i = 0; i < input_files_.size(); i++) {
    // Import the file.
    const FileDescriptor* parsed_file = importer.Import(input_files_[i]);
    if (parsed_file == NULL) return -1;

    // Enforce --disallow_services.
    if (disallow_services_ && parsed_file->service_count() > 0) {
      cerr << parsed_file->name() << ": This file contains services, but "
              "--disallow_services was used." << endl;
      return -1;
    }

    // Generate output files.
    for (int i = 0; i < output_directives_.size(); i++) {
      if (!GenerateOutput(parsed_file, output_directives_[i])) {
        return -1;
      }
    }
  }

  return 0;
}

void CommandLineInterface::Clear() {
  proto_path_.clear();
  input_files_.clear();
  output_directives_.clear();
}

bool CommandLineInterface::MakeInputsBeProtoPathRelative(
    DiskSourceTree* source_tree) {
  for (int i = 0; i < input_files_.size(); i++) {
    string virtual_file, shadowing_disk_file;
    switch (source_tree->DiskFileToVirtualFile(
        input_files_[i], &virtual_file, &shadowing_disk_file)) {
      case DiskSourceTree::SUCCESS:
        input_files_[i] = virtual_file;
        break;
      case DiskSourceTree::SHADOWED:
        cerr << input_files_[i] << ": Input is shadowed in the --proto_path "
                "by \"" << shadowing_disk_file << "\".  Either use the latter "
                "file as your input or reorder the --proto_path so that the "
                "former file's location comes first." << endl;
        return false;
      case DiskSourceTree::CANNOT_OPEN:
        cerr << input_files_[i] << ": " << strerror(errno) << endl;
        return false;
      case DiskSourceTree::NO_MAPPING:
        // First check if the file exists at all.
        if (access(input_files_[i].c_str(), F_OK) < 0) {
          // File does not even exist.
          cerr << input_files_[i] << ": " << strerror(ENOENT) << endl;
        } else {
          cerr << input_files_[i] << ": File does not reside within any path "
                  "specified using --proto_path (or -I).  You must specify a "
                  "--proto_path which encompasses this file." << endl;
        }
        return false;
    }
  }

  return true;
}

bool CommandLineInterface::ParseArguments(int argc, const char* const argv[]) {
  executable_name_ = argv[0];

  // Iterate through all arguments and parse them.
  for (int i = 1; i < argc; i++) {
    string name, value;

    if (ParseArgument(argv[i], &name, &value)) {
      // Retured true => Use the next argument as the flag value.
      if (i + 1 == argc || argv[i+1][0] == '-') {
        cerr << "Missing value for flag: " << name << endl;
        return false;
      } else {
        ++i;
        value = argv[i];
      }
    }

    if (!InterpretArgument(name, value)) return false;
  }

  // If no --proto_path was given, use the current working directory.
  if (proto_path_.empty()) {
    proto_path_.push_back(make_pair("", "."));
  }

  // Check some errror cases.
  if (input_files_.empty()) {
    cerr << "Missing input file." << endl;
    return false;
  }
  if (output_directives_.empty()) {
    cerr << "Missing output directives." << endl;
    return false;
  }

  return true;
}

bool CommandLineInterface::ParseArgument(const char* arg,
                                         string* name, string* value) {
  bool parsed_value = false;

  if (arg[0] != '-') {
    // Not a flag.
    name->clear();
    parsed_value = true;
    *value = arg;
  } else if (arg[1] == '-') {
    // Two dashes:  Multi-character name, with '=' separating name and
    //   value.
    const char* equals_pos = strchr(arg, '=');
    if (equals_pos != NULL) {
      *name = string(arg, equals_pos - arg);
      *value = equals_pos + 1;
      parsed_value = true;
    } else {
      *name = arg;
    }
  } else {
    // One dash:  One-character name, all subsequent characters are the
    //   value.
    if (arg[1] == '\0') {
      // arg is just "-".  We treat this as an input file, except that at
      // present this will just lead to a "file not found" error.
      name->clear();
      *value = arg;
      parsed_value = true;
    } else {
      *name = string(arg, 2);
      *value = arg + 2;
      parsed_value = !value->empty();
    }
  }

  // Need to return true iff the next arg should be used as the value for this
  // one, false otherwise.

  if (parsed_value) {
    // We already parsed a value for this flag.
    return false;
  }

  if (*name == "-h" || *name == "--help" ||
      *name == "--disallow_services" ||
      *name == "--version") {
    // HACK:  These are the only flags that don't take a value.
    //   They probably should not be hard-coded like this but for now it's
    //   not worth doing better.
    return false;
  }

  // Next argument is the flag value.
  return true;
}

bool CommandLineInterface::InterpretArgument(const string& name,
                                             const string& value) {
  if (name.empty()) {
    // Not a flag.  Just a filename.
    if (value.empty()) {
      cerr << "You seem to have passed an empty string as one of the "
              "arguments to " << executable_name_ << ".  This is actually "
              "sort of hard to do.  Congrats.  Unfortunately it is not valid "
              "input so the program is going to die now." << endl;
      return false;
    }

    input_files_.push_back(value);

  } else if (name == "-I" || name == "--proto_path") {
    // Java's -classpath (and some other languages) delimits path components
    // with colons.  Let's accept that syntax too just to make things more
    // intuitive.
    vector<string> parts;
    SplitStringUsing(value, kPathSeparator, &parts);

    for (int i = 0; i < parts.size(); i++) {
      string virtual_path;
      string disk_path;

      int equals_pos = parts[i].find_first_of('=');
      if (equals_pos == string::npos) {
        virtual_path = "";
        disk_path = parts[i];
      } else {
        virtual_path = parts[i].substr(0, equals_pos);
        disk_path = parts[i].substr(equals_pos + 1);
      }

      if (disk_path.empty()) {
        cerr << "--proto_path passed empty directory name.  (Use \".\" for "
                "current directory.)" << endl;
        return false;
      }

      // Make sure disk path exists, warn otherwise.
      if (access(disk_path.c_str(), F_OK) < 0) {
        cerr << disk_path << ": warning: directory does not exist." << endl;
      }

      proto_path_.push_back(make_pair(virtual_path, disk_path));
    }

  } else if (name == "-h" || name == "--help") {
    PrintHelpText();
    return false;  // Exit without running compiler.

  } else if (name == "--version") {
    if (!version_info_.empty()) {
      cout << version_info_ << endl;
    }
    cout << "libprotoc "
         << protobuf::internal::VersionString(GOOGLE_PROTOBUF_VERSION)
         << endl;
    return false;  // Exit without running compiler.

  } else if (name == "--disallow_services") {
    disallow_services_ = true;

  } else {
    // Some other flag.  Look it up in the generators list.
    GeneratorMap::const_iterator iter = generators_.find(name);
    if (iter == generators_.end()) {
      cerr << "Unknown flag: " << name << endl;
      return false;
    }

    // It's an output flag.  Add it to the output directives.
    OutputDirective directive;
    directive.name = name;
    directive.generator = iter->second.generator;

    // Split value at ':' to separate the generator parameter from the
    // filename.  However, avoid doing this if the colon is part of a valid
    // Windows-style absolute path.
    string::size_type colon_pos = value.find_first_of(':');
    if (colon_pos == string::npos || IsWindowsAbsolutePath(value)) {
      directive.output_location = value;
    } else {
      directive.parameter = value.substr(0, colon_pos);
      directive.output_location = value.substr(colon_pos + 1);
    }

    output_directives_.push_back(directive);
  }

  return true;
}

void CommandLineInterface::PrintHelpText() {
  // Sorry for indentation here; line wrapping would be uglier.
  cerr <<
"Usage: " << executable_name_ << " [OPTION] PROTO_FILE\n"
"Parse PROTO_FILE and generate output based on the options given:\n"
"  -IPATH, --proto_path=PATH   Specify the directory in which to search for\n"
"                              imports.  May be specified multiple times;\n"
"                              directories will be searched in order.  If not\n"
"                              given, the current working directory is used.\n"
"  --version                   Show version info and exit.\n"
"  -h, --help                  Show this text and exit." << endl;

  for (GeneratorMap::iterator iter = generators_.begin();
       iter != generators_.end(); ++iter) {
    // FIXME(kenton):  If the text is long enough it will wrap, which is ugly,
    //   but fixing this nicely (e.g. splitting on spaces) is probably more
    //   trouble than it's worth.
    cerr << "  " << iter->first << "=OUT_DIR "
         << string(19 - iter->first.size(), ' ')  // Spaces for alignment.
         << iter->second.help_text << endl;
  }
}

bool CommandLineInterface::GenerateOutput(
    const FileDescriptor* parsed_file,
    const OutputDirective& output_directive) {
  // Create the output directory.
  DiskOutputDirectory output_directory(output_directive.output_location);
  if (!output_directory.VerifyExistence()) {
    return false;
  }

  // Opened successfully.  Write it.

  // Call the generator.
  string error;
  if (!output_directive.generator->Generate(
      parsed_file, output_directive.parameter, &output_directory, &error)) {
    // Generator returned an error.
    cerr << output_directive.name << ": " << error << endl;
    return false;
  }

  // Check for write errors.
  if (output_directory.had_error()) {
    return false;
  }

  return true;
}


}  // namespace compiler
}  // namespace protobuf
}  // namespace google
