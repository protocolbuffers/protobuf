/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "upb/reflection/def_builder_internal.h"
#include "upb/reflection/def_type.h"
#include "upb/reflection/method_def_internal.h"
#include "upb/reflection/service_def.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_MethodDef {
  const UPB_DESC(MethodOptions) * opts;
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
                          const UPB_DESC(MethodDescriptorProto) * method_proto,
                          upb_ServiceDef* s, upb_MethodDef* m) {
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

  UPB_DEF_SET_OPTIONS(m->opts, MethodDescriptorProto, MethodOptions,
                      method_proto);
}

// Allocate and initialize an array of |n| method defs belonging to |s|.
upb_MethodDef* _upb_MethodDefs_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(MethodDescriptorProto) * const* protos, upb_ServiceDef* s) {
  upb_MethodDef* m = _upb_DefBuilder_Alloc(ctx, sizeof(upb_MethodDef) * n);
  for (int i = 0; i < n; i++) {
    create_method(ctx, protos[i], s, &m[i]);
    m[i].index = i;
  }
  return m;
}
