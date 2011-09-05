
#include <iostream>
#include "upb/def.hpp"
#include "upb/pb/glue.hpp"

static void TestSymbolTable() {
  upb::SymbolTable *s = upb::SymbolTable::New();
  upb::Status status;
  if (!upb::LoadDescriptorFileIntoSymtab(s, "tests/test.proto.pb", &status)) {
    std::cerr << "Couldn't load descriptor: " << status;
    exit(1);
  }
  const upb::MessageDef *md = s->LookupMessage("A");
  assert(md);

  s->Unref();
  md->Unref();
}

int main() {
  TestSymbolTable();
}
