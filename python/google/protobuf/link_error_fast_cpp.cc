// go/fastpythonproto

namespace google {
namespace protobuf {
namespace python {

// This exists solely to cause a build/link time error when both
// :use_fast_cpp_protos and :use_pure_python are specified in a build.;

extern "C" {

// MUST match link_error_pure_python.cc's function signature.

__attribute__((noinline)) __attribute__((optnone)) int
go_SLASH_build_deps_on_BOTH_use_fast_cpp_protos_AND_use_pure_python() {
  return 1;
}

}  // extern "C"

}  // namespace python
}  // namespace protobuf
}  // namespace google
