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

#ifndef PHP_PROTOBUF_MESSAGE_H_
#define PHP_PROTOBUF_MESSAGE_H_

#include <stdbool.h>

#include "def.h"

// Registers the PHP Message class.
void Message_ModuleInit();

// Gets a upb_Message* for the PHP object |val|, which must either be a Message
// object or 'null'. Returns true and stores the message in |msg| if the
// conversion succeeded (we can't return upb_Message* because null->NULL is a valid
// conversion). Returns false and raises a PHP error if this isn't a Message
// object or null, or if the Message object doesn't match this Descriptor.
//
// The given |arena| will be fused to this message's arena.
bool Message_GetUpbMessage(zval *val, const Descriptor *desc, upb_Arena *arena,
                           upb_Message **msg);

// Gets or creates a PHP Message object to wrap the given upb_Message* and |desc|
// and returns it in |val|. The PHP object will keep a reference to this |arena|
// to ensure the underlying message data stays alive.
//
// If |msg| is NULL, this will return a PHP null.
void Message_GetPhpWrapper(zval *val, const Descriptor *desc, upb_Message *msg,
                           zval *arena);

bool ValueEq(upb_MessageValue val1, upb_MessageValue val2, TypeInfo type);

#endif  // PHP_PROTOBUF_MESSAGE_H_
