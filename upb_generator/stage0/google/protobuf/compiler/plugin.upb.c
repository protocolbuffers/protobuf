#include <stddef.h>
#include "upb/generated_code_support.h"
#include "google/protobuf/compiler/plugin.upb.h"

#include "google/protobuf/descriptor.upb.h"
static upb_Arena* upb_BootstrapArena() {
  static upb_Arena* arena = NULL;
  if (!arena) arena = upb_Arena_New();
  return arena;
}

const upb_MiniTable* google__protobuf__compiler__Version_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$(((1";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  return mini_table;
}

const upb_MiniTable* google__protobuf__compiler__CodeGeneratorRequest_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$E13kGaG";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 15), google__protobuf__FileDescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 17), google__protobuf__FileDescriptorProto_msg_init());
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 3), google__protobuf__compiler__Version_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__compiler__CodeGeneratorResponse_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$1,lG";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 15), google__protobuf__compiler__CodeGeneratorResponse__File_msg_init());
  return mini_table;
}

const upb_MiniTable* google__protobuf__compiler__CodeGeneratorResponse__File_msg_init() {
  static upb_MiniTable* mini_table = NULL;
  static const char* mini_descriptor = "$11l13";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTable_Build(mini_descriptor, strlen(mini_descriptor),
                          upb_BootstrapArena(), NULL);
  upb_MiniTable_SetSubMessage(mini_table, (upb_MiniTableField*)upb_MiniTable_FindFieldByNumber(mini_table, 16), google__protobuf__GeneratedCodeInfo_msg_init());
  return mini_table;
}

const upb_MiniTableEnum* google_protobuf_compiler_CodeGeneratorResponse_Feature_enum_init() {
  static const upb_MiniTableEnum* mini_table = NULL;
  static const char* mini_descriptor = "!)";
  if (mini_table) return mini_table;
  mini_table =
      upb_MiniTableEnum_Build(mini_descriptor, strlen(mini_descriptor),
                              upb_BootstrapArena(), NULL);
  return mini_table;
}

