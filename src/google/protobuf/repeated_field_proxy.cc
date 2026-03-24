#include "google/protobuf/repeated_field_proxy.h"

#include <cstdint>
#include <string>

#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

#define PROTOBUF_INTERNAL_DEFINE_EXTERN_PROXY_TEMPLATES(type)              \
  template class PROTOBUF_EXPORT_TEMPLATE_DEFINE                           \
      internal::RepeatedFieldProxyBase<type>;                              \
  template class PROTOBUF_EXPORT_TEMPLATE_DEFINE                           \
      internal::RepeatedFieldProxyBase<const type>;                        \
  template class PROTOBUF_EXPORT_TEMPLATE_DEFINE                           \
      internal::RepeatedFieldProxyWithSet<type, void>;                     \
  template class PROTOBUF_EXPORT_TEMPLATE_DEFINE                           \
      internal::RepeatedFieldProxyWithPushBack<type, void>;                \
  template class PROTOBUF_EXPORT_TEMPLATE_DEFINE                           \
      internal::RepeatedFieldProxyWithEmplaceBack<type, void>;             \
  template class PROTOBUF_EXPORT_TEMPLATE_DEFINE                           \
      internal::RepeatedFieldProxyWithResize<type, void>;                  \
  template class PROTOBUF_EXPORT_TEMPLATE_DEFINE RepeatedFieldProxy<type>; \
  template class PROTOBUF_EXPORT_TEMPLATE_DEFINE RepeatedFieldProxy<const type>

PROTOBUF_INTERNAL_DEFINE_EXTERN_PROXY_TEMPLATES(bool);
PROTOBUF_INTERNAL_DEFINE_EXTERN_PROXY_TEMPLATES(int32_t);
PROTOBUF_INTERNAL_DEFINE_EXTERN_PROXY_TEMPLATES(uint32_t);
PROTOBUF_INTERNAL_DEFINE_EXTERN_PROXY_TEMPLATES(int64_t);
PROTOBUF_INTERNAL_DEFINE_EXTERN_PROXY_TEMPLATES(uint64_t);
PROTOBUF_INTERNAL_DEFINE_EXTERN_PROXY_TEMPLATES(float);
PROTOBUF_INTERNAL_DEFINE_EXTERN_PROXY_TEMPLATES(double);
PROTOBUF_INTERNAL_DEFINE_EXTERN_PROXY_TEMPLATES(absl::Cord);
PROTOBUF_INTERNAL_DEFINE_EXTERN_PROXY_TEMPLATES(std::string);
PROTOBUF_INTERNAL_DEFINE_EXTERN_PROXY_TEMPLATES(absl::string_view);

#undef PROTOBUF_INTERNAL_DEFINE_EXTERN_PROXY_TEMPLATES

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
