// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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

#import "GPBArray_PackagePrivate.h"

#import "GPBMessage_PackagePrivate.h"

// Mutable arrays use an internal buffer that can always hold a multiple of this elements.
#define kChunkSize 16
#define CapacityFromCount(x) (((x / kChunkSize) + 1) * kChunkSize)

static BOOL ArrayDefault_IsValidValue(int32_t value) {
  // Anything but the bad value marker is allowed.
  return (value != kGPBUnrecognizedEnumeratorValue);
}

//%PDDM-DEFINE VALIDATE_RANGE(INDEX, COUNT)
//%  if (INDEX >= COUNT) {
//%    [NSException raise:NSRangeException
//%                format:@"Index (%lu) beyond bounds (%lu)",
//%                       (unsigned long)INDEX, (unsigned long)COUNT];
//%  }
//%PDDM-DEFINE MAYBE_GROW_TO_SET_COUNT(NEW_COUNT)
//%  if (NEW_COUNT > _capacity) {
//%    [self internalResizeToCapacity:CapacityFromCount(NEW_COUNT)];
//%  }
//%  _count = NEW_COUNT;
//%PDDM-DEFINE SET_COUNT_AND_MAYBE_SHRINK(NEW_COUNT)
//%  _count = NEW_COUNT;
//%  if ((NEW_COUNT + (2 * kChunkSize)) < _capacity) {
//%    [self internalResizeToCapacity:CapacityFromCount(NEW_COUNT)];
//%  }

//
// Macros for the common basic cases.
//

//%PDDM-DEFINE ARRAY_INTERFACE_SIMPLE(NAME, TYPE, FORMAT)
//%#pragma mark - NAME
//%
//%@implementation GPB##NAME##Array {
//% @package
//%  TYPE *_values;
//%  NSUInteger _count;
//%  NSUInteger _capacity;
//%}
//%
//%@synthesize count = _count;
//%
//%+ (instancetype)array {
//%  return [[[self alloc] init] autorelease];
//%}
//%
//%+ (instancetype)arrayWithValue:(TYPE)value {
//%  // Cast is needed so the compiler knows what class we are invoking initWithValues: on to get
//%  // the type correct.
//%  return [[(GPB##NAME##Array*)[self alloc] initWithValues:&value count:1] autorelease];
//%}
//%
//%+ (instancetype)arrayWithValueArray:(GPB##NAME##Array *)array {
//%  return [[(GPB##NAME##Array*)[self alloc] initWithValueArray:array] autorelease];
//%}
//%
//%+ (instancetype)arrayWithCapacity:(NSUInteger)count {
//%  return [[[self alloc] initWithCapacity:count] autorelease];
//%}
//%
//%- (instancetype)init {
//%  self = [super init];
//%  // No work needed;
//%  return self;
//%}
//%
//%- (instancetype)initWithValueArray:(GPB##NAME##Array *)array {
//%  return [self initWithValues:array->_values count:array->_count];
//%}
//%
//%- (instancetype)initWithValues:(const TYPE [])values count:(NSUInteger)count {
//%  self = [self init];
//%  if (self) {
//%    if (count && values) {
//%      _values = reallocf(_values, count * sizeof(TYPE));
//%      if (_values != NULL) {
//%        _capacity = count;
//%        memcpy(_values, values, count * sizeof(TYPE));
//%        _count = count;
//%      } else {
//%        [self release];
//%        [NSException raise:NSMallocException
//%                    format:@"Failed to allocate %lu bytes",
//%                           (unsigned long)(count * sizeof(TYPE))];
//%      }
//%    }
//%  }
//%  return self;
//%}
//%
//%- (instancetype)initWithCapacity:(NSUInteger)count {
//%  self = [self initWithValues:NULL count:0];
//%  if (self && count) {
//%    [self internalResizeToCapacity:count];
//%  }
//%  return self;
//%}
//%
//%- (instancetype)copyWithZone:(NSZone *)zone {
//%  return [[GPB##NAME##Array allocWithZone:zone] initWithValues:_values count:_count];
//%}
//%
//%ARRAY_IMMUTABLE_CORE(NAME, TYPE, , FORMAT)
//%
//%- (TYPE)valueAtIndex:(NSUInteger)index {
//%VALIDATE_RANGE(index, _count)
//%  return _values[index];
//%}
//%
//%ARRAY_MUTABLE_CORE(NAME, TYPE, , FORMAT)
//%@end
//%

//
// Some core macros used for both the simple types and Enums.
//

//%PDDM-DEFINE ARRAY_IMMUTABLE_CORE(NAME, TYPE, ACCESSOR_NAME, FORMAT)
//%- (void)dealloc {
//%  NSAssert(!_autocreator,
//%           @"%@: Autocreator must be cleared before release, autocreator: %@",
//%           [self class], _autocreator);
//%  free(_values);
//%  [super dealloc];
//%}
//%
//%- (BOOL)isEqual:(GPB##NAME##Array *)other {
//%  if (self == other) {
//%    return YES;
//%  }
//%  if (![other isKindOfClass:[GPB##NAME##Array class]]) {
//%    return NO;
//%  }
//%  return (_count == other->_count
//%          && memcmp(_values, other->_values, (_count * sizeof(TYPE))) == 0);
//%}
//%
//%- (NSUInteger)hash {
//%  // Follow NSArray's lead, and use the count as the hash.
//%  return _count;
//%}
//%
//%- (NSString *)description {
//%  NSMutableString *result = [NSMutableString stringWithFormat:@"<%@ %p> { ", [self class], self];
//%  for (NSUInteger i = 0, count = _count; i < count; ++i) {
//%    if (i == 0) {
//%      [result appendFormat:@"##FORMAT##", _values[i]];
//%    } else {
//%      [result appendFormat:@", ##FORMAT##", _values[i]];
//%    }
//%  }
//%  [result appendFormat:@" }"];
//%  return result;
//%}
//%
//%- (void)enumerate##ACCESSOR_NAME##ValuesWithBlock:(void (^)(TYPE value, NSUInteger idx, BOOL *stop))block {
//%  [self enumerate##ACCESSOR_NAME##ValuesWithOptions:0 usingBlock:block];
//%}
//%
//%- (void)enumerate##ACCESSOR_NAME##ValuesWithOptions:(NSEnumerationOptions)opts
//%                  ACCESSOR_NAME$S      usingBlock:(void (^)(TYPE value, NSUInteger idx, BOOL *stop))block {
//%  // NSEnumerationConcurrent isn't currently supported (and Apple's docs say that is ok).
//%  BOOL stop = NO;
//%  if ((opts & NSEnumerationReverse) == 0) {
//%    for (NSUInteger i = 0, count = _count; i < count; ++i) {
//%      block(_values[i], i, &stop);
//%      if (stop) break;
//%    }
//%  } else if (_count > 0) {
//%    for (NSUInteger i = _count; i > 0; --i) {
//%      block(_values[i - 1], (i - 1), &stop);
//%      if (stop) break;
//%    }
//%  }
//%}

//%PDDM-DEFINE MUTATION_HOOK_None()
//%PDDM-DEFINE MUTATION_METHODS(NAME, TYPE, ACCESSOR_NAME, HOOK_1, HOOK_2)
//%- (void)add##ACCESSOR_NAME##Value:(TYPE)value {
//%  [self add##ACCESSOR_NAME##Values:&value count:1];
//%}
//%
//%- (void)add##ACCESSOR_NAME##Values:(const TYPE [])values count:(NSUInteger)count {
//%  if (values == NULL || count == 0) return;
//%MUTATION_HOOK_##HOOK_1()  NSUInteger initialCount = _count;
//%  NSUInteger newCount = initialCount + count;
//%MAYBE_GROW_TO_SET_COUNT(newCount)
//%  memcpy(&_values[initialCount], values, count * sizeof(TYPE));
//%  if (_autocreator) {
//%    GPBAutocreatedArrayModified(_autocreator, self);
//%  }
//%}
//%
//%- (void)insert##ACCESSOR_NAME##Value:(TYPE)value atIndex:(NSUInteger)index {
//%VALIDATE_RANGE(index, _count + 1)
//%MUTATION_HOOK_##HOOK_2()  NSUInteger initialCount = _count;
//%  NSUInteger newCount = initialCount + 1;
//%MAYBE_GROW_TO_SET_COUNT(newCount)
//%  if (index != initialCount) {
//%    memmove(&_values[index + 1], &_values[index], (initialCount - index) * sizeof(TYPE));
//%  }
//%  _values[index] = value;
//%  if (_autocreator) {
//%    GPBAutocreatedArrayModified(_autocreator, self);
//%  }
//%}
//%
//%- (void)replaceValueAtIndex:(NSUInteger)index with##ACCESSOR_NAME##Value:(TYPE)value {
//%VALIDATE_RANGE(index, _count)
//%MUTATION_HOOK_##HOOK_2()  _values[index] = value;
//%}

