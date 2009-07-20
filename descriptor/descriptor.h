/* Auto-generated from descriptor.proto.  Do not edit. */

#ifndef GOOGLE_PROTOBUF_DESCRIPTOR_H_
#define GOOGLE_PROTOBUF_DESCRIPTOR_H_

#include "upb_string.h"
#include "upb_array.h"

/* Enums. */

typedef enum google_protobuf_FieldDescriptorProto_Type {
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_DOUBLE = 1,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FLOAT = 2,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT64 = 3,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT64 = 4,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT32 = 5,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIXED64 = 6,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIXED32 = 7,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BOOL = 8,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_STRING = 9,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_GROUP = 10,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_MESSAGE = 11,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BYTES = 12,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT32 = 13,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_ENUM = 14,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SFIXED32 = 15,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SFIXED64 = 16,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SINT32 = 17,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SINT64 = 18
} google_protobuf_FieldDescriptorProto_Type;

typedef enum google_protobuf_FieldDescriptorProto_Label {
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_OPTIONAL = 1,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REQUIRED = 2,
  GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REPEATED = 3
} google_protobuf_FieldDescriptorProto_Label;

typedef enum google_protobuf_FileOptions_OptimizeMode {
  GOOGLE_PROTOBUF_FILEOPTIONS_SPEED = 1,
  GOOGLE_PROTOBUF_FILEOPTIONS_CODE_SIZE = 2
} google_protobuf_FileOptions_OptimizeMode;

typedef enum google_protobuf_FieldOptions_CType {
  GOOGLE_PROTOBUF_FIELDOPTIONS_CORD = 1,
  GOOGLE_PROTOBUF_FIELDOPTIONS_STRING_PIECE = 2
} google_protobuf_FieldOptions_CType;

/* Forward declarations of all message types.
 * So they can refer to each other in possibly-recursive ways. */

struct google_protobuf_FileDescriptorSet;
typedef struct google_protobuf_FileDescriptorSet
    google_protobuf_FileDescriptorSet;

struct google_protobuf_FileDescriptorProto;
typedef struct google_protobuf_FileDescriptorProto
    google_protobuf_FileDescriptorProto;

struct google_protobuf_DescriptorProto;
typedef struct google_protobuf_DescriptorProto
    google_protobuf_DescriptorProto;

struct google_protobuf_DescriptorProto_ExtensionRange;
typedef struct google_protobuf_DescriptorProto_ExtensionRange
    google_protobuf_DescriptorProto_ExtensionRange;

struct google_protobuf_FieldDescriptorProto;
typedef struct google_protobuf_FieldDescriptorProto
    google_protobuf_FieldDescriptorProto;

struct google_protobuf_EnumDescriptorProto;
typedef struct google_protobuf_EnumDescriptorProto
    google_protobuf_EnumDescriptorProto;

struct google_protobuf_EnumValueDescriptorProto;
typedef struct google_protobuf_EnumValueDescriptorProto
    google_protobuf_EnumValueDescriptorProto;

struct google_protobuf_ServiceDescriptorProto;
typedef struct google_protobuf_ServiceDescriptorProto
    google_protobuf_ServiceDescriptorProto;

struct google_protobuf_MethodDescriptorProto;
typedef struct google_protobuf_MethodDescriptorProto
    google_protobuf_MethodDescriptorProto;

struct google_protobuf_FileOptions;
typedef struct google_protobuf_FileOptions
    google_protobuf_FileOptions;

struct google_protobuf_MessageOptions;
typedef struct google_protobuf_MessageOptions
    google_protobuf_MessageOptions;

struct google_protobuf_FieldOptions;
typedef struct google_protobuf_FieldOptions
    google_protobuf_FieldOptions;

struct google_protobuf_EnumOptions;
typedef struct google_protobuf_EnumOptions
    google_protobuf_EnumOptions;

struct google_protobuf_EnumValueOptions;
typedef struct google_protobuf_EnumValueOptions
    google_protobuf_EnumValueOptions;

