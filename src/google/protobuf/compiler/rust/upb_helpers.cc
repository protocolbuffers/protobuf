#include "google/protobuf/compiler/rust/upb_helpers.h"

#include <cstdint>
#include <string>

#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/descriptor.h"
#include "upb_generator/minitable/names.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

std::string UpbMiniTableName(const Descriptor& msg) {
  return upb::generator::MiniTableMessageVarName(msg.full_name());
}

std::string QualifiedUpbMiniTableName(Context& ctx, const Descriptor& msg) {
  return absl::StrCat(RustModule(ctx, msg), UpbMiniTableName(msg));
}

uint32_t UpbMiniTableFieldIndex(const FieldDescriptor& field) {
  auto* parent = field.containing_type();
  ABSL_CHECK(parent != nullptr);

  // TODO: b/361751487 - We should get the field_index from
  // UpbDefs directly, instead of independently matching
  // the sort order here.

  uint32_t num_fields_with_lower_field_number = 0;
  for (int i = 0; i < parent->field_count(); ++i) {
    if (parent->field(i)->number() < field.number()) {
      ++num_fields_with_lower_field_number;
    }
  }

  return num_fields_with_lower_field_number;
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
