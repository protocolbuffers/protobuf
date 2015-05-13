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

#import "GPBCodedInputStream_PackagePrivate.h"

#import "GPBDictionary_PackagePrivate.h"
#import "GPBMessage_PackagePrivate.h"
#import "GPBUnknownFieldSet_PackagePrivate.h"
#import "GPBUtilities_PackagePrivate.h"
#import "GPBWireFormat.h"

static const NSUInteger kDefaultRecursionLimit = 64;

static inline void CheckSize(GPBCodedInputStreamState *state, size_t size) {
  size_t newSize = state->bufferPos + size;
  if (newSize > state->bufferSize) {
    [NSException raise:NSParseErrorException format:@""];
  }
  if (newSize > state->currentLimit) {
    // Fast forward to end of currentLimit;
    state->bufferPos = state->currentLimit;
    [NSException raise:NSParseErrorException format:@""];
  }
}

static inline int8_t ReadRawByte(GPBCodedInputStreamState *state) {
  CheckSize(state, sizeof(int8_t));
  return ((int8_t *)state->bytes)[state->bufferPos++];
}

static inline int32_t ReadRawLittleEndian32(GPBCodedInputStreamState *state) {
  CheckSize(state, sizeof(int32_t));
  int32_t value = OSReadLittleInt32(state->bytes, state->bufferPos);
  state->bufferPos += sizeof(int32_t);
  return value;
}

static inline int64_t ReadRawLittleEndian64(GPBCodedInputStreamState *state) {
  CheckSize(state, sizeof(int64_t));
  int64_t value = OSReadLittleInt64(state->bytes, state->bufferPos);
  state->bufferPos += sizeof(int64_t);
  return value;
}

static inline int32_t ReadRawVarint32(GPBCodedInputStreamState *state) {
  int8_t tmp = ReadRawByte(state);
  if (tmp >= 0) {
    return tmp;
  }
  int32_t result = tmp & 0x7f;
  if ((tmp = ReadRawByte(state)) >= 0) {
    result |= tmp << 7;
  } else {
    result |= (tmp & 0x7f) << 7;
    if ((tmp = ReadRawByte(state)) >= 0) {
      result |= tmp << 14;
    } else {
      result |= (tmp & 0x7f) << 14;
      if ((tmp = ReadRawByte(state)) >= 0) {
        result |= tmp << 21;
      } else {
        result |= (tmp & 0x7f) << 21;
        result |= (tmp = ReadRawByte(state)) << 28;
        if (tmp < 0) {
          // Discard upper 32 bits.
          for (int i = 0; i < 5; i++) {
            if (ReadRawByte(state) >= 0) {
              return result;
            }
          }
          [NSException raise:NSParseErrorException
                      format:@"Unable to read varint32"];
        }
      }
    }
  }
  return result;
}

static inline int64_t ReadRawVarint64(GPBCodedInputStreamState *state) {
  int32_t shift = 0;
  int64_t result = 0;
  while (shift < 64) {
    int8_t b = ReadRawByte(state);
    result |= (int64_t)(b & 0x7F) << shift;
    if ((b & 0x80) == 0) {
      return result;
    }
    shift += 7;
  }
  [NSException raise:NSParseErrorException format:@"Unable to read varint64"];
  return 0;
}

static inline void SkipRawData(GPBCodedInputStreamState *state, size_t size) {
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
  return ReadRawVarint32(state) != 0;
}

int32_t GPBCodedInputStreamReadTag(GPBCodedInputStreamState *state) {
  if (GPBCodedInputStreamIsAtEnd(state)) {
    state->lastTag = 0;
    return 0;
  }

  state->lastTag = ReadRawVarint32(state);
  if (state->lastTag == 0) {
    // If we actually read zero, that's not a valid tag.
    [NSException raise:NSParseErrorException
                format:@"Invalid last tag %d", state->lastTag];
  }
  return state->lastTag;
}

NSString *GPBCodedInputStreamReadRetainedString(
    GPBCodedInputStreamState *state) {
  int32_t size = ReadRawVarint32(state);
  NSString *result;
  if (size == 0) {
    result = @"";
  } else {
    CheckSize(state, size);
    result = GPBCreateGPBStringWithUTF8(&state->bytes[state->bufferPos], size);
    state->bufferPos += size;
  }
  return result;
}

NSData *GPBCodedInputStreamReadRetainedData(GPBCodedInputStreamState *state) {
  int32_t size = ReadRawVarint32(state);
  if (size < 0) return nil;
  CheckSize(state, size);
  NSData *result = [[NSData alloc] initWithBytes:state->bytes + state->bufferPos
                                          length:size];
  state->bufferPos += size;
  return result;
}

