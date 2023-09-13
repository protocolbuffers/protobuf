#include <stddef.h>
#include "upb/generated_code_support.h"
#include "google/protobuf/descriptor.upb.h"

static upb_Arena* upb_BootstrapArena() {
  static upb_Arena* arena = NULL;
  if (!arena) arena = upb_Arena_New();
  return arena;
}

const upb_MiniTable* google_protobuf_FileDescriptorSet_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 1), google_protobuf_FileDescriptorProto_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_FileDescriptorProto_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$11EGGGG33<<114";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 4), google_protobuf_DescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 5), google_protobuf_EnumDescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 6), google_protobuf_ServiceDescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 7), google_protobuf_FieldDescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 8), google_protobuf_FileOptions_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 9), google_protobuf_SourceCodeInfo_msg_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 14), google_protobuf_Edition_enum_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_DescriptorProto_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$1GGGGG3GGE";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 2), google_protobuf_FieldDescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 6), google_protobuf_FieldDescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 3), google_protobuf_DescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 4), google_protobuf_EnumDescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 5), google_protobuf_DescriptorProto_ExtensionRange_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 8), google_protobuf_OneofDescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 7), google_protobuf_MessageOptions_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 9), google_protobuf_DescriptorProto_ReservedRange_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_DescriptorProto_ExtensionRange_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$((3";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 3), google_protobuf_ExtensionRangeOptions_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_DescriptorProto_ReservedRange_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$((";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTable* google_protobuf_ExtensionRangeOptions_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$PaG4n`3t|G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 999), google_protobuf_UninterpretedOption_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 2), google_protobuf_ExtensionRangeOptions_Declaration_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 50), google_protobuf_FeatureSet_msg_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 3), google_protobuf_ExtensionRangeOptions_VerificationState_enum_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_ExtensionRangeOptions_Declaration_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$(11a//";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTable* google_protobuf_FieldDescriptorProto_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$11(44113(1f/";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 4), google_protobuf_FieldDescriptorProto_Label_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 5), google_protobuf_FieldDescriptorProto_Type_enum_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 8), google_protobuf_FieldOptions_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_OneofDescriptorProto_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$13";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 2), google_protobuf_OneofOptions_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_EnumDescriptorProto_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$1G3GE";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 2), google_protobuf_EnumValueDescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 3), google_protobuf_EnumOptions_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 4), google_protobuf_EnumDescriptorProto_EnumReservedRange_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_EnumDescriptorProto_EnumReservedRange_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$((";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTable* google_protobuf_EnumValueDescriptorProto_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$1(3";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 3), google_protobuf_EnumValueOptions_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_ServiceDescriptorProto_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$1G3";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 2), google_protobuf_MethodDescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 3), google_protobuf_ServiceOptions_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_MethodDescriptorProto_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$1113//";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 4), google_protobuf_MethodOptions_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_FileOptions_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$P1f14/1d///a/b/c/c/d11a111/a11d3t|G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 9), google_protobuf_FileOptions_OptimizeMode_enum_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 50), google_protobuf_FeatureSet_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 999), google_protobuf_UninterpretedOption_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_MessageOptions_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$P///c/c/3z}G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 12), google_protobuf_FeatureSet_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 999), google_protobuf_UninterpretedOption_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_FieldOptions_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$P4//a/4c/d//4aHG3q}G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 1), google_protobuf_FieldOptions_CType_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 6), google_protobuf_FieldOptions_JSType_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 17), google_protobuf_FieldOptions_OptionRetention_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 19), google_protobuf_FieldOptions_OptionTargetType_enum_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 20), google_protobuf_FieldOptions_EditionDefault_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 21), google_protobuf_FeatureSet_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 999), google_protobuf_UninterpretedOption_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_FieldOptions_EditionDefault_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$114";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 3), google_protobuf_Edition_enum_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_OneofOptions_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$P3e~G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 1), google_protobuf_FeatureSet_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 999), google_protobuf_UninterpretedOption_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_EnumOptions_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$Pa//b/3_~G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 7), google_protobuf_FeatureSet_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 999), google_protobuf_UninterpretedOption_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_EnumValueOptions_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$P/3/c~G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 2), google_protobuf_FeatureSet_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 999), google_protobuf_UninterpretedOption_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_ServiceOptions_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$P``/3d}G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 34), google_protobuf_FeatureSet_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 999), google_protobuf_UninterpretedOption_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_MethodOptions_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$P``/43c}G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 34), google_protobuf_MethodOptions_IdempotencyLevel_enum_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 35), google_protobuf_FeatureSet_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 999), google_protobuf_UninterpretedOption_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_UninterpretedOption_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$aG1,+ 01";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 2), google_protobuf_UninterpretedOption_NamePart_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_UninterpretedOption_NamePart_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$1N/N";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTable* google_protobuf_FeatureSet_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$P444a44";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 1), google_protobuf_FeatureSet_FieldPresence_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 2), google_protobuf_FeatureSet_EnumType_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 3), google_protobuf_FeatureSet_RepeatedFieldEncoding_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 5), google_protobuf_FeatureSet_MessageEncoding_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 6), google_protobuf_FeatureSet_JsonFormat_enum_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_FeatureSetDefaults_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$G1144";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 1), google_protobuf_FeatureSetDefaults_FeatureSetEditionDefault_msg_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 4), google_protobuf_Edition_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 5), google_protobuf_Edition_enum_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_FeatureSetDefaults_FeatureSetEditionDefault_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$134";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 3), google_protobuf_Edition_enum_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 2), google_protobuf_FeatureSet_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_SourceCodeInfo_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 1), google_protobuf_SourceCodeInfo_Location_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_SourceCodeInfo_Location_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$<M<M11aE";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTable* google_protobuf_GeneratedCodeInfo_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 1), google_protobuf_GeneratedCodeInfo_Annotation_msg_init());
  return mini_table;
}

const upb_MiniTable* google_protobuf_GeneratedCodeInfo_Annotation_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$<M1((4";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 5), google_protobuf_GeneratedCodeInfo_Annotation_Semantic_enum_init());
  return mini_table;
}

const upb_MiniTableEnum* google_protobuf_Edition_enum_init() {
  static const upb_MiniTableEnum* mini_table = NULL;
  static const char* mini_descriptor = "!)b~!ot_b)";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTableEnum_Build(mini_descriptor, strlen(mini_descriptor),
                              upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTableEnum* google_protobuf_ExtensionRangeOptions_VerificationState_enum_init() {
  static const upb_MiniTableEnum* mini_table = NULL;
  static const char* mini_descriptor = "!$";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTableEnum_Build(mini_descriptor, strlen(mini_descriptor),
                              upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTableEnum* google_protobuf_FeatureSet_EnumType_enum_init() {
  static const upb_MiniTableEnum* mini_table = NULL;
  static const char* mini_descriptor = "!)";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTableEnum_Build(mini_descriptor, strlen(mini_descriptor),
                              upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTableEnum* google_protobuf_FeatureSet_FieldPresence_enum_init() {
  static const upb_MiniTableEnum* mini_table = NULL;
  static const char* mini_descriptor = "!1";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTableEnum_Build(mini_descriptor, strlen(mini_descriptor),
                              upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTableEnum* google_protobuf_FeatureSet_JsonFormat_enum_init() {
  static const upb_MiniTableEnum* mini_table = NULL;
  static const char* mini_descriptor = "!)";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTableEnum_Build(mini_descriptor, strlen(mini_descriptor),
                              upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTableEnum* google_protobuf_FeatureSet_MessageEncoding_enum_init() {
  static const upb_MiniTableEnum* mini_table = NULL;
  static const char* mini_descriptor = "!)";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTableEnum_Build(mini_descriptor, strlen(mini_descriptor),
                              upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTableEnum* google_protobuf_FeatureSet_RepeatedFieldEncoding_enum_init() {
  static const upb_MiniTableEnum* mini_table = NULL;
  static const char* mini_descriptor = "!)";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTableEnum_Build(mini_descriptor, strlen(mini_descriptor),
                              upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTableEnum* google_protobuf_FieldDescriptorProto_Label_enum_init() {
  static const upb_MiniTableEnum* mini_table = NULL;
  static const char* mini_descriptor = "!0";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTableEnum_Build(mini_descriptor, strlen(mini_descriptor),
                              upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTableEnum* google_protobuf_FieldDescriptorProto_Type_enum_init() {
  static const upb_MiniTableEnum* mini_table = NULL;
  static const char* mini_descriptor = "!@AA1";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTableEnum_Build(mini_descriptor, strlen(mini_descriptor),
                              upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTableEnum* google_protobuf_FieldOptions_CType_enum_init() {
  static const upb_MiniTableEnum* mini_table = NULL;
  static const char* mini_descriptor = "!)";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTableEnum_Build(mini_descriptor, strlen(mini_descriptor),
                              upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTableEnum* google_protobuf_FieldOptions_JSType_enum_init() {
  static const upb_MiniTableEnum* mini_table = NULL;
  static const char* mini_descriptor = "!)";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTableEnum_Build(mini_descriptor, strlen(mini_descriptor),
                              upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTableEnum* google_protobuf_FieldOptions_OptionRetention_enum_init() {
  static const upb_MiniTableEnum* mini_table = NULL;
  static const char* mini_descriptor = "!)";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTableEnum_Build(mini_descriptor, strlen(mini_descriptor),
                              upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTableEnum* google_protobuf_FieldOptions_OptionTargetType_enum_init() {
  static const upb_MiniTableEnum* mini_table = NULL;
  static const char* mini_descriptor = "!AA";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTableEnum_Build(mini_descriptor, strlen(mini_descriptor),
                              upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTableEnum* google_protobuf_FileOptions_OptimizeMode_enum_init() {
  static const upb_MiniTableEnum* mini_table = NULL;
  static const char* mini_descriptor = "!0";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTableEnum_Build(mini_descriptor, strlen(mini_descriptor),
                              upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTableEnum* google_protobuf_GeneratedCodeInfo_Annotation_Semantic_enum_init() {
  static const upb_MiniTableEnum* mini_table = NULL;
  static const char* mini_descriptor = "!)";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTableEnum_Build(mini_descriptor, strlen(mini_descriptor),
                              upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTableEnum* google_protobuf_MethodOptions_IdempotencyLevel_enum_init() {
  static const upb_MiniTableEnum* mini_table = NULL;
  static const char* mini_descriptor = "!)";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTableEnum_Build(mini_descriptor, strlen(mini_descriptor),
                              upb_BootstrapArena(), NULL);
  return mini_table;
}

