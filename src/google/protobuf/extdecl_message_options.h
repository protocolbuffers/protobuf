#ifndef GOOGLE_PROTOBUF_EXTDECL_MESSAGE_OPTIONS_H__
#define GOOGLE_PROTOBUF_EXTDECL_MESSAGE_OPTIONS_H__

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
    kMessageOptionExtensions = {
        // go/keep-sorted start numeric=yes block=yes
        ExtensionDeclaration{
            .number = 525000000,
            .field_name = ".contentads.bidlab.automap_message",
            .type = ".contentads.bidlab.AutomapMessageOptions",
        },
        ExtensionDeclaration{
            .number = 525000001,
            .field_name = ".logs.proto.resourceidentification.registry.message_"
                          "description",
            .type = "string",
            .reserved = true,
        },
        ExtensionDeclaration{
            .number = 525000002,
            .field_name = ".devintel.datawarehouse.meta.public_table",
            .type = ".devintel.datawarehouse.meta.PublicTable",
        },
        ExtensionDeclaration{
            .number = 525000003,
            .field_name = ".resourceregistry.message_description",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000004,
            .field_name = ".home.graph.common.IamPermissionsMessageOptions"
                          ".iam_permissions",
            .type = ".home.graph.common.IamPermissionsOptions",
        },
        ExtensionDeclaration{
            .number = 525000005,
            .field_name = ".kryptos_horizons.rosetta.rosetta_message",
            .type = ".kryptos_horizons.rosetta.RosettaMessageOptions",
        },
        ExtensionDeclaration{
            .number = 525000006,
            .field_name = ".apps_annotations.msg_details",
            .type = ".apps_annotations.MessageDetails",
        },
        ExtensionDeclaration{
            .number = 525000007,
            .field_name = ".communication.media.platform.datachannel",
            .type = ".communication.media.platform.DataChannelOptions",
        },
        ExtensionDeclaration{
            .number = 525000008,
            .field_name = ".search.next.magi.magi_message_options",
            .type = ".search.next.magi.MessageOptions",
        },
        ExtensionDeclaration{
            .number = 525000010,
            .field_name = ".apps.templates.template_message",
            .type = ".apps.templates.TemplateMessageOptions",
        },
        ExtensionDeclaration{
            .number = 525000011,
            .field_name = ".prodspec.ui_metadata",
            .type = ".prodspec.UiMetadata",
        },
        ExtensionDeclaration{
            .number = 525000012,
            .field_name = ".resourceregistry.gshoe.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000013,
            .field_name =
                ".incident_response_management.skip_etag_response_validation",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000014,
            .field_name =
                ".security.commitments.commitflow.hubs.lib.item_options",
            .type = ".security.commitments.commitflow.hubs.lib.ItemOptions",
        },
        ExtensionDeclaration{
            .number = 525000015,
            .field_name = ".youtube.fixtures.creation_order",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000016,
            .field_name = ".resourceregistry.core.felix.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000017,
            .field_name = ".superroot.magi.msg_filter",
            .type = ".superroot.magi.MagiFilterSpec",
            .repeated = false,
        },
        ExtensionDeclaration{
            .number = 525000018,
            .field_name = ".superroot.magi.msg_transform",
            .type = ".superroot.magi.MagiTransformSpec",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000019,
            .field_name = ".apps_olympus_annotations.expected_below_data_class",
            .type = ".privacy.data_governance.attributes.AppsCommon.DataClass",
        },
        ExtensionDeclaration{
            .number = 525000020,
            .field_name = ".google.api.message_api_versions",
            .type = ".google.api.ApiVersionsRule",
        },
        ExtensionDeclaration{
            .number = 525000021,
            .field_name = ".spanner.span_admin.usage",
            .type = ".spanner.span_admin.Usage",
        },
        ExtensionDeclaration{
            .number = 525000022,
            .field_name = ".spanner.span_admin.object",
            .type = ".spanner.span_admin.Object",
        },
        ExtensionDeclaration{
            .number = 525000023,
            .field_name = ".spanner.span_admin.span_cli_combined_field",
            .type = ".spanner.span_admin.CombinedField",
        },
        ExtensionDeclaration{
            .number = 525000024,
            .field_name = ".resourceregistry.resolved_by_lightweight_module",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000025,
            .field_name = ".spanner.span_admin.span_cli_parsed_arg",
            .type = ".spanner.span_admin.ParsedArg",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000026,
            .field_name = ".spanner.span_admin.span_cli_parsed_flag",
            .type = ".spanner.span_admin.ParsedFlag",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000027,
            .field_name = ".resourceregistry.hardware.gsmi.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000028,
            .field_name =
                ".resourceregistry.cloud.regional_endpoints.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000029,
            .field_name = ".rene.supported_protos",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000030,
            .field_name =
                ".resourceregistry.cloud.cloud_dms.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 533714433,
            .field_name = ".ops.netopscorp.intent.proto.value_field",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 533714434,
            .field_name = ".adx.message_visibility",
            .type = ".adx.Visibility",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800938,
            .field_name = ".assistant.tools.default_fingerprint",
            .type = ".assistant.tools.fingerprints.FingerprintAnnotation",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800939,
            .field_name = ".protolayout.annotations.generic_extends",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800940,
            .field_name = ".enterprise.crm.monet.cdm.annotations.cdm_entity",
            .type = ".enterprise.crm.monet.cdm.annotations.CdmEntity",
        },
        ExtensionDeclaration{
            .number = 535800949,
            .field_name = ".annotation.storage_options",
            .type = ".annotation.StorageOptions",
        },
        ExtensionDeclaration{
            .number = 535800950,
            .field_name = ".ads_dynamo.DynamoEntityRejectionReason.entity_"
                          "rejection_reason",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800951,
            .field_name = ".enterprise.crm.monet.cdm.annotations.data_type",
            .type = ".enterprise.crm.monet.cdm.annotations.DataType",
        },
        ExtensionDeclaration{
            .number = 535800952,
            .field_name = ".enterprise.crm.monet.cdm.annotations.operator_type",
            .type = ".enterprise.crm.monet.cdm.annotations.OperatorType",
        },
        ExtensionDeclaration{
            .number = 535800953,
            .field_name =
                ".resourceregistry.core.googleadmin.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800954,
            .field_name = ".corp.legal.elm.matters.services.workflow.proto.osw_"
                          "resource_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800955,
            .field_name = ".play.gateway.adapter.phonesky.is_cacheable_request",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800956,
            .field_name = ".engage.api.tangle.entities.entity_documentation",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800957,
            .field_name = ".autosettings.policy.api.field_dependencies",
            .type = ".autosettings.api.FieldDependencies",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800958,
            .field_name = ".resourceregistry.youtube.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800959,
            .field_name = ".resourceregistry.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800960,
            .field_name = ".devintel.datawarehouse.meta.framework",
            .type = ".devintel.datawarehouse.meta.Framework",
        },
        ExtensionDeclaration{
            .number = 535800961,
            .field_name = ".nest.cloud.platform.uddm.translation.shdt.impl."
                          "proto.typewrapper.shdt_type_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800962,
            .field_name = ".nest.cloud.platform.uddm.translation.shdt.impl."
                          "proto.typewrapper.supported_traits",
            .type = ".nest.cloud.platform.uddm.translation.shdt.impl.proto."
                    "typewrapper.SupportedShdtTraits",
        },
        ExtensionDeclaration{
            .number = 535800963,
            .field_name = ".nest.cloud.platform.uddm.translation.shdt.impl."
                          "proto.traitwrapper.shdt_trait_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800964,
            .field_name = ".resourceregistry.core.nuac.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800965,
            .field_name = ".devintel.datawarehouse.meta.message_dimension",
            .type = ".devintel.datawarehouse.meta.Dimension",
        },
        ExtensionDeclaration{
            .number = 535800966,
            .field_name = ".protolayout.annotations.skip_builder_constructor",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800967,
            .field_name =
                ".resourceregistry.geo.mymaps_serving.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800968,
            .field_name = ".moneta.orchestration2.ui.internal.task.task_id",
            .type = ".moneta.orchestration2.ui.internal.task.TaskId",
        },
        ExtensionDeclaration{
            .number = 535800969,
            .field_name = ".resourceregistry.core.buyinghub.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800970,
            .field_name =
                ".resourceregistry.cloud.customer_telemetry.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800971,
            .field_name =
                ".resourceregistry.core.limiteddisables.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800972,
            .field_name =
                ".resourceregistry.hardware.pixel_security.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800973,
            .field_name = ".devintel.datawarehouse.meta.message_metric",
            .type = ".devintel.datawarehouse.meta.Metric",
        },
        ExtensionDeclaration{
            .number = 535800974,
            .field_name = ".ads.api.common.messageedit.do_not_generate",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800975,
            .field_name = ".resourceregistry.cloud.byoid_cookieservice."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800976,
            .field_name = ".resourceregistry.cloud.psh_platform."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800977,
            .field_name =
                ".resourceregistry.core.casemanagement.api.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800978,
            .field_name = ".cloud.error_message_annotation",
            .type = ".cloud.ErrorMessageAnnotation",
        },
        ExtensionDeclaration{
            .number = 535800980,
            .field_name = ".resourceregistry.cloud.seeker.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800981,
            .field_name =
                ".resourceregistry.cloud.tarsier.yacpr.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800982,
            .field_name = ".moneta.orchestration2.logging.shadow_log_proto_"
                          "behavior_override",
            .type = ".moneta.orchestration2.logging.ShadowLogProtoBehavior",
        },
        ExtensionDeclaration{
            .number = 535800983,
            .field_name =
                ".resourceregistry.payments.merchantconsole.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800984,
            .field_name =
                ".resourceregistry.commerce.gearloose.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800985,
            .field_name = ".resourceregistry.cloud.billing.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800986,
            .field_name = ".resourceregistry.youtube.sdm.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800987,
            .field_name = ".resourceregistry.cloud.mandiant_control_tower."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800988,
            .field_name = ".resourceregistry.cloud.billing.compliance.kyc."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800989,
            .field_name = ".youtube.client.forms.form",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800990,
            .field_name =
                ".resourceregistry.cloud.wlm_data_warehouse.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800991,
            .field_name = ".resourceregistry.youtube.biztech."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800992,
            .field_name =
                ".resourceregistry.ads.hex_guidance.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800993,
            .field_name = ".resourceregistry.cloud.tenantmanager."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800994,
            .field_name = ".xaa_schema.product_attribute_options",
            .type = ".xaa_schema.ProductAttributeOptions",
        },
        ExtensionDeclaration{
            .number = 535800995,
            .field_name = ".resourceregistry.cloud.gkemulticloud."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800996,
            .field_name =
                ".resourceregistry.ads.human_investigations.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800997,
            .field_name = ".resourceregistry.core.speakeasy.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800998,
            .field_name = ".search_notifications.experiments.do_not_generate_"
                          "recursive_bindings",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800999,
            .field_name = ".devintel.datawarehouse.meta.aggregation_tables",
            .type = ".devintel.datawarehouse.meta.AggregationTableV2",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801000,
            .field_name = ".resourceregistry.cloud.oidcone.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801001,
            .field_name =
                ".resourceregistry.cloud.cloud_asset.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801002,
            .field_name = ".resourceregistry.dspa.smarthome.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801003,
            .field_name = ".resourceregistry.core.d2o_xp.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801004,
            .field_name = ".resourceregistry.cloud.billing."
                          "unifiedcommitmentservice.billingaccountapi."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801005,
            .field_name = ".resourceregistry.core.liveview.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801006,
            .field_name = ".resourceregistry.payments.instrumentservice."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801007,
            .field_name = ".resourceregistry.payments.data_layer_gpayweb_"
                          "service.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801008,
            .field_name = ".resourceregistry.payments.data_layer_wallet_"
                          "service.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801009,
            .field_name = ".corp.contactcenter.speakeasy.mosaic.constraints",
            .type = ".corp.contactcenter.speakeasy.mosaic.TileConstraints",
        },
        ExtensionDeclaration{
            .number = 535801010,
            .field_name = ".resourceregistry.ads.billing_approval_service."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801011,
            .field_name =
                ".resourceregistry.cloud.prober_service.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801013,
            .field_name = ".annotation.is_exempt_from_validation",
            .type = "bool",
            .repeated = false,
        },
        ExtensionDeclaration{
            .number = 535801014,
            .field_name = ".resourceregistry.cloud.serviceprovisioning."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801015,
            .field_name =
                ".resourceregistry.ads.offline_event_infra.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801016,
            .field_name = ".signals_changefeed_options.message_options",
            .type = ".payments.consumer.datainfra.ps1."
                    "SignalsChangefeedMessageOptions",
        },
        ExtensionDeclaration{
            .number = 535801017,
            .field_name = ".resourceregistry.youtube.biztech.karajan."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801018,
            .field_name = ".resourceregistry.payments.paymentfraud.fraudreview."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801019,
            .field_name = ".symphony.data.extracted_bundle_opts",
            .type = ".symphony.data.ExtractedBundleOpts",
        },
        ExtensionDeclaration{
            .number = 535801020,
            .field_name = ".engage.api.common.identity.proto."
                          "ciram_message_meta",
            .type = ".engage.api.common.identity.proto.CiramMessageMeta",
        },
        ExtensionDeclaration{
            .number = 535801021,
            .field_name = ".resourceregistry.cloud.zerotrust.caa_gcp."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801022,
            .field_name =
                ".resourceregistry.cloud.marketplace.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801023,
            .field_name = ".resourceregistry.core.magiceye.dwh."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801024,
            .field_name = ".autosettings.api.supported_platforms",
            .type =
                ".chrome.cros.dpanel.autosettings.proto.SettingGroup.Platform",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801025,
            .field_name = ".resourceregistry.cloud.netlb.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801026,
            .field_name = ".resourceregistry.cloud.cloud_gmec.gdce_infra.asset."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801027,
            .field_name = ".resourceregistry.cloud.marketplace.producer."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801028,
            .field_name =
                ".resourceregistry.cloud.cloud_ubermint.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801029,
            .field_name =
                ".resourceregistry.cloud.compute.cloud_nat.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801030,
            .field_name = ".contentads.bidlab.reid_default",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801031,
            .field_name =
                ".devintel.datawarehouse.meta.catalog_message_options",
            .type = ".devintel.datawarehouse.meta.CatalogOptions",
        },
        ExtensionDeclaration{
            .number = 535801032,
            .field_name = ".resourceregistry.knowledge_information.bard."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801033,
            .field_name = ".resourceregistry.core.lis_eng.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801034,
            .field_name =
                ".resourceregistry.search.neco.core.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801035,
            .field_name =
                ".resourceregistry.core.onlineassessment.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801036,
            .field_name =
                ".supplychain.hw.cdm.proto.mutations.resource_options",
            .type = ".supplychain.hw.cdm.proto.mutations.ResourceOptions",
        },
        ExtensionDeclaration{
            .number = 535801037,
            .field_name = ".resourceregistry.cloud.scx.greenroom.cat.api."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801038,
            .field_name = ".resourceregistry.cloud.scx.greenroom.cat.dal."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801039,
            .field_name = ".resourceregistry.cloud.cloud_web_team.innovator_"
                          "hub.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801040,
            .field_name =
                ".resourceregistry.ads.adspam_tools.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801041,
            .field_name = ".resourceregistry.googlex.tapestry."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801042,
            .field_name =
                ".resourceregistry.ads.ad_query_tool.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801043,
            .field_name =
                ".resourceregistry.ads.drx_buyside_fe.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801044,
            .field_name = ".resourceregistry.cloud.billing.attribution."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801045,
            .field_name =
                ".resourceregistry.knowledge_information.nlp_repository."
                "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801046,
            .field_name = ".devintel.datawarehouse.meta.external_source",
            .type = ".devintel.datawarehouse.meta.ExternalSource",
        },
        ExtensionDeclaration{
            .number = 535801047,
            .field_name = ".home.graph.common.shdt_trait_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801048,
            .field_name = ".resourceregistry.core.graphview.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801049,
            .field_name =
                ".resourceregistry.cloud.cloudnet_control.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801050,
            .field_name = ".resourceregistry.dspa.dsst.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801051,
            .field_name =
                ".resourceregistry.cloud.cloud_latency.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801052,
            .field_name =
                ".resourceregistry.cloud.netlb.maglev.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801053,
            .field_name = ".resourceregistry.cloud.usps.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801054,
            .field_name =
                ".resourceregistry.cloud.billing.atat.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801055,
            .field_name = ".resourceregistry.cloud.container_threat_detection."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801056,
            .field_name =
                ".resourceregistry.cloud.phoenix_sre.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801057,
            .field_name = ".deepmind.evergreen.v1.evergreen_proto_options",
            .type = ".deepmind.evergreen.v1.EvergreenProtoOptions",
            .repeated = false,
        },
        ExtensionDeclaration{
            .number = 535801058,
            .field_name = ".resourceregistry.cloud.support.core.attachments."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801059,
            .field_name =
                ".resourceregistry.cloud.support.core.cases.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801060,
            .field_name = ".resourceregistry.cloud.support.core.comments."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801061,
            .field_name = ".resourceregistry.cloud.support.core."
                          "supportactivities.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801062,
            .field_name = ".security.data_access.asr.TestResource."
                          "impersonated_gaia_id_path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801063,
            .field_name = ".resourceregistry.cloud.cloud_partner_incentives."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801064,
            .field_name =
                ".resourceregistry.dspa.android_pixel_recorder.playback."
                "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801065,
            .field_name = ".prothon.spawn",
            .type = ".prothon.Spawn",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801066,
            .field_name = ".prothon.key",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801067,
            .field_name = ".resourceregistry.cloud.cloud_web_team.pricing."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801068,
            .field_name = ".beto.dimensions.product",
            .type = ".beto.Product",
        },
        ExtensionDeclaration{
            .number = 535801069,
            .field_name =
                ".resourceregistry.cloud.support.caspian.supportsettings."
                "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801070,
            .field_name = ".ads.adsense.gammaemail.proto.required_drx_role_id",
            .type = "int64",
        },
        ExtensionDeclaration{
            .number = 535801071,
            .field_name = ".security.data_access.asr.GmailResource."
                          "impersonated_gaia_id_path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801072,
            .field_name = ".resourceregistry.commerce.third_party_syncing."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801073,
            .field_name = ".resourceregistry.cloud.support.core.consults."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801074,
            .field_name = ".resourceregistry.cloud.support.core.escalations."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801075,
            .field_name = ".resourceregistry.cloud.support.core.slomilestones."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801076,
            .field_name = ".resourceregistry.cloud.support.core.workitems."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801077,
            .field_name = ".resourceregistry.cloud.cloud_web_team."
                          "qualification_bot.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801078,
            .field_name = ".resourceregistry.core.fido.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801079,
            .field_name = ".resourceregistry.cloud.vmtd."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801080,
            .field_name =
                ".resourceregistry.payments.security.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801081,
            .field_name = ".resourceregistry.cloud.aps_integration."
                          "resource_type_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801082,
            .field_name = ".youtube_creator.channel_entities_permission",
            .type = ".youtube_creator.ChannelEntityPermission",
        },
        ExtensionDeclaration{
            .number = 535801083,
            .field_name = ".ccc.hosted.store.ConditionalScopesAnnotation."
                          "cel_joining_operator",
            .type = ".ccc.hosted.store.ConditionalScopesOperator",
        },
        ExtensionDeclaration{
            .number = 535801084,
            .field_name = ".contentads.bidlab.bow_message",
            .type = ".contentads.bidlab.BowMessageOptions",
        },
        ExtensionDeclaration{
            .number = 535801085,
            .field_name = ".resourceregistry.cloud.psm.resource_type_path",
            .type = "string",
            .repeated = true,
        },
        // go/keep-sorted end
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_EXTDECL_MESSAGE_OPTIONS_H__
