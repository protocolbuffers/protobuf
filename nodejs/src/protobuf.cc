#include <nan.h>

#include "defs.h"
#include "int64.h"
#include "readonlyarray.h"
#include "util.h"
#include "message.h"
#include "enum.h"
#include "repeatedfield.h"
#include "map.h"
#include "encode_decode.h"

using namespace v8;
using namespace node;

namespace protobuf_js {

void init(Handle<Object> exports) {
  Int64::Init(exports);
  ReadOnlyArray::Init(exports);
  Descriptor::Init(exports);
  FieldDescriptor::Init(exports);
  OneofDescriptor::Init(exports);
  EnumDescriptor::Init(exports);
  DescriptorPool::Init(exports);
  RepeatedField::Init(exports);
  Map::Init(exports);
  ProtoMessage::Init(exports);
  ProtoEnum::Init(exports);
}

}  // namespace protobuf_js

// On OS X builds, these are required. TODO: figure out which macro defs we need
// to flip to eliminate this dependence.
extern "C" {
void upb_lock() {}
void upb_unlock() {}
}

NODE_MODULE(google_protobuf_cpp, protobuf_js::init)
