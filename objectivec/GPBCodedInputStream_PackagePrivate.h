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

// This header is private to the ProtobolBuffers library and must NOT be
// included by any sources outside this library. The contents of this file are
// subject to change at any time without notice.

#import "GPBCodedInputStream.h"

@class GPBUnknownFieldSet;
@class GPBFieldDescriptor;

typedef struct GPBCodedInputStreamState {
  const uint8_t *bytes;
  size_t bufferSize;
  size_t bufferPos;

  // For parsing subsections of an input stream you can put a hard limit on
  // how much should be read. Normally the limit is the end of the stream,
  // but you can adjust it to anywhere, and if you hit it you will be at the
  // end of the stream, until you adjust the limit.
  size_t currentLimit;
  int32_t lastTag;
  NSUInteger recursionDepth;
} GPBCodedInputStreamState;

@interface GPBCodedInputStream () {
 @package
  struct GPBCodedInputStreamState state_;
  NSData *buffer_;
}

// Group support is deprecated, so we hide this interface from users, but
// support for older data.
- (void)readGroup:(int32_t)fieldNumber
              message:(GPBMessage *)message
    extensionRegistry:(GPBExtensionRegistry *)extensionRegistry;

// Reads a group field value from the stream and merges it into the given
// UnknownFieldSet.
- (void)readUnknownGroup:(int32_t)fieldNumber
                 message:(GPBUnknownFieldSet *)message;

// Reads a map entry.
- (void)readMapEntry:(id)mapDictionary
    extensionRegistry:(GPBExtensionRegistry *)extensionRegistry
                field:(GPBFieldDescriptor *)field
        parentMessage:(GPBMessage *)parentMessage;
@end

CF_EXTERN_C_BEGIN

int32_t GPBCodedInputStreamReadTag(GPBCodedInputStreamState *state);

double GPBCodedInputStreamReadDouble(GPBCodedInputStreamState *state);
float GPBCodedInputStreamReadFloat(GPBCodedInputStreamState *state);
uint64_t GPBCodedInputStreamReadUInt64(GPBCodedInputStreamState *state);
uint32_t GPBCodedInputStreamReadUInt32(GPBCodedInputStreamState *state);
int64_t GPBCodedInputStreamReadInt64(GPBCodedInputStreamState *state);
int32_t GPBCodedInputStreamReadInt32(GPBCodedInputStreamState *state);
uint64_t GPBCodedInputStreamReadFixed64(GPBCodedInputStreamState *state);
uint32_t GPBCodedInputStreamReadFixed32(GPBCodedInputStreamState *state);
int32_t GPBCodedInputStreamReadEnum(GPBCodedInputStreamState *state);
int32_t GPBCodedInputStreamReadSFixed32(GPBCodedInputStreamState *state);
int64_t GPBCodedInputStreamReadSFixed64(GPBCodedInputStreamState *state);
int32_t GPBCodedInputStreamReadSInt32(GPBCodedInputStreamState *state);
int64_t GPBCodedInputStreamReadSInt64(GPBCodedInputStreamState *state);
BOOL GPBCodedInputStreamReadBool(GPBCodedInputStreamState *state);
NSString *GPBCodedInputStreamReadRetainedString(GPBCodedInputStreamState *state)
    __attribute((ns_returns_retained));
NSData *GPBCodedInputStreamReadRetainedBytes(GPBCodedInputStreamState *state)
    __attribute((ns_returns_retained));
NSData *GPBCodedInputStreamReadRetainedBytesNoCopy(
    GPBCodedInputStreamState *state) __attribute((ns_returns_retained));

size_t GPBCodedInputStreamPushLimit(GPBCodedInputStreamState *state,
                                    size_t byteLimit);
void GPBCodedInputStreamPopLimit(GPBCodedInputStreamState *state,
                                 size_t oldLimit);
size_t GPBCodedInputStreamBytesUntilLimit(GPBCodedInputStreamState *state);
BOOL GPBCodedInputStreamIsAtEnd(GPBCodedInputStreamState *state);
void GPBCodedInputStreamCheckLastTagWas(GPBCodedInputStreamState *state,
                                        int32_t value);

CF_EXTERN_C_END
