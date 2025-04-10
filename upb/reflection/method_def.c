// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/reflection/internal/method_def.h"

#include "upb/reflection/def_type.h"
#include "upb/reflection/internal/def_builder.h"
#include "upb/reflection/service_def.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_MethodDef {
  const UPB_DESC(MethodOptions*) opts;
  const UPB_DESC(FeatureSet*) resolved_features;
  upb_ServiceDef* service;
  const char* full_name;
  const upb_MessageDef* input_type;
  const upb_MessageDef* output_type;
  int index;
  bool client_streaming;
  bool server_streaming;
};

upb_MethodDef* _upb_MethodDef_At(const upb_MethodDef* m, int i) {
  return (upb_MethodDef*)&m[i];
}

const upb_ServiceDef* upb_MethodDef_Service(const upb_MethodDef* m) {
  return m->service;
}

const UPB_DESC(MethodOptions) * upb_MethodDef_Options(const upb_MethodDef* m) {
  return m->opts;
}

bool upb_MethodDef_HasOptions(const upb_MethodDef* m) {
  return m->opts != (void*)kUpbDefOptDefault;
}

const UPB_DESC(FeatureSet) *
    upb_MethodDef_ResolvedFeatures(const upb_MethodDef* m) {
  return m->resolved_features;
}

const char* upb_MethodDef_FullName(const upb_MethodDef* m) {
  return m->full_name;
}

const char* upb_MethodDef_Name(const upb_MethodDef* m) {
  return _upb_DefBuilder_FullToShort(m->full_name);
}

int upb_MethodDef_Index(const upb_MethodDef* m) { return m->index; }

const upb_MessageDef* upb_MethodDef_InputType(const upb_MethodDef* m) {
  return m->input_type;
}

const upb_MessageDef* upb_MethodDef_OutputType(const upb_MethodDef* m) {
  return m->output_type;
}

bool upb_MethodDef_ClientStreaming(const upb_MethodDef* m) {
  return m->client_streaming;
}

bool upb_MethodDef_ServerStreaming(const upb_MethodDef* m) {
  return m->server_streaming;
}

static void create_method(upb_DefBuilder* ctx,
                          const UPB_DESC(MethodDescriptorProto*) method_proto,
                          const UPB_DESC(FeatureSet*) parent_features,
                          upb_ServiceDef* s, upb_MethodDef* m) {
  UPB_DEF_SET_OPTIONS(m->opts, MethodDescriptorProto, MethodOptions,
                      method_proto);
  m->resolved_features = _upb_DefBuilder_ResolveFeatures(
      ctx, parent_features, UPB_DESC(MethodOptions_features)(m->opts));

  upb_StringView name = UPB_DESC(MethodDescriptorProto_name)(method_proto);

  m->service = s;
  m->full_name =
      _upb_DefBuilder_MakeFullName(ctx, upb_ServiceDef_FullName(s), name);
  m->client_streaming =
      UPB_DESC(MethodDescriptorProto_client_streaming)(method_proto);
  m->server_streaming =
      UPB_DESC(MethodDescriptorProto_server_streaming)(method_proto);
  m->input_type = _upb_DefBuilder_Resolve(
      ctx, m->full_name, m->full_name,
      UPB_DESC(MethodDescriptorProto_input_type)(method_proto),
      UPB_DEFTYPE_MSG);
  m->output_type = _upb_DefBuilder_Resolve(
      ctx, m->full_name, m->full_name,
      UPB_DESC(MethodDescriptorProto_output_type)(method_proto),
      UPB_DEFTYPE_MSG);
}

// Allocate and initialize an array of |n| method defs belonging to |s|.
upb_MethodDef* _upb_MethodDefs_New(upb_DefBuilder* ctx, int n,
                                   const UPB_DESC(MethodDescriptorProto*)
                                       const* protos,
                                   const UPB_DESC(FeatureSet*) parent_features,
                                   upb_ServiceDef* s) {
  upb_MethodDef* m = UPB_DEFBUILDER_ALLOCARRAY(ctx, upb_MethodDef, n);
  for (int i = 0; i < n; i++) {
    create_method(ctx, protos[i], parent_features, s, &m[i]);
    m[i].index = i;
  }
  return m;
}
