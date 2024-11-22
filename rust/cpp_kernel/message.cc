#include <limits>

#include "google/protobuf/message_lite.h"
#include "rust/cpp_kernel/serialized_data.h"

extern "C" {

void proto2_rust_Message_delete(google::protobuf::MessageLite* m) { delete m; }

void proto2_rust_Message_clear(google::protobuf::MessageLite* m) { m->Clear(); }

bool proto2_rust_Message_parse(google::protobuf::MessageLite* m,
                               google::protobuf::rust::SerializedData input) {
  if (input.len > std::numeric_limits<int>::max()) {
    return false;
  }
  return m->ParseFromArray(input.data, static_cast<int>(input.len));
}

bool proto2_rust_Message_serialize(const google::protobuf::MessageLite* m,
                                   google::protobuf::rust::SerializedData* output) {
  return google::protobuf::rust::SerializeMsg(m, output);
}

void proto2_rust_Message_copy_from(google::protobuf::MessageLite* dst,
                                   const google::protobuf::MessageLite& src) {
  dst->Clear();
  dst->CheckTypeAndMergeFrom(src);
}

void proto2_rust_Message_merge_from(google::protobuf::MessageLite* dst,
                                    const google::protobuf::MessageLite& src) {
  dst->CheckTypeAndMergeFrom(src);
}

}  // extern "C"
