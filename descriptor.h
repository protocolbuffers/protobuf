
enum pbstream_FieldDescriptorProtoType {
  PBSTREAM_TYPE_INT32    = 5,
  PBSTREAM_TYPE_STRING   = 9,
  PBSTREAM_TYPE_MESSAGE  = 11,
  PBSTREAM_TYPE_ENUM     = 14,
};

struct pbstream_FileDescriptorSet {
  int file_count;
  struct pbstream_FileDescriptorProto *file;
};

struct pbstream_FileDescriptorProto {
  int message_type_count;
  struct pbstream_FileDescriptorProto *message_type;
};

struct pbstream_DescriptorProto {
  char *name;
  int field_count;
  struct pbstream_FieldDescriptorProto *field;
  int nested_type_count;
  struct pbstream_DescriptorProto *nested_type;
  int enum_type_count;
  struct pbstream_EnumDescriptorProto *enum_type;
};

struct pbstream_FieldDescriptorProto {
  char *name;
  int32_t number;
  enum pbstream_FieldDescriptorProtoType type;
  char *type_name;
};

struct pbstream_EnumValueDescriptorProto {
  char *name;
  int32_t number;
};

struct pbstream_EnumDescriptorProto {
  char *name;
  int value_count;
  struct pbstream_EnumValueDescriptorProto *value;
};