//%PDDM-DEFINE ARRAY_MUTABLE_CORE(NAME, TYPE, ACCESSOR_NAME, FORMAT)
//%- (void)internalResizeToCapacity:(NSUInteger)newCapacity {
//%  _values = reallocf(_values, newCapacity * sizeof(TYPE));
//%  if (_values == NULL) {
//%    _capacity = 0;
//%    _count = 0;
//%    [NSException raise:NSMallocException
//%                format:@"Failed to allocate %lu bytes",
//%                       (unsigned long)(newCapacity * sizeof(TYPE))];
//%  }
//%  _capacity = newCapacity;
//%}
//%
//%MUTATION_METHODS(NAME, TYPE, ACCESSOR_NAME, None, None)
//%
//%- (void)add##ACCESSOR_NAME##ValuesFromArray:(GPB##NAME##Array *)array {
//%  [self add##ACCESSOR_NAME##Values:array->_values count:array->_count];
//%}
//%
//%- (void)removeValueAtIndex:(NSUInteger)index {
//%VALIDATE_RANGE(index, _count)
//%  NSUInteger newCount = _count - 1;
//%  if (index != newCount) {
//%    memmove(&_values[index], &_values[index + 1], (newCount - index) * sizeof(TYPE));
//%  }
//%SET_COUNT_AND_MAYBE_SHRINK(newCount)
//%}
//%
//%- (void)removeAll {
//%SET_COUNT_AND_MAYBE_SHRINK(0)
//%}
//%
//%- (void)exchangeValueAtIndex:(NSUInteger)idx1
//%            withValueAtIndex:(NSUInteger)idx2 {
//%VALIDATE_RANGE(idx1, _count)
//%VALIDATE_RANGE(idx2, _count)
//%  TYPE temp = _values[idx1];
//%  _values[idx1] = _values[idx2];
//%  _values[idx2] = temp;
//%}
//%

//%PDDM-EXPAND ARRAY_INTERFACE_SIMPLE(Int32, int32_t, %d)
// This block of code is generated, do not edit it directly.

#pragma mark - Int32

@implementation GPBInt32Array {
 @package
  int32_t *_values;
  NSUInteger _count;
  NSUInteger _capacity;
}

@synthesize count = _count;

+ (instancetype)array {
  return [[[self alloc] init] autorelease];
}

+ (instancetype)arrayWithValue:(int32_t)value {
  // Cast is needed so the compiler knows what class we are invoking initWithValues: on to get
  // the type correct.
  return [[(GPBInt32Array*)[self alloc] initWithValues:&value count:1] autorelease];
}

+ (instancetype)arrayWithValueArray:(GPBInt32Array *)array {
  return [[(GPBInt32Array*)[self alloc] initWithValueArray:array] autorelease];
}

+ (instancetype)arrayWithCapacity:(NSUInteger)count {
  return [[[self alloc] initWithCapacity:count] autorelease];
}

- (instancetype)init {
  self = [super init];
  // No work needed;
  return self;
}

- (instancetype)initWithValueArray:(GPBInt32Array *)array {
  return [self initWithValues:array->_values count:array->_count];
}

- (instancetype)initWithValues:(const int32_t [])values count:(NSUInteger)count {
  self = [self init];
  if (self) {
    if (count && values) {
      _values = reallocf(_values, count * sizeof(int32_t));
      if (_values != NULL) {
        _capacity = count;
        memcpy(_values, values, count * sizeof(int32_t));
        _count = count;
      } else {
        [self release];
        [NSException raise:NSMallocException
                    format:@"Failed to allocate %lu bytes",
                           (unsigned long)(count * sizeof(int32_t))];
      }
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)count {
  self = [self initWithValues:NULL count:0];
  if (self && count) {
    [self internalResizeToCapacity:count];
  }
  return self;
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBInt32Array allocWithZone:zone] initWithValues:_values count:_count];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  free(_values);
  [super dealloc];
}

- (BOOL)isEqual:(GPBInt32Array *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt32Array class]]) {
    return NO;
  }
  return (_count == other->_count
          && memcmp(_values, other->_values, (_count * sizeof(int32_t))) == 0);
}

- (NSUInteger)hash {
  // Follow NSArray's lead, and use the count as the hash.
  return _count;
}

- (NSString *)description {
  NSMutableString *result = [NSMutableString stringWithFormat:@"<%@ %p> { ", [self class], self];
  for (NSUInteger i = 0, count = _count; i < count; ++i) {
    if (i == 0) {
      [result appendFormat:@"%d", _values[i]];
    } else {
      [result appendFormat:@", %d", _values[i]];
    }
  }
  [result appendFormat:@" }"];
  return result;
}

- (void)enumerateValuesWithBlock:(void (^)(int32_t value, NSUInteger idx, BOOL *stop))block {
  [self enumerateValuesWithOptions:0 usingBlock:block];
}

- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void (^)(int32_t value, NSUInteger idx, BOOL *stop))block {
  // NSEnumerationConcurrent isn't currently supported (and Apple's docs say that is ok).
  BOOL stop = NO;
  if ((opts & NSEnumerationReverse) == 0) {
    for (NSUInteger i = 0, count = _count; i < count; ++i) {
      block(_values[i], i, &stop);
      if (stop) break;
    }
  } else if (_count > 0) {
    for (NSUInteger i = _count; i > 0; --i) {
      block(_values[i - 1], (i - 1), &stop);
      if (stop) break;
    }
  }
}

- (int32_t)valueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  return _values[index];
}

- (void)internalResizeToCapacity:(NSUInteger)newCapacity {
  _values = reallocf(_values, newCapacity * sizeof(int32_t));
  if (_values == NULL) {
    _capacity = 0;
    _count = 0;
    [NSException raise:NSMallocException
                format:@"Failed to allocate %lu bytes",
                       (unsigned long)(newCapacity * sizeof(int32_t))];
  }
  _capacity = newCapacity;
}

- (void)addValue:(int32_t)value {
  [self addValues:&value count:1];
}

- (void)addValues:(const int32_t [])values count:(NSUInteger)count {
  if (values == NULL || count == 0) return;
  NSUInteger initialCount = _count;
  NSUInteger newCount = initialCount + count;
  if (newCount > _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
  _count = newCount;
  memcpy(&_values[initialCount], values, count * sizeof(int32_t));
  if (_autocreator) {
    GPBAutocreatedArrayModified(_autocreator, self);
  }
}

- (void)insertValue:(int32_t)value atIndex:(NSUInteger)index {
  if (index >= _count + 1) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count + 1];
  }
  NSUInteger initialCount = _count;
  NSUInteger newCount = initialCount + 1;
  if (newCount > _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
  _count = newCount;
  if (index != initialCount) {
    memmove(&_values[index + 1], &_values[index], (initialCount - index) * sizeof(int32_t));
  }
  _values[index] = value;
  if (_autocreator) {
    GPBAutocreatedArrayModified(_autocreator, self);
  }
}

- (void)replaceValueAtIndex:(NSUInteger)index withValue:(int32_t)value {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  _values[index] = value;
}

- (void)addValuesFromArray:(GPBInt32Array *)array {
  [self addValues:array->_values count:array->_count];
}

- (void)removeValueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  NSUInteger newCount = _count - 1;
  if (index != newCount) {
    memmove(&_values[index], &_values[index + 1], (newCount - index) * sizeof(int32_t));
  }
  _count = newCount;
  if ((newCount + (2 * kChunkSize)) < _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
}

- (void)removeAll {
  _count = 0;
  if ((0 + (2 * kChunkSize)) < _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(0)];
  }
}

- (void)exchangeValueAtIndex:(NSUInteger)idx1
            withValueAtIndex:(NSUInteger)idx2 {
  if (idx1 >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)idx1, (unsigned long)_count];
  }
  if (idx2 >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)idx2, (unsigned long)_count];
  }
  int32_t temp = _values[idx1];
  _values[idx1] = _values[idx2];
  _values[idx2] = temp;
}

@end

//%PDDM-EXPAND ARRAY_INTERFACE_SIMPLE(UInt32, uint32_t, %u)
// This block of code is generated, do not edit it directly.

#pragma mark - UInt32

