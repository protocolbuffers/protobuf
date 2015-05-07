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

#import "GPBUnknownFieldSet_PackagePrivate.h"

#import "GPBCodedInputStream_PackagePrivate.h"
#import "GPBCodedOutputStream.h"
#import "GPBField_PackagePrivate.h"
#import "GPBUtilities.h"
#import "GPBWireFormat.h"

#pragma mark CFDictionaryKeyCallBacks

// We use a custom dictionary here because our keys are numbers and
// conversion back and forth from NSNumber was costing us performance.
// If/when we move to C++ this could be done using a std::map and some
// careful retain/release calls.

static const void *GPBUnknownFieldSetKeyRetain(CFAllocatorRef allocator,
                                               const void *value) {
#pragma unused(allocator)
  return value;
}

static void GPBUnknownFieldSetKeyRelease(CFAllocatorRef allocator,
                                         const void *value) {
#pragma unused(allocator)
#pragma unused(value)
}

static CFStringRef GPBUnknownFieldSetCopyKeyDescription(const void *value) {
  return CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%d"),
                                  (int)value);
}

static Boolean GPBUnknownFieldSetKeyEqual(const void *value1,
                                          const void *value2) {
  return value1 == value2;
}

static CFHashCode GPBUnknownFieldSetKeyHash(const void *value) {
  return (CFHashCode)value;
}

#pragma mark Helpers

static void checkNumber(int32_t number) {
  if (number == 0) {
    [NSException raise:NSInvalidArgumentException
                format:@"Zero is not a valid field number."];
  }
}

@implementation GPBUnknownFieldSet {
 @package
  CFMutableDictionaryRef fields_;
}

static void CopyWorker(const void *key, const void *value, void *context) {
#pragma unused(key)
  GPBField *field = value;
  GPBUnknownFieldSet *result = context;

  GPBField *copied = [field copy];
  [result addField:copied];
  [copied release];
}

- (id)copyWithZone:(NSZone *)zone {
  GPBUnknownFieldSet *result = [[GPBUnknownFieldSet allocWithZone:zone] init];
  if (fields_) {
    CFDictionaryApplyFunction(fields_, CopyWorker, result);
  }
  return result;
}

- (void)dealloc {
  if (fields_) {
    CFRelease(fields_);
  }
  [super dealloc];
}

- (BOOL)isEqual:(id)object {
  BOOL equal = NO;
  if ([object isKindOfClass:[GPBUnknownFieldSet class]]) {
    GPBUnknownFieldSet *set = (GPBUnknownFieldSet *)object;
    if ((fields_ == NULL) && (set->fields_ == NULL)) {
      equal = YES;
    } else if ((fields_ != NULL) && (set->fields_ != NULL)) {
      equal = CFEqual(fields_, set->fields_);
    }
  }
  return equal;
}

- (NSUInteger)hash {
  // Return the hash of the fields dictionary (or just some value).
  if (fields_) {
    return CFHash(fields_);
  }
  return (NSUInteger)[GPBUnknownFieldSet class];
}

#pragma mark - Public Methods

- (BOOL)hasField:(int32_t)number {
  ssize_t key = number;
  return fields_ ? (CFDictionaryGetValue(fields_, (void *)key) != nil) : NO;
}

- (GPBField *)getField:(int32_t)number {
  ssize_t key = number;
  GPBField *result = fields_ ? CFDictionaryGetValue(fields_, (void *)key) : nil;
  return result;
}

- (NSUInteger)countOfFields {
  return fields_ ? CFDictionaryGetCount(fields_) : 0;
}

- (NSArray *)sortedFields {
  if (!fields_) return nil;
  size_t count = CFDictionaryGetCount(fields_);
  ssize_t keys[count];
  GPBField *values[count];
  CFDictionaryGetKeysAndValues(fields_, (const void **)keys,
                               (const void **)values);
  struct GPBFieldPair {
    ssize_t key;
    GPBField *value;
  } pairs[count];
  for (size_t i = 0; i < count; ++i) {
    pairs[i].key = keys[i];
    pairs[i].value = values[i];
  };
  qsort_b(pairs, count, sizeof(struct GPBFieldPair),
          ^(const void *first, const void *second) {
            const struct GPBFieldPair *a = first;
            const struct GPBFieldPair *b = second;
            return (a->key > b->key) ? 1 : ((a->key == b->key) ? 0 : -1);
          });
  for (size_t i = 0; i < count; ++i) {
    values[i] = pairs[i].value;
  };
  return [NSArray arrayWithObjects:values count:count];
}

