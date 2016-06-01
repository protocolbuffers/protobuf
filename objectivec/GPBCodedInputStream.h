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

NS_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

/// GPBCodedInputStream exception name. Exceptions raised from
/// GPBCodedInputStream contain an underlying error in the userInfo dictionary
/// under the GPBCodedInputStreamUnderlyingErrorKey key.
extern NSString *const GPBCodedInputStreamException;

/// The key under which the underlying NSError from the exception is stored.
extern NSString *const GPBCodedInputStreamUnderlyingErrorKey;

/// NSError domain used for GPBCodedInputStream errors.
extern NSString *const GPBCodedInputStreamErrorDomain;

/// Error code for NSError with GPBCodedInputStreamErrorDomain.
typedef NS_ENUM(NSInteger, GPBCodedInputStreamErrorCode) {
  /// The size does not fit in the remaining bytes to be read.
  GPBCodedInputStreamErrorInvalidSize = -100,
  /// Attempted to read beyond the subsection limit.
  GPBCodedInputStreamErrorSubsectionLimitReached = -101,
  /// The requested subsection limit is invalid.
  GPBCodedInputStreamErrorInvalidSubsectionLimit = -102,
  /// Invalid tag read.
  GPBCodedInputStreamErrorInvalidTag = -103,
  /// Invalid UTF-8 character in a string.
  GPBCodedInputStreamErrorInvalidUTF8 = -104,
  /// Invalid VarInt read.
  GPBCodedInputStreamErrorInvalidVarInt = -105,
  /// The maximum recursion depth of messages was exceeded.
  GPBCodedInputStreamErrorRecursionDepthExceeded = -106,
};

CF_EXTERN_C_END

/// Reads and decodes protocol message fields.
///
/// The common uses of protocol buffers shouldn't need to use this class.
/// @c GPBMessage's provide a @c +parseFromData:error: and @c
/// +parseFromData:extensionRegistry:error: method that will decode a
/// message for you.
///
/// @note Subclassing of GPBCodedInputStream is NOT supported.
@interface GPBCodedInputStream : NSObject

/// Creates a new stream wrapping some data.
+ (instancetype)streamWithData:(NSData *)data;

/// Initializes a stream wrapping some data.
- (instancetype)initWithData:(NSData *)data;

/// Attempt to read a field tag, returning zero if we have reached EOF.
/// Protocol message parsers use this to read tags, since a protocol message
/// may legally end wherever a tag occurs, and zero is not a valid tag number.
- (int32_t)readTag;

/// Read and return a double.
- (double)readDouble;
/// Read and return a float.
- (float)readFloat;
/// Read and return a uint64.
- (uint64_t)readUInt64;
/// Read and return a uint32.
- (uint32_t)readUInt32;
/// Read and return an int64.
- (int64_t)readInt64;
/// Read and return an int32.
- (int32_t)readInt32;
/// Read and return a fixed64.
- (uint64_t)readFixed64;
/// Read and return a fixed32.
- (uint32_t)readFixed32;
/// Read and return an enum (int).
- (int32_t)readEnum;
/// Read and return a sfixed32.
- (int32_t)readSFixed32;
/// Read and return a sfixed64.
- (int64_t)readSFixed64;
/// Read and return a sint32.
- (int32_t)readSInt32;
/// Read and return a sint64.
- (int64_t)readSInt64;
/// Read and return a boolean.
- (BOOL)readBool;
/// Read and return a string.
- (NSString *)readString;
/// Read and return length delimited data.
- (NSData *)readBytes;

/// Read an embedded message field value from the stream.
///
/// @param message           The message to set fields on as they are read.
/// @param extensionRegistry An optional extension registry to use to lookup
///                          extensions for @c message.
- (void)readMessage:(GPBMessage *)message
  extensionRegistry:(nullable GPBExtensionRegistry *)extensionRegistry;

/// Reads and discards a single field, given its tag value.
///
/// @param tag The tag number of the field to skip.
///
/// @return NO if the tag is an endgroup tag (in which case nothing is skipped),
///         YES in all other cases.
- (BOOL)skipField:(int32_t)tag;

/// Reads and discards an entire message.  This will read either until EOF
/// or until an endgroup tag, whichever comes first.
- (void)skipMessage;

/// Check to see if the logical end of the stream has been reached.
///
/// This can return NO when there is no more data, but the current parsing
/// expected more data.
- (BOOL)isAtEnd;

/// The offset into the stream.
- (size_t)position;

/// Verifies that the last call to @c -readTag returned the given tag value.
/// This is used to verify that a nested group ended with the correct end tag.
/// Throws @c NSParseErrorException if value does not match the last tag.
///
/// @param expected The tag that was expected.
- (void)checkLastTagWas:(int32_t)expected;

@end

NS_ASSUME_NONNULL_END