@implementation GPBUInt32Array {
 @package
  uint32_t *_values;
  NSUInteger _count;
  NSUInteger _capacity;
}

@synthesize count = _count;

+ (instancetype)array {
  return [[[self alloc] init] autorelease];
}

+ (instancetype)arrayWithValue:(uint32_t)value {
  // Cast is needed so the compiler knows what class we are invoking initWithValues: on to get
  // the type correct.
  return [[(GPBUInt32Array*)[self alloc] initWithValues:&value count:1] autorelease];
}

+ (instancetype)arrayWithValueArray:(GPBUInt32Array *)array {
  return [[(GPBUInt32Array*)[self alloc] initWithValueArray:array] autorelease];
}

+ (instancetype)arrayWithCapacity:(NSUInteger)count {
  return [[[self alloc] initWithCapacity:count] autorelease];
}

- (instancetype)init {
  self = [super init];
  // No work needed;
  return self;
}

- (instancetype)initWithValueArray:(GPBUInt32Array *)array {
  return [self initWithValues:array->_values count:array->_count];
}

- (instancetype)initWithValues:(const uint32_t [])values count:(NSUInteger)count {
  self = [self init];
  if (self) {
    if (count && values) {
      _values = reallocf(_values, count * sizeof(uint32_t));
      if (_values != NULL) {
        _capacity = count;
        memcpy(_values, values, count * sizeof(uint32_t));
        _count = count;
      } else {
        [self release];
        [NSException raise:NSMallocException
                    format:@"Failed to allocate %lu bytes",
                           (unsigned long)(count * sizeof(uint32_t))];
      }
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)count {
  self = [self initWithValues:NULL count:0];
  if (self && count) {
    [self internalResizeToCapacity:count];
  }
  return self;
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBUInt32Array allocWithZone:zone] initWithValues:_values count:_count];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  free(_values);
  [super dealloc];
}

- (BOOL)isEqual:(GPBUInt32Array *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt32Array class]]) {
    return NO;
  }
  return (_count == other->_count
          && memcmp(_values, other->_values, (_count * sizeof(uint32_t))) == 0);
}

- (NSUInteger)hash {
  // Follow NSArray's lead, and use the count as the hash.
  return _count;
}

- (NSString *)description {
  NSMutableString *result = [NSMutableString stringWithFormat:@"<%@ %p> { ", [self class], self];
  for (NSUInteger i = 0, count = _count; i < count; ++i) {
    if (i == 0) {
      [result appendFormat:@"%u", _values[i]];
    } else {
      [result appendFormat:@", %u", _values[i]];
    }
  }
  [result appendFormat:@" }"];
  return result;
}

- (void)enumerateValuesWithBlock:(void (^)(uint32_t value, NSUInteger idx, BOOL *stop))block {
  [self enumerateValuesWithOptions:0 usingBlock:block];
}

- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void (^)(uint32_t value, NSUInteger idx, BOOL *stop))block {
  // NSEnumerationConcurrent isn't currently supported (and Apple's docs say that is ok).
  BOOL stop = NO;
  if ((opts & NSEnumerationReverse) == 0) {
    for (NSUInteger i = 0, count = _count; i < count; ++i) {
      block(_values[i], i, &stop);
      if (stop) break;
    }
  } else if (_count > 0) {
    for (NSUInteger i = _count; i > 0; --i) {
      block(_values[i - 1], (i - 1), &stop);
      if (stop) break;
    }
  }
}

- (uint32_t)valueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  return _values[index];
}

- (void)internalResizeToCapacity:(NSUInteger)newCapacity {
  _values = reallocf(_values, newCapacity * sizeof(uint32_t));
  if (_values == NULL) {
    _capacity = 0;
    _count = 0;
    [NSException raise:NSMallocException
                format:@"Failed to allocate %lu bytes",
                       (unsigned long)(newCapacity * sizeof(uint32_t))];
  }
  _capacity = newCapacity;
}

- (void)addValue:(uint32_t)value {
  [self addValues:&value count:1];
}

- (void)addValues:(const uint32_t [])values count:(NSUInteger)count {
  if (values == NULL || count == 0) return;
  NSUInteger initialCount = _count;
  NSUInteger newCount = initialCount + count;
  if (newCount > _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
  _count = newCount;
  memcpy(&_values[initialCount], values, count * sizeof(uint32_t));
  if (_autocreator) {
    GPBAutocreatedArrayModified(_autocreator, self);
  }
}

- (void)insertValue:(uint32_t)value atIndex:(NSUInteger)index {
  if (index >= _count + 1) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count + 1];
  }
  NSUInteger initialCount = _count;
  NSUInteger newCount = initialCount + 1;
  if (newCount > _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
  _count = newCount;
  if (index != initialCount) {
    memmove(&_values[index + 1], &_values[index], (initialCount - index) * sizeof(uint32_t));
  }
  _values[index] = value;
  if (_autocreator) {
    GPBAutocreatedArrayModified(_autocreator, self);
  }
}

- (void)replaceValueAtIndex:(NSUInteger)index withValue:(uint32_t)value {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  _values[index] = value;
}

- (void)addValuesFromArray:(GPBUInt32Array *)array {
  [self addValues:array->_values count:array->_count];
}

- (void)removeValueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  NSUInteger newCount = _count - 1;
  if (index != newCount) {
    memmove(&_values[index], &_values[index + 1], (newCount - index) * sizeof(uint32_t));
  }
  _count = newCount;
  if ((newCount + (2 * kChunkSize)) < _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
}

- (void)removeAll {
  _count = 0;
  if ((0 + (2 * kChunkSize)) < _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(0)];
  }
}

- (void)exchangeValueAtIndex:(NSUInteger)idx1
            withValueAtIndex:(NSUInteger)idx2 {
  if (idx1 >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)idx1, (unsigned long)_count];
  }
  if (idx2 >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)idx2, (unsigned long)_count];
  }
  uint32_t temp = _values[idx1];
  _values[idx1] = _values[idx2];
  _values[idx2] = temp;
}

@end

//%PDDM-EXPAND ARRAY_INTERFACE_SIMPLE(Int64, int64_t, %lld)
// This block of code is generated, do not edit it directly.

#pragma mark - Int64

@implementation GPBInt64Array {
 @package
  int64_t *_values;
  NSUInteger _count;
  NSUInteger _capacity;
}

@synthesize count = _count;

+ (instancetype)array {
  return [[[self alloc] init] autorelease];
}

+ (instancetype)arrayWithValue:(int64_t)value {
  // Cast is needed so the compiler knows what class we are invoking initWithValues: on to get
  // the type correct.
  return [[(GPBInt64Array*)[self alloc] initWithValues:&value count:1] autorelease];
}

+ (instancetype)arrayWithValueArray:(GPBInt64Array *)array {
  return [[(GPBInt64Array*)[self alloc] initWithValueArray:array] autorelease];
}

+ (instancetype)arrayWithCapacity:(NSUInteger)count {
  return [[[self alloc] initWithCapacity:count] autorelease];
}

- (instancetype)init {
  self = [super init];
  // No work needed;
  return self;
}

- (instancetype)initWithValueArray:(GPBInt64Array *)array {
  return [self initWithValues:array->_values count:array->_count];
}

- (instancetype)initWithValues:(const int64_t [])values count:(NSUInteger)count {
  self = [self init];
  if (self) {
    if (count && values) {
      _values = reallocf(_values, count * sizeof(int64_t));
      if (_values != NULL) {
        _capacity = count;
        memcpy(_values, values, count * sizeof(int64_t));
        _count = count;
      } else {
        [self release];
        [NSException raise:NSMallocException
                    format:@"Failed to allocate %lu bytes",
                           (unsigned long)(count * sizeof(int64_t))];
      }
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)count {
  self = [self initWithValues:NULL count:0];
  if (self && count) {
    [self internalResizeToCapacity:count];
  }
  return self;
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBInt64Array allocWithZone:zone] initWithValues:_values count:_count];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  free(_values);
  [super dealloc];
}

- (BOOL)isEqual:(GPBInt64Array *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt64Array class]]) {
    return NO;
  }
  return (_count == other->_count
          && memcmp(_values, other->_values, (_count * sizeof(int64_t))) == 0);
}

- (NSUInteger)hash {
  // Follow NSArray's lead, and use the count as the hash.
  return _count;
}

- (NSString *)description {
  NSMutableString *result = [NSMutableString stringWithFormat:@"<%@ %p> { ", [self class], self];
  for (NSUInteger i = 0, count = _count; i < count; ++i) {
    if (i == 0) {
      [result appendFormat:@"%lld", _values[i]];
    } else {
      [result appendFormat:@", %lld", _values[i]];
    }
  }
  [result appendFormat:@" }"];
  return result;
}