NSData *GPBCodedInputStreamReadRetainedDataNoCopy(
    GPBCodedInputStreamState *state) {
  int32_t size = ReadRawVarint32(state);
  if (size < 0) return nil;
  CheckSize(state, size);
  // Cast is safe because freeWhenDone is NO.
  NSData *result = [[NSData alloc]
      initWithBytesNoCopy:(void *)(state->bytes + state->bufferPos)
                   length:size
             freeWhenDone:NO];
  state->bufferPos += size;
  return result;
}

size_t GPBCodedInputStreamPushLimit(GPBCodedInputStreamState *state,
                                    size_t byteLimit) {
  byteLimit += state->bufferPos;
  size_t oldLimit = state->currentLimit;
  if (byteLimit > oldLimit) {
    [NSException raise:NSInvalidArgumentException
                format:@"byteLimit > oldLimit: %tu > %tu", byteLimit, oldLimit];
  }
  state->currentLimit = byteLimit;
  return oldLimit;
}

void GPBCodedInputStreamPopLimit(GPBCodedInputStreamState *state,
                                 size_t oldLimit) {
  state->currentLimit = oldLimit;
}

size_t GPBCodedInputStreamBytesUntilLimit(GPBCodedInputStreamState *state) {
  if (state->currentLimit == SIZE_T_MAX) {
    return state->currentLimit;
  }

  return state->currentLimit - state->bufferPos;
}

BOOL GPBCodedInputStreamIsAtEnd(GPBCodedInputStreamState *state) {
  return (state->bufferPos == state->bufferSize) ||
         (state->bufferPos == state->currentLimit);
}

void GPBCodedInputStreamCheckLastTagWas(GPBCodedInputStreamState *state,
                                        int32_t value) {
  if (state->lastTag != value) {
    [NSException raise:NSParseErrorException
                format:@"Last tag: %d should be %d", state->lastTag, value];
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
    state_.currentLimit = NSUIntegerMax;
  }
  return self;
}

- (void)dealloc {
  [buffer_ release];
  [super dealloc];
}

- (int32_t)readTag {
  return GPBCodedInputStreamReadTag(&state_);
}

- (void)checkLastTagWas:(int32_t)value {
  GPBCodedInputStreamCheckLastTagWas(&state_, value);
}

- (BOOL)skipField:(int32_t)tag {
  switch (GPBWireFormatGetTagWireType(tag)) {
    case GPBWireFormatVarint:
      GPBCodedInputStreamReadInt32(&state_);
      return YES;
    case GPBWireFormatFixed64:
      SkipRawData(&state_, sizeof(int64_t));
      return YES;
    case GPBWireFormatLengthDelimited:
      SkipRawData(&state_, ReadRawVarint32(&state_));
      return YES;
    case GPBWireFormatStartGroup:
      [self skipMessage];
      GPBCodedInputStreamCheckLastTagWas(
          &state_, GPBWireFormatMakeTag(GPBWireFormatGetTagFieldNumber(tag),
                                        GPBWireFormatEndGroup));
      return YES;
    case GPBWireFormatEndGroup:
      return NO;
    case GPBWireFormatFixed32:
      SkipRawData(&state_, sizeof(int32_t));
      return YES;
  }
  [NSException raise:NSParseErrorException format:@"Invalid tag %d", tag];
  return NO;
}

