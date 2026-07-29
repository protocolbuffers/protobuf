#include <iostream>

#include "rust/test/cc_shared_library_interop/foo.pb.h"

extern "C" {
void bar();
}

namespace {
void print_cpp_foo() {
  third_party::protobuf::rust::test::cc_shared_library_interop::Foo foo;
  foo.set_text("Hello from C++!");
  foo.set_value(123);
  std::cout << "C++ proto: " << foo.ShortDebugString() << std::endl;
}
}  // namespace

void baz() {
  print_cpp_foo();
  bar();
}
