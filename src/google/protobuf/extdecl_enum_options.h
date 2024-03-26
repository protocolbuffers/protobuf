#ifndef GOOGLE_PROTOBUF_EXTDECL_ENUM_OPTIONS_H__
#define GOOGLE_PROTOBUF_EXTDECL_ENUM_OPTIONS_H__

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
    kEnumOptionExtensions = {
        // go/keep-sorted start numeric=yes block=yes
        ExtensionDeclaration{
            .number = 525000000,
            .field_name = ".cloud.error_space_annotation",
            .type = ".cloud.ErrorSpaceAnnotation",
        },
        ExtensionDeclaration{
            .number = 525000001,
            .field_name = ".apps_annotations.enum_details",
            .type = ".apps_annotations.EnumDetails",
        },
        ExtensionDeclaration{
            .number = 525000010,
            .field_name = ".apps.templates.template_enum",
            .type = ".apps.templates.TemplateEnumOptions",
        },
        ExtensionDeclaration{
            .number = 525000011,
            .field_name = ".adx.enum_visibility",
            .type = ".adx.Visibility",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000012,
            .field_name =
                ".enterprise.crm.monet.cdm.annotations.enums.cdm_entity",
            .type = ".enterprise.crm.monet.cdm.annotations.CdmEntity",
        },
        ExtensionDeclaration{
            .number = 525000013,
            .field_name = ".liteoptions.testing.enum_option",
            .type = "int32",
        },
        ExtensionDeclaration{
            .number = 525000014,
            .field_name =
                ".privacy.context.ondevice.explicit_privacy_requirements",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000015,
            .field_name = ".usps.packet_events.event_scope",
            .type = ".usps.packet_events.PacketEventScope.EventScope",
        },
        ExtensionDeclaration{
            .number = 525000016,
            .field_name = ".notifications.platform.execution.storage."
                          "estimations.unmapped_filter_code",
            .type = ".gamma_history.FilteredOutData.FilterStatus",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000017,
            .field_name =
                ".ccc.hosted.store.constants.EssAnnotations.es_entity_type",
            .type = ".ccc.hosted.store.constants.EntityType",
        },
        ExtensionDeclaration{
            .number = 525000018,
            .field_name = ".google.api.enum_api_versions",
            .type = ".google.api.ApiVersionsRule",
        },
        ExtensionDeclaration{
            .number = 525000019,
            .field_name = ".devintel.datawarehouse.meta.enum_dimension",
            .type = ".devintel.datawarehouse.meta.Dimension",
        },
        ExtensionDeclaration{
            .number = 525000020,
            .field_name = ".devintel.datawarehouse.meta.enum_metric",
            .type = ".devintel.datawarehouse.meta.Metric",
        },
        ExtensionDeclaration{
            .number = 525000021,
            .field_name = ".ptoken.verdict_reason_enum_metadata",
            .type = ".ptoken.VerdictReasonEnumMetadata",
        },
        ExtensionDeclaration{
            .number = 525000022,
            .field_name = ".google.internal.contactcenter.configuration.v2."
                          "create_bot_config_error_domain",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000023,
            .field_name = ".google.internal.contactcenter.configuration.v2."
                          "update_bot_config_error_domain",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000024,
            .field_name = ".nest.cloud.platform.uddm.translation.shdt.impl."
                          "proto.traitwrapper."
                          "shdt_enum_values_prefix",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000025,
            .field_name = ".google.internal.contactcenter.objects.v1."
                          "error_domain",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000026,
            .field_name = ".logs.bizbuilder.event_namespace_metadata",
            .type = ".logs.bizbuilder.EventNamespaceMetadata",
        },
        // go/keep-sorted end
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_EXTDECL_ENUM_OPTIONS_H__
