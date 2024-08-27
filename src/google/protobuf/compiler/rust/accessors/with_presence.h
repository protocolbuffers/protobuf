#ifndef GOOGLE_PROTOBUF_COMPILER_RUST_ACCESSORS_WITH_PRESENCE_H__
#define GOOGLE_PROTOBUF_COMPILER_RUST_ACCESSORS_WITH_PRESENCE_H__

#include "google/protobuf/compiler/rust/accessors/accessor_case.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/descriptor.h"

// Helper functions for generating the common accessors that any with-presence
// field have (the hassers, clearers, and the Optional<> getter).

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

void WithPresenceAccessorsInMsgImpl(Context& ctx, const FieldDescriptor& field,
                                    AccessorCase accessor_case);

void WithPresenceAccessorsInExternC(Context& ctx, const FieldDescriptor& field);

void WithPresenceAccessorsInThunkCc(Context& ctx, const FieldDescriptor& field);

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_RUST_ACCESSORS_WITH_PRESENCE_H__
