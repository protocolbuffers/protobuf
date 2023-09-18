// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBCodedInputStream_PackagePrivate.h"

#import "GPBDictionary_PackagePrivate.h"
#import "GPBMessage_PackagePrivate.h"
#import "GPBUnknownFieldSet_PackagePrivate.h"
#import "GPBUtilities_PackagePrivate.h"
#import "GPBWireFormat.h"

NSString *const GPBCodedInputStreamException = GPBNSStringifySymbol(GPBCodedInputStreamException);

NSString *const GPBCodedInputStreamUnderlyingErrorKey =
    GPBNSStringifySymbol(GPBCodedInputStreamUnderlyingErrorKey);

NSString *const GPBCodedInputStreamErrorDomain =
    GPBNSStringifySymbol(GPBCodedInputStreamErrorDomain);

// Matching:
// https://github.com/protocolbuffers/protobuf/blob/main/java/core/src/main/java/com/google/protobuf/CodedInputStream.java#L62
//  private static final int DEFAULT_RECURSION_LIMIT = 100;
// https://github.com/protocolbuffers/protobuf/blob/main/src/google/protobuf/io/coded_stream.cc#L86
//  int CodedInputStream::default_recursion_limit_ = 100;
static const NSUInteger kDefaultRecursionLimit = 100;

static void RaiseException(NSInteger code, NSString *reason) {
  NSDictionary *errorInfo = nil;
  if ([reason length]) {
    errorInfo = @{GPBErrorReasonKey : reason};
  }
  NSError *error = [NSError errorWithDomain:GPBCodedInputStreamErrorDomain
                                       code:code
                                   userInfo:errorInfo];

  NSDictionary *exceptionInfo = @{GPBCodedInputStreamUnderlyingErrorKey : error};
  [[NSException exceptionWithName:GPBCodedInputStreamException reason:reason
                         userInfo:exceptionInfo] raise];
}

GPB_INLINE void CheckRecursionLimit(GPBCodedInputStreamState *state) {
  if (state->recursionDepth >= kDefaultRecursionLimit) {
    RaiseException(GPBCodedInputStreamErrorRecursionDepthExceeded, nil);
  }
}

GPB_INLINE void CheckFieldSize(uint64_t size) {
  // Bytes and Strings have a max size of 2GB. And since messages are on the wire as bytes/length
  // delimited, they also have a 2GB size limit. The C++ does the same sort of enforcement (see
  // parse_context, delimited_message_util, message_lite, etc.).
  // https://protobuf.dev/programming-guides/encoding/#cheat-sheet
  if (size > 0x7fffffff) {
    // TODO: Maybe a different error code for this, but adding one is a breaking
    // change so reuse an existing one.
    RaiseException(GPBCodedInputStreamErrorInvalidSize, nil);
  }
}

static void CheckSize(GPBCodedInputStreamState *state, size_t size) {
  size_t newSize = state->bufferPos + size;
  if (newSize > state->bufferSize) {
    RaiseException(GPBCodedInputStreamErrorInvalidSize, nil);
  }
  if (newSize > state->currentLimit) {
    // Fast forward to end of currentLimit;
    state->bufferPos = state->currentLimit;
    RaiseException(GPBCodedInputStreamErrorSubsectionLimitReached, nil);
  }
}

static int8_t ReadRawByte(GPBCodedInputStreamState *state) {
  CheckSize(state, sizeof(int8_t));
  return ((int8_t *)state->bytes)[state->bufferPos++];
}

static int32_t ReadRawLittleEndian32(GPBCodedInputStreamState *state) {
  CheckSize(state, sizeof(int32_t));
  // Not using OSReadLittleInt32 because it has undocumented dependency
  // on reads being aligned.
  int32_t value;
  memcpy(&value, state->bytes + state->bufferPos, sizeof(int32_t));
  value = OSSwapLittleToHostInt32(value);
  state->bufferPos += sizeof(int32_t);
  return value;
}

static int64_t ReadRawLittleEndian64(GPBCodedInputStreamState *state) {
  CheckSize(state, sizeof(int64_t));
  // Not using OSReadLittleInt64 because it has undocumented dependency
  // on reads being aligned.
  int64_t value;
  memcpy(&value, state->bytes + state->bufferPos, sizeof(int64_t));
  value = OSSwapLittleToHostInt64(value);
  state->bufferPos += sizeof(int64_t);
  return value;
}

static int64_t ReadRawVarint64(GPBCodedInputStreamState *state) {
  int32_t shift = 0;
  int64_t result = 0;
  while (shift < 64) {
    int8_t b = ReadRawByte(state);
    result |= (int64_t)((uint64_t)(b & 0x7F) << shift);
    if ((b & 0x80) == 0) {
      return result;
    }
    shift += 7;
  }
  RaiseException(GPBCodedInputStreamErrorInvalidVarInt, @"Invalid VarInt64");
  return 0;
}

