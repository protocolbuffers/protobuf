#ifndef GOOGLE_PROTOBUF_EXTDECL_ENUM_VALUE_OPTIONS_H__
#define GOOGLE_PROTOBUF_EXTDECL_ENUM_VALUE_OPTIONS_H__

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
    kEnumValueOptionExtensions = {
        // go/keep-sorted start numeric=yes block=yes
        ExtensionDeclaration{
            .number = 525000000,
            .field_name = ".humboldt.mnemonic",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000001,
            .field_name = ".video_core.product",
            .type = ".video_core.Product",
        },
        ExtensionDeclaration{
            .number = 525000002,
            .field_name = ".vr.perception.spatial_audio.AudioTypeExtension."
                          "audio_type_info",
            .type = ".vr.perception.spatial_audio.AudioTypeExtension",
        },
        ExtensionDeclaration{
            .number = 525000003,
            .field_name = ".car.planner_evaluation.DollarTreeFeature.data_type",
            .type = ".car.planner_evaluation.DollarTreeFeature.DataType",
        },
        ExtensionDeclaration{
            .number = 525000004,
            .field_name = ".ga.gafe4.admin.actions.changehistory."
                          "EntityTypeDescriptor.ext",
            .type = ".ga.gafe4.admin.actions.changehistory."
                    "EntityTypeDescriptor",
        },
        ExtensionDeclaration{
            .number = 525000005,
            .field_name = ".google.internal.mothership.magi.v1.client_config",
            .type = ".google.internal.mothership.magi.v1.ClientConfig",
        },
        ExtensionDeclaration{
            .number = 525000010,
            .field_name = ".apps.templates.template_enum_value",
            .type = ".apps.templates.TemplateEnumValueOptions",
        },
        ExtensionDeclaration{
            .number = 525000011,
            .field_name = ".nova.logs.type_config",
            .type = ".nova.logs.ProtoTypeConfig",
        },
        ExtensionDeclaration{
            .number = 525000012,
            .field_name = ".guitar.workflow.status_type",
            .type = ".guitar.workflow.StatusType",
        },
        ExtensionDeclaration{
            .number = 525000013,
            .field_name = ".travel.ads.analysis.tafmd",
            .type = ".travel.ads.analysis.TravelAdsFormatMetadata",
        },
        ExtensionDeclaration{
            .number = 525000014,
            .field_name = ".cloud.cluster.efficiency.data.common.proto."
                          "gce_ssd_assignment",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000015,
            .field_name = ".id.metrics.semantics.cdw.magiceye_setting_name",
            .type = ".magiceye.dwh.product_specific.SettingName.Enum",
        },
        ExtensionDeclaration{
            .number = 525000016,
            .field_name = ".id.metrics.semantics.cdw.magiceye_state",
            .type = ".magiceye.dwh.product_specific.Setting.State.Enum",
        },
        ExtensionDeclaration{
            .number = 525000017,
            .field_name = ".id.metrics.semantics.cdw.brahms_promo_ui_type",
            .type = ".privacy.consent.PromoUiType",
        },
        ExtensionDeclaration{
            .number = 525000018,
            .field_name =
                ".f1.test.EnumValueAnnotations.test_message_annotation",
            .type = ".f1.test.EnumValueAnnotations",
        },
        ExtensionDeclaration{
            .number = 525000019,
            .field_name =
                ".f1.test.EnumValueAnnotations.test_string_annotation",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000020,
            .field_name = ".apres.prod_universe_enum_metadata",
            .type = ".apres.ProdUniverseEnumMetadata",
        },
        ExtensionDeclaration{
            .number = 525000021,
            .field_name =
                ".moneta.orchestration2.ui.internal.script.persist_operation",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000022,
            .field_name = ".drive_compass.routing_info_extension",
            .type = "int32",
        },
        ExtensionDeclaration{
            .number = 525000023,
            .field_name = ".drive_compass.compound_annotation_type",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000024,
            .field_name = ".ads_integrity.data.knowledgeset.severity",
            .type = ".ads_integrity.data.knowledgeset."
                    "AnalyzeRestrictionsResponse.RestrictionResponse.Severity",
        },
        ExtensionDeclaration{
            .number = 525000025,
            .field_name = ".adx.enum_value_visibility",
            .type = ".adx.Visibility",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000026,
            .field_name = ".petacat.classification_type",
            .type = ".petacat.PetacatTFDefaultOptions.ClassificationType",
        },
        ExtensionDeclaration{
            .number = 525000027,
            .field_name = ".ccai_insights.internal.metric_sql_annotation.enum_"
                          "value_annotation",
            .type = ".ccai_insights.internal.metric_sql_annotation."
                    "MetricSqlAnnotation",
        },
        ExtensionDeclaration{
            .number = 525000028,
            .field_name = ".bizbuilder.backend.geo_policy_dimension_id",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000029,
            .field_name = ".bizbuilder.backend.non_appealable",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000030,
            .field_name = ".bizbuilder.backend.non_notifiable",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000031,
            .field_name = ".quality_timebased.safe_to_show_to_external_users",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000032,
            .field_name = ".play.console.ui.api.pages.escalator_queue",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000033,
            .field_name = ".podcasts.corpus.ingestion.proto.error_metadata",
            .type = ".podcasts.corpus.ingestion.proto.ErrorMetadata",
        },
        ExtensionDeclaration{
            .number = 525000034,
            .field_name = ".cloud.containers.artifacts.metadata.action_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000035,
            .field_name = ".net_inventory.options.hide_from_ocellus",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000036,
            .field_name = ".identity_checkpoint.template_path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000037,
            .field_name = ".merchant.termsofservice.version_product_type",
            .type = ".merchant.termsofservice.ProductType",
        },
        ExtensionDeclaration{
            .number = 525000038,
            .field_name = ".merchant.termsofservice.stable_link",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000039,
            .field_name = ".merchant.termsofservice.order",
            .type = "uint32",
        },
        ExtensionDeclaration{
            .number = 525000040,
            .field_name = ".identity_verificationriskassessment.group_id",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000041,
            .field_name =
                ".bizbuilder.backend.preferred_geo_policy_dimension_id",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000042,
            .field_name = ".recaptcha.property_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000043,
            .field_name = ".support_automation.discovery.alias",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000044,
            .field_name =
                ".quality_featuresafety.historical_decisions_in_qrewrite",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000045,
            .field_name = ".apres.cloud_universe_enum_metadata",
            .type = ".apres.CloudUniverseEnumMetadata",
        },
        ExtensionDeclaration{
            .number = 525000046,
            .field_name = ".organiccommerciality.lens_sic_score_threshold",
            .type = "int32",
        },
        ExtensionDeclaration{
            .number = 525000047,
            .field_name =
                ".cloud.ux.api.contentmanagement.common.template_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000048,
            .field_name = ".car.PlannerComponent.tags",
            .type = ".car.PlannerComponent.Tag",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000049,
            .field_name = ".corp.legal.elm.matters.services.workflow.proto.osw_"
                          "resource_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000050,
            .field_name = ".music.corpus.infra.bass.supported",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000051,
            .field_name = ".payments.dataplatform.metadata.string_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000052,
            .field_name = ".payments.dataplatform.metadata.supported_scenarios",
            .type = ".payments.dataplatform.metadata.ScenarioType",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000053,
            .field_name = ".youtube_admintools_yurt.review.api.shared."
                          "extensions.display_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000054,
            .field_name = ".video_schemas.wipeout_entity_namespace",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000055,
            .field_name =
                ".ads_publisher_quality.policy.DemandSegment.search_types",
            .type = ".ads_base.SearchType",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000056,
            .field_name =
                ".enterprise.crm.monet.cdm.annotations.enums.cdm_attribute",
            .type = ".enterprise.crm.monet.cdm.annotations.CdmAttribute",
        },
        ExtensionDeclaration{
            .number = 525000057,
            .field_name = ".xaa_features.meet_feature",
            .type = ".xaa_features.MeetFeature.Enum",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000058,
            .field_name =
                ".google.cloud.advisorynotifications.v1main.mandatory",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000059,
            .field_name = ".cloud.serverless.infrastructure.controlplane.api."
                          "proto.error.code",
            .type = ".google.rpc.Code",
        },
        ExtensionDeclaration{
            .number = 525000060,
            .field_name = ".xaa_features.chat_feature",
            .type = ".xaa_features.ChatFeature.Enum",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000061,
            .field_name = ".identity_httpauthapi.canonical_code",
            .type = ".util.error.Code",
        },
        ExtensionDeclaration{
            .number = 525000062,
            .field_name = ".serverless.region_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000063,
            .field_name = ".robotics.dataops.tools.display_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000064,
            .field_name =
                ".production.resources.mlresources.common.proto.extensions."
                "custom_string_value",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000065,
            .field_name = ".quality_orbit.asteroid_belt.broad_intent",
            .type = ".intent_brain.BroadIntent",
        },
        ExtensionDeclaration{
            .number = 525000066,
            .field_name = ".ghs.enable_goldfish_write",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000067,
            .field_name = ".liteoptions.testing.enum_value_option",
            .type = "int32",
        },
        ExtensionDeclaration{
            .number = 525000068,
            .field_name = ".unified_drilldown.block_placement_options",
            .type = ".unified_drilldown.BlockPlacementOptions",
        },
        ExtensionDeclaration{
            .number = 525000069,
            .field_name = ".assistant.lamda.annotators.usecase_ontology.label",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000070,
            .field_name =
                ".assistant.lamda.annotators.usecase_ontology.description",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000071,
            .field_name = ".assistant.lamda.annotators.usecase_ontology.level1",
            .type = ".assistant.lamda.annotators.usecase_ontology.Level1."
                    "UseCasePurpose",
        },
        ExtensionDeclaration{
            .number = 525000072,
            .field_name = ".ghs.require_fprint",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000073,
            .field_name = ".ghs.require_business_photos",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000074,
            .field_name = ".privacy.context.ondevice.privacy_requirements",
            .type = ".privacy.context.ondevice.PrivacyRequirements",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000075,
            .field_name = ".unified_drilldown.cgi_arg_options",
            .type = ".unified_drilldown.CgiArgOptions",
        },
        ExtensionDeclaration{
            .number = 525000076,
            .field_name = ".via.features.on_demand_fetched",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000077,
            .field_name = ".growth.search.marketing.platform.display_options",
            .type = ".growth.search.marketing.platform.DisplayOptions",
        },
        ExtensionDeclaration{
            .number = 525000078,
            .field_name = ".crash_analysis.reporting.ipc.field_id_options",
            .type = ".crash_analysis.reporting.ipc.FieldIDOptions",
        },
        ExtensionDeclaration{
            .number = 525000079,
            .field_name = ".quality_featuresafety.log_availability_date",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000080,
            .field_name = ".wireless.android.camera.syshealth.analysis."
                          "regression.proto.canonical",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000081,
            .field_name =
                ".knowledge.verticals.weather.modeling.protos.legacy_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000082,
            .field_name = ".knowledge.verticals.weather.modeling.protos.units",
            .type = ".knowledge.verticals.weather.modeling.protos.Units",
        },
        ExtensionDeclaration{
            .number = 525000083,
            .field_name = ".cloud.security.assurant.compliancecenter.proto."
                          "capability_group_does_not_exist",
            .type = "bool"},
        ExtensionDeclaration{
            .number = 525000084,
            .field_name =
                ".corp.legal.elm.portal.services.matters.date_range_value",
            .type = ".corp.legal.elm.portal.services.matters."
                    "RelativeDateRangeValue"},
        ExtensionDeclaration{
            .number = 525000085,
            .field_name = ".security_tarsier.QueryParameterMetadata.value",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000086,
            .field_name = ".droidguard.decode_after",
            .type = ".droidguard.Signal",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000087,
            .field_name =
                ".knowledge.verticals.weather.modeling.protos.variable_id",
            .type = "int32",
        },
        ExtensionDeclaration{.number = 525000088,
                             .field_name = ".soji.state_metadata",
                             .type = ".soji.StateMetadata"},
        ExtensionDeclaration{
            .number = 525000089,
            .field_name = ".wmconsole.suit.robots.content_issue_status",
            .type = ".wmconsole.suit.robots.ContentIssueStatus",
        },
        ExtensionDeclaration{
            .number = 525000090,
            .field_name = ".video.youtube.shorts.discovery.exploration.corpus_"
                          "diverted.automation.proto.operator_rep",
            .type = "string"},
        ExtensionDeclaration{
            .number = 525000091,
            .field_name = ".concepts.encoder_custom_string",
            .type = "string",
        },
        ExtensionDeclaration{.number = 525000092,
                             .field_name = ".usps.packet_events.event_message",
                             .type = "string"},
        ExtensionDeclaration{
            .number = 525000093,
            .field_name = ".notifications.preferences.url_path_component",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000094,
            .field_name = ".superroot.travel_rich_content.is_osrp",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000095,
            .field_name = ".ads.registration_lib.source_type_attributes",
            .type = ".ads.registration_lib.ConfigSourceTypeAttributes"},
        ExtensionDeclaration{
            .number = 525000096,
            .field_name =
                ".cloud.boq.client.alloydbproducer.config.location_name",
            .type = "string"},
        ExtensionDeclaration{.number = 525000097,
                             .field_name = ".ase.draco.category_options",
                             .type = ".ase.draco.LabelCategoryOptions"},
        ExtensionDeclaration{
            .number = 525000098,
            .field_name = ".oncall.oncall_error",
            .type = ".oncall.ErrorTypeOptions",
        },
        ExtensionDeclaration{
            .number = 525000099,
            .field_name =
                ".cloud_cluster.instance_manager.tpu_isolation_version",
            .type = ".cloud_vmm_proto.PciPassthroughTpuSpec.IsolationVersion",
        },
        ExtensionDeclaration{
            .number = 525000100,
            .field_name =
                ".play_analytics_civita.associated_source_data_status",
            .type = ".play_analytics_civita.CivitaEnum.SourceDataStatus",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000101,
            .field_name =
                ".com.google.nbu.paisa.social.campaign.clp.common.config."
                "BritanniaCodeValidationConfig.variant_metadata",
            .type = ".com.google.nbu.paisa.social.campaign.clp.common.config."
                    "BritanniaCodeValidationConfig.VariantMetadata",
        },
        ExtensionDeclaration{
            .number = 525000102,
            .field_name = ".notifications.platform.execution.storage."
                          "estimations.filter_code",
            .type = ".gamma_history.FilteredOutData.FilterStatus",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000103,
            .field_name = ".notifications.platform.execution.storage."
                          "estimations.successor",
            .type = ".notifications.platform.execution.storage.estimations."
                    "EvaluationEvent",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000104,
            .field_name =
                ".cloud.ux.api.contentmanagement.page.revision_tag_type_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000105,
            .field_name =
                ".privacy.signal_propagation_monitoring.GapCause.gap_metadata",
            .type =
                ".privacy.signal_propagation_monitoring.GapCause.GapMetadata",
        },
        ExtensionDeclaration{
            .number = 525000106,
            .field_name = ".commerce.bizbuilder.datameasurements.viewmanager."
                          "proto.input_source_metadata",
            .type = ".commerce.bizbuilder.datameasurements.viewmanager.proto."
                    "InputSourceMetadata",
        },
        ExtensionDeclaration{
            .number = 525000107,
            .field_name = ".cloud.ai.platform.common.resource_identity",
            .type = ".cloud.ai.platform.common.ResourceIdentityOption",
        },
        ExtensionDeclaration{
            .number = 525000108,
            .field_name = ".platforms_syshealth.proto.ActionDirectiveMetadata."
                          "directive_info",
            .type = ".platforms_syshealth.proto.ActionDirectiveMetadata",
        },
        ExtensionDeclaration{
            .number = 525000109,
            .field_name = ".transconsole.localization.keymaker.proto."
                          "localizationlaunch.display_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000110,
            .field_name = ".perftools.gwp.category_options",
            .type = ".perftools.gwp.CategoryOptions",
        },
        ExtensionDeclaration{
            .number = 525000111,
            .field_name = ".voice.processing_state_type",
            .type = ".voice.ProcessingStateType",
        },
        ExtensionDeclaration{
            .number = 525000112,
            .field_name = ".communication.gtp.product_options",
            .type = ".communication.gtp.ProductOptions",
        },
        ExtensionDeclaration{
            .number = 525000113,
            .field_name = ".android.os.statsd.statsd_version_enforcement_enum",
            .type = ".android.os.statsd.StatsdVersionEnforcement",
        },
        ExtensionDeclaration{
            .number = 525000114,
            .field_name = ".play_analytics_civita.number_of_days",
            .type = "int32",
        },
        ExtensionDeclaration{
            .number = 525000115,
            .field_name = ".cloud.security.assurant.compliancecenter.proto."
                          "service_without_gpl_id",
            .type = "bool"},
        ExtensionDeclaration{
            .number = 525000116,
            .field_name =
                ".ccc.hosted.store.constants.EssAnnotations.ess_attribute",
            .type = ".ccc.hosted.store.ess.api.SettingId.Attribute",
        },
        ExtensionDeclaration{
            .number = 525000117,
            .field_name =
                ".ccc.hosted.store.constants.EssAnnotations.proto_type_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000118,
            .field_name =
                ".ccc.hosted.store.constants.EssAnnotations.ess_mva_prefix",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000119,
            .field_name = ".xaa_features.workspace_growth_feature",
            .type = ".xaa_features.WorkspaceGrowthFeature.Enum",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 525000120,
            .field_name =
                ".ccc.hosted.commerce.console.proto.promotion.category_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000121,
            .field_name = ".ccc.hosted.store.constants.EssEntityTypeAnnotation."
                          "ess_entity_namespace",
            .type = ".ccc.hosted.store.ess.api.EntityNamespace",
        },
        ExtensionDeclaration{
            .number = 525000122,
            .field_name = ".humboldt.metadata",
            .type = ".humboldt.LineageIdMetadata",
        },
        ExtensionDeclaration{
            .number = 525000123,
            .field_name = ".google.api.enum_value_api_versions",
            .type = ".google.api.ApiVersionsRule",
        },
        ExtensionDeclaration{
            .number = 525000124,
            .field_name = ".supplychain.diagnostics.description",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000125,
            .field_name = ".ads_integrity.proto.label_namespace",
            .type = ".ads_integrity.proto.LabelNamespace.Enum",
        },
        ExtensionDeclaration{
            .number = 525000126,
            .field_name = ".travel.enterprise.flights.versions.oe.config",
            .type = ".travel.enterprise.flights.versions.oe.OfferEngineVersion",
        },
        ExtensionDeclaration{
            .number = 525000127,
            .field_name = ".recaptcha.recaptcha_backend_info",
            .type = ".recaptcha.BackendInfo",
        },
        ExtensionDeclaration{
            .number = 525000128,
            .field_name = ".ghs.allows_whatsapp_messaging",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 525000129,
            .field_name = ".id.hijackingintervention.af_action",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 525000130,
            .field_name = ".travel.enterprise.flights.versions.prs"
                          ".config",
            .type = ".travel.enterprise.flights.versions.prs."
                    "ParameterResolutionVersion",
        },
        ExtensionDeclaration{
            .number = 525000131,
            .field_name = ".car.deep_nets.config",
            .type = ".car.deep_nets.ModelFamilyConfig",
        },
        ExtensionDeclaration{
            .number = 525075384,
            .field_name = ".id.metrics.semantics.cdw.consent_purpose",
            .type = ".privacy.consent.ConsentPurpose",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801012,
            .field_name = ".incident_response_management.incident_phase",
            .type = ".incident_response_management.IncidentPhase",
        },
        ExtensionDeclaration{
            .number = 535801013,
            .field_name =
                ".production.resources.mlresources.common.proto.extensions."
                "resource_translation",
            .type = ".production.resources.mlresources.common.proto."
                    "ResourceTranslation",
        },
        ExtensionDeclaration{
            .number = 535801014,
            .field_name = ".quality_featuresafety.historical_decisions_config",
            .type = ".quality_featuresafety.HistoricalDecisionsConfig",
        },
        ExtensionDeclaration{
            .number = 535801015,
            .field_name = ".engage.ui.support.insights.frontend.join_info",
            .type = ".engage.ui.support.insights.frontend.JoinInfo",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801016,
            .field_name = ".engage.ui.support.insights.frontend."
                          "selectability_info",
            .type = ".engage.ui.support.insights.frontend.Source",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801017,
            .field_name = ".gaia_auth.hide_in_config_insights",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801018,
            .field_name = ".wireless_android_play_analytics.mezzo_namespace",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801019,
            .field_name =
                ".nbu.paisa.vendorgateway.banks.upi.refund_reason_code",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801020,
            .field_name = ".security_tarsier.protoext.Alias.label",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801021,
            .field_name = ".ads_enums.served_asset_field_type",
            .type = ".ads_enums."
                    "ServedAssetFieldTypePB.Enum",
        },
        ExtensionDeclaration{
            .number = 535801022,
            .field_name =
                ".unified_drilldown.top_refinements_bar_state_options",
            .type = ".unified_drilldown.TopRefinementsBarStateOptions",
        },
        ExtensionDeclaration{
            .number = 535801023,
            .field_name = ".recaptcha.user_agent_regex",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801024,
            .field_name =
                ".moneta.orchestration2.ui.internal.script.event_rule_required",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801025,
            .field_name = ".id.metrics.semantics.cdw.consent_product_surface",
            .type = ".privacy.consent.ProductSurface",
        },
        ExtensionDeclaration{
            .number = 535801026,
            .field_name = ".logs.proto.generative_language.classifier_config",
            .type = ".logs.proto.generative_language.ClassifierConfig",
        },
        ExtensionDeclaration{
            .number = 535801027,
            .field_name = ".wireless.android.telemetry.stability.clusters."
                          "version_rollout",
            .type =
                ".wireless.android.telemetry.stability.clusters.VersionRollout",
        },
        ExtensionDeclaration{
            .number = 535801028,
            .field_name = ".id.metrics.semantics.cdw.consent_decision",
            .type = ".identity_consent.Decision",
        },
        ExtensionDeclaration{
            .number = 535801029,
            .field_name = ".id.metrics.semantics.cdw.ogm_ui_color_theme",
            .type = ".onegoogle.logging.mobile.OneGoogleMobileConsentEvents."
                    "Metadata.ColorTheme",
        },
        ExtensionDeclaration{
            .number = 535801030,
            .field_name =
                ".id.metrics.semantics.cdw.identity_consent_ui_color_theme",
            .type = ".identity_consent.UiParameters.ColorTheme",
        },
        ExtensionDeclaration{
            .number = 535801031,
            .field_name = ".id.metrics.semantics.cdw.ogm_ui_dismissibility",
            .type = ".onegoogle.logging.mobile.OneGoogleMobileConsentEvents."
                    "Metadata.Dismissibility",
        },
        ExtensionDeclaration{
            .number = 535801032,
            .field_name =
                ".id.metrics.semantics.cdw.identity_consent_ui_dismissibility",
            .type = ".identity_consent.UiParameters.Dismissibility",
        },
        ExtensionDeclaration{
            .number = 535801033,
            .field_name = ".id.metrics.semantics.cdw.ogm_ui_render_type",
            .type = ".onegoogle.logging.mobile.OneGoogleMobileConsentEvents."
                    "Metadata.ConsentRenderType",
        },
        ExtensionDeclaration{
            .number = 535801034,
            .field_name = ".id.metrics.semantics.cdw.consent_ui_interaction",
            .type = ".identity_consent.UiInteraction",
        },
        ExtensionDeclaration{
            .number = 535801035,
            .field_name =
                ".superroot.apis.fulfillment.finalizer_corresponding_id",
            .type = ".superroot.apis.fulfillment.FinalizerCorrespondingId",
        },
        ExtensionDeclaration{
            .number = 535801036,
            .field_name = ".superroot.magi.only_uses_qrs",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801037,
            .field_name =
                ".searchanalytics.policy.proto.policy_status_priority",
            .type = "int32",
        },
        ExtensionDeclaration{
            .number = 535801038,
            .field_name = ".cloud.ux.api.webplatform.cgcpage.ve_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801039,
            .field_name = ".personalization_suggest.is_in_olympus_scope",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801040,
            .field_name = ".cloud_interconnect.continent_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801041,
            .field_name =
                ".id.metrics.semantics.cdw.compliance_controls_setting_name",
            .type = ".compliance_controls.CorePlatformService",
        },
        ExtensionDeclaration{
            .number = 535801042,
            .field_name = ".assistant.lamda.annotators.usecase_ontology.level2",
            .type =
                ".assistant.lamda.annotators.usecase_ontology.Level2.UserValue",
        },
        ExtensionDeclaration{
            .number = 535801043,
            .field_name =
                ".commerce.bizbuilder.recommendations.proto.proto_field_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801044,
            .field_name = ".commerce.bizbuilder.recommendations.proto.key_type",
            .type =
                ".commerce.bizbuilder.recommendations.proto.CorrelationKeyType",
        },
        ExtensionDeclaration{
            .number = 535801045,
            .field_name = ".id.metrics.semantics.cdw.consent_dismiss_method",
            .type = ".identity_consent.DismissMethod",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801046,
            .field_name =
                ".id.metrics.semantics.cdw.consent_flow_not_completed_reason",
            .type = ".identity_consent.ConsentFlowNotCompletedReason",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801047,
            .field_name =
                ".id.metrics.semantics.cdw.consent_eligibility_status",
            .type = ".footprints.transparencyandcontrol.proto."
                    "ConsentEligibilityStatus",
        },
        ExtensionDeclaration{
            .number = 535801048,
            .field_name =
                ".id.metrics.semantics.cdw.primitives_client_interface_type",
            .type = ".logs.proto.identity_consent_ui.SurfaceMetadata."
                    "ClientInterfaceType",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801049,
            .field_name = ".supplychain.gdc.cmms.rcm.proto.v1.metric_value",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801050,
            .field_name = ".cloud.ux.api.common.tenants.display_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801051,
            .field_name = ".cloud.helix.AlterColumnSetOptions.allowed_option",
            .type = ".cloud.helix.AlterColumnSetOptions.OptionMask",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801052,
            .field_name =
                ".google.corp.finance.seal.estonia.proto.v1.tower_description",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801053,
            .field_name =
                ".logs.proto.generative_language.colab_classifier_config",
            .type = ".logs.proto.generative_language.ColabClassifierConfig",
        },
        ExtensionDeclaration{
            .number = 535801054,
            .field_name = ".wireless_android_play_analytics.enum_description",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801055,
            .field_name =
                ".production.resources.mlresources.common.proto.locus_type",
            .type = ".production.resources.mlresources.common.proto.LocusType",
        },
        ExtensionDeclaration{
            .number = 535801056,
            .field_name = ".vr.perception.shoggoth.ShoggothCameraIdProto."
                          "logical_camera_metadata",
            .type = ".vr.perception.shoggoth.ShoggothCameraIdProto."
                    "LogicalCameraMetadata",
        },
        ExtensionDeclaration{
            .number = 535801057,
            .field_name = ".id.metrics.semantics.cdw.ftc_decision",
            .type = ".footprints.transparencyandcontrol.proto.Decision",
        },
        ExtensionDeclaration{
            .number = 535801058,
            .field_name = ".id.metrics.semantics.cdw.ftc_screen_id",
            .type = ".footprints.transparencyandcontrol.proto.ConsentScreenId",
        },
        ExtensionDeclaration{
            .number = 535801059,
            .field_name = ".ads.base.enums.min_ytma_version",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801060,
            .field_name = ".youtube_admintools_yurt.review.api.shared."
                          "extensions.badge_spec",
            .type = ".youtube_admintools_yurt.review.api.shared.BadgeSpec",
        },
        ExtensionDeclaration{
            .number = 535801061,
            .field_name = ".youtube_admintools_yurt.review.api.shared."
                          "extensions.description",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801062,
            .field_name = ".youtube_admintools_yurt.review.api.shared."
                          "extensions.icon",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801063,
            .field_name = ".vr.perception.shoggoth.ShoggothPixelFormatProto."
                          "pixel_format_metadata",
            .type = ".vr.perception.shoggoth.ShoggothPixelFormatProto."
                    "PixelFormatMetadata",
        },
        ExtensionDeclaration{
            .number = 535801064,
            .field_name = ".moneta.orchestration2.ui.internal."
                          "script.supports_post_action",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801065,
            .field_name = ".coltrane.internal.match_src_vapi_alternate",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801066,
            .field_name = ".enterprise.support.core_dse.support_widgets.proto."
                          "widget_tag_metadata",
            .type = ".enterprise.support.core_dse.support_widgets.proto."
                    "WidgetTagMetadata",
        },
        ExtensionDeclaration{
            .number = 535801067,
            .field_name =
                ".commerce.bizbuilder.recommendations.proto.ranking_token_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801068,
            .field_name =
                ".music_generation_proto.dynamic_creation_asset_error",
            .type = ".youtube_dynamic_creation_asset.DynamicCreationAssetError."
                    "Type",
        },
        ExtensionDeclaration{
            .number = 535801069,
            .field_name = ".cloud.ux.api.common.location.alt_display_texts",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801070,
            .field_name = ".geo.food.menu.api.kg_mid",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801071,
            .field_name = ".cloud.ux.api.common.search.display_text",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801072,
            .field_name = ".cloud.ux.api.common.search.alt_display_texts",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801073,
            .field_name = ".wireless.android.camera.syshealth.analysis."
                          "regression.proto.enum_options",
            .type = ".wireless.android.camera.syshealth.analysis.regression."
                    "proto.EnumOptions",
        },
        ExtensionDeclaration{
            .number = 535801074,
            .field_name = ".cloud_cluster_testing_e2e_test_persistentdisk"
                          "_perf_npi.instance_test",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801075,
            .field_name = ".cloud.helix.CloudRegionExt.turnup_in_progress",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801076,
            .field_name = ".car.SimOperationMode.mode_number",
            .type = "int64",
        },
        ExtensionDeclaration{
            .number = 535801077,
            .field_name =
                ".cloud.ux.api.contentmanagement.common.golden_schema_filename",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801078,
            .field_name = ".blobstore2.metadata_proto.lro_op_type_options",
            .type = ".blobstore2.metadata_proto.LroOpTypeOptions",
        },
        ExtensionDeclaration{
            .number = 535801079,
            .field_name = ".youtube_logs_attribution.configs.PartnerConfig."
                          "YtForecastPartnerOptions.string_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801080,
            .field_name = ".ccc.hosted.upsell.proto.eligibility."
                          "IneligibleReasonAnnotation.sequoia_enqueue_required",
            .type = "bool",
            .reserved = true,
        },
        ExtensionDeclaration{
            .number = 535801081,
            .field_name = ".brella_trusted.brella_task_config",
            .type = ".brella_trusted.BrellaTaskConfig",
        },
        ExtensionDeclaration{
            .number = 535801082,
            .field_name = ".storage.databasecenter.proto.common.partner."
                          "checkpoint_enabled",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801083,
            .field_name = ".ptoken.verdict_reason_metadata",
            .type = ".ptoken.VerdictReasonMetadata",
        },
        ExtensionDeclaration{
            .number = 535801084,
            .field_name = ".xaa_schema.CoarseBucketShortName.bucket_range",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801085,
            .field_name = ".xaa_schema.CoarseBucketShortName.min_range",
            .type = "int64",
        },
        ExtensionDeclaration{
            .number = 535801086,
            .field_name = ".xaa_schema.CoarseBucketShortName.full_bucket_name",
            .type = ".xaa_schema.CoarseBucketName.Enum",
        },
        ExtensionDeclaration{
            .number = 535801087,
            .field_name = ".xaa_schema.FineBucketRange.bucket_range",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801088,
            .field_name = ".xaa_schema.FineBucketRange.min_range",
            .type = "int64",
        },
        ExtensionDeclaration{
            .number = 535801089,
            .field_name = ".cloud.ux.api.webplatform.search.filter_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801090,
            .field_name = ".travel.flights.FlightsExperiment.spackle",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801091,
            .field_name = ".travel.flights.FlightsExperiment.pqpx",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801092,
            .field_name = ".travel.flights.FlightsExperiment.disruptive",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801093,
            .field_name =
                ".logs.proto.generative_language.goldmine_classifier_config",
            .type = ".logs.proto.generative_language.GoldmineClassifierConfig",
        },
        ExtensionDeclaration{
            .number = 535801094,
            .field_name = ".assistant_tools.orchestrator.is_fetch",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801095,
            .field_name = ".assistant_tools.orchestrator.is_scrape",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801096,
            .field_name = ".assistant_tools.orchestrator.is_unretryable",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801097,
            .field_name = ".google.corp.cloud.cre.data.v1.official_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801098,
            .field_name = ".rocket_eventcodes_extension.title",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801099,
            .field_name = ".syshealth_repairs_psis.IncidentBlockage.Blockage."
                          "source",
            .type = ".syshealth_repairs_psis.IncidentBlockage.Source.Enum",
        },
        ExtensionDeclaration{
            .number = 535801100,
            .field_name = ".assistant.logs.processed.srm_is_experimental",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801101,
            .field_name = ".music_generation_proto.error_category",
            .type = ".music_generation_proto.MusicGenerationError."
                    "ErrorCategory",
        },
        ExtensionDeclaration{
            .number = 535801102,
            .field_name = ".logplex.sources.target",
            .type = ".logplex.sources.FormatTarget",
        },
        ExtensionDeclaration{
            .number = 535801103,
            .field_name =
                ".google.internal.fitbit.api.premium.v1v3.enum_string",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801104,
            .field_name = ".viaduct.gridaware.api.ImageAnnotation.label_type",
            .type = ".viaduct.gridaware.api.ImageAnnotation.LabelType",
        },
        ExtensionDeclaration{
            .number = 535801105,
            .field_name = ".cloud.ux.api.webplatform.search.order_by_string",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801106,
            .field_name = ".xaa_features.time_management_feature",
            .type = ".xaa_features.TimeManagementFeature.Enum",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801107,
            .field_name = ".abuse.ares.storage.features.geo_moderation."
                          "moderation_initiation_reason",
            .type = ".abuse.ares.storage.features.geo_moderation."
                    "GeoModerationEvent.ModerationInitiationReason",
        },
        ExtensionDeclaration{
            .number = 535801108,
            .field_name = ".hotels.vacation_rentals.auto_abandon",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801109,
            .field_name = ".wireless_android_play_analytics.product_id",
            .type = "int32",
        },
        ExtensionDeclaration{
            .number = 535801110,
            .field_name = ".salesincentives.deal.corporate_entity_value",
            .type = ".salesincentives.deal.CorporateEntityValue",
        },
        ExtensionDeclaration{
            .number = 535801111,
            .field_name = ".ads.mixer.richad.smartass_format_limits",
            .type = ".ads.mixer.richad.SmartassFormatLimits",
        },
        ExtensionDeclaration{
            .number = 535801112,
            .field_name = ".ads.mixer.BlockControlFlowStates.next",
            .type = ".ads.mixer.BlockControlFlowStates.State",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801113,
            .field_name = ".ads.mixer.BlockControlFlowStates.start",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801114,
            .field_name = ".ads.mixer.BlockControlFlowStates.done",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801115,
            .field_name = ".via.cubs.inference.llm_label_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801116,
            .field_name = ".serverless.base.events.event_code_metadata",
            .type = ".serverless.base.events.EventCodeMetadata",
            .reserved = true,
        },
        ExtensionDeclaration{
            .number = 535801117,
            .field_name = ".privacy_dharma_protos_storage_gap_monitoring.mpm_"
                          "age.display_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801118,
            .field_name = ".news.recon.artifacts.max_num_characters",
            .type = "int32",
        },
        ExtensionDeclaration{
            .number = 535801119,
            .field_name = ".car.ml_data.feature_store.feature_type",
            .type = ".car.ml_data.feature_store.FeatureType",
        },
        ExtensionDeclaration{
            .number = 535801120,
            .field_name = ".xaa_features.rasta_classification",
            .type = ".xaa_features.RastaMetricClassification",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801121,
            .field_name = ".cloud.ux.api.common.location.short_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801122,
            .field_name = ".apps.dynamite.shared.legacy_user_setting",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801123,
            .field_name = ".google.corp.dcrelay.calendar.v1.string_value",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801124,
            .field_name = ".ccc.hosted.accesscontrol.proto.audit_log_msg_val",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801125,
            .field_name = ".cloud_cluster_testing_e2e_test_persistentdisk"
                          "_perf_npi.clustermanager",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801126,
            .field_name =
                ".assistant.lamda.annotators.usecase_ontology.eval_level2",
            .type = ".assistant.lamda.annotators.usecase_ontology.EvalLevel2."
                    "EvalUseCase",
        },
        ExtensionDeclaration{
            .number = 535801127,
            .field_name = ".privacy_dharma_protos_storage_gap_monitoring.mpm_"
                          "age.JobAgeSingleSelectFilterExts.display_name2",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801128,
            .field_name = ".cloud.ai.data_tools.inverse_of",
            .type = ".cloud.ai.data_tools.ExampleRelationship.Type",
        },
        ExtensionDeclaration{
            .number = 535801129,
            .field_name =
                ".enterprise.crm.monet.workload.proto.workload_pillar",
            .type = ".enterprise.crm.monet.workload.proto.WorkloadPillar",
        },
        ExtensionDeclaration{
            .number = 535801130,
            .field_name = ".pmw.alcedo.issuesync.proto.string_value",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801131,
            .field_name =
                ".cloud.ux.api.webplatform.cgcpage.module_converter_type",
            .type = ".cloud.ux.api.webplatform.cgcpage.ModuleConverterType",
        },
        ExtensionDeclaration{
            .number = 535801132,
            .field_name = ".cloud.cluster.manager.tpc.tpc_universe_annotation",
            .type = ".cloud.cluster.manager.tpc.TpcUniverseAnnotation",
        },
        ExtensionDeclaration{
            .number = 535801133,
            .field_name =
                ".cloud.ux.api.webplatform.cgcpage.revision_tag_type_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801134,
            .field_name = ".cloud.security.assurant.compliancecenter.proto."
                          "exempt_pst_mapping",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801135,
            .field_name =
                ".fitbit.platform.dataexport.hfd.metadata.proto.folder_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801136,
            .field_name = ".cloud_main_common.cloud_main_picklist_value",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801137,
            .field_name = ".id.metrics.semantics.cdw.ogm_ui_client_render_type",
            .type = ".logs.proto.identity_consent_ui."
                    "MobileClientConsentFlowEvents.Metadata.ConsentRenderType",
        },
        ExtensionDeclaration{
            .number = 535801138,
            .field_name = ".id.metrics.semantics.cdw.ogm_ui_client_color_theme",
            .type = ".logs.proto.identity_consent_ui."
                    "MobileClientConsentFlowEvents.Metadata.ColorTheme",
        },
        ExtensionDeclaration{
            .number = 535801139,
            .field_name =
                ".id.metrics.semantics.cdw.ogm_ui_client_dismissibility",
            .type = ".logs.proto.identity_consent_ui."
                    "MobileClientConsentFlowEvents.Metadata.Dismissibility",
        },
        ExtensionDeclaration{
            .number = 535801140,
            .field_name =
                ".fitbit.platform.commerce.premium.proto.v1.enum_string",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801141,
            .field_name =
                ".corp.servicedesk.contentstack.protos.serialized_value",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801142,
            .field_name = ".ads.keyval.config.default_threshold",
            .type = "double",
        },
        ExtensionDeclaration{
            .number = 535801143,
            .field_name = ".ads.keyval.config.default_trigger_duration",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801144,
            .field_name = ".search.mcf.hamsa.manualreview.display_value",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801145,
            .field_name = ".via.cubs.inference.llm_label_application",
            .type = ".via.cubs.inference.LlmLabelApplication",
        },
        ExtensionDeclaration{
            .number = 535801146,
            .field_name = ".apps.dynamite.backend.dlp_scan_result",
            .type = ".apps.dynamite.backend.DlpScanResult",
        },
        ExtensionDeclaration{
            .number = 535801147,
            .field_name = ".ad_assistant.extraction_model_enum_options",
            .type = ".ad_assistant.ExtractionModelEnumOptions",
        },
        ExtensionDeclaration{
            .number = 535801148,
            .field_name = ".enterprise.crm.common.proto.enum_value_metadata",
            .type = ".enterprise.crm.common.proto.EnumValueMetadata",
        },
        ExtensionDeclaration{
            .number = 535801149,
            .field_name = ".ads.geo.media.quality.quality_version_metadata",
            .type = ".ads.geo.media.quality.QualityVersionMetadata",
        },
        ExtensionDeclaration{
            .number = 535801150,
            .field_name = ".wireless_android_play_playlog.shared.stats_event."
                          "histogram_options",
            .type = ".wireless_android_play_playlog.shared.stats_event."
                    "StatsEventHistogramOptions",
        },
        ExtensionDeclaration{
            .number = 535801151,
            .field_name =
                ".cloud_cluster.instance_manager.accelerator_category",
            .type = ".cloud_cluster.instance_manager.AcceleratorCategory",
        },
        ExtensionDeclaration{
            .number = 535801152,
            .field_name = ".superroot.magi.needs_m1_summary_inputs",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801153,
            .field_name = ".google.production.sisyphus.cloud.pd.slurper."
                          "resource_type_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801154,
            .field_name = ".table.MetadataFieldAnnotation.enum_options",
            .type = ".table.MetadataFieldAnnotation",
        },
        ExtensionDeclaration{
            .number = 535801155,
            .field_name = ".cloud_cluster.instance_manager.gsys_plugin",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801156,
            .field_name =
                ".cloud.ux.api.contentmanagement.common.version_number",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801157,
            .field_name = ".logs.proto.serverless.common.phase_metadata",
            .type = ".logs.proto.serverless.common.PhaseMetadata",
        },
        ExtensionDeclaration{
            .number = 535801158,
            .field_name = ".logs.proto.serverless.common.journey_metadata",
            .type = ".logs.proto.serverless.common.JourneyMetadata",
        },
        ExtensionDeclaration{
            .number = 535801159,
            .field_name =
                ".cloud_cluster.instance_manager.gpu_count_scale_factor",
            .type = "uint32",
        },
        ExtensionDeclaration{
            .number = 535801160,
            .field_name =
                ".youtube.ecommerce.shopping.video_representative_entry_point",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801161,
            .field_name = ".via.freeze_old_videos",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801162,
            .field_name = ".corp.legal.removals.userreports.string_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801163,
            .field_name = ".pmw.alcedo.issuesync.string_value",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801164,
            .field_name = ".usps.drop_reason_id.drop_severity",
            .type = ".usps.drop_reason_id.PacketDropSeverity.DropSeverity"},
        ExtensionDeclaration{
            .number = 535801165,
            .field_name = ".usps.drop_reason_id.drop_logging_period_sec",
            .type = "uint32"},
        ExtensionDeclaration{
            .number = 535801166,
            .field_name = ".apps.dynamite.shared.parent_path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801167,
            .field_name = ".apps.dynamite.shared.field_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801168,
            .field_name =
                ".java_com_google_abuse_admin_clients_firebolt_service_"
                "abuseprotectionv2_significantevents.signal_set",
            .type = ".abuse.ares.storage.features.geo_moderation.SignalSet",
        },
        ExtensionDeclaration{
            .number = 535801169,
            .field_name = ".coroner.collection.blob_type_options",
            .type = ".coroner.collection.BlobTypeOptions"},
        ExtensionDeclaration{
            .number = 535801170,
            .field_name = ".via.cubs.ReducedSetsMetadata.reduced_set_rank",
            .type = "int32"},
        ExtensionDeclaration{
            .number = 535801171,
            .field_name =
                ".java_com_google_ads_homeservices_verification_"
                "employeeidentityverificationservice_proto.attribute_type",
            .type = "string"},
        ExtensionDeclaration{
            .number = 535801172,
            .field_name =
                ".java_com_google_ads_homeservices_verification_"
                "employeeidentityverificationservice_proto.attribute_status",
            .type = "string"},
        ExtensionDeclaration{
            .number = 535801173,
            .field_name =
                ".id.metrics.semantics.adsprivacyconsumerata.ve_type_id",
            .type = "int32",
            .repeated = true},
        ExtensionDeclaration{
            .number = 535801174,
            .field_name = ".bug_workbench.standard_value.short_name",
            .type = "string"},
        ExtensionDeclaration{
            .number = 535801175,
            .field_name = ".java_com_google_corp_contactcenter_responder_proto."
                          "string_representation",
            .type = "string"},
        ExtensionDeclaration{
            .number = 535801176,
            .field_name = ".serverless.base.error_code_metadata",
            .type = ".serverless.base.ErrorCodeMetadata",
        },
        ExtensionDeclaration{.number = 535801177,
                             .field_name = ".search.apps.cache.short_name",
                             .type = "string"},
        ExtensionDeclaration{
            .number = 535801178,
            .field_name = ".search.hamsa.manualreview.display_value",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801179,
            .field_name = ".cloud.ux.api.common.page.template_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801180,
            .field_name =
                ".music.corpus.infra.bass.default_id_map_writer_process",
            .type = ".music.corpus.infra.internal.IdMapWriterProcess.Process"},
        ExtensionDeclaration{
            .number = 535801181,
            .field_name = ".music.corpus.infra.bass.default_queue_priority",
            .type = ".music.corpus.infra.internal.BassQueuePayload.Priority"},
        ExtensionDeclaration{
            .number = 535801182,
            .field_name = ".enterprise.partner.incentives.protos.string_value",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801183,
            .field_name = ".droidguard.collect_signal_exception",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801184,
            .field_name =
                ".mtest.klondike.persistentdisk.pd_perf.cluster_manager_name",
            .type = "string"},
        ExtensionDeclaration{
            .number = 535801185,
            .field_name = ".corp_networking_netprobe.enum_flag",
            .type = ".corp_networking_netprobe.Flag",
        },
        ExtensionDeclaration{
            .number = 535801186,
            .field_name = ".music_generation_proto.duplo_feature_version",
            .type = ".video.legos.duplo.FeatureVersion"},
        ExtensionDeclaration{
            .number = 535801187,
            .field_name = ".corp.legal.discovery.scrapezilla.v1.SnapshotFilter."
                          "schema_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801188,
            .field_name =
                ".corp.legal.discovery.scrapezilla.v1.SnapshotFilter.type",
            .type = ".corp.legal.discovery.scrapezilla.v1.SnapshotFilter.Type",
        },
        ExtensionDeclaration{
            .number = 535801189,
            .field_name =
                ".cloud.ux.api.contentmanagement.common.legacy_string_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801190,
            .field_name = ".cloud.ux.api.common.page.status_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801191,
            .field_name = ".configs.production.prodspec.fraggle_spanner_dev."
                          "BudgetGroup.info",
            .type = ".configs.production.prodspec.fraggle_spanner_dev."
                    "BudgetGroup.Info",
        },
        ExtensionDeclaration{
            .number = 535801192,
            .field_name = ".id.verification.service.info",
            .type = ".id.verification.service.VerificationKindInfo",
        },
        ExtensionDeclaration{
            .number = 535801193,
            .field_name = ".cloud.kubernetes.proto.error_categorization",
            .type = ".cloud.kubernetes.proto.ErrorCategorization",
        },
        ExtensionDeclaration{
            .number = 535801194,
            .field_name = ".onegoogle.kansas.dma_exemption",
            .type = ".onegoogle.kansas.DmaProcessingExemption",
        },
        ExtensionDeclaration{
            .number = 535801195,
            .field_name = ".yt_tns_reviews.core.review_dimensions."
                          "certification.operational_vertical",
            .type = ".yt_tns_reviews.core.vertical.OperationalVertical",
        },
        ExtensionDeclaration{
            .number = 535801196,
            .field_name = ".googlebase.verticals.ReportingContextGroup.member",
            .type = ".googlebase.verticals.ReportingContextId.Id",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801197,
            .field_name = ".subscriptions.entitlement.plugin.proto.shared.mini_"
                          "subscription_conceptual_sku_features",
            .type = ".subscriptions.entitlement.plugin.proto.shared."
                    "MiniSubscriptionConceptualSkuFeatures",
        },
        ExtensionDeclaration{
            .number = 535801198,
            .field_name = ".fitbit.platform.hfd.storage.spanner.schema.team."
                          "exercise.personal_record_type_metadata",
            .type = ".fitbit.platform.hfd.storage.spanner.schema.team.exercise."
                    "PersonalRecordTypeMetadata",
        },
        ExtensionDeclaration{
            .number = 535801199,
            .field_name = ".analysis_lavaflow.dg_annotations.Privacy."
                          "enforcement_mode_params",
            .type = ".analysis_lavaflow.dg_annotations.Privacy."
                    "EnforcementModeParams",
        },
        ExtensionDeclaration{
            .number = 535801200,
            .field_name = ".net_model_unm.proto.non_fungible",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801201,
            .field_name = ".backend_picker.rtt_ms",
            .type = "int32",
        },
        ExtensionDeclaration{
            .number = 535801202,
            .field_name = ".monitoring.autoalert.proto.alert_name_suffix",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801203,
            .field_name = ".monitoring.autoalert.proto.alert_full_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801204,
            .field_name = ".monitoring.autoalert.proto.alert_name_prefix",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801205,
            .field_name = ".logs.proto.ads.youtube.demand_source_options",
            .type = ".logs.proto.ads.youtube.DemandSourceOptions",
        },
        ExtensionDeclaration{
            .number = 535801206,
            .field_name = ".id.metrics.semantics.adsprivacyconsumerata.is_"
                          "tuning_interaction",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801207,
            .field_name = ".id.metrics.semantics.adsprivacyconsumerata.is_"
                          "engagement_interaction",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801208,
            .field_name = ".social.frontend.searchconsole.data."
                          "SearchAppearanceOptions.api_display_string_override",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801209,
            .field_name = ".ccc.hosted.commerce.enum_string",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801210,
            .field_name =
                ".java.com.google.apps.docs.performance.aggrt_copy_type",
            .type = ".java.com.google.apps.docs.performance.CopyType",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801211,
            .field_name = ".ads.adsui.experiment_store.proto.blackrock_options",
            .type =
                ".ads.adsui.experiment_store.proto.BlackrockExperimentOptions",
        },
        ExtensionDeclaration{
            .number = 535801212,
            .field_name = ".personalization.settings.api.lams_client_metadata",
            .type = ".personalization.settings.api.LamsClientMetadata",
        },
        ExtensionDeclaration{
            .number = 535801213,
            .field_name =
                ".google.api.launch.launchchecker.proto.checker_options",
            .type = ".google.api.launch.launchchecker.proto.CheckerManifest",
        },
        ExtensionDeclaration{
            .number = 535801214,
            .field_name = ".apps.drive.frontend.impressions.proto.drive_error_"
                          "severity_reason",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801215,
            .field_name = ".generic_appflows.reliability_info",
            .type = ".generic_appflows.ReliabilityInfo",
        },
        ExtensionDeclaration{
            .number = 535801216,
            .field_name = ".cloud.security.assurant.compliancecenter.proto."
                          "display_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801217,
            .field_name =
                ".moneta.reviews3.domains.screening.queue_enabling_experiment",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801218,
            .field_name =
                ".moneta.reviews3.domains.screening.queue_disabling_experiment",
            .type = "string",
            .repeated = true,
        },
        ExtensionDeclaration{
            .number = 535801219,
            .field_name = ".superroot.magi.search_llm_orchestration_flow",
            .type = ".logs.proto.gws.SearchLLMOrchestrationFlow",
        },
        ExtensionDeclaration{
            .number = 535801220,
            .field_name = ".com.google.cloud.kubernetes.koala.spanner.protos."
                          "display_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801221,
            .field_name = ".cloud_blockstore.agent.invariant_owner",
            .type = ".cloud_blockstore.agent.PdInvariantOwnerMetadata",
        },
        ExtensionDeclaration{
            .number = 535801222,
            .field_name = ".evaluation.standards.projects.product_class."
                          "product_class_options",
            .type = ".evaluation.standards.projects.product_class."
                    "ProductClassOptions",
        },
        ExtensionDeclaration{
            .number = 535801223,
            .field_name = ".ads.keyval.config.pure_project_alert",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801224,
            .field_name = ".sandtrout.consent_state_def",
            .type = ".sandtrout.ConsentStateDef",
        },
        ExtensionDeclaration{
            .number = 535801225,
            .field_name = ".enterprise.partner.incentives.protos.description",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801226,
            .field_name = ".logs.proto.apps_dynamite.insights_event_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801227,
            .field_name = ".logs.proto.apps_dynamite.insights_action_type",
            .type = ".logs.proto.apps_dynamite.AuditEvent.ActionType",
        },
        ExtensionDeclaration{
            .number = 535801228,
            .field_name = ".logs.proto.apps_dynamite.insights_event_type_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801229,
            .field_name = ".logs.proto.apps_dynamite.insights_enum_value_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801230,
            .field_name = ".dremel.tools.query_diagnosis.display_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801231,
            .field_name = ".abuse.admin.clients.oauth.service.config.review_"
                          "check_group_option",
            .type = ".abuse.admin.clients.oauth.service.config."
                    "ReviewCheckGroupOptions",
        },
        ExtensionDeclaration{
            .number = 535801232,
            .field_name = ".quality.result_filtering.result_filter_kind",
            .type = ".quality.result_filtering.ResultFilterKind",
        },
        ExtensionDeclaration{
            .number = 535801233,
            .field_name = ".ptoken.external_verdict_reason_metadata",
            .type = ".ptoken.ExternalVerdictReasonMetadata",
        },
        ExtensionDeclaration{
            .number = 535801234,
            .field_name = ".google.internal.geo.baselayerroadaccess.v1alpha."
                          "ObjectAttributes.object_type_id",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801235,
            .field_name = ".cloud_cluster_testing_e2e_test_persistentdisk"
                          "_perf_npi.gce_project_continuous",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801236,
            .field_name = ".cloud.sre.ccrs.cic_dysfunction.catalog.tag_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801237,
            .field_name = ".car.instant_eval.snapshot_evaluator_type_metadata",
            .type = ".car.analysis.AnalyzerOptions",
        },
        ExtensionDeclaration{
            .number = 535801238,
            .field_name =
                ".gws.tools.adinjsl.Extension.AttributeSetter.attr_key",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801239,
            .field_name = ".search_engagement.highlight.common.enum_string",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801240,
            .field_name = ".superroot.magi.never_used_as_initial_step",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801241,
            .field_name = ".search.apps.label",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801242,
            .field_name = ".forge.retriable",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801243,
            .field_name = ".desktop.services.logging.description",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801244,
            .field_name = ".via.cubs.inference.large_model_label_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801245,
            .field_name = ".via.cubs.inference.large_model_label_application",
            .type = ".via.cubs.inference.LargeModelLabelApplication",
        },
        ExtensionDeclaration{
            .number = 535801246,
            .field_name = ".quality_fringe.sudo.label_metadata",
            .type = ".quality_fringe.sudo.LabelMetadata",
        },
        ExtensionDeclaration{
            .number = 535801247,
            .field_name =
                ".wireless.android.tv.livetv.freeplay.video_codec_settings",
            .type = ".wireless.android.tv.livetv.freeplay.VideoCodecSettings",
        },
        ExtensionDeclaration{
            .number = 535801248,
            .field_name =
                ".wireless.android.tv.livetv.freeplay.audio_codec_settings",
            .type = ".wireless.android.tv.livetv.freeplay.AudioCodecSettings",
        },
        ExtensionDeclaration{
            .number = 535801249,
            .field_name = ".com.google.cloud.ux.backends.cloudconnect."
                          "marketingcontentgeneration.config.string_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801250,
            .field_name = ".cloud.ux.api.contentmanagement.common.golden_"
                          "schema_folder_path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801251,
            .field_name = ".beto.dimensions.status_bucket",
            .type = ".beto.StatusBucket",
        },
        ExtensionDeclaration{
            .number = 535801252,
            .field_name = ".beto.dimensions.stage",
            .type = ".beto.Stage",
        },
        ExtensionDeclaration{
            .number = 535801253,
            .field_name =
                ".cloud_cluster_fleet_deployment_system.channel_metadata",
            .type = ".cloud_cluster_fleet_deployment_system.ChannelMetadata",
        },
        ExtensionDeclaration{
            .number = 535801254,
            .field_name =
                ".cloud_cluster_fleet_deployment_system.cohort_schema_metadata",
            .type =
                ".cloud_cluster_fleet_deployment_system.CohortSchemaMetadata",
        },
        ExtensionDeclaration{
            .number = 535801255,
            .field_name =
                ".hw.customerinsights.dwh.product_hierarchy.dma_product_id",
            .type = "int64",
        },
        ExtensionDeclaration{
            .number = 535801256,
            .field_name =
                ".quality.dialog_manager.core.annotations.association",
            .type = ".quality.dialog_manager.core.annotations.Association",
        },
        ExtensionDeclaration{
            .number = 535801257,
            .field_name = ".gcs.errors.gcs_legacy_error_details",
            .type = ".gcs.errors.GcsLegacyErrorDetails",
        },
        ExtensionDeclaration{
            .number = 535801258,
            .field_name = ".apps.people.media.client_loggable",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801259,
            .field_name = ".superroot.magi.diverts_to_mars",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801260,
            .field_name = ".ads.awapps.anji.proto.infra.appdata.EnumOptions."
                          "exclude_from_prefetch_response_to_client",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801261,
            .field_name = ".ads.homeservices.auction."
                          "GlsPerPositionAuctionConfig.filter_priority",
            .type = "int32",
        },
        ExtensionDeclaration{
            .number = 535801262,
            .field_name = ".beto.common_stage",
            .type = ".beto.CommonStage",
        },
        ExtensionDeclaration{
            .number = 535801263,
            .field_name =
                ".humboldt.analysis.assistant_dimensions.dimension_metadata",
            .type = ".humboldt.analysis.assistant_dimensions.DimensionMetadata",
        },
        ExtensionDeclaration{
            .number = 535801264,
            .field_name =
                ".humboldt.analysis.assistant_latency_metrics.group_metadata",
            .type = ".humboldt.analysis.assistant_latency_metrics."
                    "LatencyMetricGroupMetadata",
        },
        ExtensionDeclaration{
            .number = 535801265,
            .field_name =
                ".humboldt.analysis.assistant_latency_metrics.metric_metadata",
            .type = ".humboldt.analysis.assistant_latency_metrics."
                    "LatencyMetricMetadata",
        },
        ExtensionDeclaration{
            .number = 535801266,
            .field_name = ".com.google.ads.publisher.domain.entity.experiment."
                          "config.experiment_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801267,
            .field_name = ".com.google.ads.publisher.domain.entity.experiment."
                          "config.experiment_setting_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801268,
            .field_name = ".via.cubs.inference.large_model_trigger_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801269,
            .field_name = ".ccc.hosted.upsell.proto.promodata.common."
                          "DiscountAnnotation.numerical_discount_value",
            .type = "int32",
        },
        ExtensionDeclaration{
            .number = 535801270,
            .field_name = ".logs.bizbuilder.event_taxonomy_metadata",
            .type = ".logs.bizbuilder.EventTaxonomyMetadata",
        },
        ExtensionDeclaration{
            .number = 535801271,
            .field_name = ".developers.console.tt.acl",
            .type = ".developers.console.tt.Acl",
        },
        ExtensionDeclaration{
            .number = 535801272,
            .field_name = ".identity_accesscontrol.header_options",
            .type = ".identity_accesscontrol.ForwardedHttpHeaderTypeOptions",
        },
        ExtensionDeclaration{
            .number = 535801273,
            .field_name = ".ads.classifier.fatcat.datalabeling.corpus_name",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801274,
            .field_name = ".cloud.security.assurant.compliancecenter.proto."
                          "description",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801275,
            .field_name = ".assistant.grounding.association",
            .type = ".quality.dialog_manager.core.annotations.Client",
        },
        ExtensionDeclaration{
            .number = 535801276,
            .field_name = ".beto.dimensions.path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801277,
            .field_name = ".ads.monitoring.smart_alerting.explainer."
                          "attribution_options_path",
            .type = "string",
        },
        ExtensionDeclaration{
            .number = 535801278,
            .field_name =
                ".chrome.cros.capai.cap_ai_request_enum_value_decorators",
            .type = ".chrome.cros.capai.CapAIRequestEnumValueDecorators",
        },
        ExtensionDeclaration{
            .number = 535801279,
            .field_name = ".youtube_analytics.ProcellaFields.metadata",
            .type = ".youtube_analytics.ProcellaFields.Metadata",
        },
        ExtensionDeclaration{
            .number = 535801280,
            .field_name = ".moneta.security.api.hash_plaintext",
            .type = "bool",
        },
        ExtensionDeclaration{
            .number = 535801281,
            .field_name = ".footprints.activity_controls.data_sharing_cps",
            .type = ".compliance_controls.CorePlatformService",
        },
        // go/keep-sorted end
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_EXTDECL_ENUM_VALUE_OPTIONS_H__
