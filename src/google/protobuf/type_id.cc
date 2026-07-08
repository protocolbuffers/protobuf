#include "google/protobuf/type_id.h"

#include "absl/strings/string_view.h"
#include "google/protobuf/class_data.h"

namespace google {
namespace protobuf {

absl::string_view TypeId::name() const {
  if (!data_->is_lite) {
    // For !LITE messages, we use the descriptor method function.
    return data_->full().descriptor_methods()->get_type_name(data_);
  }

  // For LITE messages, the type name is accessed via ClassDataLite.
  return static_cast<const internal::ClassDataLite*>(data_)->type_name();
}

}  // namespace protobuf
}  // namespace google
