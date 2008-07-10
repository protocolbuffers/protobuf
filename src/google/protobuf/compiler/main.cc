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

#include <google/protobuf/compiler/command_line_interface.h>
#include <google/protobuf/compiler/cpp/cpp_generator.h>
#include <google/protobuf/compiler/python/python_generator.h>
#include <google/protobuf/compiler/java/java_generator.h>


int main(int argc, char* argv[]) {

  google::protobuf::compiler::CommandLineInterface cli;

  // Proto2 C++
  google::protobuf::compiler::cpp::CppGenerator cpp_generator;
  cli.RegisterGenerator("--cpp_out", &cpp_generator,
                        "Generate C++ header and source.");

  // Proto2 Java
  google::protobuf::compiler::java::JavaGenerator java_generator;
  cli.RegisterGenerator("--java_out", &java_generator,
                        "Generate Java source file.");


  // Proto2 Python
  google::protobuf::compiler::python::Generator py_generator;
  cli.RegisterGenerator("--python_out", &py_generator,
                        "Generate Python source file.");

  return cli.Run(argc, argv);
}