struct google_protobuf_ServiceOptions;
typedef struct google_protobuf_ServiceOptions
    google_protobuf_ServiceOptions;

struct google_protobuf_MethodOptions;
typedef struct google_protobuf_MethodOptions
    google_protobuf_MethodOptions;

struct google_protobuf_UninterpretedOption;
typedef struct google_protobuf_UninterpretedOption
    google_protobuf_UninterpretedOption;

struct google_protobuf_UninterpretedOption_NamePart;
typedef struct google_protobuf_UninterpretedOption_NamePart
    google_protobuf_UninterpretedOption_NamePart;

/* The message definitions themselves. */

struct google_protobuf_FileDescriptorSet {
  union {
    uint8_t bytes[1];
    struct {
      bool file:1;  /* = 1, repeated. */
    } has;
  } set_flags;
  UPB_MSG_ARRAY(google_protobuf_FileDescriptorProto)* file;
};
UPB_DEFINE_MSG_ARRAY(google_protobuf_FileDescriptorSet)

struct google_protobuf_FileDescriptorProto {
  union {
    uint8_t bytes[1];
    struct {
      bool name:1;  /* = 1, optional. */
      bool package:1;  /* = 2, optional. */
      bool dependency:1;  /* = 3, repeated. */
      bool message_type:1;  /* = 4, repeated. */
      bool enum_type:1;  /* = 5, repeated. */
      bool service:1;  /* = 6, repeated. */
      bool extension:1;  /* = 7, repeated. */
      bool options:1;  /* = 8, optional. */
    } has;
  } set_flags;
  struct upb_string* name;
  struct upb_string* package;
  struct upb_string_array* dependency;
  UPB_MSG_ARRAY(google_protobuf_DescriptorProto)* message_type;
  UPB_MSG_ARRAY(google_protobuf_EnumDescriptorProto)* enum_type;
  UPB_MSG_ARRAY(google_protobuf_ServiceDescriptorProto)* service;
  UPB_MSG_ARRAY(google_protobuf_FieldDescriptorProto)* extension;
  google_protobuf_FileOptions* options;
};
UPB_DEFINE_MSG_ARRAY(google_protobuf_FileDescriptorProto)

struct google_protobuf_DescriptorProto_ExtensionRange {
  union {
    uint8_t bytes[1];
    struct {
      bool start:1;  /* = 1, optional. */
      bool end:1;  /* = 2, optional. */
    } has;
  } set_flags;
  int32_t start;
  int32_t end;
};
UPB_DEFINE_MSG_ARRAY(google_protobuf_DescriptorProto_ExtensionRange)
struct google_protobuf_DescriptorProto {
  union {
    uint8_t bytes[1];
    struct {
      bool name:1;  /* = 1, optional. */
      bool field:1;  /* = 2, repeated. */
      bool nested_type:1;  /* = 3, repeated. */
      bool enum_type:1;  /* = 4, repeated. */
      bool extension_range:1;  /* = 5, repeated. */
      bool extension:1;  /* = 6, repeated. */
      bool options:1;  /* = 7, optional. */
    } has;
  } set_flags;
  struct upb_string* name;
  UPB_MSG_ARRAY(google_protobuf_FieldDescriptorProto)* field;
  UPB_MSG_ARRAY(google_protobuf_FieldDescriptorProto)* extension;
  UPB_MSG_ARRAY(google_protobuf_DescriptorProto)* nested_type;
  UPB_MSG_ARRAY(google_protobuf_EnumDescriptorProto)* enum_type;
  UPB_MSG_ARRAY(google_protobuf_DescriptorProto_ExtensionRange)* extension_range;
  google_protobuf_MessageOptions* options;
};
UPB_DEFINE_MSG_ARRAY(google_protobuf_DescriptorProto)

