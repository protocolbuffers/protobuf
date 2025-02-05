#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <fcntl.h>
#else
#include <unistd.h>
#endif

#include "google/protobuf/descriptor.pb.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/absl_log.h"
#include "absl/strings/escaping.h"

#if defined(_WIN32)
#include "google/protobuf/io/io_win32.h"

// DO NOT include <io.h>, instead create functions in io_win32.{h,cc} and
// import them like we do below.
using google::protobuf::io::win32::setmode;
#endif

ABSL_FLAG(std::string, encoding, "octal",
          "The encoding to use for the output.");
ABSL_FLAG(std::string, defaults_path, "defaults_path",
          "The path to the compile_edition_defaults file to embed.");
ABSL_FLAG(std::string, template_path, "template_path",
          "The template to use for generating the output file.");
ABSL_FLAG(std::string, output_path, "output_path",
          "The path to the the output file.");
ABSL_FLAG(
    std::string, placeholder, "placeholder",
    "The placeholder to replace with a serialized string in the template.");

int defaults_escape(const std::string& defaults_path,
                    const std::string& encoding, std::string& out_content) {
  std::ifstream defaults_file(defaults_path);
  if (!defaults_file.is_open()) {
    std::cerr << "Could not open defaults file " << defaults_path << std::endl;
    return 1;
  }

  google::protobuf::FeatureSetDefaults defaults;
  if (!defaults.ParseFromIstream(&defaults_file)) {
    std::cerr << "Unable to parse edition defaults " << defaults_path
              << std::endl;
    defaults_file.close();
    return 1;
  }

  defaults_file.close();

  std::string content = {};
  defaults.SerializeToString(&content);
  if (encoding == "base64") {
    content = absl::Base64Escape(content);
  } else if (encoding == "octal") {
    content = absl::CEscape(content);
  } else {
    ABSL_LOG(FATAL) << "Unknown encoding: " << encoding;
    return 1;
  }

  out_content = content;
  return 0;
}

int read_to_string(const std::string& path, std::string& out_content) {
  std::ifstream input_file(path);
  if (!input_file.is_open()) {
    std::cerr << "Could not open file " << path << std::endl;
    return 1;
  }

  std::ostringstream buffer;
  buffer << input_file.rdbuf();
  out_content = buffer.str();
  input_file.close();

  return 0;
}

int replace_placeholder(std::string& out_content,
                        const std::string& placeholder,
                        const std::string& replacement) {
  size_t pos = 0;

  while ((pos = out_content.find(placeholder, pos)) != std::string::npos) {
    out_content.replace(pos, placeholder.length(), replacement);
    pos += replacement.length();
  }

  return 0;
}

int write(const std::string& path, const std::string& content) {
  std::ofstream output_file(path);
  if (!output_file.is_open()) {
    std::cerr << "Could not write to file " << path << std::endl;
    return 1;
  }

  output_file << content;
  output_file.close();

  return 0;
}

int main(int argc, char* argv[]) {
  absl::ParseCommandLine(argc, argv);
#ifdef _WIN32
  setmode(STDOUT_FILENO, _O_BINARY);
#endif
  std::string encoding = absl::GetFlag(FLAGS_encoding);
  std::string defaults_path = absl::GetFlag(FLAGS_defaults_path);
  std::string template_path = absl::GetFlag(FLAGS_template_path);
  std::string output_path = absl::GetFlag(FLAGS_output_path);
  std::string placeholder = absl::GetFlag(FLAGS_placeholder);

  std::string replacement = {};
  if (defaults_escape(defaults_path, encoding, replacement)) {
    return 1;
  }

  std::string content = {};
  if (read_to_string(template_path, content)) {
    return 1;
  }

  if (replace_placeholder(content, placeholder, replacement)) {
    return 1;
  }

  if (write(output_path, content)) {
    return 1;
  }

  return 0;
}
