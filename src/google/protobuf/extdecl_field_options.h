#ifndef GOOGLE_PROTOBUF_EXTDECL_FIELD_OPTIONS_H__
#define GOOGLE_PROTOBUF_EXTDECL_FIELD_OPTIONS_H__

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
    kFieldOptionExtensions = {
        // go/keep-sorted start numeric=yes block=yes

        ExtensionDeclaration{
            .number = 525000000,
            .field_name = ".contentads.bidlab.automap_field",
            .type = ".contentads.bidlab.AutomapFieldOptions",
        },
        ExtensionDeclaration{
            .number = 525000001,
            .field_name = ".omen.streaming_config_annotations",
            .type = ".omen.StreamingRuleUpdateConfigAnnotations",
        },
        ExtensionDeclaration{
            .number = 525000002,
            .field_name =
                ".supplychain.esp.factdata.common.proto.fact_data_type",
            .type = ".supplychain.esp.factdata.common.proto.FactDataType",
        },
        ExtensionDeclaration{
            .number = 525000003,
            .field_name = ".security.data_access.asr.cloud_build_resource",
            .type = ".security.data_access.asr.CloudBuildResource",
        },
        ExtensionDeclaration{
            .number = 525000004,
            .field_name =
                ".logs.proto.resourceidentification.registry.field_description",
            .type = "string",
            .reserved = true,
        },
        ExtensionDeclaration{
            .number = 525000005,
            .field_name =
                ".youtube.api.innertube.logging.annotations.is_post_processed",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000006,
            .field_name = ".evaluation.data.trimmed_in_metadata",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000007,
            .field_name = ".security.data_access.asr.CloudArmorResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000008,
            .field_name = ".security.data_access.asr.ApigeeResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000009,
            .field_name = ".kryptos_horizons.rosetta.rosetta_field",
            .type = ".kryptos_horizons.rosetta.RosettaFieldOptions",
        },
        ExtensionDeclaration{
            .number = 525000010,
            .field_name = ".apps.templates.template_field",
            .type = ".apps.templates.TemplateFieldOptions",
        },
        ExtensionDeclaration{
            .number = 525000011,
            .field_name = ".growth.googlegrowth.videoplatform.studio.proto."
                          "expression_field",
            .type = ".growth.googlegrowth.videoplatform.studio.proto."
                    "ExpressionField",
        },
        ExtensionDeclaration{
            .number = 525000012,
            .field_name = ".security.data_access.asr.YouTubeCoreResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000013,
            .field_name =
                ".longtail_ads.SuggestionResponseAnnotations.data_type",
            .type = ".longtail_ads.SuggestionResponseAnnotations.DataType",
        },
        ExtensionDeclaration{
            .number = 525000014,
            .field_name = ".commerce.datastore.dma_data_label",
            .type = ".commerce.datastore.DmaDataLabel",
        },
        ExtensionDeclaration{
            .number = 525000015,
            .field_name =
                ".youtube.api.innertube.vapi_innertube_conversion_ignore",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000016,
            .field_name = ".logs.proto.wireless.android.stats.platform."
                          "westworld.restriction_category",
            .type = ".logs.proto.wireless.android.stats.platform.westworld."
                    "RestrictionCategory",
        },
        ExtensionDeclaration{
            .number = 525000017,
            .field_name = ".logs.proto.wireless.android.stats.platform."
                          "westworld.field_restriction_option",
            .type = ".logs.proto.wireless.android.stats.platform.westworld."
                    "FieldRestrictionOption",
        },
        ExtensionDeclaration{
            .number = 525000018,
            .field_name =
                ".monitoring.monarch.tools.monarchspec.plugins.exporter."
                "visible_in_checked_in_file",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000019,
            .field_name = ".hamilton_compliance.dma_options",
            .type = ".hamilton_compliance.DMAOptions",
        },
        ExtensionDeclaration{
            .number = 525000020,
            .field_name =
                ".play.console.dataplatform.statsgeneration.ingestion.common."
                "field_for_input_named",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000021,
            .field_name =
                ".play.console.dataplatform.statsgeneration.ingestion.common."
                "output_partition_data_date_field",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000022,
            .field_name = ".apps_annotations.field_details",
            .type = ".apps_annotations.FieldDetails",
        },
        ExtensionDeclaration{
            .number = 525000023,
            .field_name = ".ccai_insights.internal.metric_sql_annotation.field_"
                          "annotation",
            .type = ".ccai_insights.internal.metric_sql_annotation."
                    "MetricSqlAnnotation",
        },
        ExtensionDeclaration{
            .number = 525000024,
            .field_name = ".security.data_access.asr.LookerCoreResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000025,
            .field_name = ".ccai_insights.internal.metric_schema_annotation."
                          "field_annotation",
            .type = ".ccai_insights.internal.metric_schema_annotation."
                    "MetricSchemaAnnotation",
        },
        ExtensionDeclaration{
            .number = 525000026,
            .field_name =
                ".security.data_access.asr.SecureSourceManagerResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000027,
            .field_name = ".uppercase.iam.semantic_type",
            .type = ".uppercase.iam.SemanticType",
        },
        ExtensionDeclaration{
            .number = 525000028,
            .field_name = ".android.os.statsd.statsd_version_enforcement",
            .type = ".android.os.statsd.StatsdVersionEnforcement",
        },
        ExtensionDeclaration{
            .number = 525000029,
            .field_name = ".moneta.vendorgateway.internal.api.conversation."
                          "analytics.annotation.issuer_id",
            .type = ".moneta.vendorgateway.internal.api.conversation.analytics."
                    "annotation.IssuerIdMetadata",
        },
        ExtensionDeclaration{
            .number = 525000030,
            .field_name = ".youtube.api.innertube.logging.annotations.clearcut_"
                          "annotations",
            .type = ".youtube.api.innertube.logging.annotations."
                    "ClearcutAnnotations",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000031,
            .field_name = ".superroot.magi.filter",
            .type = ".superroot.magi.MagiFilterSpec",
            .repeated = false,
        },
        ExtensionDeclaration{
            .number = 525000032,
            .field_name = ".superroot.magi.transform",
            .type = ".superroot.magi.MagiTransformSpec",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000033,
            .field_name = ".gws.common.api.sokoban.FeatureClass.metadata",
            .type = ".gws.common.api.sokoban.FeatureClassMetadata",
        },
        ExtensionDeclaration{
            .number = 525000034,
            .field_name = ".security.data_access.asr.CloudLoggingResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000035,
            .field_name = ".proto_data_type.data_type",
            .type = ".proto_data_type.ProtoDataType",
            .repeated = false,
        },
        ExtensionDeclaration{
            .number = 525000036,
            .field_name = ".wallet.valuables.pkpass.is_localizable",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000037,
            .field_name = ".google.api.field_api_versions",
            .type = ".google.api.ApiVersionsRule",
        },
        ExtensionDeclaration{
            .number = 525000038,
            .field_name = ".spanner.span_admin.flag",
            .type = ".spanner.span_admin.Flag",
        },
        ExtensionDeclaration{
            .number = 525000039,
            .field_name = ".spanner.span_admin.arg",
            .type = ".spanner.span_admin.Arg",
        },
        ExtensionDeclaration{
            .number = 525000040,
            .field_name = ".searchanalytics.query_schema.granularity",
            .type = ".searchanalytics.query_schema.GranularityOption",
            .repeated = true,
            .reserved = true,
        },
        ExtensionDeclaration{.number = 525000041,
                             .field_name = ".xaa_schema.taxonomy_mapping",
                             .type = ".xaa_schema.TaxonomyMapping",
                             .repeated = true},
        ExtensionDeclaration{
            .number = 525000042,
            .field_name = ".sudata.suda_field_type",
            .type = ".sudata.SudaFieldTypeEnum",
        },
        ExtensionDeclaration{
            .number = 525000043,
            .field_name = ".searchanalytics.query_schema.granularity_option",
            .type = ".searchanalytics.query_schema.GranularityOption",
        },
        ExtensionDeclaration{
            .number = 525000044,
            .field_name = ".activemobility.metric_field_option",
            .type = ".activemobility.MetricFieldOption",
        },
        ExtensionDeclaration{
            .number = 525000045,
            .field_name = ".xaa_schema.dimension_mapping",
            .type = ".xaa_schema.DimensionMapping",
        },
        ExtensionDeclaration{
            .number = 525000046,
            .field_name = ".gtp_iridessa.AudioCodec.sample_rate_khz",
            .type = "int32",
        },
        ExtensionDeclaration{
            .number = 525000047,
            .field_name = ".merchant.identity.storage.task_type_metadata",
            .type = ".merchant.identity.storage.TaskTypeMetadata",
        },
        ExtensionDeclaration{
            .number = 535800911,
            .field_name =
                ".contentads.privacy.policy.policy_decision_cardinality",
            .type = "int32",
        },
        ExtensionDeclaration{
            .number = 535800912,
            .field_name = ".youtube.fixtures.default_reference",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800913,
            .field_name = ".gls.leads.api.field_behavior",
            .type = ".gls.leads.api.FieldBehavior",
            .reserved = true,
        },
        ExtensionDeclaration{
            .number = 535800914,
            .field_name = ".gls.leads.api.field_category",
            .type = ".gls.leads.api.FieldCategory",
            .reserved = true,
        },
        ExtensionDeclaration{
            .number = 535800915,
            .field_name = ".security.data_access.asr.CloudDnsResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800916,
            .field_name =
                ".apps.dynamite.dynamite_unobfuscated_space_id_routing",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800917,
            .field_name =
                ".apps.dynamite.dynamite_unobfuscated_user_id_routing",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800918,
            .field_name = ".assistant.verticals.conversationalcare.actions_"
                          "graph.excluded_from_serving",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800919,
            .field_name = ".car.nproto.override_key_type",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800920,
            .field_name = ".car.nproto.override_value_type",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800921,
            .field_name = ".car.nproto.value_unit",
            .type = ".car.typesafe_units.TypesafeUnit",
        },
        ExtensionDeclaration{
            .number = 535800922,
            .field_name = ".resourceregistry.field_description",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800923,
            .field_name =
                ".apps.dynamite.dynamite_unobfuscated_customer_id_routing",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800924,
            .field_name = ".security.data_access.asr.DirectoryResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800925,
            .field_name = ".gls.api.field_behavior",
            .type = ".gls.api.FieldBehavior",
            .reserved = true,
        },
        ExtensionDeclaration{
            .number = 535800926,
            .field_name = ".gls.api.field_category",
            .type = ".gls.api.FieldCategory",
            .reserved = true,
        },
        ExtensionDeclaration{
            .number = 535800927,
            .field_name = ".faceauth.is_window_size",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800928,
            .field_name = ".moneta.reporting.proto.annotation.field_meta",
            .type = ".moneta.reporting.proto.annotation.FieldMeta",
        },
        ExtensionDeclaration{
            .number = 535800929,
            .field_name = ".engage.api.tangle.entities.source_from",
            .type = ".engage.api.tangle.entities.SupportCasesAnnotationEnum."
                    "SourceFrom",
        },
        ExtensionDeclaration{
            .number = 535800930,
            .field_name = ".resourceregistry.field_path",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800931,
            .field_name = ".annotation.key",
            .type = "int32",
        },
        ExtensionDeclaration{
            .number = 535800932,
            .field_name = ".drive_compass.extract",
            .type = ".drive_compass.EntityId.EntityIdType",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800933,
            .field_name = ".drive_compass.contains",
            .type = ".drive_compass.EntityId.EntityIdType",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800934,
            .field_name = ".drive_compass.compound",
            .type = ".drive_compass.Compound.CompoundType",
        },
        ExtensionDeclaration{
            .number = 535800935,
            .field_name = ".adx.field_visibility",
            .type = ".adx.Visibility",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800936,
            .field_name = ".smartass.brain.configure.layer_options",
            .type = ".smartass.brain.configure.LayerOptions",
        },
        ExtensionDeclaration{
            .number = 535800937,
            .field_name = ".logs_youtube.mapped_field",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800938,
            .field_name = ".assistant.tools.fingerprint",
            .type = ".assistant.tools.fingerprints.FingerprintAnnotation",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800939,
            .field_name = ".ads.monitoring.smart_alerting.exclude_in_hash",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800940,
            .field_name = ".contentads.adx.mixer.auction.testing.precision",
            .type = "uint32",
        },
        ExtensionDeclaration{
            .number = 535800941,
            .field_name = ".identity_checkpoint.template_path_prefix",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800942,
            .field_name = ".merchant.termsofservice.target_product_type",
            .type = ".merchant.termsofservice.ProductType",
        },
        ExtensionDeclaration{
            .number = 535800943,
            .field_name = ".security.data_access.asr.SeekerResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800944,
            .field_name = ".protolayout.annotations.matching_static_field",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800945,
            .field_name = ".protolayout.annotations.getter_visibility",
            .type = ".protolayout.annotations.MethodVisibility",
        },
        ExtensionDeclaration{
            .number = 535800946,
            .field_name =
                ".apps.dynamite.dynamite_attachment_resource_name_routing",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800947,
            .field_name =
                ".security.data_access.asr.UnifiedCloudSearchResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800948,
            .field_name = ".aquarius.configuration.read_file",
            .type = ".aquarius.configuration.ReadFileOptions",
        },
        ExtensionDeclaration{
            .number = 535800949,
            .field_name = ".customer_support.intelligence.proto.crowd_compute."
                          "classification_options",
            .type = ".customer_support.intelligence.proto.crowd_compute."
                    "ClassificationFieldOptions",
        },
        ExtensionDeclaration{
            .number = 535800950,
            .field_name = ".play.growthhub.loyalty.api.v1.quest.data_source",
            .type = ".play.growthhub.loyalty.api.v1.quest.QuestStepDataSource",
        },
        ExtensionDeclaration{
            .number = 535800951,
            .field_name = ".ctms.use_lower_criticality",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800952,
            .field_name = ".hamilton.retain_all_record_versions",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800953,
            .field_name =
                ".ads.a1.training.a1_feature_annotation_field_options",
            .type = ".ads.a1.training.A1FeatureAnnotationFieldOptions",
        },
        ExtensionDeclaration{
            .number = 535800954,
            .field_name = ".corp.contactcenter.speakeasy.session_data_"
                          "reference_by_field_path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800955,
            .field_name = ".viaduct.structuredtf.key_mapping",
            .type = ".viaduct.structuredtf.KeyMapping",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800956,
            .field_name =
                ".security.data_access.asr.CloudDataFusionResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800957,
            .field_name = ".cloud_vpcsc.repeated_field",
            .type = ".cloud_vpcsc.RepeatedFieldOptions",
        },
        ExtensionDeclaration{
            .number = 535800958,
            .field_name = ".enterprise.crm.monet.cdm.annotations.cdm_attribute",
            .type = ".enterprise.crm.monet.cdm.annotations.CdmAttribute",
        },
        ExtensionDeclaration{
            .number = 535800959,
            .field_name = ".security.data_access.asr.GDiffResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800960,
            .field_name = ".corp.legal.elm.matters.services.workflow.proto."
                          "workflow_option",
            .type = ".corp.legal.elm.matters.services.workflow.proto."
                    "WorkflowOption",
        },
        ExtensionDeclaration{
            .number = 535800961,
            .field_name = ".news.publishercenter.authprocessor.proto.id_type",
            .type = ".news.publishercenter.authprocessor.proto."
                    "PublisherCenterIdType",
        },
        ExtensionDeclaration{
            .number = 535800962,
            .field_name = ".aquarius.configuration.metadata",
            .type = ".aquarius.configuration.MetadataOptions",
        },
        ExtensionDeclaration{
            .number = 535800963,
            .field_name = ".assistant.lamda.logs.annotator_type",
            .type = ".assistant.lamda.logs.AnnotatorType",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800964,
            .field_name = ".com.google.cloud.boq.clientapi.gke.anthos.convert."
                          "include_in_patch",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800965,
            .field_name = ".com.google.cloud.boq.clientapi.gke.anthos.convert."
                          "include_in_patch_for_experiment",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800966,
            .field_name = ".google.internal.cloud.console.clientapi.gke.anthos."
                          "include_in_patch",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800967,
            .field_name = ".google.internal.cloud.console.clientapi.gke.anthos."
                          "include_in_patch_for_experiment",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800968,
            .field_name = ".play_analytics_civita.pms_event_type",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800969,
            .field_name =
                ".cloud_datastore.proto_checksum_plugin.exclude_from_checksum",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800970,
            .field_name =
                ".cloud.ux.api.webplatform.pricing.estimate.request.options",
            .type = ".cloud.ux.api.webplatform.pricing.estimate.request."
                    "EstimateRequestOptions",
        },
        ExtensionDeclaration{
            .number = 535800971,
            .field_name = ".hi.proto_print",
            .type = ".hi.ProtoPrintOptions",
        },
        ExtensionDeclaration{
            .number = 535800972,
            .field_name = ".security.data_access.asr.CloudConsoleResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800973,
            .field_name = ".cloud.serverless.infrastructure.controlplane.api."
                          "proto.core.is_resource_name",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800974,
            .field_name = ".apps.dynamite.dynamite_space_id_routing",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800975,
            .field_name = ".gcs_idp.insights_attributes_field_options",
            .type = ".gcs_idp.InsightsAttributesFieldOptions",
        },
        ExtensionDeclaration{
            .number = 535800976,
            .field_name = ".google.ads.admanager.annotations.search_behavior",
            .type = ".google.ads.admanager.annotations.SearchBehavior",
        },
        ExtensionDeclaration{
            .number = 535800977,
            .field_name = ".security.data_access.asr.WarpSpaceResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800978,
            .field_name = ".autosettings.api.field_mutability",
            .type = ".autosettings.api.FieldMutabilityEnum",
        },
        ExtensionDeclaration{
            .number = 535800979,
            .field_name = ".video.profiling.draper.flow.draper_flow",
            .type = ".video.profiling.draper.flow.FlowMetadata",
        },
        ExtensionDeclaration{
            .number = 535800980,
            .field_name = ".annotation.gin",
            .type = ".annotation.GinType",
        },
        ExtensionDeclaration{
            .number = 535800981,
            .field_name =
                ".apps.drive.activity.dataclassification.activity_data_type",
            .type = ".apps.drive.activity.dataclassification.ActivityDataType",
        },
        ExtensionDeclaration{
            .number = 535800982,
            .field_name = ".video.partner.rightsflow.karajan.proto."
                          "enabled_product",
            .type = ".video.partner.rightsflow.karajan.proto.Product",
        },
        ExtensionDeclaration{
            .number = 535800983,
            .field_name =
                ".security.data_access.asr.CloudVpcAccessResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800984,
            .field_name = ".moneta.schema.tapandpay.databunker_migration",
            .type = ".moneta.schema.tapandpay.DatabunkerMigration",
        },
        ExtensionDeclaration{
            .number = 535800985,
            .field_name = ".youtube.ecommerce.promo_targeting.ps_merge_info",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800986,
            .field_name = ".moneta.case_format",
            .type = ".moneta.StringCaseFormat",
        },
        ExtensionDeclaration{
            .number = 535800987,
            .field_name = ".vr.perception.ProtoFieldResolution.options",
            .type = ".vr.perception.ProtoFieldResolution",
        },
        ExtensionDeclaration{
            .number = 535800988,
            .field_name = ".security.data_access.asr.DriveResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800989,
            .field_name = ".security.data_access.asr.StateraResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800990,
            .field_name = ".play.gateway.adapter.phonesky.is_cacheable_field",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800991,
            .field_name = ".security.data_access.asr.GSuiteResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800992,
            .field_name = ".autosettings.api.field_order_priority",
            .type = "int32",
        },
        ExtensionDeclaration{
            .number = 535800993,
            .field_name = ".ads.api.tangle.output_only",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535800994,
            .field_name =
                ".security.data_access.asr.CloudDataplexResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800995,
            .field_name =
                ".security.data_access.asr.CloudDataflowResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535800996,
            .field_name = ".cloud.helix.storage.RowOptions.options",
            .type = ".cloud.helix.storage.RowOptions",
        },
        ExtensionDeclaration{
            .number = 535800997,
            .field_name =
                ".ads.monitoring.smart_alerting.ads_component_affiliation",
            .type = ".ads.monitoring.smart_alerting.Component",
        },
        ExtensionDeclaration{
            .number = 535800998,
            .field_name = ".madden.config.client.client_type_restrict",
            .type = ".madden.config.platform.PlatformClientType",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535800999,
            .field_name =
                ".security.data_access.asr.DocumentWarehouseResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801000,
            .field_name = ".superroot.knowledge.media.supported_vertical",
            .type = ".superroot.knowledge.media.RecosVertical.Enum",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801001,
            .field_name = ".engage.ui.support.insights.configurator.proto."
                          "configuration_metadata",
            .type = ".engage.ui.support.insights.configurator.proto."
                    "ConfigurationMetadata",
        },
        ExtensionDeclaration{
            .number = 535801002,
            .field_name =
                ".search_notifications.experiments.sno_experiment_flag_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801003,
            .field_name = ".security.data_access.asr.CloudSpannerResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801004,
            .field_name =
                ".security.data_access.asr.YouTubeChannelResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801005,
            .field_name = ".security.data_access.asr.YouTubeVideoResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801006,
            .field_name = ".indexing.embeddings.inference.udfs."
                          "SemanticUnderstandingInputWrapper.rene_input_type",
            .type = ".rene.InputData.InputType",
        },
        ExtensionDeclaration{
            .number = 535801007,
            .field_name = ".webmentions.test_stage",
            .type = ".webmentions.WebMentionsStageOption",
        },
        ExtensionDeclaration{
            .number = 535801008,
            .field_name = ".webmentions.lightweight_stage",
            .type = ".webmentions.WebMentionsStageOption",
        },
        ExtensionDeclaration{
            .number = 535801009,
            .field_name = ".webmentions.final_stage",
            .type = ".webmentions.WebMentionsStageOption",
        },
        ExtensionDeclaration{
            .number = 535801010,
            .field_name = ".ocean.EntityData.column",
            .type = ".ocean.EntityData.Column",
        },
        ExtensionDeclaration{
            .number = 535801011,
            .field_name =
                ".autosettings.disable_legacy_read_write_experiment_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801012,
            .field_name =
                ".security.data_access.asr.ResourceCacheResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801013,
            .field_name = ".gwiz.openapi.http_status_code",
            .type = "int32",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801014,
            .field_name =
                ".dremel.federated_format_field_options.is_partition_key",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801015,
            .field_name = ".security.data_access.asr.ChronicleResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801016,
            .field_name = ".adh.pipeline.adh_bq_clustering_order",
            .type = ".adh.pipeline.BqClusteringOrder",
        },
        ExtensionDeclaration{
            .number = 535801017,
            .field_name = ".security.data_access.asr.CloudIamResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801018,
            .field_name = ".ant_trails.event.trigger_output_only",
            .type = "bool",
            .reserved = true,
        },
        ExtensionDeclaration{
            .number = 535801019,
            .field_name = ".play.suggest.search_home.serving",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801020,
            .field_name = ".hamilton.required_role_information",
            .type = ".hamilton.RequiredRoleInformation",
        },
        ExtensionDeclaration{
            .number = 535801021,
            .field_name = ".id.metrics.semantics.dma52_exemption_bug",
            .type = "uint64",
        },
        ExtensionDeclaration{
            .number = 535801022,
            .field_name = ".gls.api.annotation",
            .type = ".gls.api.FieldAnnotation",
        },
        ExtensionDeclaration{
            .number = 535801023,
            .field_name =
                ".quality_qlogs_qsignals.tools_protogen.field_options",
            .type =
                ".quality_qlogs_qsignals.tools_protogen.ProtoGenFieldOptions",
        },
        ExtensionDeclaration{
            .number = 535801024,
            .field_name = ".devintel.datawarehouse.meta.entity_key",
            .type = ".devintel.datawarehouse.meta.Entity",
        },
        ExtensionDeclaration{
            .number = 535801025,
            .field_name = ".security.data_access.asr.CloudTPUResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801026,
            .field_name = ".blockchain_indexing.bigquery.schema_options",
            .type = ".blockchain_indexing.bigquery.SchemaFieldOptions",
        },
        ExtensionDeclaration{
            .number = 535801027,
            .field_name =
                ".security.data_access.asr.CloudMonitoringUptimeResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801028,
            .field_name = ".protolayout.annotations.hide_getter",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801029,
            .field_name = ".devintel.datawarehouse.meta.dimension",
            .type = ".devintel.datawarehouse.meta.Dimension",
        },
        ExtensionDeclaration{
            .number = 535801030,
            .field_name = ".qsignals.field",
            .type = ".qsignals.ProtoGenFieldOptions",
        },
        ExtensionDeclaration{
            .number = 535801031,
            .field_name = ".brunsviga.aggregation_operator",
            .type = ".brunsviga.MetricAggregationOperator.AggregationOperator"},
        ExtensionDeclaration{.number = 535801032,
                             .field_name = ".brunsviga.count",
                             .type = "int64"},
        ExtensionDeclaration{
            .number = 535801033,
            .field_name = ".ads.publisher.api.tangle.admanager.tools"
                          ".qubosschemagen.field",
            .type = ".ads.publisher.api.tangle.admanager.tools"
                    ".qubosschemagen.QubosSchemaFieldOptions",
        },
        ExtensionDeclaration{
            .number = 535801034,
            .field_name = ".protolayout.annotations.setter_visibility",
            .type = ".protolayout.annotations.MethodVisibility",
        },
        ExtensionDeclaration{
            .number = 535801035,
            .field_name =
                ".com.google.android.libraries.search.conformance.compliance."
                "dma.dataisolation.proto.pds_dma_justification",
            .type = ".com.google.android.libraries.search.conformance."
                    "compliance.dma.dataisolation.proto."
                    "DmaSafeJustification",
            .reserved = true,
        },
        ExtensionDeclaration{
            .number = 535801036,
            .field_name = ".nbu.paisa.merchants.merchantdevices.payload"
                          ".payload_json_fieldname",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801037,
            .field_name = ".apcamera.applicable_soc_range",
            .type = ".apcamera.ApplicableSocRange",
        },
        ExtensionDeclaration{
            .number = 535801038,
            .field_name = ".apcamera.param_rules",
            .type = ".apcamera.HyperParametersRules",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801039,
            .field_name = ".nest.thirdpartyservices.gateway.mixer_metadata",
            .type = ".nest.thirdpartyservices.gateway.MixerMetadataDescriptor",
        },
        ExtensionDeclaration{
            .number = 535801040,
            .field_name = ".prototiller.metadata.Field.is_open_enum",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801041,
            .field_name =
                ".support.guidance.chat.plx.v1.gse_data_localization_option",
            .type = ".support.guidance.chat.plx.v1.GseDataLocalizationOption",
        },
        ExtensionDeclaration{
            .number = 535801042,
            .field_name = ".security.data_access.asr."
                          "CloudMonitoringDashboardResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801043,
            .field_name = ".moneta.vendorgateway.outboundrequestconnector."
                          "migration.wrapped_message_origin",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801044,
            .field_name =
                ".labs.tailwind.logging.sensitive_content.sensitive_type",
            .type = ".labs.tailwind.logging.sensitive_content.SensitiveType",
        },
        ExtensionDeclaration{
            .number = 535801045,
            .field_name = ".devintel.datawarehouse.meta.metric",
            .type = ".devintel.datawarehouse.meta.Metric",
        },
        ExtensionDeclaration{
            .number = 535801046,
            .field_name = ".protolayout.annotations.repeated_field_size_range",
            .type = ".protolayout.annotations.IntRange",
        },
        ExtensionDeclaration{
            .number = 535801047,
            .field_name = ".purse.napa_field_options",
            .type = ".purse.NapaFieldOptions",
        },
        ExtensionDeclaration{
            .number = 535801048,
            .field_name = ".apps.terminus.command.command_extension_info",
            .type = ".apps.terminus.command.CommandExtensionInfo",
        },
        ExtensionDeclaration{
            .number = 535801049,
            .field_name = ".security.data_access.asr.LookerStudioResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801050,
            .field_name = ".corp.legal.nala.proto.api.ui.query_param",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801051,
            .field_name =
                ".ccc.hosted.growth.domainreg.include_in_audit_logging",
            .type = "bool"},
        ExtensionDeclaration{
            .number = 535801052,
            .field_name = ".security.data_access.asr.CloudNatResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801053,
            .field_name = ".kythe.lang_textproto.fileref_field_options",
            .type = ".kythe.lang_textproto.FilerefFieldOptions",
        },
        ExtensionDeclaration{
            .number = 535801054,
            .field_name = ".search.rendering.is_renderable_child",
            .type = "bool"},
        ExtensionDeclaration{
            .number = 535801055,
            .field_name = ".contentads.bidlab.tf_example_field",
            .type = ".contentads.bidlab.TfExampleFieldOptions",
        },
        ExtensionDeclaration{
            .number = 535801056,
            .field_name = ".signals_changefeed_options.field_options",
            .type = ".payments.consumer.datainfra.ps1."
                    "SignalsChangefeedFieldOptions"},
        ExtensionDeclaration{
            .number = 535801057,
            .field_name = ".apcamera.param_presentation",
            .type = ".apcamera.HyperParametersPresentation",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801058,
            .field_name = ".payments.logging.orchestration.metadata_source",
            .type = ".payments.logging.orchestration.MetadataSource",
        },
        ExtensionDeclaration{
            .number = 535801059,
            .field_name = ".net.google.protobuf.contrib.proto_checksum_plugin.exclude_"
                          "from_checksum",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801060,
            .field_name =
                ".supplychain.hw.dextr.proto.execution.cdm_field_name",
            .type = "string",
            .repeated = true,
            .reserved = true,
        },
        ExtensionDeclaration{
            .number = 535801061,
            .field_name =
                ".supplychain.hw.dextr.proto.execution.enabled_environment",
            .type = ".supplychain.hw.dextr.proto.execution.DextrEnvironment",
            .repeated = true,
            .reserved = true,
        },
        ExtensionDeclaration{
            .number = 535801062,
            .field_name = ".supplychain.hw.dextr.proto.execution.cdm_api_name",
            .type = ".supplychain.hw.dextr.proto.execution.CdmApi",
            .reserved = true,
        },
        ExtensionDeclaration{
            .number = 535801063,
            .field_name =
                ".supplychain.hw.dextr.proto.execution.attribute_group_type",
            .type =
                ".google.internal.hardware.supplychain.dextr.v1.AttributeGroup",
            .reserved = true,
        },
        ExtensionDeclaration{
            .number = 535801064,
            .field_name = ".hamilton.use_version_deletions",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801065,
            .field_name =
                ".security.data_access.asr.CloudWorkstationsResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801066,
            .field_name = ".groups_rosters_compass.roster_id_routing",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801067,
            .field_name = ".groups_rosters_compass.member_id_routing",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801068,
            .field_name = ".youtube_panama.proto.data_dependency_value_type",
            .type = ".youtube_panama.proto.DataDependency.Id.Type",
        },
        ExtensionDeclaration{
            .number = 535801069,
            .field_name =
                ".security.data_access.asr.AccessApprovalResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801070,
            .field_name = ".apps.dynamite.shared.allow_commit_timestamp",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801071,
            .field_name = ".supplychain.hw.dextr.proto.execution.dextr_field",
            .type = ".supplychain.hw.dextr.proto.execution.DextrProtoField",
        },
        ExtensionDeclaration{
            .number = 535801072,
            .field_name =
                ".ads.adsense.gammaemail.proto.email_history_website_url_tag",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801073,
            .field_name = ".astro_mdu.image_signal",
            .type = ".astro_mdu.ImageSignalOptions",
        },
        ExtensionDeclaration{
            .number = 535801074,
            .field_name = ".video_profiling_draper_spannerapi.field_metadata",
            .type = ".video_profiling_draper_spannerapi.FieldMetadata",
        },
        ExtensionDeclaration{
            .number = 535801075,
            .field_name = ".wireless.android.partner.bts.extraction.proto."
                          "unidirectional_relationship",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801076,
            .field_name = ".chrome.cros.proto.policyconfig.has_policy_mapping",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801077,
            .field_name = ".kids.infra.analytics.tools.codegen.generators."
                          "protos.field_options",
            .type = ".kids.infra.analytics.tools.codegen.generators.protos."
                    "ProcessedSourceFieldOptions",
        },
        ExtensionDeclaration{
            .number = 535801078,
            .field_name =
                ".security.data_access.asr.FinancialServicesResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801079,
            .field_name =
                ".security.data_access.asr.ChemistPolicyStoreResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801080,
            .field_name =
                ".search.now.discover.sfp.coreux.is_prefab_experimental",
            .type = "bool"},
        ExtensionDeclaration{
            .number = 535801081,
            .field_name = ".corp.legal.elm.matters.proto.value_type_mapping",
            .type = ".corp.legal.elm.matters.proto.ValueType.DataType"},
        ExtensionDeclaration{
            .number = 535801082,
            .field_name = ".production.resources.warehouse.hierarchy."
                          "demandcuration.service.proto.property_level",
            .type = ".production.resources.warehouse.hierarchy.demandcuration."
                    "service.proto.PropertyLevel",
        },
        ExtensionDeclaration{
            .number = 535801083,
            .field_name = ".security.data_access.asr.SpeechToTextResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801084,
            .field_name = ".security.data_access.asr.CloudNetsloResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801085,
            .field_name = ".geo.earth.proto.allowed_sources",
            .type = ".geo.earth.proto.CommandSource",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801086,
            .field_name =
                ".communication.synapse.analytics.data_validation.sql_template",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801087,
            .field_name =
                ".corp.servicedesk.contentstack.protos.serialized_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801088,
            .field_name = ".apps.dynamite.dynamite_daas_queue_key_routing",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801089,
            .field_name = ".apps.dynamite.dynamite_daas_queue_name_routing",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801090,
            .field_name = ".youtube_admin.admin_tools.review_queue.sdm_ui_"
                          "backend.dimension_type",
            .type = ".youtube_admin.admin_tools.review_queue.sdm_ui_backend."
                    "DimensionType",
        },
        ExtensionDeclaration{
            .number = 535801091,
            .field_name = ".security.data_access.asr.CloudLatencyResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801092,
            .field_name =
                ".com.google.android.libraries.search.conformance.compliance."
                "dma.dataisolation.proto.pds_dma_explanation",
            .type = "string",
            .reserved = true,
        },
        ExtensionDeclaration{
            .number = 535801093,
            .field_name = ".symphony.data.required",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801094,
            .field_name = ".contentads.proto_interface.enable_ccv_mutation",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801095,
            .field_name = ".commerce.datastore.counterfactual",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801096,
            .field_name = ".agsa.dma.dataisolation.justification",
            .type = ".agsa.dma.dataisolation.DmaSafeJustification",
        },
        ExtensionDeclaration{
            .number = 535801097,
            .field_name = ".agsa.dma.dataisolation.explanation",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801098,
            .field_name = ".apps_annotations.msg_type_name",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801099,
            .field_name = ".apps_annotations.enum_type_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801100,
            .field_name = ".nest.cloud.platform.uddm.translation.shdt.impl."
                          "proto.traitwrapper."
                          "shdt_uddm_trait_feature_map_value_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801101,
            .field_name = ".corp.xanadu.dsf.excluded",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801102,
            .field_name = ".security.data_access.asr.GoopsCloudResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801103,
            .field_name = ".search.neco.genbe.identifiers.id_type",
            .type = ".search.neco.genbe.identifiers.IdType",
        },
        ExtensionDeclaration{
            .number = 535801104,
            .field_name = ".annotation.gae_gws",
            .type = ".annotation.GaeGwsType",
        },
        ExtensionDeclaration{
            .number = 535801105,
            .field_name = ".symphony.data.dim_config",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801106,
            .field_name = ".security.data_access.asr.OidcOneResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801107,
            .field_name = ".corp.gteml.featurestore.product_ids",
            .type = "int32",
            .repeated = true},
        ExtensionDeclaration{
            .number = 535801108,
            .field_name = ".google.cloud.bigquery.storage.v1.uint64_as_integer",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801109,
            .field_name = ".dremel.ddl.allow_alter_array_operator",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801110,
            .field_name = ".security.data_access.asr.CloudDlpResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801111,
            .field_name = ".devintel.datawarehouse.meta.key_options",
            .type = ".devintel.datawarehouse.meta.KeyOptions",
        },
        ExtensionDeclaration{
            .number = 535801112,
            .field_name = ".contentads.bidlab.reid",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801113,
            .field_name = ".corp_networking_netprobe.field_flag",
            .type = ".corp_networking_netprobe.Flag",
        },
        ExtensionDeclaration{
            .number = 535801114,
            .field_name = ".ga.gafe4.core.actions.index.preload_options",
            .type = ".ga.gafe4.core.actions.index.PreloadOptions",
        },
        ExtensionDeclaration{
            .number = 535801115,
            .field_name =
                ".apps.dynamite.dynamite_pantheon_project_number_routing",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801116,
            .field_name = ".devintel.datawarehouse.meta.catalog_options",
            .type = ".devintel.datawarehouse.meta.CatalogOptions",
        },
        ExtensionDeclaration{
            .number = 535801117,
            .field_name = ".search.apps.s4a_tunable_param_opts",
            .type = ".search.apps.TunableParamOptions",
        },
        ExtensionDeclaration{
            .number = 535801118,
            .field_name =
                ".security.data_access.asr.CloudNetControlResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801119,
            .field_name = ".protoplan.field_configuration",
            .type = ".protoplan.FieldConfiguration",
        },
        ExtensionDeclaration{
            .number = 535801120,
            .field_name = ".google.ads.admanager.annotations.pql_column_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801121,
            .field_name =
                ".google.ads.admanager.annotations.client_filter_field_path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801122,
            .field_name = ".google.cloud.securitycenter.v1.ccc",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801123,
            .field_name = ".google.cloud.securitycenter.v2.ccc",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801124,
            .field_name =
                ".supplychain.hw.cdm.proto.mutations.resource_field_options",
            .type = ".supplychain.hw.cdm.proto.mutations.ResourceFieldOptions",
        },
        ExtensionDeclaration{
            .number = 535801125,
            .field_name =
                ".platforms.datacenter.turnups.proto.TaskInfo.partial_key",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801126,
            .field_name = ".nest.cloud.platform.uddm.translation.shdt.impl."
                          "proto.traitwrapper."
                          "shdt_matter_cluster_field_mapping",
            .type = ".nest.cloud.platform.uddm.translation.shdt.impl."
                    "proto.traitwrapper.ExtendedClusterMapping",
        },
        ExtensionDeclaration{
            .number = 535801127,
            .field_name = ".symphony.data.extract_from",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801128,
            .field_name = ".security.data_access.asr."
                          "CloudMonitoringServiceResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801129,
            .field_name = ".xaa_schema.g1_subscription_mapping",
            .type = ".xaa_schema.G1SubscriptionMappingExtension",
        },
        ExtensionDeclaration{
            .number = 535801130,
            .field_name = ".security.data_access.asr."
                          "CloudMonitoringAlertPolicyResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801131,
            .field_name = ".engage.pipelines.sales.assignment.dataingestion."
                          "protos.output.gcs.appdev_use_fresh_snapshot",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801132,
            .field_name = ".search.next.magi.magi_field_options",
            .type = ".search.next.magi.FieldOptions",
        },
        ExtensionDeclaration{
            .number = 535801133,
            .field_name = ".security.data_access.asr."
                          "CloudMonitoringServiceMonitoringResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801134,
            .field_name = ".merchant.identity.attribute_type",
            .type = ".merchant.identity.AttributeId.Type",
        },
        ExtensionDeclaration{
            .number = 535801135,
            .field_name = ".apps_annotations.filter_options",
            .type = ".apps_annotations.FilterOptions",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801136,
            .field_name = ".google.ads.admanager.annotations.resource_name_for",
            .type = ".google.ads.admanager.annotations.ResourceNameFor",
        },
        ExtensionDeclaration{
            .number = 535801137,
            .field_name = ".apps.dynamite."
                          "dynamite_customer_resource_name_routing",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801138,
            .field_name = ".search.llmresult.is_llm_result",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801139,
            .field_name = ".logs.proto.apps_dynamite.insights_parameter_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801140,
            .field_name = ".communication.rtctools.proto.spanner.field_options",
            .type =
                ".communication.rtctools.proto.spanner.RowWrapperFieldOptions",
        },
        ExtensionDeclaration{
            .number = 535801141,
            .field_name = ".cloud.dataform.blackbox.method",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801142,
            .field_name = ".security.data_access.asr."
                          "CloudArmorAdaptiveProtectionResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801143,
            .field_name = ".play_analytics_pms.pms_event_type",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801144,
            .field_name = ".youtube_panama.foreign_key_type",
            .type = ".youtube_panama.SemanticGroupForeignKey.Type",
        },
        ExtensionDeclaration{
            .number = 535801145,
            .field_name = ".lamda.schema.cpp_supported",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801146,
            .field_name = ".cloud.helix.experiment",
            .type = ".cloud.helix.Experiment",
        },
        ExtensionDeclaration{
            .number = 535801147,
            .field_name = ".dremel.experiment",
            .type = ".cloud.helix.Experiment",
        },
        ExtensionDeclaration{
            .number = 535801148,
            .field_name = ".commerce.metrics.platform.field_metadata",
            .type = ".commerce.metrics.platform.FieldMetadata",
        },
        ExtensionDeclaration{
            .number = 535801149,
            .field_name = ".bigquery.platform.user.key",
            .type = ".bigquery.platform.user.Key",
        },
        ExtensionDeclaration{
            .number = 535801150,
            .field_name = ".communication.rtctools.proto.signalsapi.dcrpc_"
                          "signaling_logging_annotations",
            .type = ".communication.rtctools.proto.signalsapi.MethodLogExt",
        },
        ExtensionDeclaration{
            .number = 535801151,
            .field_name = ".drive_compass.extract_map_key",
            .type = ".drive_compass.EntityId.EntityIdType",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801152,
            .field_name = ".dremel.ddl.allowed_in_external_statement",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801153,
            .field_name = ".dremel.ddl.only_in_external_statement",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801154,
            .field_name = ".quality_fringe.sudo.example_input_metadata",
            .type = ".quality_fringe.sudo.ExampleInputMetadata"},
        ExtensionDeclaration{
            .number = 535801155,
            .field_name = ".quality_fringe.sudo.example_output_metadata",
            .type = ".quality_fringe.sudo.ExampleOutputMetadata"},
        ExtensionDeclaration{
            .number = 535801156,
            .field_name = ".language_labs.genai.generative_service_annotations."
                          "accounting_behavior",
            .type = ".language_labs.genai.generative_service_annotations."
                    "AccountingBehavior",
        },
        ExtensionDeclaration{
            .number = 535801157,
            .field_name = ".prothon.pull",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801158,
            .field_name = ".prothon.mutate",
            .type = ".prothon.Mutate",
        },
        ExtensionDeclaration{
            .number = 535801159,
            .field_name = ".symphony.data.dim",
            .type = ".symphony.data.Dim",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801160,
            .field_name = ".security.data_access.asr."
                          "CloudMonitoringMetricsScopeResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801161,
            .field_name = ".beto.dimensions.field_id",
            .type = ".beto.dimensions.FieldId",
        },
        ExtensionDeclaration{
            .number = 535801162,
            .field_name = ".beto.semantics.usage_semantic_type",
            .type = ".beto.semantics.UsageSemanticType",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801163,
            .field_name = ".quality_rankembed_release.rankembed_task_id",
            .type = ".quality_rankembed_release.TaskId",
        },
        ExtensionDeclaration{
            .number = 535801164,
            .field_name = ".nova.set_to_commit_timestamp",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801165,
            .field_name = ".via.abcd_attribute_id",
            .type = ".via.AbcdAttributeId.Id",
        },
        ExtensionDeclaration{
            .number = 535801166,
            .field_name = ".logs.proto.wireless.android.stats.platform."
                          "westworld.sanitizer",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801167,
            .field_name =
                ".security.data_access.asr.CloudRecommenderResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801168,
            .field_name = ".ccc.hosted.domain.checkpoint_type",
            .type = ".ccc.hosted.domain.CheckpointType",
        },
        ExtensionDeclaration{
            .number = 535801169,
            .field_name = ".com.google.fitbit.platform.device.protocol.proto."
                          "generator_options",
            .type = ".com.google.fitbit.platform.device.protocol.proto."
                    "ProtocolGeneratorOptions",
        },
        ExtensionDeclaration{
            .number = 535801170,
            .field_name = ".protolayout.annotations.required_not_enforced",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801171,
            .field_name = ".growth.search.marketing.platform.app.is_effort_id",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801172,
            .field_name = ".google.cloud.aiplatform.common.gen_ai_artifact_"
                          "type",
            .type = ".google.cloud.aiplatform.common.FieldAnnotations."
                    "GenAiArtifactType",
        },
        ExtensionDeclaration{
            .number = 535801173,
            .field_name = ".cloud.kubernetes.proto.owner_team",
            .type = ".cloud.kubernetes.proto.FieldOwnerTeam",
        },
        ExtensionDeclaration{
            .number = 535801174,
            .field_name =
                ".security.data_access.asr.CloudMlMetadataResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801175,
            .field_name =
                ".security.data_access.asr.PsmSubscriptionResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801176,
            .field_name = ".autosettings.compiler_type",
            .type = ".autosettings.CompilerType",
        },
        ExtensionDeclaration{
            .number = 535801177,
            .field_name = ".apcamera.tdm_rules",
            .type = ".apcamera.TdmParametersRules",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801178,
            .field_name =
                ".security.data_access.asr.CloudInterconnectResource.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801179,
            .field_name = ".cloud.cluster.manager.experiment_annotations",
            .type = ".cloud.cluster.manager.ExperimentAnnotations",
        },
        ExtensionDeclaration{
            .number = 535801180,
            .field_name = ".mensa.keyspec",
            .type = ".mensa.KeyspecOptions",
        },
        ExtensionDeclaration{
            .number = 535801181,
            .field_name = ".annotation.identifier_type",
            .type = ".drp.datamodel.Identifier.Type",
        },
        ExtensionDeclaration{
            .number = 535801182,
            .field_name = ".chrome.cros.capai.cap_ai_request_field_decorators",
            .type = ".chrome.cros.capai.CapAIRequestProtoFieldDecorators",
        },
        ExtensionDeclaration{
            .number = 535801183,
            .field_name =
                ".ccc.hosted.store.ConditionalScopesAnnotation.cel_field_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801184,
            .field_name = ".ccc.hosted.store.ConditionalScopesAnnotation.cel_"
                          "field_operator",
            .type = ".ccc.hosted.store.ConditionalScopesOperator",
        },
        // go/keep-sorted end
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_EXTDECL_FIELD_OPTIONS_H__
