#include "rust/cpp_kernel/map.h"

#include <cstdint>
#include <string>
#include <utility>

#include "google/protobuf/map.h"
#include "rust/cpp_kernel/strings.h"

extern "C" {

void __rust_proto_thunk__UntypedMapIterator_increment(
    google::protobuf::internal::UntypedMapIterator* iter) {
  iter->PlusPlus();
}

__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(int32_t, i32, int32_t,
                                                   int32_t, value, cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(uint32_t, u32, uint32_t,
                                                   uint32_t, value, cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(float, f32, float, float,
                                                   value, cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(double, f64, double, double,
                                                   value, cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(bool, bool, bool, bool,
                                                   value, cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(uint64_t, u64, uint64_t,
                                                   uint64_t, value, cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(int64_t, i64, int64_t,
                                                   int64_t, value, cpp_value);
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(
    std::string, ProtoBytes, google::protobuf::rust_internal::PtrAndLen, std::string*,
    std::move(*value),
    google::protobuf::rust_internal::PtrAndLen(cpp_value.data(), cpp_value.size()));
__PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(
    std::string, ProtoString, google::protobuf::rust_internal::PtrAndLen, std::string*,
    std::move(*value),
    google::protobuf::rust_internal::PtrAndLen(cpp_value.data(), cpp_value.size()));

}  // extern "C"