static int32_t ReadRawVarint32(GPBCodedInputStreamState *state) {
  return (int32_t)ReadRawVarint64(state);
}

static void SkipRawData(GPBCodedInputStreamState *state, size_t size) {
  CheckSize(state, size);
  state->bufferPos += size;
}

double GPBCodedInputStreamReadDouble(GPBCodedInputStreamState *state) {
  int64_t value = ReadRawLittleEndian64(state);
  return GPBConvertInt64ToDouble(value);
}

float GPBCodedInputStreamReadFloat(GPBCodedInputStreamState *state) {
  int32_t value = ReadRawLittleEndian32(state);
  return GPBConvertInt32ToFloat(value);
}

uint64_t GPBCodedInputStreamReadUInt64(GPBCodedInputStreamState *state) {
  uint64_t value = ReadRawVarint64(state);
  return value;
}

uint32_t GPBCodedInputStreamReadUInt32(GPBCodedInputStreamState *state) {
  uint32_t value = ReadRawVarint32(state);
  return value;
}

int64_t GPBCodedInputStreamReadInt64(GPBCodedInputStreamState *state) {
  int64_t value = ReadRawVarint64(state);
  return value;
}

int32_t GPBCodedInputStreamReadInt32(GPBCodedInputStreamState *state) {
  int32_t value = ReadRawVarint32(state);
  return value;
}

uint64_t GPBCodedInputStreamReadFixed64(GPBCodedInputStreamState *state) {
  uint64_t value = ReadRawLittleEndian64(state);
  return value;
}

uint32_t GPBCodedInputStreamReadFixed32(GPBCodedInputStreamState *state) {
  uint32_t value = ReadRawLittleEndian32(state);
  return value;
}

int32_t GPBCodedInputStreamReadEnum(GPBCodedInputStreamState *state) {
  int32_t value = ReadRawVarint32(state);
  return value;
}

int32_t GPBCodedInputStreamReadSFixed32(GPBCodedInputStreamState *state) {
  int32_t value = ReadRawLittleEndian32(state);
  return value;
}

int64_t GPBCodedInputStreamReadSFixed64(GPBCodedInputStreamState *state) {
  int64_t value = ReadRawLittleEndian64(state);
  return value;
}

int32_t GPBCodedInputStreamReadSInt32(GPBCodedInputStreamState *state) {
  int32_t value = GPBDecodeZigZag32(ReadRawVarint32(state));
  return value;
}

int64_t GPBCodedInputStreamReadSInt64(GPBCodedInputStreamState *state) {
  int64_t value = GPBDecodeZigZag64(ReadRawVarint64(state));
  return value;
}

BOOL GPBCodedInputStreamReadBool(GPBCodedInputStreamState *state) {
  return ReadRawVarint64(state) != 0;
}

int32_t GPBCodedInputStreamReadTag(GPBCodedInputStreamState *state) {
  if (GPBCodedInputStreamIsAtEnd(state)) {
    state->lastTag = 0;
    return 0;
  }

  state->lastTag = ReadRawVarint32(state);
  // Tags have to include a valid wireformat.
  if (!GPBWireFormatIsValidTag(state->lastTag)) {
    RaiseException(GPBCodedInputStreamErrorInvalidTag, @"Invalid wireformat in tag.");
  }
  // Zero is not a valid field number.
  if (GPBWireFormatGetTagFieldNumber(state->lastTag) == 0) {
    RaiseException(GPBCodedInputStreamErrorInvalidTag,
                   @"A zero field number on the wire is invalid.");
  }
  return state->lastTag;
}

NSString *GPBCodedInputStreamReadRetainedString(GPBCodedInputStreamState *state) {
  uint64_t size = GPBCodedInputStreamReadUInt64(state);
  CheckFieldSize(size);
  NSUInteger ns_size = (NSUInteger)size;
  NSString *result;
  if (size == 0) {
    result = @"";
  } else {
    CheckSize(state, size);
    result = [[NSString alloc] initWithBytes:&state->bytes[state->bufferPos]
                                      length:ns_size
                                    encoding:NSUTF8StringEncoding];
    state->bufferPos += size;
    if (!result) {
#ifdef DEBUG
      // https://developers.google.com/protocol-buffers/docs/proto#scalar
      NSLog(@"UTF-8 failure, is some field type 'string' when it should be "
            @"'bytes'?");
#endif
      RaiseException(GPBCodedInputStreamErrorInvalidUTF8, nil);
    }
  }
  return result;
}