struct google_protobuf_FieldDescriptorProto {
  union {
    uint8_t bytes[1];
    struct {
      bool name:1;  /* = 1, optional. */
      bool extendee:1;  /* = 2, optional. */
      bool number:1;  /* = 3, optional. */
      bool label:1;  /* = 4, optional. */
      bool type:1;  /* = 5, optional. */
      bool type_name:1;  /* = 6, optional. */
      bool default_value:1;  /* = 7, optional. */
      bool options:1;  /* = 8, optional. */
    } has;
  } set_flags;
  struct upb_string* name;
  int32_t number;
  int32_t label; /* enum google.protobuf.FieldDescriptorProto.Label */
  int32_t type; /* enum google.protobuf.FieldDescriptorProto.Type */
  struct upb_string* type_name;
  struct upb_string* extendee;
  struct upb_string* default_value;
  google_protobuf_FieldOptions* options;
};
UPB_DEFINE_MSG_ARRAY(google_protobuf_FieldDescriptorProto)

struct google_protobuf_EnumDescriptorProto {
  union {
    uint8_t bytes[1];
    struct {
      bool name:1;  /* = 1, optional. */
      bool value:1;  /* = 2, repeated. */
      bool options:1;  /* = 3, optional. */
    } has;
  } set_flags;
  struct upb_string* name;
  UPB_MSG_ARRAY(google_protobuf_EnumValueDescriptorProto)* value;
  google_protobuf_EnumOptions* options;
};
UPB_DEFINE_MSG_ARRAY(google_protobuf_EnumDescriptorProto)

struct google_protobuf_EnumValueDescriptorProto {
  union {
    uint8_t bytes[1];
    struct {
      bool name:1;  /* = 1, optional. */
      bool number:1;  /* = 2, optional. */
      bool options:1;  /* = 3, optional. */
    } has;
  } set_flags;
  struct upb_string* name;
  int32_t number;
  google_protobuf_EnumValueOptions* options;
};
UPB_DEFINE_MSG_ARRAY(google_protobuf_EnumValueDescriptorProto)

struct google_protobuf_ServiceDescriptorProto {
  union {
    uint8_t bytes[1];
    struct {
      bool name:1;  /* = 1, optional. */
      bool method:1;  /* = 2, repeated. */
      bool options:1;  /* = 3, optional. */
    } has;
  } set_flags;
  struct upb_string* name;
  UPB_MSG_ARRAY(google_protobuf_MethodDescriptorProto)* method;
  google_protobuf_ServiceOptions* options;
};
UPB_DEFINE_MSG_ARRAY(google_protobuf_ServiceDescriptorProto)

struct google_protobuf_MethodDescriptorProto {
  union {
    uint8_t bytes[1];
    struct {
      bool name:1;  /* = 1, optional. */
      bool input_type:1;  /* = 2, optional. */
      bool output_type:1;  /* = 3, optional. */
      bool options:1;  /* = 4, optional. */
    } has;
  } set_flags;
  struct upb_string* name;
  struct upb_string* input_type;
  struct upb_string* output_type;
  google_protobuf_MethodOptions* options;
};
UPB_DEFINE_MSG_ARRAY(google_protobuf_MethodDescriptorProto)

struct google_protobuf_FileOptions {
  union {
    uint8_t bytes[1];
    struct {
      bool java_package:1;  /* = 1, optional. */
      bool java_outer_classname:1;  /* = 8, optional. */
      bool optimize_for:1;  /* = 9, optional. */
      bool java_multiple_files:1;  /* = 10, optional. */
      bool uninterpreted_option:1;  /* = 999, repeated. */
    } has;
  } set_flags;
  struct upb_string* java_package;
  struct upb_string* java_outer_classname;
  bool java_multiple_files;
  int32_t optimize_for; /* enum google.protobuf.FileOptions.OptimizeMode */
  UPB_MSG_ARRAY(google_protobuf_UninterpretedOption)* uninterpreted_option;
};
UPB_DEFINE_MSG_ARRAY(google_protobuf_FileOptions)

