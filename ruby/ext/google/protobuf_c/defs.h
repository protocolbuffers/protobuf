// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef RUBY_PROTOBUF_DEFS_H_
#define RUBY_PROTOBUF_DEFS_H_

#include "protobuf.h"
#include "ruby-upb.h"

// -----------------------------------------------------------------------------
// TypeInfo
// -----------------------------------------------------------------------------

// This bundles a upb_CType and msgdef/enumdef when appropriate. This is
// convenient for functions that need type information but cannot necessarily
// assume a upb_FieldDef will be available.
//
// For example, Google::Protobuf::Map and Google::Protobuf::RepeatedField can
// be constructed with type information alone:
//
//   # RepeatedField will internally store the type information in a TypeInfo.
//   Google::Protobuf::RepeatedField.new(:message, FooMessage)

typedef struct {
  upb_CType type;
  union {
    const upb_MessageDef* msgdef;  // When type == kUpb_CType_Message
    const upb_EnumDef* enumdef;    // When type == kUpb_CType_Enum
  } def;
} TypeInfo;

static inline TypeInfo TypeInfo_get(const upb_FieldDef* f) {
  TypeInfo ret = {upb_FieldDef_CType(f), {NULL}};
  switch (ret.type) {
    case kUpb_CType_Message:
      ret.def.msgdef = upb_FieldDef_MessageSubDef(f);
      break;
    case kUpb_CType_Enum:
      ret.def.enumdef = upb_FieldDef_EnumSubDef(f);
      break;
    default:
      break;
  }
  return ret;
}

TypeInfo TypeInfo_FromClass(int argc, VALUE* argv, int skip_arg,
                            VALUE* type_class, VALUE* init_arg);

static inline TypeInfo TypeInfo_from_type(upb_CType type) {
  TypeInfo ret = {type};
  assert(type != kUpb_CType_Message && type != kUpb_CType_Enum);
  return ret;
}

// -----------------------------------------------------------------------------
// Other utilities
// -----------------------------------------------------------------------------

VALUE Descriptor_DefToClass(const upb_MessageDef* m);

// Returns the underlying msgdef, enumdef, or symtab (respectively) for the
// given Descriptor, EnumDescriptor, or DescriptorPool Ruby object.
const upb_EnumDef* EnumDescriptor_GetEnumDef(VALUE enum_desc_rb);
const upb_DefPool* DescriptorPool_GetSymtab(VALUE desc_pool_rb);
const upb_MessageDef* Descriptor_GetMsgDef(VALUE desc_rb);

// Returns a upb field type for the given Ruby symbol
// (eg. :float => kUpb_CType_Float).
upb_CType ruby_to_fieldtype(VALUE type);

// The singleton generated pool (a DescriptorPool object).
extern VALUE generated_pool;

// Call at startup to register all types in this module.
void Defs_register(VALUE module);

#endif  // RUBY_PROTOBUF_DEFS_H_