- (void)enumerateValuesWithBlock:(void (^)(int64_t value, NSUInteger idx, BOOL *stop))block {
  [self enumerateValuesWithOptions:0 usingBlock:block];
}

- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void (^)(int64_t value, NSUInteger idx, BOOL *stop))block {
  // NSEnumerationConcurrent isn't currently supported (and Apple's docs say that is ok).
  BOOL stop = NO;
  if ((opts & NSEnumerationReverse) == 0) {
    for (NSUInteger i = 0, count = _count; i < count; ++i) {
      block(_values[i], i, &stop);
      if (stop) break;
    }
  } else if (_count > 0) {
    for (NSUInteger i = _count; i > 0; --i) {
      block(_values[i - 1], (i - 1), &stop);
      if (stop) break;
    }
  }
}

- (int64_t)valueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  return _values[index];
}

- (void)internalResizeToCapacity:(NSUInteger)newCapacity {
  _values = reallocf(_values, newCapacity * sizeof(int64_t));
  if (_values == NULL) {
    _capacity = 0;
    _count = 0;
    [NSException raise:NSMallocException
                format:@"Failed to allocate %lu bytes",
                       (unsigned long)(newCapacity * sizeof(int64_t))];
  }
  _capacity = newCapacity;
}

- (void)addValue:(int64_t)value {
  [self addValues:&value count:1];
}

- (void)addValues:(const int64_t [])values count:(NSUInteger)count {
  if (values == NULL || count == 0) return;
  NSUInteger initialCount = _count;
  NSUInteger newCount = initialCount + count;
  if (newCount > _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
  _count = newCount;
  memcpy(&_values[initialCount], values, count * sizeof(int64_t));
  if (_autocreator) {
    GPBAutocreatedArrayModified(_autocreator, self);
  }
}

- (void)insertValue:(int64_t)value atIndex:(NSUInteger)index {
  if (index >= _count + 1) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count + 1];
  }
  NSUInteger initialCount = _count;
  NSUInteger newCount = initialCount + 1;
  if (newCount > _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
  _count = newCount;
  if (index != initialCount) {
    memmove(&_values[index + 1], &_values[index], (initialCount - index) * sizeof(int64_t));
  }
  _values[index] = value;
  if (_autocreator) {
    GPBAutocreatedArrayModified(_autocreator, self);
  }
}

- (void)replaceValueAtIndex:(NSUInteger)index withValue:(int64_t)value {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  _values[index] = value;
}

- (void)addValuesFromArray:(GPBInt64Array *)array {
  [self addValues:array->_values count:array->_count];
}

- (void)removeValueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  NSUInteger newCount = _count - 1;
  if (index != newCount) {
    memmove(&_values[index], &_values[index + 1], (newCount - index) * sizeof(int64_t));
  }
  _count = newCount;
  if ((newCount + (2 * kChunkSize)) < _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
}

- (void)removeAll {
  _count = 0;
  if ((0 + (2 * kChunkSize)) < _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(0)];
  }
}

- (void)exchangeValueAtIndex:(NSUInteger)idx1
            withValueAtIndex:(NSUInteger)idx2 {
  if (idx1 >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)idx1, (unsigned long)_count];
  }
  if (idx2 >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)idx2, (unsigned long)_count];
  }
  int64_t temp = _values[idx1];
  _values[idx1] = _values[idx2];
  _values[idx2] = temp;
}

@end

//%PDDM-EXPAND ARRAY_INTERFACE_SIMPLE(UInt64, uint64_t, %llu)
// This block of code is generated, do not edit it directly.

#pragma mark - UInt64

@implementation GPBUInt64Array {
 @package
  uint64_t *_values;
  NSUInteger _count;
  NSUInteger _capacity;
}

@synthesize count = _count;

+ (instancetype)array {
  return [[[self alloc] init] autorelease];
}

+ (instancetype)arrayWithValue:(uint64_t)value {
  // Cast is needed so the compiler knows what class we are invoking initWithValues: on to get
  // the type correct.
  return [[(GPBUInt64Array*)[self alloc] initWithValues:&value count:1] autorelease];
}

+ (instancetype)arrayWithValueArray:(GPBUInt64Array *)array {
  return [[(GPBUInt64Array*)[self alloc] initWithValueArray:array] autorelease];
}

+ (instancetype)arrayWithCapacity:(NSUInteger)count {
  return [[[self alloc] initWithCapacity:count] autorelease];
}

- (instancetype)init {
  self = [super init];
  // No work needed;
  return self;
}

- (instancetype)initWithValueArray:(GPBUInt64Array *)array {
  return [self initWithValues:array->_values count:array->_count];
}

- (instancetype)initWithValues:(const uint64_t [])values count:(NSUInteger)count {
  self = [self init];
  if (self) {
    if (count && values) {
      _values = reallocf(_values, count * sizeof(uint64_t));
      if (_values != NULL) {
        _capacity = count;
        memcpy(_values, values, count * sizeof(uint64_t));
        _count = count;
      } else {
        [self release];
        [NSException raise:NSMallocException
                    format:@"Failed to allocate %lu bytes",
                           (unsigned long)(count * sizeof(uint64_t))];
      }
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)count {
  self = [self initWithValues:NULL count:0];
  if (self && count) {
    [self internalResizeToCapacity:count];
  }
  return self;
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBUInt64Array allocWithZone:zone] initWithValues:_values count:_count];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  free(_values);
  [super dealloc];
}

- (BOOL)isEqual:(GPBUInt64Array *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt64Array class]]) {
    return NO;
  }
  return (_count == other->_count
          && memcmp(_values, other->_values, (_count * sizeof(uint64_t))) == 0);
}

- (NSUInteger)hash {
  // Follow NSArray's lead, and use the count as the hash.
  return _count;
}

- (NSString *)description {
  NSMutableString *result = [NSMutableString stringWithFormat:@"<%@ %p> { ", [self class], self];
  for (NSUInteger i = 0, count = _count; i < count; ++i) {
    if (i == 0) {
      [result appendFormat:@"%llu", _values[i]];
    } else {
      [result appendFormat:@", %llu", _values[i]];
    }
  }
  [result appendFormat:@" }"];
  return result;
}

- (void)enumerateValuesWithBlock:(void (^)(uint64_t value, NSUInteger idx, BOOL *stop))block {
  [self enumerateValuesWithOptions:0 usingBlock:block];
}

- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void (^)(uint64_t value, NSUInteger idx, BOOL *stop))block {
  // NSEnumerationConcurrent isn't currently supported (and Apple's docs say that is ok).
  BOOL stop = NO;
  if ((opts & NSEnumerationReverse) == 0) {
    for (NSUInteger i = 0, count = _count; i < count; ++i) {
      block(_values[i], i, &stop);
      if (stop) break;
    }
  } else if (_count > 0) {
    for (NSUInteger i = _count; i > 0; --i) {
      block(_values[i - 1], (i - 1), &stop);
      if (stop) break;
    }
  }
}

- (uint64_t)valueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  return _values[index];
}

- (void)internalResizeToCapacity:(NSUInteger)newCapacity {
  _values = reallocf(_values, newCapacity * sizeof(uint64_t));
  if (_values == NULL) {
    _capacity = 0;
    _count = 0;
    [NSException raise:NSMallocException
                format:@"Failed to allocate %lu bytes",
                       (unsigned long)(newCapacity * sizeof(uint64_t))];
  }
  _capacity = newCapacity;
}

- (void)addValue:(uint64_t)value {
  [self addValues:&value count:1];
}

- (void)addValues:(const uint64_t [])values count:(NSUInteger)count {
  if (values == NULL || count == 0) return;
  NSUInteger initialCount = _count;
  NSUInteger newCount = initialCount + count;
  if (newCount > _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
  _count = newCount;
  memcpy(&_values[initialCount], values, count * sizeof(uint64_t));
  if (_autocreator) {
    GPBAutocreatedArrayModified(_autocreator, self);
  }
}

- (void)insertValue:(uint64_t)value atIndex:(NSUInteger)index {
  if (index >= _count + 1) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count + 1];
  }
  NSUInteger initialCount = _count;
  NSUInteger newCount = initialCount + 1;
  if (newCount > _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
  _count = newCount;
  if (index != initialCount) {
    memmove(&_values[index + 1], &_values[index], (initialCount - index) * sizeof(uint64_t));
  }
  _values[index] = value;
  if (_autocreator) {
    GPBAutocreatedArrayModified(_autocreator, self);
  }
}