- (void)skipMessage {
  while (YES) {
    int32_t tag = GPBCodedInputStreamReadTag(&state_);
    if (tag == 0 || ![self skipField:tag]) {
      return;
    }
  }
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
    extensionRegistry:(GPBExtensionRegistry *)extensionRegistry {
  if (state_.recursionDepth >= kDefaultRecursionLimit) {
    [NSException raise:NSParseErrorException
                format:@"recursionDepth(%tu) >= %tu", state_.recursionDepth,
                       kDefaultRecursionLimit];
  }
  ++state_.recursionDepth;
  [message mergeFromCodedInputStream:self extensionRegistry:extensionRegistry];
  GPBCodedInputStreamCheckLastTagWas(
      &state_, GPBWireFormatMakeTag(fieldNumber, GPBWireFormatEndGroup));
  --state_.recursionDepth;
}

- (void)readUnknownGroup:(int32_t)fieldNumber
                 message:(GPBUnknownFieldSet *)message {
  if (state_.recursionDepth >= kDefaultRecursionLimit) {
    [NSException raise:NSParseErrorException
                format:@"recursionDepth(%tu) >= %tu", state_.recursionDepth,
                       kDefaultRecursionLimit];
  }
  ++state_.recursionDepth;
  [message mergeFromCodedInputStream:self];
  GPBCodedInputStreamCheckLastTagWas(
      &state_, GPBWireFormatMakeTag(fieldNumber, GPBWireFormatEndGroup));
  --state_.recursionDepth;
}

- (void)readMessage:(GPBMessage *)message
    extensionRegistry:(GPBExtensionRegistry *)extensionRegistry {
  int32_t length = ReadRawVarint32(&state_);
  if (state_.recursionDepth >= kDefaultRecursionLimit) {
    [NSException raise:NSParseErrorException
                format:@"recursionDepth(%tu) >= %tu", state_.recursionDepth,
                       kDefaultRecursionLimit];
  }
  size_t oldLimit = GPBCodedInputStreamPushLimit(&state_, length);
  ++state_.recursionDepth;
  [message mergeFromCodedInputStream:self extensionRegistry:extensionRegistry];
  GPBCodedInputStreamCheckLastTagWas(&state_, 0);
  --state_.recursionDepth;
  GPBCodedInputStreamPopLimit(&state_, oldLimit);
}

- (void)readMapEntry:(id)mapDictionary
    extensionRegistry:(GPBExtensionRegistry *)extensionRegistry
                field:(GPBFieldDescriptor *)field
        parentMessage:(GPBMessage *)parentMessage {
  int32_t length = ReadRawVarint32(&state_);
  if (state_.recursionDepth >= kDefaultRecursionLimit) {
    [NSException raise:NSParseErrorException
                format:@"recursionDepth(%tu) >= %tu", state_.recursionDepth,
                       kDefaultRecursionLimit];
  }
  size_t oldLimit = GPBCodedInputStreamPushLimit(&state_, length);
  ++state_.recursionDepth;
  GPBDictionaryReadEntry(mapDictionary, self, extensionRegistry, field,
                         parentMessage);
  GPBCodedInputStreamCheckLastTagWas(&state_, 0);
  --state_.recursionDepth;
  GPBCodedInputStreamPopLimit(&state_, oldLimit);
}

- (NSData *)readData {
  return [GPBCodedInputStreamReadRetainedData(&state_) autorelease];
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

@end

@implementation GPBString {
 @package
  CFStringRef string_;
  unsigned char *utf8_;
  NSUInteger utf8Len_;

  // This lock is used to gate access to utf8_.  Once GPBStringInitStringValue()
  // has been called, string_ will be filled in, and utf8_ will be NULL.
  OSSpinLock lock_;

  BOOL hasBOM_;
  BOOL is7BitAscii_;
}

// Returns true if the passed in bytes are 7 bit ascii.
// This routine needs to be fast.
static inline bool AreBytesIn7BitASCII(const uint8_t *bytes, NSUInteger len) {
// In the loops below, it's more efficient to collect rather than do
// conditional at every step.
#if __LP64__
  // Align bytes. This is especially important in case of 3 byte BOM.
  while (len > 0 && ((size_t)bytes & 0x07)) {
    if (*bytes++ & 0x80) return false;
    len--;
  }
  while (len >= 32) {
    uint64_t val = *(const uint64_t *)bytes;
    uint64_t hiBits = (val & 0x8080808080808080ULL);
    bytes += 8;
    val = *(const uint64_t *)bytes;
    hiBits |= (val & 0x8080808080808080ULL);
    bytes += 8;
    val = *(const uint64_t *)bytes;
    hiBits |= (val & 0x8080808080808080ULL);
    bytes += 8;
    val = *(const uint64_t *)bytes;
    if (hiBits | (val & 0x8080808080808080ULL)) return false;
    bytes += 8;
    len -= 32;
  }

  while (len >= 16) {
    uint64_t val = *(const uint64_t *)bytes;
    uint64_t hiBits = (val & 0x8080808080808080ULL);
    bytes += 8;
    val = *(const uint64_t *)bytes;
    if (hiBits | (val & 0x8080808080808080ULL)) return false;
    bytes += 8;
    len -= 16;
  }

  while (len >= 8) {
    uint64_t val = *(const uint64_t *)bytes;
    if (val & 0x8080808080808080ULL) return false;
    bytes += 8;
    len -= 8;
  }
#else   // __LP64__
  // Align bytes. This is especially important in case of 3 byte BOM.
  while (len > 0 && ((size_t)bytes & 0x03)) {
    if (*bytes++ & 0x80) return false;
    len--;
  }
  while (len >= 16) {
    uint32_t val = *(const uint32_t *)bytes;
    uint32_t hiBits = (val & 0x80808080U);
    bytes += 4;
    val = *(const uint32_t *)bytes;
    hiBits |= (val & 0x80808080U);
    bytes += 4;
    val = *(const uint32_t *)bytes;
    hiBits |= (val & 0x80808080U);
    bytes += 4;
    val = *(const uint32_t *)bytes;
    if (hiBits | (val & 0x80808080U)) return false;
    bytes += 4;
    len -= 16;
  }

  while (len >= 8) {
    uint32_t val = *(const uint32_t *)bytes;
    uint32_t hiBits = (val & 0x80808080U);
    bytes += 4;
    val = *(const uint32_t *)bytes;
    if (hiBits | (val & 0x80808080U)) return false;
    bytes += 4;
    len -= 8;
  }
#endif  // __LP64__

  while (len >= 4) {
    uint32_t val = *(const uint32_t *)bytes;
    if (val & 0x80808080U) return false;
    bytes += 4;
    len -= 4;
  }

  while (len--) {
    if (*bytes++ & 0x80) return false;
  }

  return true;
}

static inline void GPBStringInitStringValue(GPBString *string) {
  OSSpinLockLock(&string->lock_);
  GPBStringInitStringValueAlreadyLocked(string);
  OSSpinLockUnlock(&string->lock_);
}

static void GPBStringInitStringValueAlreadyLocked(GPBString *string) {
  if (string->string_ == NULL && string->utf8_ != NULL) {
    // Using kCFAllocatorMalloc for contentsDeallocator, as buffer in
    // string->utf8_ is being handed off.
    string->string_ = CFStringCreateWithBytesNoCopy(
        NULL, string->utf8_, string->utf8Len_, kCFStringEncodingUTF8, false,
        kCFAllocatorMalloc);
    if (!string->string_) {
#ifdef DEBUG
      // https://developers.google.com/protocol-buffers/docs/proto#scalar
      NSLog(@"UTF8 failure, is some field type 'string' when it should be "
            @"'bytes'?");
#endif
      string->string_ = CFSTR("");
      string->utf8Len_ = 0;
      // On failure, we have to clean up the buffer.
      free(string->utf8_);
    }
    string->utf8_ = NULL;
  }
}

GPBString *GPBCreateGPBStringWithUTF8(const void *bytes, NSUInteger length) {
  GPBString *result = [[GPBString alloc] initWithBytes:bytes length:length];
  return result;
}

- (instancetype)initWithBytes:(const void *)bytes length:(NSUInteger)length {
  self = [super init];
  if (self) {
    utf8_ = malloc(length);
    memcpy(utf8_, bytes, length);
    utf8Len_ = length;
    lock_ = OS_SPINLOCK_INIT;
    is7BitAscii_ = AreBytesIn7BitASCII(bytes, length);
    if (length >= 3 && memcmp(utf8_, "\xef\xbb\xbf", 3) == 0) {
      // We can't just remove the BOM from the string here, because in the case
      // where we have > 1 BOM at the beginning of the string, we will remove one,
      // and the internal NSString we create will remove the next one, and we will
      // end up with a GPBString != NSString issue.
      // We also just can't remove all the BOMs because then we would end up with
      // potential cases where a GPBString and an NSString made with the same
      // UTF8 buffer would in fact be different.
      // We record the fact we have a BOM, and use it as necessary to simulate
      // what NSString would return for various calls.
      hasBOM_ = YES;
#if DEBUG
      // Sending BOMs across the line is just wasting bits.
      NSLog(@"Bad data? String should not have BOM!");
#endif  // DEBUG
    }
  }
  return self;
}

- (void)dealloc {
  if (string_ != NULL) {
    CFRelease(string_);
  }
  if (utf8_ != NULL) {
    free(utf8_);
  }
  [super dealloc];
}

// Required NSString overrides.
- (NSUInteger)length {
  if (is7BitAscii_) {
    return utf8Len_;
  } else {
    GPBStringInitStringValue(self);
    return CFStringGetLength(string_);
  }
}

- (unichar)characterAtIndex:(NSUInteger)anIndex {
  OSSpinLockLock(&lock_);
  if (is7BitAscii_ && utf8_) {
    unichar result = utf8_[anIndex];
    OSSpinLockUnlock(&lock_);
    return result;
  } else {
    GPBStringInitStringValueAlreadyLocked(self);
    OSSpinLockUnlock(&lock_);
    return CFStringGetCharacterAtIndex(string_, anIndex);
  }
}

// Override a couple of methods that typically want high performance.

- (id)copyWithZone:(NSZone *)zone {
  GPBStringInitStringValue(self);
  return [(NSString *)string_ copyWithZone:zone];
}

- (id)mutableCopyWithZone:(NSZone *)zone {
  GPBStringInitStringValue(self);
  return [(NSString *)string_ mutableCopyWithZone:zone];
}

- (NSUInteger)hash {
  // Must convert to string here to make sure that the hash is always
  // consistent no matter what state the GPBString is in.
  GPBStringInitStringValue(self);
  return CFHash(string_);
}

- (BOOL)isEqual:(id)object {
  if (self == object) {
    return YES;
  }
  if ([object isKindOfClass:[NSString class]]) {
    GPBStringInitStringValue(self);
    return CFStringCompare(string_, (CFStringRef)object, 0) ==
           kCFCompareEqualTo;
  }
  return NO;
}

- (void)getCharacters:(unichar *)buffer range:(NSRange)aRange {
  OSSpinLockLock(&lock_);
  if (is7BitAscii_ && utf8_) {
    unsigned char *bytes = &(utf8_[aRange.location]);
    for (NSUInteger i = 0; i < aRange.length; ++i) {
      buffer[i] = bytes[i];
    }
    OSSpinLockUnlock(&lock_);
  } else {
    GPBStringInitStringValueAlreadyLocked(self);
    OSSpinLockUnlock(&lock_);
    CFStringGetCharacters(string_, CFRangeMake(aRange.location, aRange.length),
                          buffer);
  }
}

- (NSUInteger)lengthOfBytesUsingEncoding:(NSStringEncoding)encoding {
  if ((encoding == NSUTF8StringEncoding) ||
      (encoding == NSASCIIStringEncoding && is7BitAscii_)) {
    return utf8Len_ - (hasBOM_ ? 3 : 0);
  } else {
    GPBStringInitStringValue(self);
    return [(NSString *)string_ lengthOfBytesUsingEncoding:encoding];
  }
}

- (BOOL)getBytes:(void *)buffer
         maxLength:(NSUInteger)maxLength
        usedLength:(NSUInteger *)usedLength
          encoding:(NSStringEncoding)encoding
           options:(NSStringEncodingConversionOptions)options
             range:(NSRange)range
    remainingRange:(NSRangePointer)remainingRange {
  // [NSString getBytes:maxLength:usedLength:encoding:options:range:remainingRange]
  // does not return reliable results if the maxLength argument is 0
  // (Radar 16385183). Therefore we have special cased it as a slow case so
  // that it behaves however Apple intends it to behave. It should be a rare
  // case.
  //
  // [NSString getBytes:maxLength:usedLength:encoding:options:range:remainingRange]
  // does not return reliable results if the range is outside of the strings
  // length (Radar 16396177). Therefore we have special cased it as a slow
  // case so that it behaves however Apple intends it to behave.  It should
  // be a rare case.
  //
  // We can optimize the UTF8StringEncoding and NSASCIIStringEncoding with no
  // options cases.
  if ((options == 0) &&
      (encoding == NSUTF8StringEncoding || encoding == NSASCIIStringEncoding) &&
      (maxLength != 0) &&
      (NSMaxRange(range) <= utf8Len_)) {
    // Might be able to optimize it.
    OSSpinLockLock(&lock_);
    if (is7BitAscii_ && utf8_) {
      NSUInteger length = range.length;
      length = (length < maxLength) ? length : maxLength;
      memcpy(buffer, utf8_ + range.location, length);
      if (usedLength) {
        *usedLength = length;
      }
      if (remainingRange) {
        remainingRange->location = range.location + length;
        remainingRange->length = range.length - length;
      }
      OSSpinLockUnlock(&lock_);
      if (length > 0) {
        return YES;
      } else {
        return NO;
      }
    } else {
      GPBStringInitStringValueAlreadyLocked(self);
      OSSpinLockUnlock(&lock_);
    }
  } else {
    GPBStringInitStringValue(self);
  }
  return [(NSString *)string_ getBytes:buffer
                             maxLength:maxLength
                            usedLength:usedLength
                              encoding:encoding
                               options:options
                                 range:range
                        remainingRange:remainingRange];
}

@end
