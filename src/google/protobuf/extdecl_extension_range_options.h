#ifndef GOOGLE_PROTOBUF_EXTDECL_EXTENSION_RANGE_OPTIONS_H__
#define GOOGLE_PROTOBUF_EXTDECL_EXTENSION_RANGE_OPTIONS_H__

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
    kExtensionRageOptionExtensions = {
        // go/keep-sorted start numeric=yes block=yes
        ExtensionDeclaration{
            .number = 525000000,
            .field_name = ".proto.commandline.unittest.crumple.zone.my_field",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000001,
            .field_name = ".ads.matching.LocalServicesId.entity_id_extension",
            .type = ".ads.matching.LocalServicesId",
        },
        ExtensionDeclaration{.number = 530333333, .reserved = true},
        // go/keep-sorted end
};
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_EXTDECL_EXTENSION_RANGE_OPTIONS_H__
