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

#import <Foundation/Foundation.h>

@class GPBMessage;
@class GPBExtensionRegistry;

// Reads and decodes protocol message fields.
// Subclassing of GPBCodedInputStream is NOT supported.
@interface GPBCodedInputStream : NSObject

+ (instancetype)streamWithData:(NSData *)data;
- (instancetype)initWithData:(NSData *)data;

// Attempt to read a field tag, returning zero if we have reached EOF.
// Protocol message parsers use this to read tags, since a protocol message
// may legally end wherever a tag occurs, and zero is not a valid tag number.
- (int32_t)readTag;

- (double)readDouble;
- (float)readFloat;
- (uint64_t)readUInt64;
- (uint32_t)readUInt32;
- (int64_t)readInt64;
- (int32_t)readInt32;
- (uint64_t)readFixed64;
- (uint32_t)readFixed32;
- (int32_t)readEnum;
- (int32_t)readSFixed32;
- (int64_t)readSFixed64;
- (int32_t)readSInt32;
- (int64_t)readSInt64;
- (BOOL)readBool;
- (NSString *)readString;
- (NSData *)readData;

// Read an embedded message field value from the stream.
- (void)readMessage:(GPBMessage *)message
    extensionRegistry:(GPBExtensionRegistry *)extensionRegistry;

// Reads and discards a single field, given its tag value. Returns NO if the
// tag is an endgroup tag, in which case nothing is skipped.  Otherwise,
// returns YES.
- (BOOL)skipField:(int32_t)tag;

// Reads and discards an entire message.  This will read either until EOF
// or until an endgroup tag, whichever comes first.
- (void)skipMessage;

// Verifies that the last call to readTag() returned the given tag value.
// This is used to verify that a nested group ended with the correct end tag.
// Throws NSParseErrorException if value does not match the last tag.
- (void)checkLastTagWas:(int32_t)value;

@end