- (void)replaceValueAtIndex:(NSUInteger)index withValue:(uint64_t)value {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  _values[index] = value;
}

- (void)addValuesFromArray:(GPBUInt64Array *)array {
  [self addValues:array->_values count:array->_count];
}

- (void)removeValueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  NSUInteger newCount = _count - 1;
  if (index != newCount) {
    memmove(&_values[index], &_values[index + 1], (newCount - index) * sizeof(uint64_t));
  }
  _count = newCount;
  if ((newCount + (2 * kChunkSize)) < _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
}

- (void)removeAll {
  _count = 0;
  if ((0 + (2 * kChunkSize)) < _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(0)];
  }
}

- (void)exchangeValueAtIndex:(NSUInteger)idx1
            withValueAtIndex:(NSUInteger)idx2 {
  if (idx1 >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)idx1, (unsigned long)_count];
  }
  if (idx2 >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)idx2, (unsigned long)_count];
  }
  uint64_t temp = _values[idx1];
  _values[idx1] = _values[idx2];
  _values[idx2] = temp;
}

@end

//%PDDM-EXPAND ARRAY_INTERFACE_SIMPLE(Float, float, %f)
// This block of code is generated, do not edit it directly.

#pragma mark - Float

@implementation GPBFloatArray {
 @package
  float *_values;
  NSUInteger _count;
  NSUInteger _capacity;
}

@synthesize count = _count;

+ (instancetype)array {
  return [[[self alloc] init] autorelease];
}

+ (instancetype)arrayWithValue:(float)value {
  // Cast is needed so the compiler knows what class we are invoking initWithValues: on to get
  // the type correct.
  return [[(GPBFloatArray*)[self alloc] initWithValues:&value count:1] autorelease];
}

+ (instancetype)arrayWithValueArray:(GPBFloatArray *)array {
  return [[(GPBFloatArray*)[self alloc] initWithValueArray:array] autorelease];
}

+ (instancetype)arrayWithCapacity:(NSUInteger)count {
  return [[[self alloc] initWithCapacity:count] autorelease];
}

- (instancetype)init {
  self = [super init];
  // No work needed;
  return self;
}

- (instancetype)initWithValueArray:(GPBFloatArray *)array {
  return [self initWithValues:array->_values count:array->_count];
}

- (instancetype)initWithValues:(const float [])values count:(NSUInteger)count {
  self = [self init];
  if (self) {
    if (count && values) {
      _values = reallocf(_values, count * sizeof(float));
      if (_values != NULL) {
        _capacity = count;
        memcpy(_values, values, count * sizeof(float));
        _count = count;
      } else {
        [self release];
        [NSException raise:NSMallocException
                    format:@"Failed to allocate %lu bytes",
                           (unsigned long)(count * sizeof(float))];
      }
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)count {
  self = [self initWithValues:NULL count:0];
  if (self && count) {
    [self internalResizeToCapacity:count];
  }
  return self;
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBFloatArray allocWithZone:zone] initWithValues:_values count:_count];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  free(_values);
  [super dealloc];
}

- (BOOL)isEqual:(GPBFloatArray *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBFloatArray class]]) {
    return NO;
  }
  return (_count == other->_count
          && memcmp(_values, other->_values, (_count * sizeof(float))) == 0);
}

- (NSUInteger)hash {
  // Follow NSArray's lead, and use the count as the hash.
  return _count;
}

- (NSString *)description {
  NSMutableString *result = [NSMutableString stringWithFormat:@"<%@ %p> { ", [self class], self];
  for (NSUInteger i = 0, count = _count; i < count; ++i) {
    if (i == 0) {
      [result appendFormat:@"%f", _values[i]];
    } else {
      [result appendFormat:@", %f", _values[i]];
    }
  }
  [result appendFormat:@" }"];
  return result;
}

- (void)enumerateValuesWithBlock:(void (^)(float value, NSUInteger idx, BOOL *stop))block {
  [self enumerateValuesWithOptions:0 usingBlock:block];
}

- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void (^)(float value, NSUInteger idx, BOOL *stop))block {
  // NSEnumerationConcurrent isn't currently supported (and Apple's docs say that is ok).
  BOOL stop = NO;
  if ((opts & NSEnumerationReverse) == 0) {
    for (NSUInteger i = 0, count = _count; i < count; ++i) {
      block(_values[i], i, &stop);
      if (stop) break;
    }
  } else if (_count > 0) {
    for (NSUInteger i = _count; i > 0; --i) {
      block(_values[i - 1], (i - 1), &stop);
      if (stop) break;
    }
  }
}

- (float)valueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  return _values[index];
}

- (void)internalResizeToCapacity:(NSUInteger)newCapacity {
  _values = reallocf(_values, newCapacity * sizeof(float));
  if (_values == NULL) {
    _capacity = 0;
    _count = 0;
    [NSException raise:NSMallocException
                format:@"Failed to allocate %lu bytes",
                       (unsigned long)(newCapacity * sizeof(float))];
  }
  _capacity = newCapacity;
}

- (void)addValue:(float)value {
  [self addValues:&value count:1];
}

- (void)addValues:(const float [])values count:(NSUInteger)count {
  if (values == NULL || count == 0) return;
  NSUInteger initialCount = _count;
  NSUInteger newCount = initialCount + count;
  if (newCount > _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
  _count = newCount;
  memcpy(&_values[initialCount], values, count * sizeof(float));
  if (_autocreator) {
    GPBAutocreatedArrayModified(_autocreator, self);
  }
}

- (void)insertValue:(float)value atIndex:(NSUInteger)index {
  if (index >= _count + 1) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count + 1];
  }
  NSUInteger initialCount = _count;
  NSUInteger newCount = initialCount + 1;
  if (newCount > _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
  _count = newCount;
  if (index != initialCount) {
    memmove(&_values[index + 1], &_values[index], (initialCount - index) * sizeof(float));
  }
  _values[index] = value;
  if (_autocreator) {
    GPBAutocreatedArrayModified(_autocreator, self);
  }
}

- (void)replaceValueAtIndex:(NSUInteger)index withValue:(float)value {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  _values[index] = value;
}

- (void)addValuesFromArray:(GPBFloatArray *)array {
  [self addValues:array->_values count:array->_count];
}

- (void)removeValueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  NSUInteger newCount = _count - 1;
  if (index != newCount) {
    memmove(&_values[index], &_values[index + 1], (newCount - index) * sizeof(float));
  }
  _count = newCount;
  if ((newCount + (2 * kChunkSize)) < _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
}

- (void)removeAll {
  _count = 0;
  if ((0 + (2 * kChunkSize)) < _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(0)];
  }
}

- (void)exchangeValueAtIndex:(NSUInteger)idx1
            withValueAtIndex:(NSUInteger)idx2 {
  if (idx1 >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)idx1, (unsigned long)_count];
  }
  if (idx2 >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)idx2, (unsigned long)_count];
  }
  float temp = _values[idx1];
  _values[idx1] = _values[idx2];
  _values[idx2] = temp;
}

@end

//%PDDM-EXPAND ARRAY_INTERFACE_SIMPLE(Double, double, %lf)
// This block of code is generated, do not edit it directly.

#pragma mark - Double

@implementation GPBDoubleArray {
 @package
  double *_values;
  NSUInteger _count;
  NSUInteger _capacity;
}

@synthesize count = _count;

+ (instancetype)array {
  return [[[self alloc] init] autorelease];
}

+ (instancetype)arrayWithValue:(double)value {
  // Cast is needed so the compiler knows what class we are invoking initWithValues: on to get
  // the type correct.
  return [[(GPBDoubleArray*)[self alloc] initWithValues:&value count:1] autorelease];
}

+ (instancetype)arrayWithValueArray:(GPBDoubleArray *)array {
  return [[(GPBDoubleArray*)[self alloc] initWithValueArray:array] autorelease];
}

+ (instancetype)arrayWithCapacity:(NSUInteger)count {
  return [[[self alloc] initWithCapacity:count] autorelease];
}

- (instancetype)init {
  self = [super init];
  // No work needed;
  return self;
}

- (instancetype)initWithValueArray:(GPBDoubleArray *)array {
  return [self initWithValues:array->_values count:array->_count];
}

