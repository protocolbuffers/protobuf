// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// -----------------------------------------------------------------------------
// Exposing inlined UPB functions. Strictly free of dependencies on
// Ruby interpreter internals.

#include "ruby-upb.h"

upb_Arena* Arena_create() { return upb_Arena_Init(NULL, 0, &upb_alloc_global); }

google_protobuf_FileDescriptorProto* FileDescriptorProto_parse(
    const char* serialized_file_proto, size_t length, upb_Arena* arena) {
  return google_protobuf_FileDescriptorProto_parse(serialized_file_proto,
                                                   length, arena);
}

char* EnumDescriptor_serialized_options(const upb_EnumDef* enumdef,
                                        size_t* size, upb_Arena* arena) {
  const google_protobuf_EnumOptions* opts = upb_EnumDef_Options(enumdef);
  char* serialized = google_protobuf_EnumOptions_serialize(opts, arena, size);
  return serialized;
}

char* FileDescriptor_serialized_options(const upb_FileDef* filedef,
                                        size_t* size, upb_Arena* arena) {
  const google_protobuf_FileOptions* opts = upb_FileDef_Options(filedef);
  char* serialized = google_protobuf_FileOptions_serialize(opts, arena, size);
  return serialized;
}

char* Descriptor_serialized_options(const upb_MessageDef* msgdef, size_t* size,
                                    upb_Arena* arena) {
  const google_protobuf_MessageOptions* opts = upb_MessageDef_Options(msgdef);
  char* serialized =
      google_protobuf_MessageOptions_serialize(opts, arena, size);
  return serialized;
}

char* OneOfDescriptor_serialized_options(const upb_OneofDef* oneofdef,
                                         size_t* size, upb_Arena* arena) {
  const google_protobuf_OneofOptions* opts = upb_OneofDef_Options(oneofdef);
  char* serialized = google_protobuf_OneofOptions_serialize(opts, arena, size);
  return serialized;
}

char* FieldDescriptor_serialized_options(const upb_FieldDef* fielddef,
                                         size_t* size, upb_Arena* arena) {
  const google_protobuf_FieldOptions* opts = upb_FieldDef_Options(fielddef);
  char* serialized = google_protobuf_FieldOptions_serialize(opts, arena, size);
  return serialized;
}

char* ServiceDescriptor_serialized_options(const upb_ServiceDef* servicedef,
                                           size_t* size, upb_Arena* arena) {
  const google_protobuf_ServiceOptions* opts =
      upb_ServiceDef_Options(servicedef);
  char* serialized =
      google_protobuf_ServiceOptions_serialize(opts, arena, size);
  return serialized;
}

char* MethodDescriptor_serialized_options(const upb_MethodDef* methoddef,
                                          size_t* size, upb_Arena* arena) {
  const google_protobuf_MethodOptions* opts = upb_MethodDef_Options(methoddef);
  char* serialized = google_protobuf_MethodOptions_serialize(opts, arena, size);
  return serialized;
}
