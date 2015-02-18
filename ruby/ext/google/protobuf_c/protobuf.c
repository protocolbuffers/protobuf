// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
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

#include "protobuf.h"

// -----------------------------------------------------------------------------
// Global map from upb {msg,enum}defs to wrapper Descriptor/EnumDescriptor
// instances.
// -----------------------------------------------------------------------------

// This is a hash table from def objects (encoded by converting pointers to
// Ruby integers) to MessageDef/EnumDef instances (as Ruby values).
VALUE upb_def_to_ruby_obj_map;

void add_def_obj(const void* def, VALUE value) {
  rb_hash_aset(upb_def_to_ruby_obj_map, ULL2NUM((intptr_t)def), value);
}

VALUE get_def_obj(const void* def) {
  return rb_hash_aref(upb_def_to_ruby_obj_map, ULL2NUM((intptr_t)def));
}

// -----------------------------------------------------------------------------
// Utilities.
// -----------------------------------------------------------------------------

// Raises a Ruby error if |status| is not OK, using its error message.
void check_upb_status(const upb_status* status, const char* msg) {
  if (!upb_ok(status)) {
    rb_raise(rb_eRuntimeError, "%s: %s\n", msg, upb_status_errmsg(status));
  }
}

// String encodings: we look these up once, at load time, and then cache them
// here.
rb_encoding* kRubyStringUtf8Encoding;
rb_encoding* kRubyStringASCIIEncoding;
rb_encoding* kRubyString8bitEncoding;

// -----------------------------------------------------------------------------
// Initialization/entry point.
// -----------------------------------------------------------------------------

// This must be named "Init_protobuf_c" because the Ruby module is named
// "protobuf_c" -- the VM looks for this symbol in our .so.
void Init_protobuf_c() {
  VALUE google = rb_define_module("Google");
  VALUE protobuf = rb_define_module_under(google, "Protobuf");
  VALUE internal = rb_define_module_under(protobuf, "Internal");
  DescriptorPool_register(protobuf);
  Descriptor_register(protobuf);
  FieldDescriptor_register(protobuf);
  OneofDescriptor_register(protobuf);
  EnumDescriptor_register(protobuf);
  MessageBuilderContext_register(internal);
  OneofBuilderContext_register(internal);
  EnumBuilderContext_register(internal);
  Builder_register(internal);
  RepeatedField_register(protobuf);
  Map_register(protobuf);

  rb_define_singleton_method(protobuf, "encode", Google_Protobuf_encode, 1);
  rb_define_singleton_method(protobuf, "decode", Google_Protobuf_decode, 2);
  rb_define_singleton_method(protobuf, "encode_json",
                             Google_Protobuf_encode_json, 1);
  rb_define_singleton_method(protobuf, "decode_json",
                             Google_Protobuf_decode_json, 2);

  rb_define_singleton_method(protobuf, "deep_copy",
                             Google_Protobuf_deep_copy, 1);

  kRubyStringUtf8Encoding = rb_utf8_encoding();
  kRubyStringASCIIEncoding = rb_usascii_encoding();
  kRubyString8bitEncoding = rb_ascii8bit_encoding();

  upb_def_to_ruby_obj_map = rb_hash_new();
  rb_gc_register_address(&upb_def_to_ruby_obj_map);
}