- (instancetype)initWithValues:(const double [])values count:(NSUInteger)count {
  self = [self init];
  if (self) {
    if (count && values) {
      _values = reallocf(_values, count * sizeof(double));
      if (_values != NULL) {
        _capacity = count;
        memcpy(_values, values, count * sizeof(double));
        _count = count;
      } else {
        [self release];
        [NSException raise:NSMallocException
                    format:@"Failed to allocate %lu bytes",
                           (unsigned long)(count * sizeof(double))];
      }
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)count {
  self = [self initWithValues:NULL count:0];
  if (self && count) {
    [self internalResizeToCapacity:count];
  }
  return self;
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBDoubleArray allocWithZone:zone] initWithValues:_values count:_count];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  free(_values);
  [super dealloc];
}

- (BOOL)isEqual:(GPBDoubleArray *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBDoubleArray class]]) {
    return NO;
  }
  return (_count == other->_count
          && memcmp(_values, other->_values, (_count * sizeof(double))) == 0);
}

- (NSUInteger)hash {
  // Follow NSArray's lead, and use the count as the hash.
  return _count;
}

- (NSString *)description {
  NSMutableString *result = [NSMutableString stringWithFormat:@"<%@ %p> { ", [self class], self];
  for (NSUInteger i = 0, count = _count; i < count; ++i) {
    if (i == 0) {
      [result appendFormat:@"%lf", _values[i]];
    } else {
      [result appendFormat:@", %lf", _values[i]];
    }
  }
  [result appendFormat:@" }"];
  return result;
}

- (void)enumerateValuesWithBlock:(void (^)(double value, NSUInteger idx, BOOL *stop))block {
  [self enumerateValuesWithOptions:0 usingBlock:block];
}

- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void (^)(double value, NSUInteger idx, BOOL *stop))block {
  // NSEnumerationConcurrent isn't currently supported (and Apple's docs say that is ok).
  BOOL stop = NO;
  if ((opts & NSEnumerationReverse) == 0) {
    for (NSUInteger i = 0, count = _count; i < count; ++i) {
      block(_values[i], i, &stop);
      if (stop) break;
    }
  } else if (_count > 0) {
    for (NSUInteger i = _count; i > 0; --i) {
      block(_values[i - 1], (i - 1), &stop);
      if (stop) break;
    }
  }
}

- (double)valueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  return _values[index];
}

- (void)internalResizeToCapacity:(NSUInteger)newCapacity {
  _values = reallocf(_values, newCapacity * sizeof(double));
  if (_values == NULL) {
    _capacity = 0;
    _count = 0;
    [NSException raise:NSMallocException
                format:@"Failed to allocate %lu bytes",
                       (unsigned long)(newCapacity * sizeof(double))];
  }
  _capacity = newCapacity;
}

- (void)addValue:(double)value {
  [self addValues:&value count:1];
}

- (void)addValues:(const double [])values count:(NSUInteger)count {
  if (values == NULL || count == 0) return;
  NSUInteger initialCount = _count;
  NSUInteger newCount = initialCount + count;
  if (newCount > _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
  _count = newCount;
  memcpy(&_values[initialCount], values, count * sizeof(double));
  if (_autocreator) {
    GPBAutocreatedArrayModified(_autocreator, self);
  }
}

- (void)insertValue:(double)value atIndex:(NSUInteger)index {
  if (index >= _count + 1) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count + 1];
  }
  NSUInteger initialCount = _count;
  NSUInteger newCount = initialCount + 1;
  if (newCount > _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
  _count = newCount;
  if (index != initialCount) {
    memmove(&_values[index + 1], &_values[index], (initialCount - index) * sizeof(double));
  }
  _values[index] = value;
  if (_autocreator) {
    GPBAutocreatedArrayModified(_autocreator, self);
  }
}

- (void)replaceValueAtIndex:(NSUInteger)index withValue:(double)value {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  _values[index] = value;
}

- (void)addValuesFromArray:(GPBDoubleArray *)array {
  [self addValues:array->_values count:array->_count];
}

- (void)removeValueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  NSUInteger newCount = _count - 1;
  if (index != newCount) {
    memmove(&_values[index], &_values[index + 1], (newCount - index) * sizeof(double));
  }
  _count = newCount;
  if ((newCount + (2 * kChunkSize)) < _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
}

- (void)removeAll {
  _count = 0;
  if ((0 + (2 * kChunkSize)) < _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(0)];
  }
}

- (void)exchangeValueAtIndex:(NSUInteger)idx1
            withValueAtIndex:(NSUInteger)idx2 {
  if (idx1 >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)idx1, (unsigned long)_count];
  }
  if (idx2 >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)idx2, (unsigned long)_count];
  }
  double temp = _values[idx1];
  _values[idx1] = _values[idx2];
  _values[idx2] = temp;
}

@end

//%PDDM-EXPAND ARRAY_INTERFACE_SIMPLE(Bool, BOOL, %d)
// This block of code is generated, do not edit it directly.

#pragma mark - Bool

@implementation GPBBoolArray {
 @package
  BOOL *_values;
  NSUInteger _count;
  NSUInteger _capacity;
}

@synthesize count = _count;

+ (instancetype)array {
  return [[[self alloc] init] autorelease];
}

+ (instancetype)arrayWithValue:(BOOL)value {
  // Cast is needed so the compiler knows what class we are invoking initWithValues: on to get
  // the type correct.
  return [[(GPBBoolArray*)[self alloc] initWithValues:&value count:1] autorelease];
}

+ (instancetype)arrayWithValueArray:(GPBBoolArray *)array {
  return [[(GPBBoolArray*)[self alloc] initWithValueArray:array] autorelease];
}

+ (instancetype)arrayWithCapacity:(NSUInteger)count {
  return [[[self alloc] initWithCapacity:count] autorelease];
}

- (instancetype)init {
  self = [super init];
  // No work needed;
  return self;
}

- (instancetype)initWithValueArray:(GPBBoolArray *)array {
  return [self initWithValues:array->_values count:array->_count];
}

- (instancetype)initWithValues:(const BOOL [])values count:(NSUInteger)count {
  self = [self init];
  if (self) {
    if (count && values) {
      _values = reallocf(_values, count * sizeof(BOOL));
      if (_values != NULL) {
        _capacity = count;
        memcpy(_values, values, count * sizeof(BOOL));
        _count = count;
      } else {
        [self release];
        [NSException raise:NSMallocException
                    format:@"Failed to allocate %lu bytes",
                           (unsigned long)(count * sizeof(BOOL))];
      }
    }
  }
  return self;
}

- (instancetype)initWithCapacity:(NSUInteger)count {
  self = [self initWithValues:NULL count:0];
  if (self && count) {
    [self internalResizeToCapacity:count];
  }
  return self;
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBBoolArray allocWithZone:zone] initWithValues:_values count:_count];
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  free(_values);
  [super dealloc];
}

- (BOOL)isEqual:(GPBBoolArray *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBBoolArray class]]) {
    return NO;
  }
  return (_count == other->_count
          && memcmp(_values, other->_values, (_count * sizeof(BOOL))) == 0);
}

- (NSUInteger)hash {
  // Follow NSArray's lead, and use the count as the hash.
  return _count;
}

- (NSString *)description {
  NSMutableString *result = [NSMutableString stringWithFormat:@"<%@ %p> { ", [self class], self];
  for (NSUInteger i = 0, count = _count; i < count; ++i) {
    if (i == 0) {
      [result appendFormat:@"%d", _values[i]];
    } else {
      [result appendFormat:@", %d", _values[i]];
    }
  }
  [result appendFormat:@" }"];
  return result;
}

- (void)enumerateValuesWithBlock:(void (^)(BOOL value, NSUInteger idx, BOOL *stop))block {
  [self enumerateValuesWithOptions:0 usingBlock:block];
}

- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void (^)(BOOL value, NSUInteger idx, BOOL *stop))block {
  // NSEnumerationConcurrent isn't currently supported (and Apple's docs say that is ok).
  BOOL stop = NO;
  if ((opts & NSEnumerationReverse) == 0) {
    for (NSUInteger i = 0, count = _count; i < count; ++i) {
      block(_values[i], i, &stop);
      if (stop) break;
    }
  } else if (_count > 0) {
    for (NSUInteger i = _count; i > 0; --i) {
      block(_values[i - 1], (i - 1), &stop);
      if (stop) break;
    }
  }
}

- (BOOL)valueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  return _values[index];
}

- (void)internalResizeToCapacity:(NSUInteger)newCapacity {
  _values = reallocf(_values, newCapacity * sizeof(BOOL));
  if (_values == NULL) {
    _capacity = 0;
    _count = 0;
    [NSException raise:NSMallocException
                format:@"Failed to allocate %lu bytes",
                       (unsigned long)(newCapacity * sizeof(BOOL))];
  }
  _capacity = newCapacity;
}

