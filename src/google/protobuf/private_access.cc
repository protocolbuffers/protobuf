#include "google/protobuf/private_access.h"

#include "google/protobuf/extension_set.h"
#include "google/protobuf/message_lite.h"

namespace google {
namespace protobuf {
namespace internal {

ExtensionSet* PrivateAccess::GetExtensionSet(MessageLite& msg) {
  auto* tc_table = msg.GetTcParseTable();
  if (tc_table->extension_offset == 0) return nullptr;
  return reinterpret_cast<ExtensionSet*>(reinterpret_cast<char*>(&msg) +
                                         tc_table->extension_offset);
}

const ExtensionSet* PrivateAccess::GetExtensionSet(const MessageLite& msg) {
  auto* tc_table = msg.GetTcParseTable();
  if (tc_table->extension_offset == 0) return nullptr;
  return reinterpret_cast<const ExtensionSet*>(
      reinterpret_cast<const char*>(&msg) + tc_table->extension_offset);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