NSData *GPBCodedInputStreamReadRetainedBytes(GPBCodedInputStreamState *state) {
  uint64_t size = GPBCodedInputStreamReadUInt64(state);
  CheckFieldSize(size);
  NSUInteger ns_size = (NSUInteger)size;
  CheckSize(state, size);
  NSData *result = [[NSData alloc] initWithBytes:state->bytes + state->bufferPos length:ns_size];
  state->bufferPos += size;
  return result;
}

NSData *GPBCodedInputStreamReadRetainedBytesNoCopy(GPBCodedInputStreamState *state) {
  uint64_t size = GPBCodedInputStreamReadUInt64(state);
  CheckFieldSize(size);
  NSUInteger ns_size = (NSUInteger)size;
  CheckSize(state, size);
  // Cast is safe because freeWhenDone is NO.
  NSData *result = [[NSData alloc] initWithBytesNoCopy:(void *)(state->bytes + state->bufferPos)
                                                length:ns_size
                                          freeWhenDone:NO];
  state->bufferPos += size;
  return result;
}

size_t GPBCodedInputStreamPushLimit(GPBCodedInputStreamState *state, size_t byteLimit) {
  byteLimit += state->bufferPos;
  size_t oldLimit = state->currentLimit;
  if (byteLimit > oldLimit) {
    RaiseException(GPBCodedInputStreamErrorInvalidSubsectionLimit, nil);
  }
  state->currentLimit = byteLimit;
  return oldLimit;
}

void GPBCodedInputStreamPopLimit(GPBCodedInputStreamState *state, size_t oldLimit) {
  state->currentLimit = oldLimit;
}

size_t GPBCodedInputStreamBytesUntilLimit(GPBCodedInputStreamState *state) {
  return state->currentLimit - state->bufferPos;
}

BOOL GPBCodedInputStreamIsAtEnd(GPBCodedInputStreamState *state) {
  return (state->bufferPos == state->bufferSize) || (state->bufferPos == state->currentLimit);
}

void GPBCodedInputStreamCheckLastTagWas(GPBCodedInputStreamState *state, int32_t value) {
  if (state->lastTag != value) {
    RaiseException(GPBCodedInputStreamErrorInvalidTag, @"Unexpected tag read");
  }
}

@implementation GPBCodedInputStream

+ (instancetype)streamWithData:(NSData *)data {
  return [[[self alloc] initWithData:data] autorelease];
}

- (instancetype)initWithData:(NSData *)data {
  if ((self = [super init])) {
#ifdef DEBUG
    NSCAssert([self class] == [GPBCodedInputStream class],
              @"Subclassing of GPBCodedInputStream is not allowed.");
#endif
    buffer_ = [data retain];
    state_.bytes = (const uint8_t *)[data bytes];
    state_.bufferSize = [data length];
    state_.currentLimit = state_.bufferSize;
  }
  return self;
}

- (void)dealloc {
  [buffer_ release];
  [super dealloc];
}

// Direct access is use for speed, to avoid even internally declaring things
// read/write, etc. The warning is enabled in the project to ensure code calling
// protos can turn on -Wdirect-ivar-access without issues.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdirect-ivar-access"

- (int32_t)readTag {
  return GPBCodedInputStreamReadTag(&state_);
}

- (void)checkLastTagWas:(int32_t)value {
  GPBCodedInputStreamCheckLastTagWas(&state_, value);
}

- (BOOL)skipField:(int32_t)tag {
  NSAssert(GPBWireFormatIsValidTag(tag), @"Invalid tag");
  switch (GPBWireFormatGetTagWireType(tag)) {
    case GPBWireFormatVarint:
      GPBCodedInputStreamReadInt32(&state_);
      return YES;
    case GPBWireFormatFixed64:
      SkipRawData(&state_, sizeof(int64_t));
      return YES;
    case GPBWireFormatLengthDelimited: {
      uint64_t size = GPBCodedInputStreamReadUInt64(&state_);
      CheckFieldSize(size);
      SkipRawData(&state_, size);
      return YES;
    }
    case GPBWireFormatStartGroup:
      [self skipMessage];
      GPBCodedInputStreamCheckLastTagWas(
          &state_,
          GPBWireFormatMakeTag(GPBWireFormatGetTagFieldNumber(tag), GPBWireFormatEndGroup));
      return YES;
    case GPBWireFormatEndGroup:
      return NO;
    case GPBWireFormatFixed32:
      SkipRawData(&state_, sizeof(int32_t));
      return YES;
  }
}

- (void)skipMessage {
  while (YES) {
    int32_t tag = GPBCodedInputStreamReadTag(&state_);
    if (tag == 0 || ![self skipField:tag]) {
      return;
    }
  }
}