- (void)addValue:(BOOL)value {
  [self addValues:&value count:1];
}

- (void)addValues:(const BOOL [])values count:(NSUInteger)count {
  if (values == NULL || count == 0) return;
  NSUInteger initialCount = _count;
  NSUInteger newCount = initialCount + count;
  if (newCount > _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
  _count = newCount;
  memcpy(&_values[initialCount], values, count * sizeof(BOOL));
  if (_autocreator) {
    GPBAutocreatedArrayModified(_autocreator, self);
  }
}

- (void)insertValue:(BOOL)value atIndex:(NSUInteger)index {
  if (index >= _count + 1) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count + 1];
  }
  NSUInteger initialCount = _count;
  NSUInteger newCount = initialCount + 1;
  if (newCount > _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
  _count = newCount;
  if (index != initialCount) {
    memmove(&_values[index + 1], &_values[index], (initialCount - index) * sizeof(BOOL));
  }
  _values[index] = value;
  if (_autocreator) {
    GPBAutocreatedArrayModified(_autocreator, self);
  }
}

- (void)replaceValueAtIndex:(NSUInteger)index withValue:(BOOL)value {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  _values[index] = value;
}

- (void)addValuesFromArray:(GPBBoolArray *)array {
  [self addValues:array->_values count:array->_count];
}

- (void)removeValueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  NSUInteger newCount = _count - 1;
  if (index != newCount) {
    memmove(&_values[index], &_values[index + 1], (newCount - index) * sizeof(BOOL));
  }
  _count = newCount;
  if ((newCount + (2 * kChunkSize)) < _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
}

- (void)removeAll {
  _count = 0;
  if ((0 + (2 * kChunkSize)) < _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(0)];
  }
}

- (void)exchangeValueAtIndex:(NSUInteger)idx1
            withValueAtIndex:(NSUInteger)idx2 {
  if (idx1 >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)idx1, (unsigned long)_count];
  }
  if (idx2 >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)idx2, (unsigned long)_count];
  }
  BOOL temp = _values[idx1];
  _values[idx1] = _values[idx2];
  _values[idx2] = temp;
}

@end

//%PDDM-EXPAND-END (7 expansions)

#pragma mark - Enum

@implementation GPBEnumArray {
 @package
  GPBEnumValidationFunc _validationFunc;
  int32_t *_values;
  NSUInteger _count;
  NSUInteger _capacity;
}

@synthesize count = _count;
@synthesize validationFunc = _validationFunc;

+ (instancetype)array {
  return [[[self alloc] initWithValidationFunction:NULL] autorelease];
}

+ (instancetype)arrayWithValidationFunction:(GPBEnumValidationFunc)func {
  return [[[self alloc] initWithValidationFunction:func] autorelease];
}

+ (instancetype)arrayWithValidationFunction:(GPBEnumValidationFunc)func
                                   rawValue:(int32_t)value {
  return [[[self alloc] initWithValidationFunction:func
                                         rawValues:&value
                                             count:1] autorelease];
}

+ (instancetype)arrayWithValueArray:(GPBEnumArray *)array {
  return [[(GPBEnumArray*)[self alloc] initWithValueArray:array] autorelease];
}

+ (instancetype)arrayWithValidationFunction:(GPBEnumValidationFunc)func
                                   capacity:(NSUInteger)count {
  return [[[self alloc] initWithValidationFunction:func capacity:count] autorelease];
}

- (instancetype)init {
  return [self initWithValidationFunction:NULL];
}

- (instancetype)initWithValueArray:(GPBEnumArray *)array {
  return [self initWithValidationFunction:array->_validationFunc
                                rawValues:array->_values
                                    count:array->_count];
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func {
  self = [super init];
  if (self) {
    _validationFunc = (func != NULL ? func : ArrayDefault_IsValidValue);
  }
  return self;
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func
                                 rawValues:(const int32_t [])values
                                     count:(NSUInteger)count {
  self = [self initWithValidationFunction:func];
  if (self) {
    if (count && values) {
      _values = reallocf(_values, count * sizeof(int32_t));
      if (_values != NULL) {
        _capacity = count;
        memcpy(_values, values, count * sizeof(int32_t));
        _count = count;
      } else {
        [self release];
        [NSException raise:NSMallocException
                    format:@"Failed to allocate %lu bytes",
                           (unsigned long)(count * sizeof(int32_t))];
      }
    }
  }
  return self;
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func
                                  capacity:(NSUInteger)count {
  self = [self initWithValidationFunction:func];
  if (self && count) {
    [self internalResizeToCapacity:count];
  }
  return self;
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBEnumArray allocWithZone:zone]
             initWithValidationFunction:_validationFunc
                              rawValues:_values
                                  count:_count];
}

//%PDDM-EXPAND ARRAY_IMMUTABLE_CORE(Enum, int32_t, Raw, %d)
// This block of code is generated, do not edit it directly.

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  free(_values);
  [super dealloc];
}

- (BOOL)isEqual:(GPBEnumArray *)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBEnumArray class]]) {
    return NO;
  }
  return (_count == other->_count
          && memcmp(_values, other->_values, (_count * sizeof(int32_t))) == 0);
}

- (NSUInteger)hash {
  // Follow NSArray's lead, and use the count as the hash.
  return _count;
}

- (NSString *)description {
  NSMutableString *result = [NSMutableString stringWithFormat:@"<%@ %p> { ", [self class], self];
  for (NSUInteger i = 0, count = _count; i < count; ++i) {
    if (i == 0) {
      [result appendFormat:@"%d", _values[i]];
    } else {
      [result appendFormat:@", %d", _values[i]];
    }
  }
  [result appendFormat:@" }"];
  return result;
}

- (void)enumerateRawValuesWithBlock:(void (^)(int32_t value, NSUInteger idx, BOOL *stop))block {
  [self enumerateRawValuesWithOptions:0 usingBlock:block];
}

- (void)enumerateRawValuesWithOptions:(NSEnumerationOptions)opts
                           usingBlock:(void (^)(int32_t value, NSUInteger idx, BOOL *stop))block {
  // NSEnumerationConcurrent isn't currently supported (and Apple's docs say that is ok).
  BOOL stop = NO;
  if ((opts & NSEnumerationReverse) == 0) {
    for (NSUInteger i = 0, count = _count; i < count; ++i) {
      block(_values[i], i, &stop);
      if (stop) break;
    }
  } else if (_count > 0) {
    for (NSUInteger i = _count; i > 0; --i) {
      block(_values[i - 1], (i - 1), &stop);
      if (stop) break;
    }
  }
}
//%PDDM-EXPAND-END ARRAY_IMMUTABLE_CORE(Enum, int32_t, Raw, %d)

- (int32_t)valueAtIndex:(NSUInteger)index {
//%PDDM-EXPAND VALIDATE_RANGE(index, _count)
// This block of code is generated, do not edit it directly.

  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
//%PDDM-EXPAND-END VALIDATE_RANGE(index, _count)
  int32_t result = _values[index];
  if (!_validationFunc(result)) {
    result = kGPBUnrecognizedEnumeratorValue;
  }
  return result;
}

- (int32_t)rawValueAtIndex:(NSUInteger)index {
//%PDDM-EXPAND VALIDATE_RANGE(index, _count)
// This block of code is generated, do not edit it directly.

  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
//%PDDM-EXPAND-END VALIDATE_RANGE(index, _count)
  return _values[index];
}

- (void)enumerateValuesWithBlock:(void (^)(int32_t value, NSUInteger idx, BOOL *stop))block {
  [self enumerateValuesWithOptions:0 usingBlock:block];
}

- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void (^)(int32_t value, NSUInteger idx, BOOL *stop))block {
  // NSEnumerationConcurrent isn't currently supported (and Apple's docs say that is ok).
  BOOL stop = NO;
  GPBEnumValidationFunc func = _validationFunc;
  if ((opts & NSEnumerationReverse) == 0) {
    int32_t *scan = _values;
    int32_t *end = scan + _count;
    for (NSUInteger i = 0; scan < end; ++i, ++scan) {
      int32_t value = *scan;
      if (!func(value)) {
        value = kGPBUnrecognizedEnumeratorValue;
      }
      block(value, i, &stop);
      if (stop) break;
    }
  } else if (_count > 0) {
    int32_t *end = _values;
    int32_t *scan = end + (_count - 1);
    for (NSUInteger i = (_count - 1); scan >= end; --i, --scan) {
      int32_t value = *scan;
      if (!func(value)) {
        value = kGPBUnrecognizedEnumeratorValue;
      }
      block(value, i, &stop);
      if (stop) break;
    }
  }
}

