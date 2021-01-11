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

// Converts |ruby_val| to a upb_msgval according to |type_info|.
//
// The |arena| parameter indicates the lifetime of the container where this
// value will be assigned. It is used as follows:
// - If type is string or bytes, the string data will be copied into |arena|.
// - If type is message, and we need to auto-construct a message due to implicit
//   conversions (eg. Time -> Google::Protobuf::Timestamp), the new message
//   will be created in |arena|.
// - If type is message and the Ruby value is a message instance, we will fuse
//   the message's arena into |arena|, to ensure that this message outlives the
//   container.
upb_msgval Convert_RubyToUpb(VALUE ruby_val, const char *name,
                             TypeInfo type_info, upb_arena *arena);

// Converts |upb_val| to a Ruby VALUE according to |type_info|. This may involve
// creating a Ruby wrapper object.
//
// The |arena| parameter indicates the arena that owns the lifetime of
// |upb_val|. Any Ruby wrapper object that is created will reference |arena|
// and ensure it outlives the wrapper.
VALUE Convert_UpbToRuby(upb_msgval upb_val, TypeInfo type_info, VALUE arena);

// Creates a deep copy of |msgval| in |arena|.
upb_msgval Msgval_DeepCopy(upb_msgval msgval, TypeInfo type_info,
                           upb_arena *arena);

// Returns true if |val1| and |val2| are equal. Their type is given by
// |type_info|.
bool Msgval_IsEqual(upb_msgval val1, upb_msgval val2, TypeInfo type_info);

// Returns a hash value for the given upb_msgval.
uint64_t Msgval_GetHash(upb_msgval val, TypeInfo type_info, uint64_t seed);

#endif  // RUBY_PROTOBUF_CONVERT_H_
