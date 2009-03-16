
/* FileDescriptorSet */
struct pbstream_FieldDescriptorProto pbstream_FileDescriptorSet_fields[] = {
  {"file", 1, PBSTREAM_LABEL_OPTIONAL, PBSTREAM_TYPE_MESSAGE, "FileDescriptorProto"},
};
struct pbstream_DescriptorProto pbstream_FileDescriptorSet_desc = {
  "FileDescriptorSet", .field_count=1, .field = pbstream_FileDescriptorSet_fields,
};

/* FileDescriptorProto */
struct pbstream_FieldDescriptorProto pbstream_FileDescriptorProto_fields[] = {
  {"message_type", 4, PBSTREAM_LABEL_REPEATED, PBSTREAM_TYPE_MESSAGE, "DescriptorProto"},
};
struct pbstream_DescriptorProto pbstream_FileDescriptorProto_desc = {
  "FileDescriptorProto", .field_count=1, .field = pbstream_FileDescriptorProto_fields
};

/* DescriptorProto */
struct pbstream_FieldDescriptorProto pbstream_DescriptorProto_fields[] = {
  {"name", 1, PBSTREAM_LABEL_OPTIONAL, PBSTREAM_TYPE_STRING},
  {"field", 2, PBSTREAM_LABEL_REPEATED, PBSTREAM_TYPE_MESSAGE, "FieldDescriptorProto"},
  {"nested_type", 3, PBSTREAM_LABEL_REPEATED, PBSTREAM_TYPE_MESSAGE, "DescriptorProto"},
  {"enum_type", 4, PBSTREAM_LABEL_REPEATED, PBSTREAM_TYPE_MESSAGE, "EnumDescriptorProto"},
};
struct pbstream_DescriptorProto pbstream_DescriptorProto_desc = {
  "DescriptorProto", .field_count=4, .field = pbstream_DescriptorProto_fields
};

/* FieldDescriptorProto */
struct pbstream_FieldDescriptorProto pbstream_FieldDescriptorProto_fields[] = {
  {"name", 1, PBSTREAM_LABEL_OPTIONAL, PBSTREAM_TYPE_STRING},
  {"number", 3, PBSTREAM_LABEL_OPTIONAL, PBSTREAM_TYPE_INT32},
  {"type", 5, PBSTREAM_LABEL_OPTIONAL, PBSTREAM_TYPE_ENUM, "Type"},
  {"type_name", 6, PBSTREAM_LABEL_OPTIONAL, PBSTREAM_TYPE_STRING},
};
struct pbstream_DescriptorProto pbstream_FieldDescriptorProto_desc = {
  "FieldDescriptorProto", .field_count=4, .field = pbstream_FieldDescriptorProto_fields
};

/* EnumDescriptorProto */
struct pbstream_FieldDescriptorProto pbstream_EnumDescriptorProto_fields[] = {
  {"name", 1, PBSTREAM_LABEL_OPTIONAL, PBSTREAM_TYPE_STRING},
  {"value", 2, PBSTREAM_LABEL_REPEATED, PBSTREAM_TYPE_MESSAGE, "EnumValueDescriptorProto"},
};
struct pbstream_DescriptorProto pbstream_EnumDescriptorProto_desc = {
  "EnumDescriptorProto", .field_count=2, .field = pbstream_EnumDescriptorProto_fields
};

/* EnumValueDescriptorProto */
struct pbstream_FieldDescriptorProto pbstream_EnumValueDescriptorProto_fields[] = {
  {"name", 1, PBSTREAM_LABEL_OPTIONAL, PBSTREAM_TYPE_STRING},
  {"number", 2, PBSTREAM_LABEL_OPTIONAL, PBSTREAM_TYPE_INT32},
};
struct pbstream_DescriptorProto pbstream_EnumValueDescriptorProto_desc = {
  "EnumDescriptorProto", .field_count=4, .field = pbstream_EnumValueDescriptorProto_fields
};