- (BOOL)isAtEnd {
  return GPBCodedInputStreamIsAtEnd(&state_);
}

- (size_t)position {
  return state_.bufferPos;
}

- (size_t)pushLimit:(size_t)byteLimit {
  return GPBCodedInputStreamPushLimit(&state_, byteLimit);
}

- (void)popLimit:(size_t)oldLimit {
  GPBCodedInputStreamPopLimit(&state_, oldLimit);
}

- (double)readDouble {
  return GPBCodedInputStreamReadDouble(&state_);
}

- (float)readFloat {
  return GPBCodedInputStreamReadFloat(&state_);
}

- (uint64_t)readUInt64 {
  return GPBCodedInputStreamReadUInt64(&state_);
}

- (int64_t)readInt64 {
  return GPBCodedInputStreamReadInt64(&state_);
}

- (int32_t)readInt32 {
  return GPBCodedInputStreamReadInt32(&state_);
}

- (uint64_t)readFixed64 {
  return GPBCodedInputStreamReadFixed64(&state_);
}

- (uint32_t)readFixed32 {
  return GPBCodedInputStreamReadFixed32(&state_);
}

- (BOOL)readBool {
  return GPBCodedInputStreamReadBool(&state_);
}

- (NSString *)readString {
  return [GPBCodedInputStreamReadRetainedString(&state_) autorelease];
}

- (void)readGroup:(int32_t)fieldNumber
              message:(GPBMessage *)message
    extensionRegistry:(id<GPBExtensionRegistry>)extensionRegistry {
  CheckRecursionLimit(&state_);
  ++state_.recursionDepth;
  [message mergeFromCodedInputStream:self extensionRegistry:extensionRegistry];
  GPBCodedInputStreamCheckLastTagWas(&state_,
                                     GPBWireFormatMakeTag(fieldNumber, GPBWireFormatEndGroup));
  --state_.recursionDepth;
}

- (void)readUnknownGroup:(int32_t)fieldNumber message:(GPBUnknownFieldSet *)message {
  CheckRecursionLimit(&state_);
  ++state_.recursionDepth;
  [message mergeFromCodedInputStream:self];
  GPBCodedInputStreamCheckLastTagWas(&state_,
                                     GPBWireFormatMakeTag(fieldNumber, GPBWireFormatEndGroup));
  --state_.recursionDepth;
}

- (void)readMessage:(GPBMessage *)message
    extensionRegistry:(id<GPBExtensionRegistry>)extensionRegistry {
  CheckRecursionLimit(&state_);
  uint64_t length = GPBCodedInputStreamReadUInt64(&state_);
  CheckFieldSize(length);
  size_t oldLimit = GPBCodedInputStreamPushLimit(&state_, length);
  ++state_.recursionDepth;
  [message mergeFromCodedInputStream:self extensionRegistry:extensionRegistry];
  GPBCodedInputStreamCheckLastTagWas(&state_, 0);
  --state_.recursionDepth;
  GPBCodedInputStreamPopLimit(&state_, oldLimit);
}

- (void)readMapEntry:(id)mapDictionary
    extensionRegistry:(id<GPBExtensionRegistry>)extensionRegistry
                field:(GPBFieldDescriptor *)field
        parentMessage:(GPBMessage *)parentMessage {
  CheckRecursionLimit(&state_);
  uint64_t length = GPBCodedInputStreamReadUInt64(&state_);
  CheckFieldSize(length);
  size_t oldLimit = GPBCodedInputStreamPushLimit(&state_, length);
  ++state_.recursionDepth;
  GPBDictionaryReadEntry(mapDictionary, self, extensionRegistry, field, parentMessage);
  GPBCodedInputStreamCheckLastTagWas(&state_, 0);
  --state_.recursionDepth;
  GPBCodedInputStreamPopLimit(&state_, oldLimit);
}

- (NSData *)readBytes {
  return [GPBCodedInputStreamReadRetainedBytes(&state_) autorelease];
}

- (uint32_t)readUInt32 {
  return GPBCodedInputStreamReadUInt32(&state_);
}

- (int32_t)readEnum {
  return GPBCodedInputStreamReadEnum(&state_);
}

- (int32_t)readSFixed32 {
  return GPBCodedInputStreamReadSFixed32(&state_);
}

- (int64_t)readSFixed64 {
  return GPBCodedInputStreamReadSFixed64(&state_);
}

- (int32_t)readSInt32 {
  return GPBCodedInputStreamReadSInt32(&state_);
}

- (int64_t)readSInt64 {
  return GPBCodedInputStreamReadSInt64(&state_);
}

#pragma clang diagnostic pop

@end
