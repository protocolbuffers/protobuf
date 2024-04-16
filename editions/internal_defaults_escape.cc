#include <iostream>
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

// DO NOT include <io.h>, instead create functions in io_win32.{h,cc} and import
// them like we do below.
using google::protobuf::io::win32::setmode;
#endif

ABSL_FLAG(std::string, encoding, "octal",
          "The encoding to use for the output.");

int main(int argc, char *argv[]) {
  absl::ParseCommandLine(argc, argv);
#ifdef _WIN32
  setmode(STDIN_FILENO, _O_BINARY);
  setmode(STDOUT_FILENO, _O_BINARY);
#endif
  google::protobuf::FeatureSetDefaults defaults;
  if (!defaults.ParseFromFileDescriptor(STDIN_FILENO)) {
    std::cerr << argv[0] << ": unable to parse edition defaults." << std::endl;
    return 1;
  }
  std::string output;
  defaults.SerializeToString(&output);
  std::string encoding = absl::GetFlag(FLAGS_encoding);
  if (encoding == "base64") {
    std::cout << absl::Base64Escape(output);
  } else if (encoding == "octal") {
    std::cout << absl::CEscape(output);
  } else {
    ABSL_LOG(FATAL) << "Unknown encoding: " << encoding;
  }
  return 0;
}