#pragma mark - Internal Methods

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)output {
  if (!fields_) return;
  size_t count = CFDictionaryGetCount(fields_);
  ssize_t keys[count];
  GPBField *values[count];
  CFDictionaryGetKeysAndValues(fields_, (const void **)keys,
                               (const void **)values);
  if (count > 1) {
    struct GPBFieldPair {
      ssize_t key;
      GPBField *value;
    } pairs[count];

    for (size_t i = 0; i < count; ++i) {
      pairs[i].key = keys[i];
      pairs[i].value = values[i];
    };
    qsort_b(pairs, count, sizeof(struct GPBFieldPair),
            ^(const void *first, const void *second) {
              const struct GPBFieldPair *a = first;
              const struct GPBFieldPair *b = second;
              return (a->key > b->key) ? 1 : ((a->key == b->key) ? 0 : -1);
            });
    for (size_t i = 0; i < count; ++i) {
      GPBField *value = pairs[i].value;
      [value writeToOutput:output];
    }
  } else {
    [values[0] writeToOutput:output];
  }
}

- (NSString *)description {
  NSMutableString *description = [NSMutableString
      stringWithFormat:@"<%@ %p>: TextFormat: {\n", [self class], self];
  NSString *textFormat = GPBTextFormatForUnknownFieldSet(self, @"  ");
  [description appendString:textFormat];
  [description appendString:@"}"];
  return description;
}

static void GPBUnknownFieldSetSerializedSize(const void *key, const void *value,
                                             void *context) {
#pragma unused(key)
  GPBField *field = value;
  size_t *result = context;
  *result += [field serializedSize];
}

- (size_t)serializedSize {
  size_t result = 0;
  if (fields_) {
    CFDictionaryApplyFunction(fields_, GPBUnknownFieldSetSerializedSize,
                              &result);
  }
  return result;
}

static void GPBUnknownFieldSetWriteAsMessageSetTo(const void *key,
                                                  const void *value,
                                                  void *context) {
#pragma unused(key)
  GPBField *field = value;
  GPBCodedOutputStream *output = context;
  [field writeAsMessageSetExtensionToOutput:output];
}

- (void)writeAsMessageSetTo:(GPBCodedOutputStream *)output {
  if (fields_) {
    CFDictionaryApplyFunction(fields_, GPBUnknownFieldSetWriteAsMessageSetTo,
                              output);
  }
}

static void GPBUnknownFieldSetSerializedSizeAsMessageSet(const void *key,
                                                         const void *value,
                                                         void *context) {
#pragma unused(key)
  GPBField *field = value;
  size_t *result = context;
  *result += [field serializedSizeAsMessageSetExtension];
}

- (size_t)serializedSizeAsMessageSet {
  size_t result = 0;
  if (fields_) {
    CFDictionaryApplyFunction(
        fields_, GPBUnknownFieldSetSerializedSizeAsMessageSet, &result);
  }
  return result;
}

- (NSData *)data {
  NSMutableData *data = [NSMutableData dataWithLength:self.serializedSize];
  GPBCodedOutputStream *output =
      [[GPBCodedOutputStream alloc] initWithData:data];
  [self writeToCodedOutputStream:output];
  [output release];
  return data;
}

+ (BOOL)isFieldTag:(int32_t)tag {
  return GPBWireFormatGetTagWireType(tag) != GPBWireFormatEndGroup;
}

- (void)addField:(GPBField *)field {
  int32_t number = [field number];
  checkNumber(number);
  if (!fields_) {
    CFDictionaryKeyCallBacks keyCallBacks = {
        // See description above for reason for using custom dictionary.
        0, GPBUnknownFieldSetKeyRetain, GPBUnknownFieldSetKeyRelease,
        GPBUnknownFieldSetCopyKeyDescription, GPBUnknownFieldSetKeyEqual,
        GPBUnknownFieldSetKeyHash,
    };
    fields_ = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &keyCallBacks,
                                        &kCFTypeDictionaryValueCallBacks);
  }
  ssize_t key = number;
  CFDictionarySetValue(fields_, (const void *)key, field);
}

