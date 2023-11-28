#ifndef GOOGLE_PROTOBUF_EXTDECL_METHOD_OPTIONS_H__
#define GOOGLE_PROTOBUF_EXTDECL_METHOD_OPTIONS_H__

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
    kMethodOptionExtensions = {
        // go/keep-sorted start numeric=yes block=yes
        ExtensionDeclaration{
            .number = 525000000,
            .field_name = ".drive_compass.entity_extraction_type",
            .type =
                ".drive_compass.MessageEntityExtraction.EntityExtractionType",
        },
        ExtensionDeclaration{
            .number = 525000001,
            .field_name = ".production.rpc.request_key.annotation",
            .type = ".production.rpc.request_key.RequestKeyAnnotation",
        },
        ExtensionDeclaration{
            .number = 525000002,
            .field_name = ".uppercase.iam.required_permissions",
            .type = ".uppercase.iam.RequiredPermissions",
        },
        ExtensionDeclaration{
            .number = 525000003,
            .field_name = ".google.cloud.kms.v1.method_type",
            .type = ".google.cloud.kms.v1.KmsMethodType",
        },
        ExtensionDeclaration{
            .number = 525000004,
            .field_name = ".google.api.method_api_versions",
            .type = ".google.api.ApiVersionsRule",
        },
        ExtensionDeclaration{
            .number = 525000005,
            .field_name = ".security.data_access.admin.scoping.dg_annotations",
            .type = ".privacy.data_governance.attributes.Annotations",
        },
        ExtensionDeclaration{
            .number = 525000006,
            .field_name = ".prototiller.metadata.Method.format",
            .type = ".prototiller.metadata.MethodFormat",
        },
        ExtensionDeclaration{
            .number = 525000007,
            .field_name =
                ".growth.search.marketing.platform.app.smp_rpc_options",
            .type =
                ".growth.search.marketing.platform.app.SmpServiceCallOptions",
        },
        // go/keep-sorted end
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_EXTDECL_METHOD_OPTIONS_H__
