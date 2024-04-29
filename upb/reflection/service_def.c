// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/reflection/internal/service_def.h"

#include "upb/reflection/def_type.h"
#include "upb/reflection/internal/def_builder.h"
#include "upb/reflection/internal/file_def.h"
#include "upb/reflection/internal/method_def.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_ServiceDef {
  const UPB_DESC(ServiceOptions) * opts;
  const upb_FileDef* file;
  const char* full_name;
  upb_MethodDef* methods;
  int method_count;
  int index;
};

upb_ServiceDef* _upb_ServiceDef_At(const upb_ServiceDef* s, int index) {
  return (upb_ServiceDef*)&s[index];
}

const UPB_DESC(ServiceOptions) *
    upb_ServiceDef_Options(const upb_ServiceDef* s) {
  return s->opts;
}

bool upb_ServiceDef_HasOptions(const upb_ServiceDef* s) {
  return s->opts != (void*)kUpbDefOptDefault;
}

const char* upb_ServiceDef_FullName(const upb_ServiceDef* s) {
  return s->full_name;
}

const char* upb_ServiceDef_Name(const upb_ServiceDef* s) {
  return _upb_DefBuilder_FullToShort(s->full_name);
}

int upb_ServiceDef_Index(const upb_ServiceDef* s) { return s->index; }

const upb_FileDef* upb_ServiceDef_File(const upb_ServiceDef* s) {
  return s->file;
}

int upb_ServiceDef_MethodCount(const upb_ServiceDef* s) {
  return s->method_count;
}

const upb_MethodDef* upb_ServiceDef_Method(const upb_ServiceDef* s, int i) {
  return (i < 0 || i >= s->method_count) ? NULL
                                         : _upb_MethodDef_At(s->methods, i);
}

const upb_MethodDef* upb_ServiceDef_FindMethodByName(const upb_ServiceDef* s,
                                                     const char* name) {
  for (int i = 0; i < s->method_count; i++) {
    const upb_MethodDef* m = _upb_MethodDef_At(s->methods, i);
    if (strcmp(name, upb_MethodDef_Name(m)) == 0) {
      return m;
    }
  }
  return NULL;
}

static void create_service(upb_DefBuilder* ctx,
                           const UPB_DESC(ServiceDescriptorProto) * svc_proto,
                           upb_ServiceDef* s) {
  upb_StringView name;
  size_t n;

  // Must happen before _upb_DefBuilder_Add()
  s->file = _upb_DefBuilder_File(ctx);

  name = UPB_DESC(ServiceDescriptorProto_name)(svc_proto);
  const char* package = _upb_FileDef_RawPackage(s->file);
  s->full_name = _upb_DefBuilder_MakeFullName(ctx, package, name);
  _upb_DefBuilder_Add(ctx, s->full_name,
                      _upb_DefType_Pack(s, UPB_DEFTYPE_SERVICE));

  const UPB_DESC(MethodDescriptorProto)* const* methods =
      UPB_DESC(ServiceDescriptorProto_method)(svc_proto, &n);
  s->method_count = n;
  s->methods = _upb_MethodDefs_New(ctx, n, methods, s);

  UPB_DEF_SET_OPTIONS(s->opts, ServiceDescriptorProto, ServiceOptions,
                      svc_proto);
}

upb_ServiceDef* _upb_ServiceDefs_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(ServiceDescriptorProto) * const* protos) {
  _upb_DefType_CheckPadding(sizeof(upb_ServiceDef));

  upb_ServiceDef* s = _upb_DefBuilder_Alloc(ctx, sizeof(upb_ServiceDef) * n);
  for (int i = 0; i < n; i++) {
    create_service(ctx, protos[i], &s[i]);
    s[i].index = i;
  }
  return s;
}
