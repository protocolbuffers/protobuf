#include <stddef.h>
#include "upb/generated_code_support.h"
#include "google/protobuf/descriptor.upb.h"

static upb_Arena* upb_BootstrapArena() {
  static upb_Arena* arena = NULL;
  if (!arena) arena = upb_Arena_New();
  return arena;
}

const upb_MiniTable* google__protobuf__FileDescriptorSet_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 1), google__protobuf__FileDescriptorProto_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__FileDescriptorProto_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$11EGGGG33<<1a4";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 4), google__protobuf__DescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 5), google__protobuf__EnumDescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 6), google__protobuf__ServiceDescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 7), google__protobuf__FieldDescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 8), google__protobuf__FileOptions_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 9), google__protobuf__SourceCodeInfo_msg_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 14), google_protobuf_Edition_enum_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__DescriptorProto_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$1GGGGG3GGE";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 2), google__protobuf__FieldDescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 6), google__protobuf__FieldDescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 3), google__protobuf__DescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 4), google__protobuf__EnumDescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 5), google__protobuf__DescriptorProto__ExtensionRange_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 8), google__protobuf__OneofDescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 7), google__protobuf__MessageOptions_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 9), google__protobuf__DescriptorProto__ReservedRange_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__DescriptorProto__ExtensionRange_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$((3";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 3), google__protobuf__ExtensionRangeOptions_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__DescriptorProto__ReservedRange_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$((";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTable* google__protobuf__ExtensionRangeOptions_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$PaG4n`3t|G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 999), google__protobuf__UninterpretedOption_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 2), google__protobuf__ExtensionRangeOptions__Declaration_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 50), google__protobuf__FeatureSet_msg_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 3), google_protobuf_ExtensionRangeOptions_VerificationState_enum_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__ExtensionRangeOptions__Declaration_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$(11a//";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTable* google__protobuf__FieldDescriptorProto_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$11(44113(1f/";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 4), google_protobuf_FieldDescriptorProto_Label_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 5), google_protobuf_FieldDescriptorProto_Type_enum_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 8), google__protobuf__FieldOptions_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__OneofDescriptorProto_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$13";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 2), google__protobuf__OneofOptions_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__EnumDescriptorProto_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$1G3GE";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 2), google__protobuf__EnumValueDescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 3), google__protobuf__EnumOptions_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 4), google__protobuf__EnumDescriptorProto__EnumReservedRange_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__EnumDescriptorProto__EnumReservedRange_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$((";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTable* google__protobuf__EnumValueDescriptorProto_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$1(3";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 3), google__protobuf__EnumValueOptions_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__ServiceDescriptorProto_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$1G3";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 2), google__protobuf__MethodDescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 3), google__protobuf__ServiceOptions_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__MethodDescriptorProto_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$1113//";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 4), google__protobuf__MethodOptions_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__FileOptions_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$P1f14/1d///a/b/c/c/d11a111b11d3t|G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 9), google_protobuf_FileOptions_OptimizeMode_enum_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 50), google__protobuf__FeatureSet_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 999), google__protobuf__UninterpretedOption_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__MessageOptions_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$P///c/c/3z}G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 12), google__protobuf__FeatureSet_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 999), google__protobuf__UninterpretedOption_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__FieldOptions_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$P4//a/4c/d//4aHG33p}G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 1), google_protobuf_FieldOptions_CType_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 6), google_protobuf_FieldOptions_JSType_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 17), google_protobuf_FieldOptions_OptionRetention_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 19), google_protobuf_FieldOptions_OptionTargetType_enum_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 20), google__protobuf__FieldOptions__EditionDefault_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 21), google__protobuf__FeatureSet_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 22), google__protobuf__FieldOptions__FeatureSupport_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 999), google__protobuf__UninterpretedOption_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__FieldOptions__EditionDefault_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$a14";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 3), google_protobuf_Edition_enum_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__FieldOptions__FeatureSupport_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$4414";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 1), google_protobuf_Edition_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 2), google_protobuf_Edition_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 4), google_protobuf_Edition_enum_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__OneofOptions_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$P3e~G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 1), google__protobuf__FeatureSet_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 999), google__protobuf__UninterpretedOption_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__EnumOptions_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$Pa//b/3_~G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 7), google__protobuf__FeatureSet_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 999), google__protobuf__UninterpretedOption_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__EnumValueOptions_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$P/3/3b~G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 2), google__protobuf__FeatureSet_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 4), google__protobuf__FieldOptions__FeatureSupport_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 999), google__protobuf__UninterpretedOption_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__ServiceOptions_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$P``/3d}G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 34), google__protobuf__FeatureSet_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 999), google__protobuf__UninterpretedOption_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__MethodOptions_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$P``/43c}G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 34), google_protobuf_MethodOptions_IdempotencyLevel_enum_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 35), google__protobuf__FeatureSet_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 999), google__protobuf__UninterpretedOption_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__UninterpretedOption_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$aG1,+ 01";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 2), google__protobuf__UninterpretedOption__NamePart_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__UninterpretedOption__NamePart_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$1N/N";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTable* google__protobuf__FeatureSet_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$P444444";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 1), google_protobuf_FeatureSet_FieldPresence_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 2), google_protobuf_FeatureSet_EnumType_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 3), google_protobuf_FeatureSet_RepeatedFieldEncoding_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 4), google_protobuf_FeatureSet_Utf8Validation_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 5), google_protobuf_FeatureSet_MessageEncoding_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 6), google_protobuf_FeatureSet_JsonFormat_enum_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__FeatureSetDefaults_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$Gb44";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 1), google__protobuf__FeatureSetDefaults__FeatureSetEditionDefault_msg_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 4), google_protobuf_Edition_enum_init());
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 5), google_protobuf_Edition_enum_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__FeatureSetDefaults__FeatureSetEditionDefault_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$b433";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubEnum(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 3), google_protobuf_Edition_enum_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 4), google__protobuf__FeatureSet_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 5), google__protobuf__FeatureSet_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__SourceCodeInfo_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 1), google__protobuf__SourceCodeInfo__Location_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__SourceCodeInfo__Location_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$<M<M11aE";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTable* google__protobuf__GeneratedCodeInfo_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$G";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 1), google__protobuf__GeneratedCodeInfo__Annotation_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__GeneratedCodeInfo__Annotation_msg_init() {
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
  static const char* mini_descriptor = "!)~z!|a1qt_b)|i}{~~`!";
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

const upb_MiniTableEnum* google_protobuf_FeatureSet_Utf8Validation_enum_init() {
  static const upb_MiniTableEnum* mini_table = NULL;
  static const char* mini_descriptor = "!/";
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

