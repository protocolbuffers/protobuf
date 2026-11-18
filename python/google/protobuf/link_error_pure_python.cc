// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// go/fastpythonproto

namespace google {
namespace protobuf {
namespace python {

// This exists solely to cause a build/link time error when more than one of
// :use_upb_protos, :use_fast_cpp_protos and :use_pure_python are specified
// in a build.;

extern "C" {

// MUST match link_error_pure_python.cc's function signature.

__attribute__((noinline)) __attribute__((optnone)) int
go_SLASH_build_deps_on_multiple_python_proto_backends() {
  return 0;
}

}  // extern "C"

}  // namespace python
}  // namespace protobuf
}  // namespace google
