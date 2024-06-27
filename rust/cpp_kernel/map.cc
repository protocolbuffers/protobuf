#include "rust/cpp_kernel/map.h"

#include <cstddef>
#include <cstdint>
#include <string>

#include "google/protobuf/map.h"
#include "rust/cpp_kernel/strings.h"

extern "C" {

void __rust_proto_thunk__UntypedMapIterator_increment(
    google::protobuf::internal::UntypedMapIterator* iter) {
  iter->PlusPlus();
}

google::protobuf::internal::UntypedMapBase* rust_proto_map_new() {
  return new google::protobuf::internal::UntypedMapBase(nullptr);
}

void rust_proto_map_free(google::protobuf::internal::UntypedMapBase* m) { delete m; }

void rust_proto_map_clear(google::protobuf::internal::UntypedMapBase* m) {
  google::protobuf::internal::UntypedMapBase other(nullptr);
  m->InternalSwap(&other);
}

size_t rust_proto_map_size(google::protobuf::internal::UntypedMapBase* m) {
  return m->size();
}

google::protobuf::internal::UntypedMapIterator rust_proto_map_iter(
    google::protobuf::internal::UntypedMapBase* m) {
  return m->begin();
}

__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(int32_t, i32, int32_t, value,
                                                   cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(uint32_t, u32, uint32_t,
                                                   value, cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(float, f32, float, value,
                                                   cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(double, f64, double, value,
                                                   cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(bool, bool, bool, value,
                                                   cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(uint64_t, u64, uint64_t,
                                                   value, cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(int64_t, i64, int64_t, value,
                                                   cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(
    std::string, ProtoBytes, google::protobuf::rust_internal::PtrAndLen,
    std::string(value.ptr, value.len),
    google::protobuf::rust_internal::PtrAndLen(cpp_value.data(), cpp_value.size()));
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(
    std::string, ProtoString, google::protobuf::rust_internal::PtrAndLen,
    std::string(value.ptr, value.len),
    google::protobuf::rust_internal::PtrAndLen(cpp_value.data(), cpp_value.size()));

}  // extern "C"