//%PDDM-EXPAND ARRAY_MUTABLE_CORE(Enum, int32_t, Raw, %d)
// This block of code is generated, do not edit it directly.

- (void)internalResizeToCapacity:(NSUInteger)newCapacity {
  _values = reallocf(_values, newCapacity * sizeof(int32_t));
  if (_values == NULL) {
    _capacity = 0;
    _count = 0;
    [NSException raise:NSMallocException
                format:@"Failed to allocate %lu bytes",
                       (unsigned long)(newCapacity * sizeof(int32_t))];
  }
  _capacity = newCapacity;
}

- (void)addRawValue:(int32_t)value {
  [self addRawValues:&value count:1];
}

- (void)addRawValues:(const int32_t [])values count:(NSUInteger)count {
  if (values == NULL || count == 0) return;
  NSUInteger initialCount = _count;
  NSUInteger newCount = initialCount + count;
  if (newCount > _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
  _count = newCount;
  memcpy(&_values[initialCount], values, count * sizeof(int32_t));
  if (_autocreator) {
    GPBAutocreatedArrayModified(_autocreator, self);
  }
}

- (void)insertRawValue:(int32_t)value atIndex:(NSUInteger)index {
  if (index >= _count + 1) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count + 1];
  }
  NSUInteger initialCount = _count;
  NSUInteger newCount = initialCount + 1;
  if (newCount > _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
  _count = newCount;
  if (index != initialCount) {
    memmove(&_values[index + 1], &_values[index], (initialCount - index) * sizeof(int32_t));
  }
  _values[index] = value;
  if (_autocreator) {
    GPBAutocreatedArrayModified(_autocreator, self);
  }
}

- (void)replaceValueAtIndex:(NSUInteger)index withRawValue:(int32_t)value {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  _values[index] = value;
}

- (void)addRawValuesFromArray:(GPBEnumArray *)array {
  [self addRawValues:array->_values count:array->_count];
}

- (void)removeValueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  NSUInteger newCount = _count - 1;
  if (index != newCount) {
    memmove(&_values[index], &_values[index + 1], (newCount - index) * sizeof(int32_t));
  }
  _count = newCount;
  if ((newCount + (2 * kChunkSize)) < _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
}

- (void)removeAll {
  _count = 0;
  if ((0 + (2 * kChunkSize)) < _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(0)];
  }
}

- (void)exchangeValueAtIndex:(NSUInteger)idx1
            withValueAtIndex:(NSUInteger)idx2 {
  if (idx1 >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)idx1, (unsigned long)_count];
  }
  if (idx2 >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)idx2, (unsigned long)_count];
  }
  int32_t temp = _values[idx1];
  _values[idx1] = _values[idx2];
  _values[idx2] = temp;
}

//%PDDM-EXPAND MUTATION_METHODS(Enum, int32_t, , EnumValidationList, EnumValidationOne)
// This block of code is generated, do not edit it directly.

- (void)addValue:(int32_t)value {
  [self addValues:&value count:1];
}

- (void)addValues:(const int32_t [])values count:(NSUInteger)count {
  if (values == NULL || count == 0) return;
  GPBEnumValidationFunc func = _validationFunc;
  for (NSUInteger i = 0; i < count; ++i) {
    if (!func(values[i])) {
      [NSException raise:NSInvalidArgumentException
                  format:@"%@: Attempt to set an unknown enum value (%d)",
                         [self class], values[i]];
    }
  }
  NSUInteger initialCount = _count;
  NSUInteger newCount = initialCount + count;
  if (newCount > _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
  _count = newCount;
  memcpy(&_values[initialCount], values, count * sizeof(int32_t));
  if (_autocreator) {
    GPBAutocreatedArrayModified(_autocreator, self);
  }
}

- (void)insertValue:(int32_t)value atIndex:(NSUInteger)index {
  if (index >= _count + 1) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count + 1];
  }
  if (!_validationFunc(value)) {
    [NSException raise:NSInvalidArgumentException
                format:@"%@: Attempt to set an unknown enum value (%d)",
                       [self class], value];
  }
  NSUInteger initialCount = _count;
  NSUInteger newCount = initialCount + 1;
  if (newCount > _capacity) {
    [self internalResizeToCapacity:CapacityFromCount(newCount)];
  }
  _count = newCount;
  if (index != initialCount) {
    memmove(&_values[index + 1], &_values[index], (initialCount - index) * sizeof(int32_t));
  }
  _values[index] = value;
  if (_autocreator) {
    GPBAutocreatedArrayModified(_autocreator, self);
  }
}

- (void)replaceValueAtIndex:(NSUInteger)index withValue:(int32_t)value {
  if (index >= _count) {
    [NSException raise:NSRangeException
                format:@"Index (%lu) beyond bounds (%lu)",
                       (unsigned long)index, (unsigned long)_count];
  }
  if (!_validationFunc(value)) {
    [NSException raise:NSInvalidArgumentException
                format:@"%@: Attempt to set an unknown enum value (%d)",
                       [self class], value];
  }
  _values[index] = value;
}
//%PDDM-EXPAND-END (2 expansions)

//%PDDM-DEFINE MUTATION_HOOK_EnumValidationList()
//%  GPBEnumValidationFunc func = _validationFunc;
//%  for (NSUInteger i = 0; i < count; ++i) {
//%    if (!func(values[i])) {
//%      [NSException raise:NSInvalidArgumentException
//%                  format:@"%@: Attempt to set an unknown enum value (%d)",
//%                         [self class], values[i]];
//%    }
//%  }
//%
//%PDDM-DEFINE MUTATION_HOOK_EnumValidationOne()
//%  if (!_validationFunc(value)) {
//%    [NSException raise:NSInvalidArgumentException
//%                format:@"%@: Attempt to set an unknown enum value (%d)",
//%                       [self class], value];
//%  }
//%

@end

#pragma mark - NSArray Subclass

@implementation GPBAutocreatedArray {
  NSMutableArray *_array;
}

- (void)dealloc {
  NSAssert(!_autocreator,
           @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  [_array release];
  [super dealloc];
}

#pragma mark Required NSArray overrides

- (NSUInteger)count {
  return [_array count];
}

- (id)objectAtIndex:(NSUInteger)idx {
  return [_array objectAtIndex:idx];
}

#pragma mark Required NSMutableArray overrides

// Only need to call GPBAutocreatedArrayModified() when adding things since
// we only autocreate empty arrays.

- (void)insertObject:(id)anObject atIndex:(NSUInteger)idx {
  if (_array == nil) {
    _array = [[NSMutableArray alloc] init];
  }
  [_array insertObject:anObject atIndex:idx];

  if (_autocreator) {
    GPBAutocreatedArrayModified(_autocreator, self);
  }
}

- (void)removeObject:(id)anObject {
  [_array removeObject:anObject];
}

- (void)removeObjectAtIndex:(NSUInteger)idx {
  [_array removeObjectAtIndex:idx];
}

- (void)addObject:(id)anObject {
  if (_array == nil) {
    _array = [[NSMutableArray alloc] init];
  }
  [_array addObject:anObject];

  if (_autocreator) {
    GPBAutocreatedArrayModified(_autocreator, self);
  }
}

- (void)removeLastObject {
  [_array removeLastObject];
}

- (void)replaceObjectAtIndex:(NSUInteger)idx withObject:(id)anObject {
  [_array replaceObjectAtIndex:idx withObject:anObject];
}

#pragma mark Extra things hooked

- (id)copyWithZone:(NSZone *)zone {
  if (_array == nil) {
    _array = [[NSMutableArray alloc] init];
  }
  return [_array copyWithZone:zone];
}

- (id)mutableCopyWithZone:(NSZone *)zone {
  if (_array == nil) {
    _array = [[NSMutableArray alloc] init];
  }
  return [_array mutableCopyWithZone:zone];
}

- (NSUInteger)countByEnumeratingWithState:(NSFastEnumerationState *)state
                                  objects:(id __unsafe_unretained [])buffer
                                    count:(NSUInteger)len {
  return [_array countByEnumeratingWithState:state objects:buffer count:len];
}

- (void)enumerateObjectsUsingBlock:(void (^)(id obj, NSUInteger idx, BOOL *stop))block {
  [_array enumerateObjectsUsingBlock:block];
}

- (void)enumerateObjectsWithOptions:(NSEnumerationOptions)opts
                         usingBlock:(void (^)(id obj, NSUInteger idx, BOOL *stop))block {
  [_array enumerateObjectsWithOptions:opts usingBlock:block];
}

@end
