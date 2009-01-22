// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

#include <stdio.h>
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
#include <google/protobuf/text_format.h>
#include <google/protobuf/dynamic_message.h>
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
#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
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
// with a drive letter.  Example:  "C:\foo".  TODO(kenton):  Share this with
// copy in importer.cc?
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

void SetFdToTextMode(int fd) {
#ifdef _WIN32
  if (_setmode(fd, _O_TEXT) == -1) {
    // This should never happen, I think.
    GOOGLE_LOG(WARNING) << "_setmode(" << fd << ", _O_TEXT): " << strerror(errno);
  }
#endif
  // (Text and binary are the same on non-Windows platforms.)
}

void SetFdToBinaryMode(int fd) {
#ifdef _WIN32
  if (_setmode(fd, _O_BINARY) == -1) {
    // This should never happen, I think.
    GOOGLE_LOG(WARNING) << "_setmode(" << fd << ", _O_BINARY): " << strerror(errno);
  }
#endif
  // (Text and binary are the same on non-Windows platforms.)
}

}  // namespace

// A MultiFileErrorCollector that prints errors to stderr.
class CommandLineInterface::ErrorPrinter : public MultiFileErrorCollector,
                                           public io::ErrorCollector {
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

  // implements io::ErrorCollector -----------------------------------
  void AddError(int line, int column, const string& message) {
    AddError("input", line, column, message);
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
  : mode_(MODE_COMPILE),
    imports_in_descriptor_set_(false),
    disallow_services_(false),
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
  if (!ParseArguments(argc, argv)) return 1;

  // Set up the source tree.
  DiskSourceTree source_tree;
  for (int i = 0; i < proto_path_.size(); i++) {
    source_tree.MapPath(proto_path_[i].first, proto_path_[i].second);
  }

  // Map input files to virtual paths if necessary.
  if (!inputs_are_proto_path_relative_) {
    if (!MakeInputsBeProtoPathRelative(&source_tree)) {
      return 1;
    }
  }

  // Allocate the Importer.
  ErrorPrinter error_collector;
  Importer importer(&source_tree, &error_collector);

  vector<const FileDescriptor*> parsed_files;

  // Parse each file and generate output.
  for (int i = 0; i < input_files_.size(); i++) {
    // Import the file.
    const FileDescriptor* parsed_file = importer.Import(input_files_[i]);
    if (parsed_file == NULL) return 1;
    parsed_files.push_back(parsed_file);

    // Enforce --disallow_services.
    if (disallow_services_ && parsed_file->service_count() > 0) {
      cerr << parsed_file->name() << ": This file contains services, but "
              "--disallow_services was used." << endl;
      return 1;
    }

    if (mode_ == MODE_COMPILE) {
      // Generate output files.
      for (int i = 0; i < output_directives_.size(); i++) {
        if (!GenerateOutput(parsed_file, output_directives_[i])) {
          return 1;
        }
      }
    }
  }

  if (!descriptor_set_name_.empty()) {
    if (!WriteDescriptorSet(parsed_files)) {
      return 1;
    }
  }

  if (mode_ == MODE_ENCODE || mode_ == MODE_DECODE) {
    if (codec_type_.empty()) {
      // HACK:  Define an EmptyMessage type to use for decoding.
      DescriptorPool pool;
      FileDescriptorProto file;
      file.set_name("empty_message.proto");
      file.add_message_type()->set_name("EmptyMessage");
      GOOGLE_CHECK(pool.BuildFile(file) != NULL);
      codec_type_ = "EmptyMessage";
      if (!EncodeOrDecode(&pool)) {
        return 1;
      }
    } else {
      if (!EncodeOrDecode(importer.pool())) {
        return 1;
      }
    }
  }

  return 0;
}

void CommandLineInterface::Clear() {
  // Clear all members that are set by Run().  Note that we must not clear
  // members which are set by other methods before Run() is called.
  executable_name_.clear();
  proto_path_.clear();
  input_files_.clear();
  output_directives_.clear();
  codec_type_.clear();
  descriptor_set_name_.clear();

  mode_ = MODE_COMPILE;
  imports_in_descriptor_set_ = false;
  disallow_services_ = false;
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
      // Returned true => Use the next argument as the flag value.
      if (i + 1 == argc || argv[i+1][0] == '-') {
        cerr << "Missing value for flag: " << name << endl;
        if (name == "--decode") {
          cerr << "To decode an unknown message, use --decode_raw." << endl;
        }
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
  bool decoding_raw = (mode_ == MODE_DECODE) && codec_type_.empty();
  if (decoding_raw && !input_files_.empty()) {
    cerr << "When using --decode_raw, no input files should be given." << endl;
    return false;
  } else if (!decoding_raw && input_files_.empty()) {
    cerr << "Missing input file." << endl;
    return false;
  }
  if (mode_ == MODE_COMPILE && output_directives_.empty() &&
      descriptor_set_name_.empty()) {
    cerr << "Missing output directives." << endl;
    return false;
  }
  if (imports_in_descriptor_set_ && descriptor_set_name_.empty()) {
    cerr << "--include_imports only makes sense when combined with "
            "--descriptor_set_out." << endl;
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
      *name == "--include_imports" ||
      *name == "--version" ||
      *name == "--decode_raw") {
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

  } else if (name == "-o" || name == "--descriptor_set_out") {
    if (!descriptor_set_name_.empty()) {
      cerr << name << " may only be passed once." << endl;
      return false;
    }
    if (value.empty()) {
      cerr << name << " requires a non-empty value." << endl;
      return false;
    }
    if (mode_ != MODE_COMPILE) {
      cerr << "Cannot use --encode or --decode and generate descriptors at the "
              "same time." << endl;
      return false;
    }
    descriptor_set_name_ = value;

  } else if (name == "--include_imports") {
    if (imports_in_descriptor_set_) {
      cerr << name << " may only be passed once." << endl;
      return false;
    }
    imports_in_descriptor_set_ = true;

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

  } else if (name == "--encode" || name == "--decode" ||
             name == "--decode_raw") {
    if (mode_ != MODE_COMPILE) {
      cerr << "Only one of --encode and --decode can be specified." << endl;
      return false;
    }
    if (!output_directives_.empty() || !descriptor_set_name_.empty()) {
      cerr << "Cannot use " << name
           << " and generate code or descriptors at the same time." << endl;
      return false;
    }

    mode_ = (name == "--encode") ? MODE_ENCODE : MODE_DECODE;

    if (value.empty() && name != "--decode_raw") {
      cerr << "Type name for " << name << " cannot be blank." << endl;
      if (name == "--decode") {
        cerr << "To decode an unknown message, use --decode_raw." << endl;
      }
      return false;
    } else if (!value.empty() && name == "--decode_raw") {
      cerr << "--decode_raw does not take a parameter." << endl;
      return false;
    }

    codec_type_ = value;

  } else {
    // Some other flag.  Look it up in the generators list.
    GeneratorMap::const_iterator iter = generators_.find(name);
    if (iter == generators_.end()) {
      cerr << "Unknown flag: " << name << endl;
      return false;
    }

    // It's an output flag.  Add it to the output directives.
    if (mode_ != MODE_COMPILE) {
      cerr << "Cannot use --encode or --decode and generate code at the "
              "same time." << endl;
      return false;
    }

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
"Usage: " << executable_name_ << " [OPTION] PROTO_FILES\n"
"Parse PROTO_FILES and generate output based on the options given:\n"
"  -IPATH, --proto_path=PATH   Specify the directory in which to search for\n"
"                              imports.  May be specified multiple times;\n"
"                              directories will be searched in order.  If not\n"
"                              given, the current working directory is used.\n"
"  --version                   Show version info and exit.\n"
"  -h, --help                  Show this text and exit.\n"
"  --encode=MESSAGE_TYPE       Read a text-format message of the given type\n"
"                              from standard input and write it in binary\n"
"                              to standard output.  The message type must\n"
"                              be defined in PROTO_FILES or their imports.\n"
"  --decode=MESSAGE_TYPE       Read a binary message of the given type from\n"
"                              standard input and write it in text format\n"
"                              to standard output.  The message type must\n"
"                              be defined in PROTO_FILES or their imports.\n"
"  --decode_raw                Read an arbitrary protocol message from\n"
"                              standard input and write the raw tag/value\n"
"                              pairs in text format to standard output.  No\n"
"                              PROTO_FILES should be given when using this\n"
"                              flag.\n"
"  -oFILE,                     Writes a FileDescriptorSet (a protocol buffer,\n"
"    --descriptor_set_out=FILE defined in descriptor.proto) containing all of\n"
"                              the input files to FILE.\n"
"  --include_imports           When using --descriptor_set_out, also include\n"
"                              all dependencies of the input files in the\n"
"                              set, so that the set is self-contained." << endl;

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
    cerr << parsed_file->name() << ": " << output_directive.name << ": "
         << error << endl;
    return false;
  }

  // Check for write errors.
  if (output_directory.had_error()) {
    return false;
  }

  return true;
}

bool CommandLineInterface::EncodeOrDecode(const DescriptorPool* pool) {
  // Look up the type.
  const Descriptor* type = pool->FindMessageTypeByName(codec_type_);
  if (type == NULL) {
    cerr << "Type not defined: " << codec_type_ << endl;
    return false;
  }

  DynamicMessageFactory dynamic_factory(pool);
  scoped_ptr<Message> message(dynamic_factory.GetPrototype(type)->New());

  if (mode_ == MODE_ENCODE) {
    SetFdToTextMode(STDIN_FILENO);
    SetFdToBinaryMode(STDOUT_FILENO);
  } else {
    SetFdToBinaryMode(STDIN_FILENO);
    SetFdToTextMode(STDOUT_FILENO);
  }

  io::FileInputStream in(STDIN_FILENO);
  io::FileOutputStream out(STDOUT_FILENO);

  if (mode_ == MODE_ENCODE) {
    // Input is text.
    ErrorPrinter error_collector;
    TextFormat::Parser parser;
    parser.RecordErrorsTo(&error_collector);
    parser.AllowPartialMessage(true);

    if (!parser.Parse(&in, message.get())) {
      cerr << "Failed to parse input." << endl;
      return false;
    }
  } else {
    // Input is binary.
    if (!message->ParsePartialFromZeroCopyStream(&in)) {
      cerr << "Failed to parse input." << endl;
      return false;
    }
  }

  if (!message->IsInitialized()) {
    cerr << "warning:  Input message is missing required fields:  "
         << message->InitializationErrorString() << endl;
  }

  if (mode_ == MODE_ENCODE) {
    // Output is binary.
    if (!message->SerializePartialToZeroCopyStream(&out)) {
      cerr << "output: I/O error." << endl;
      return false;
    }
  } else {
    // Output is text.
    if (!TextFormat::Print(*message, &out)) {
      cerr << "output: I/O error." << endl;
      return false;
    }
  }

  return true;
}

bool CommandLineInterface::WriteDescriptorSet(
    const vector<const FileDescriptor*> parsed_files) {
  FileDescriptorSet file_set;
  set<const FileDescriptor*> already_added;
  vector<const FileDescriptor*> to_add(parsed_files);

  while (!to_add.empty()) {
    const FileDescriptor* file = to_add.back();
    to_add.pop_back();
    if (already_added.insert(file).second) {
      // This file was not already in the set.
      file->CopyTo(file_set.add_file());

      if (imports_in_descriptor_set_) {
        // Add all of this file's dependencies.
        for (int i = 0; i < file->dependency_count(); i++) {
          to_add.push_back(file->dependency(i));
        }
      }
    }
  }

  int fd;
  do {
    fd = open(descriptor_set_name_.c_str(),
              O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
  } while (fd < 0 && errno == EINTR);

  if (fd < 0) {
    perror(descriptor_set_name_.c_str());
    return false;
  }

  io::FileOutputStream out(fd);
  if (!file_set.SerializeToZeroCopyStream(&out)) {
    cerr << descriptor_set_name_ << ": " << strerror(out.GetErrno()) << endl;
    out.Close();
    return false;
  }
  if (!out.Close()) {
    cerr << descriptor_set_name_ << ": " << strerror(out.GetErrno()) << endl;
    return false;
  }

  return true;
}


}  // namespace compiler
}  // namespace protobuf
}  // namespace google
