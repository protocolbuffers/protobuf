// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "absl/log/initialize.h"
#include "google/protobuf/compiler/command_line_interface.h"
#include "google/protobuf/compiler/cpp/generator.h"
#include "google/protobuf/compiler/csharp/csharp_generator.h"
#include "google/protobuf/compiler/java/generator.h"
#include "google/protobuf/compiler/kotlin/generator.h"
#include "google/protobuf/compiler/objectivec/generator.h"
#include "google/protobuf/compiler/php/php_generator.h"
#include "google/protobuf/compiler/python/generator.h"
#include "google/protobuf/compiler/python/pyi_generator.h"
#include "google/protobuf/compiler/ruby/ruby_generator.h"
#include "google/protobuf/compiler/rust/generator.h"

#ifdef DISABLE_PROTOC_CONFIG
#include "google/protobuf/compiler/allowlists/allowlist.h"
#endif  // DISABLE_PROTOC_CONFIG

// Must be included last.
#include "google/protobuf/port_def.inc"

#ifdef _MSC_VER
#include <windows.h>
#include <shellapi.h>
#endif

namespace google {
namespace protobuf {
namespace compiler {

int ProtobufMain(int argc, char* argv[]) {
  absl::InitializeLog();

  CommandLineInterface cli;
  cli.AllowPlugins("protoc-");
#ifdef GOOGLE_PROTOBUF_RUNTIME_INCLUDE_BASE
  cli.set_opensource_runtime(true);
#endif

  // Proto2 C++
  cpp::CppGenerator cpp_generator;
  cli.RegisterGenerator("--cpp_out", "--cpp_opt", &cpp_generator,
                        "Generate C++ header and source.");

#ifdef GOOGLE_PROTOBUF_RUNTIME_INCLUDE_BASE
  cpp_generator.set_opensource_runtime(true);
  cpp_generator.set_runtime_include_base(GOOGLE_PROTOBUF_RUNTIME_INCLUDE_BASE);
#endif

  // Proto2 Java
  java::JavaGenerator java_generator;
  cli.RegisterGenerator("--java_out", "--java_opt", &java_generator,
                        "Generate Java source file.");

#ifdef GOOGLE_PROTOBUF_RUNTIME_INCLUDE_BASE
  java_generator.set_opensource_runtime(true);
#endif

  // Proto2 Kotlin
  kotlin::KotlinGenerator kt_generator;
  cli.RegisterGenerator("--kotlin_out", "--kotlin_opt", &kt_generator,
                        "Generate Kotlin file.");


  // Proto2 Python
  python::Generator py_generator;
  cli.RegisterGenerator("--python_out", "--python_opt", &py_generator,
                        "Generate Python source file.");

#ifdef GOOGLE_PROTOBUF_RUNTIME_INCLUDE_BASE
  py_generator.set_opensource_runtime(true);
#endif

  // Python pyi
  python::PyiGenerator pyi_generator;
  cli.RegisterGenerator("--pyi_out", &pyi_generator,
                        "Generate python pyi stub.");

  // PHP
  php::Generator php_generator;
  cli.RegisterGenerator("--php_out", "--php_opt", &php_generator,
                        "Generate PHP source file.");

  // Ruby
  ruby::Generator rb_generator;
  cli.RegisterGenerator("--ruby_out", "--ruby_opt", &rb_generator,
                        "Generate Ruby source file.");

  // CSharp
  csharp::Generator csharp_generator;
  cli.RegisterGenerator("--csharp_out", "--csharp_opt", &csharp_generator,
                        "Generate C# source file.");

  // Objective-C
  objectivec::ObjectiveCGenerator objc_generator;
  cli.RegisterGenerator("--objc_out", "--objc_opt", &objc_generator,
                        "Generate Objective-C header and source.");

  // Rust
  rust::RustGenerator rust_generator;
  cli.RegisterGenerator("--rust_out", "--rust_opt", &rust_generator,
                        "Generate Rust sources.");
#ifdef DISABLE_PROTOC_CONFIG
  auto cleanup = internal::DisableAllowlistInternalOnly();
#endif  // DISABLE_PROTOC_CONFIG
  return cli.Run(argc, argv);
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#ifdef _MSC_VER
std::string ToMultiByteUtf8String(const wchar_t* input) {
  int size = WideCharToMultiByte(CP_UTF8, 0, input, wcslen(input), 0, 0,
                                 nullptr, nullptr);
  std::string result(size, 0);
  if (size)
    WideCharToMultiByte(CP_UTF8, 0, input, wcslen(input), &result[0], size,
                        nullptr, nullptr);
  return result;
}

int main(int argc, char* argv[]) {
  wchar_t** wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
  char** argv_mbcs = new char*[argc];
  for (int i = 0; i < argc; i++) {
    std::string* multibyte_string = new auto(ToMultiByteUtf8String(wargv[i]));
    argv_mbcs[i] = const_cast<char*>(multibyte_string->c_str());
  }
  return google::protobuf::compiler::ProtobufMain(argc, argv_mbcs);
}
#else
int main(int argc, char* argv[]) {
  return google::protobuf::compiler::ProtobufMain(argc, argv);
}
#endif

#include "google/protobuf/port_undef.inc"
