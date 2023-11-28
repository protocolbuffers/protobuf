#ifndef GOOGLE_PROTOBUF_EXTDECL_FILE_OPTIONS_H__
#define GOOGLE_PROTOBUF_EXTDECL_FILE_OPTIONS_H__

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
    kFileOptionExtensions = {
        // go/keep-sorted start numeric=yes block=yes
        ExtensionDeclaration{
            .number = 525000010,
            .field_name = ".apps.templates.template_file",
            .type = ".apps.templates.TemplateFileOptions",
        },
        ExtensionDeclaration{
            .number = 525000011,
            .field_name = ".gdce_boundary_proxy.proto_db_include",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000012,
            .field_name =
                ".cloud_web3.data_analytics.bigquery_schema.schema_options",
            .type =
                ".cloud_web3.data_analytics.bigquery_schema.SchemaFileOptions",
        },
        ExtensionDeclaration{
            .number = 525000013,
            .field_name = ".corp.gteml.featurestore.feature_group",
            .type = ".corp.gteml.featurestore.FeatureGroup"},
        ExtensionDeclaration{
            .number = 525000014,
            .field_name = ".cloud.error_message_file_annotation",
            .type = ".cloud.ErrorMessageFileAnnotation",
        },
        ExtensionDeclaration{
            .number = 525000015,
            .field_name =
                ".ads.api.common.messageedit.do_not_generate_message_edits",
            .type = "bool",
        },
        // go/keep-sorted end
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_EXTDECL_FILE_OPTIONS_H__