- (GPBField *)mutableFieldForNumber:(int32_t)number create:(BOOL)create {
  ssize_t key = number;
  GPBField *existing =
      fields_ ? CFDictionaryGetValue(fields_, (const void *)key) : nil;
  if (!existing && create) {
    existing = [[GPBField alloc] initWithNumber:number];
    // This retains existing.
    [self addField:existing];
    [existing release];
  }
  return existing;
}

static void GPBUnknownFieldSetMergeUnknownFields(const void *key,
                                                 const void *value,
                                                 void *context) {
#pragma unused(key)
  GPBField *field = value;
  GPBUnknownFieldSet *self = context;

  int32_t number = [field number];
  checkNumber(number);
  GPBField *oldField = [self mutableFieldForNumber:number create:NO];
  if (oldField) {
    [oldField mergeFromField:field];
  } else {
    // Merge only comes from GPBMessage's mergeFrom:, so it means we are on
    // mutable message and are an mutable instance, so make sure we need
    // mutable fields.
    GPBField *fieldCopy = [field copy];
    [self addField:fieldCopy];
    [fieldCopy release];
  }
}

- (void)mergeUnknownFields:(GPBUnknownFieldSet *)other {
  if (other && other->fields_) {
    CFDictionaryApplyFunction(other->fields_,
                              GPBUnknownFieldSetMergeUnknownFields, self);
  }
}

- (void)mergeFromData:(NSData *)data {
  GPBCodedInputStream *input = [[GPBCodedInputStream alloc] initWithData:data];
  [self mergeFromCodedInputStream:input];
  [input checkLastTagWas:0];
  [input release];
}

- (void)mergeVarintField:(int32_t)number value:(int32_t)value {
  checkNumber(number);
  [[self mutableFieldForNumber:number create:YES] addVarint:value];
}

- (BOOL)mergeFieldFrom:(int32_t)tag input:(GPBCodedInputStream *)input {
  int32_t number = GPBWireFormatGetTagFieldNumber(tag);
  GPBCodedInputStreamState *state = &input->state_;
  switch (GPBWireFormatGetTagWireType(tag)) {
    case GPBWireFormatVarint: {
      GPBField *field = [self mutableFieldForNumber:number create:YES];
      [field addVarint:GPBCodedInputStreamReadInt64(state)];
      return YES;
    }
    case GPBWireFormatFixed64: {
      GPBField *field = [self mutableFieldForNumber:number create:YES];
      [field addFixed64:GPBCodedInputStreamReadFixed64(state)];
      return YES;
    }
    case GPBWireFormatLengthDelimited: {
      NSData *data = GPBCodedInputStreamReadRetainedData(state);
      GPBField *field = [self mutableFieldForNumber:number create:YES];
      [field addLengthDelimited:data];
      [data release];
      return YES;
    }
    case GPBWireFormatStartGroup: {
      GPBUnknownFieldSet *unknownFieldSet = [[GPBUnknownFieldSet alloc] init];
      [input readUnknownGroup:number message:unknownFieldSet];
      GPBField *field = [self mutableFieldForNumber:number create:YES];
      [field addGroup:unknownFieldSet];
      [unknownFieldSet release];
      return YES;
    }
    case GPBWireFormatEndGroup:
      return NO;
    case GPBWireFormatFixed32: {
      GPBField *field = [self mutableFieldForNumber:number create:YES];
      [field addFixed32:GPBCodedInputStreamReadFixed32(state)];
      return YES;
    }
  }
}

- (void)mergeMessageSetMessage:(int32_t)number data:(NSData *)messageData {
  [[self mutableFieldForNumber:number create:YES]
      addLengthDelimited:messageData];
}

- (void)addUnknownMapEntry:(int32_t)fieldNum value:(NSData *)data {
  GPBField *field = [self mutableFieldForNumber:fieldNum create:YES];
  [field addLengthDelimited:data];
}

- (void)mergeFromCodedInputStream:(GPBCodedInputStream *)input {
  while (YES) {
    int32_t tag = GPBCodedInputStreamReadTag(&input->state_);
    if (tag == 0 || ![self mergeFieldFrom:tag input:input]) {
      break;
    }
  }
}

- (void)getTags:(int32_t *)tags {
  if (!fields_) return;
  size_t count = CFDictionaryGetCount(fields_);
  ssize_t keys[count];
  CFDictionaryGetKeysAndValues(fields_, (const void **)keys, NULL);
  for (size_t i = 0; i < count; ++i) {
    tags[i] = (int32_t)keys[i];
  }
}

@end