struct google_protobuf_MessageOptions {
  union {
    uint8_t bytes[1];
    struct {
      bool message_set_wire_format:1;  /* = 1, optional. */
      bool uninterpreted_option:1;  /* = 999, repeated. */
    } has;
  } set_flags;
  bool message_set_wire_format;
  UPB_MSG_ARRAY(google_protobuf_UninterpretedOption)* uninterpreted_option;
};
UPB_DEFINE_MSG_ARRAY(google_protobuf_MessageOptions)

struct google_protobuf_FieldOptions {
  union {
    uint8_t bytes[1];
    struct {
      bool ctype:1;  /* = 1, optional. */
      bool experimental_map_key:1;  /* = 9, optional. */
      bool uninterpreted_option:1;  /* = 999, repeated. */
    } has;
  } set_flags;
  int32_t ctype; /* enum google.protobuf.FieldOptions.CType */
  struct upb_string* experimental_map_key;
  UPB_MSG_ARRAY(google_protobuf_UninterpretedOption)* uninterpreted_option;
};
UPB_DEFINE_MSG_ARRAY(google_protobuf_FieldOptions)

struct google_protobuf_EnumOptions {
  union {
    uint8_t bytes[1];
    struct {
      bool uninterpreted_option:1;  /* = 999, repeated. */
    } has;
  } set_flags;
  UPB_MSG_ARRAY(google_protobuf_UninterpretedOption)* uninterpreted_option;
};
UPB_DEFINE_MSG_ARRAY(google_protobuf_EnumOptions)

struct google_protobuf_EnumValueOptions {
  union {
    uint8_t bytes[1];
    struct {
      bool uninterpreted_option:1;  /* = 999, repeated. */
    } has;
  } set_flags;
  UPB_MSG_ARRAY(google_protobuf_UninterpretedOption)* uninterpreted_option;
};
UPB_DEFINE_MSG_ARRAY(google_protobuf_EnumValueOptions)

struct google_protobuf_ServiceOptions {
  union {
    uint8_t bytes[1];
    struct {
      bool uninterpreted_option:1;  /* = 999, repeated. */
    } has;
  } set_flags;
  UPB_MSG_ARRAY(google_protobuf_UninterpretedOption)* uninterpreted_option;
};
UPB_DEFINE_MSG_ARRAY(google_protobuf_ServiceOptions)

struct google_protobuf_MethodOptions {
  union {
    uint8_t bytes[1];
    struct {
      bool uninterpreted_option:1;  /* = 999, repeated. */
    } has;
  } set_flags;
  UPB_MSG_ARRAY(google_protobuf_UninterpretedOption)* uninterpreted_option;
};
UPB_DEFINE_MSG_ARRAY(google_protobuf_MethodOptions)

struct google_protobuf_UninterpretedOption_NamePart {
  union {
    uint8_t bytes[1];
    struct {
      bool name_part:1;  /* = 1, required. */
      bool is_extension:1;  /* = 2, required. */
    } has;
  } set_flags;
  struct upb_string* name_part;
  bool is_extension;
};
UPB_DEFINE_MSG_ARRAY(google_protobuf_UninterpretedOption_NamePart)
struct google_protobuf_UninterpretedOption {
  union {
    uint8_t bytes[1];
    struct {
      bool name:1;  /* = 2, repeated. */
      bool identifier_value:1;  /* = 3, optional. */
      bool positive_int_value:1;  /* = 4, optional. */
      bool negative_int_value:1;  /* = 5, optional. */
      bool double_value:1;  /* = 6, optional. */
      bool string_value:1;  /* = 7, optional. */
    } has;
  } set_flags;
  UPB_MSG_ARRAY(google_protobuf_UninterpretedOption_NamePart)* name;
  struct upb_string* identifier_value;
  uint64_t positive_int_value;
  int64_t negative_int_value;
  double double_value;
  struct upb_string* string_value;
};
UPB_DEFINE_MSG_ARRAY(google_protobuf_UninterpretedOption)

extern google_protobuf_FileDescriptorProto google_protobuf_filedescriptor;

#endif  /* GOOGLE_PROTOBUF_DESCRIPTOR_H_ */
