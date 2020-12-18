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

#ifndef RUBY_PROTOBUF_CONVERT_H_
#define RUBY_PROTOBUF_CONVERT_H_

#include <ruby/ruby.h>

#include "protobuf.h"
#include "ruby-upb.h"

// Converts |ruby_val| to a upb_msgval according to |type|. If type is
// UPB_TYPE_MESSAGE, then |desc| must be the Descriptor for this message type.
// If type is string, message, or bytes, then |arena| will be used to copy
// string data or fuse this arena to the given message's arena.
upb_msgval Convert_RubyToUpb(VALUE ruby_val, const char *name,
                             TypeInfo type_info, upb_arena *arena);

// Converts |upb_val| to a Ruby VALUE according to |type_info|. This may involve
// creating a Ruby wrapper object. If type == UPB_TYPE_MESSAGE, then |desc| must
// be the Descriptor for this message type. Any newly created wrapper object
// will reference |arena|.
VALUE Convert_UpbToRuby(upb_msgval upb_val, TypeInfo type_info, VALUE arena);

upb_msgval Message_DeepCopyMsgval(upb_msgval msgval, TypeInfo type_info,
                                  upb_arena* arena);
bool Message_MsgvalEqual(upb_msgval val1, upb_msgval val2, TypeInfo type_info);
uint64_t Message_HashMsgval(upb_msgval val, TypeInfo type_info, uint64_t seed);

#endif  // RUBY_PROTOBUF_CONVERT_H_
