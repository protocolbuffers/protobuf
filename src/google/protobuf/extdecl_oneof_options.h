#ifndef GOOGLE_PROTOBUF_EXTDECL_ONEOF_OPTIONS_H__
#define GOOGLE_PROTOBUF_EXTDECL_ONEOF_OPTIONS_H__

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
    kOneofOptionExtensions = {
        // go/keep-sorted start numeric=yes block=yes
        ExtensionDeclaration{
            .number = 525000010,
            .field_name = ".apps.templates.template_oneof",
            .type = ".apps.templates.TemplateOneofOptions",
        },
        ExtensionDeclaration{
            .number = 525000011,
            .field_name = ".spanner.span_admin.oneof_flag",
            .type = ".spanner.span_admin.Flag",
        },
        ExtensionDeclaration{
            .number = 525000012,
            .field_name = ".spanner.span_admin.oneof_arg",
            .type = ".spanner.span_admin.Arg",
        },
        ExtensionDeclaration{
            .number = 525000013,
            .field_name = ".proto_best_practices.oneof_optouts",
            .type = ".proto_best_practices.FindingOptouts",
        },
        // go/keep-sorted end
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_EXTDECL_ONEOF_OPTIONS_H__
