#ifndef GOOGLE_PROTOBUF_EXTDECL_STREAM_OPTIONS_H__
#define GOOGLE_PROTOBUF_EXTDECL_STREAM_OPTIONS_H__

#include <initializer_list>

#include "google/protobuf/extension_declaration.h"

namespace google {
namespace protobuf {
namespace internal {

// When choosing an extension number, prefer using the next available value
// greater than or equal to 525 000 000.
//
// Remember that extension numbers only need to be unique per-proto. They do
// not need to be globally unique.
//
// See go/extension-declarations#descriptor-proto for more information.
static constexpr std::initializer_list<const ExtensionDeclaration>
    kStreamOptions = {
        // go/keep-sorted start numeric=yes block=yes

        // Please remove this placeholder after adding the first extension.
        ExtensionDeclaration{
            .number = kRequireExtDeclNum,
            .field_name = ".placeholder",
            .type = ".placeholder",
        },
        // go/keep-sorted end
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_EXTDECL_STREAM_OPTIONS_H__
