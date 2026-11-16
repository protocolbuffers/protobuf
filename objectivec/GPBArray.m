// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBArray.h"
#import "GPBArray_PackagePrivate.h"

#import "GPBMessage.h"
#import "GPBMessage_PackagePrivate.h"

// Direct access is use for speed, to avoid even internally declaring things
// read/write, etc. The warning is enabled in the project to ensure code calling
// protos can turn on -Wdirect-ivar-access without issues.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdirect-ivar-access"

// Mutable arrays use an internal buffer that can always hold a multiple of this elements.
#define kChunkSize 16
#define CapacityFromCount(x) (((x / kChunkSize) + 1) * kChunkSize)

static BOOL ArrayDefault_IsValidValue(int32_t value) {
  // Anything but the bad value marker is allowed.
  return (value != kGPBUnrecognizedEnumeratorValue);
}

// @@protoc_objc_exemplar: GPBArray.m:array_implementation(Int32)
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
  return [[(GPBInt32Array *)[self alloc] initWithValues:&value count:1] autorelease];
}

+ (instancetype)arrayWithValueArray:(GPBInt32Array *)array {
  return [[(GPBInt32Array *)[self alloc] initWithValueArray:array] autorelease];
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

- (instancetype)initWithValues:(const int32_t[])values count:(NSUInteger)count {
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
        [NSException
             raise:NSMallocException
            format:@"Failed to allocate %lu bytes", (unsigned long)(count * sizeof(int32_t))];
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
  NSAssert(!_autocreator, @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  free(_values);
  [super dealloc];
}

- (BOOL)isEqual:(id)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt32Array class]]) {
    return NO;
  }
  GPBInt32Array *otherArray = other;
  return (_count == otherArray->_count &&
          memcmp(_values, otherArray->_values, (_count * sizeof(int32_t))) == 0);
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

- (void)enumerateValuesWithBlock:(void(NS_NOESCAPE ^)(int32_t value, NSUInteger idx, BOOL *stop))
                                     block {
  [self enumerateValuesWithOptions:(NSEnumerationOptions)0 usingBlock:block];
}

- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void(NS_NOESCAPE ^)(int32_t value, NSUInteger idx, BOOL *stop))
                                       block {
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
  }
  return _values[index];
}

- (void)internalResizeToCapacity:(NSUInteger)newCapacity {
  _values = reallocf(_values, newCapacity * sizeof(int32_t));
  if (_values == NULL) {
    _capacity = 0;
    _count = 0;
    [NSException
         raise:NSMallocException
        format:@"Failed to allocate %lu bytes", (unsigned long)(newCapacity * sizeof(int32_t))];
  }
  _capacity = newCapacity;
}

- (void)addValue:(int32_t)value {
  [self addValues:&value count:1];
}

- (void)addValues:(const int32_t[])values count:(NSUInteger)count {
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count + 1];
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
  }
  _values[index] = value;
}

- (void)addValuesFromArray:(GPBInt32Array *)array {
  [self addValues:array->_values count:array->_count];
}

- (void)removeValueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
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

- (void)exchangeValueAtIndex:(NSUInteger)idx1 withValueAtIndex:(NSUInteger)idx2 {
  if (idx1 >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)idx1, (unsigned long)_count];
  }
  if (idx2 >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)idx2, (unsigned long)_count];
  }
  int32_t temp = _values[idx1];
  _values[idx1] = _values[idx2];
  _values[idx2] = temp;
}

@end
// @@protoc_objc_exemplar_end

// @@protoc_objc_autogen: GPBArray.m:array_implementation
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
  return [[(GPBUInt32Array *)[self alloc] initWithValues:&value count:1] autorelease];
}

+ (instancetype)arrayWithValueArray:(GPBUInt32Array *)array {
  return [[(GPBUInt32Array *)[self alloc] initWithValueArray:array] autorelease];
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

- (instancetype)initWithValues:(const uint32_t[])values count:(NSUInteger)count {
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
        [NSException
             raise:NSMallocException
            format:@"Failed to allocate %lu bytes", (unsigned long)(count * sizeof(uint32_t))];
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
  NSAssert(!_autocreator, @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  free(_values);
  [super dealloc];
}

- (BOOL)isEqual:(id)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt32Array class]]) {
    return NO;
  }
  GPBUInt32Array *otherArray = other;
  return (_count == otherArray->_count &&
          memcmp(_values, otherArray->_values, (_count * sizeof(uint32_t))) == 0);
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

- (void)enumerateValuesWithBlock:(void(NS_NOESCAPE ^)(uint32_t value, NSUInteger idx, BOOL *stop))
                                     block {
  [self enumerateValuesWithOptions:(NSEnumerationOptions)0 usingBlock:block];
}

- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void(NS_NOESCAPE ^)(uint32_t value, NSUInteger idx, BOOL *stop))
                                       block {
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
  }
  return _values[index];
}

- (void)internalResizeToCapacity:(NSUInteger)newCapacity {
  _values = reallocf(_values, newCapacity * sizeof(uint32_t));
  if (_values == NULL) {
    _capacity = 0;
    _count = 0;
    [NSException
         raise:NSMallocException
        format:@"Failed to allocate %lu bytes", (unsigned long)(newCapacity * sizeof(uint32_t))];
  }
  _capacity = newCapacity;
}

- (void)addValue:(uint32_t)value {
  [self addValues:&value count:1];
}

- (void)addValues:(const uint32_t[])values count:(NSUInteger)count {
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count + 1];
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
  }
  _values[index] = value;
}

- (void)addValuesFromArray:(GPBUInt32Array *)array {
  [self addValues:array->_values count:array->_count];
}

- (void)removeValueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
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

- (void)exchangeValueAtIndex:(NSUInteger)idx1 withValueAtIndex:(NSUInteger)idx2 {
  if (idx1 >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)idx1, (unsigned long)_count];
  }
  if (idx2 >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)idx2, (unsigned long)_count];
  }
  uint32_t temp = _values[idx1];
  _values[idx1] = _values[idx2];
  _values[idx2] = temp;
}

@end

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
  return [[(GPBInt64Array *)[self alloc] initWithValues:&value count:1] autorelease];
}

+ (instancetype)arrayWithValueArray:(GPBInt64Array *)array {
  return [[(GPBInt64Array *)[self alloc] initWithValueArray:array] autorelease];
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

- (instancetype)initWithValues:(const int64_t[])values count:(NSUInteger)count {
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
        [NSException
             raise:NSMallocException
            format:@"Failed to allocate %lu bytes", (unsigned long)(count * sizeof(int64_t))];
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
  NSAssert(!_autocreator, @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  free(_values);
  [super dealloc];
}

- (BOOL)isEqual:(id)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBInt64Array class]]) {
    return NO;
  }
  GPBInt64Array *otherArray = other;
  return (_count == otherArray->_count &&
          memcmp(_values, otherArray->_values, (_count * sizeof(int64_t))) == 0);
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

- (void)enumerateValuesWithBlock:(void(NS_NOESCAPE ^)(int64_t value, NSUInteger idx, BOOL *stop))
                                     block {
  [self enumerateValuesWithOptions:(NSEnumerationOptions)0 usingBlock:block];
}

- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void(NS_NOESCAPE ^)(int64_t value, NSUInteger idx, BOOL *stop))
                                       block {
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
  }
  return _values[index];
}

- (void)internalResizeToCapacity:(NSUInteger)newCapacity {
  _values = reallocf(_values, newCapacity * sizeof(int64_t));
  if (_values == NULL) {
    _capacity = 0;
    _count = 0;
    [NSException
         raise:NSMallocException
        format:@"Failed to allocate %lu bytes", (unsigned long)(newCapacity * sizeof(int64_t))];
  }
  _capacity = newCapacity;
}

- (void)addValue:(int64_t)value {
  [self addValues:&value count:1];
}

- (void)addValues:(const int64_t[])values count:(NSUInteger)count {
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count + 1];
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
  }
  _values[index] = value;
}

- (void)addValuesFromArray:(GPBInt64Array *)array {
  [self addValues:array->_values count:array->_count];
}

- (void)removeValueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
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

- (void)exchangeValueAtIndex:(NSUInteger)idx1 withValueAtIndex:(NSUInteger)idx2 {
  if (idx1 >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)idx1, (unsigned long)_count];
  }
  if (idx2 >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)idx2, (unsigned long)_count];
  }
  int64_t temp = _values[idx1];
  _values[idx1] = _values[idx2];
  _values[idx2] = temp;
}

@end

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
  return [[(GPBUInt64Array *)[self alloc] initWithValues:&value count:1] autorelease];
}

+ (instancetype)arrayWithValueArray:(GPBUInt64Array *)array {
  return [[(GPBUInt64Array *)[self alloc] initWithValueArray:array] autorelease];
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

- (instancetype)initWithValues:(const uint64_t[])values count:(NSUInteger)count {
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
        [NSException
             raise:NSMallocException
            format:@"Failed to allocate %lu bytes", (unsigned long)(count * sizeof(uint64_t))];
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
  NSAssert(!_autocreator, @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  free(_values);
  [super dealloc];
}

- (BOOL)isEqual:(id)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBUInt64Array class]]) {
    return NO;
  }
  GPBUInt64Array *otherArray = other;
  return (_count == otherArray->_count &&
          memcmp(_values, otherArray->_values, (_count * sizeof(uint64_t))) == 0);
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

- (void)enumerateValuesWithBlock:(void(NS_NOESCAPE ^)(uint64_t value, NSUInteger idx, BOOL *stop))
                                     block {
  [self enumerateValuesWithOptions:(NSEnumerationOptions)0 usingBlock:block];
}

- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void(NS_NOESCAPE ^)(uint64_t value, NSUInteger idx, BOOL *stop))
                                       block {
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
  }
  return _values[index];
}

- (void)internalResizeToCapacity:(NSUInteger)newCapacity {
  _values = reallocf(_values, newCapacity * sizeof(uint64_t));
  if (_values == NULL) {
    _capacity = 0;
    _count = 0;
    [NSException
         raise:NSMallocException
        format:@"Failed to allocate %lu bytes", (unsigned long)(newCapacity * sizeof(uint64_t))];
  }
  _capacity = newCapacity;
}

- (void)addValue:(uint64_t)value {
  [self addValues:&value count:1];
}

- (void)addValues:(const uint64_t[])values count:(NSUInteger)count {
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count + 1];
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
  }
  _values[index] = value;
}

- (void)addValuesFromArray:(GPBUInt64Array *)array {
  [self addValues:array->_values count:array->_count];
}

- (void)removeValueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
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

- (void)exchangeValueAtIndex:(NSUInteger)idx1 withValueAtIndex:(NSUInteger)idx2 {
  if (idx1 >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)idx1, (unsigned long)_count];
  }
  if (idx2 >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)idx2, (unsigned long)_count];
  }
  uint64_t temp = _values[idx1];
  _values[idx1] = _values[idx2];
  _values[idx2] = temp;
}

@end

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
  return [[(GPBFloatArray *)[self alloc] initWithValues:&value count:1] autorelease];
}

+ (instancetype)arrayWithValueArray:(GPBFloatArray *)array {
  return [[(GPBFloatArray *)[self alloc] initWithValueArray:array] autorelease];
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

- (instancetype)initWithValues:(const float[])values count:(NSUInteger)count {
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
                    format:@"Failed to allocate %lu bytes", (unsigned long)(count * sizeof(float))];
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
  NSAssert(!_autocreator, @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  free(_values);
  [super dealloc];
}

- (BOOL)isEqual:(id)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBFloatArray class]]) {
    return NO;
  }
  GPBFloatArray *otherArray = other;
  return (_count == otherArray->_count &&
          memcmp(_values, otherArray->_values, (_count * sizeof(float))) == 0);
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

- (void)enumerateValuesWithBlock:(void(NS_NOESCAPE ^)(float value, NSUInteger idx, BOOL *stop))
                                     block {
  [self enumerateValuesWithOptions:(NSEnumerationOptions)0 usingBlock:block];
}

- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void(NS_NOESCAPE ^)(float value, NSUInteger idx, BOOL *stop))
                                       block {
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
  }
  return _values[index];
}

- (void)internalResizeToCapacity:(NSUInteger)newCapacity {
  _values = reallocf(_values, newCapacity * sizeof(float));
  if (_values == NULL) {
    _capacity = 0;
    _count = 0;
    [NSException
         raise:NSMallocException
        format:@"Failed to allocate %lu bytes", (unsigned long)(newCapacity * sizeof(float))];
  }
  _capacity = newCapacity;
}

- (void)addValue:(float)value {
  [self addValues:&value count:1];
}

- (void)addValues:(const float[])values count:(NSUInteger)count {
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count + 1];
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
  }
  _values[index] = value;
}

- (void)addValuesFromArray:(GPBFloatArray *)array {
  [self addValues:array->_values count:array->_count];
}

- (void)removeValueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
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

- (void)exchangeValueAtIndex:(NSUInteger)idx1 withValueAtIndex:(NSUInteger)idx2 {
  if (idx1 >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)idx1, (unsigned long)_count];
  }
  if (idx2 >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)idx2, (unsigned long)_count];
  }
  float temp = _values[idx1];
  _values[idx1] = _values[idx2];
  _values[idx2] = temp;
}

@end

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
  return [[(GPBDoubleArray *)[self alloc] initWithValues:&value count:1] autorelease];
}

+ (instancetype)arrayWithValueArray:(GPBDoubleArray *)array {
  return [[(GPBDoubleArray *)[self alloc] initWithValueArray:array] autorelease];
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

- (instancetype)initWithValues:(const double[])values count:(NSUInteger)count {
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
        [NSException
             raise:NSMallocException
            format:@"Failed to allocate %lu bytes", (unsigned long)(count * sizeof(double))];
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
  NSAssert(!_autocreator, @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  free(_values);
  [super dealloc];
}

- (BOOL)isEqual:(id)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBDoubleArray class]]) {
    return NO;
  }
  GPBDoubleArray *otherArray = other;
  return (_count == otherArray->_count &&
          memcmp(_values, otherArray->_values, (_count * sizeof(double))) == 0);
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

- (void)enumerateValuesWithBlock:(void(NS_NOESCAPE ^)(double value, NSUInteger idx, BOOL *stop))
                                     block {
  [self enumerateValuesWithOptions:(NSEnumerationOptions)0 usingBlock:block];
}

- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void(NS_NOESCAPE ^)(double value, NSUInteger idx, BOOL *stop))
                                       block {
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
  }
  return _values[index];
}

- (void)internalResizeToCapacity:(NSUInteger)newCapacity {
  _values = reallocf(_values, newCapacity * sizeof(double));
  if (_values == NULL) {
    _capacity = 0;
    _count = 0;
    [NSException
         raise:NSMallocException
        format:@"Failed to allocate %lu bytes", (unsigned long)(newCapacity * sizeof(double))];
  }
  _capacity = newCapacity;
}

- (void)addValue:(double)value {
  [self addValues:&value count:1];
}

- (void)addValues:(const double[])values count:(NSUInteger)count {
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count + 1];
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
  }
  _values[index] = value;
}

- (void)addValuesFromArray:(GPBDoubleArray *)array {
  [self addValues:array->_values count:array->_count];
}

- (void)removeValueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
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

- (void)exchangeValueAtIndex:(NSUInteger)idx1 withValueAtIndex:(NSUInteger)idx2 {
  if (idx1 >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)idx1, (unsigned long)_count];
  }
  if (idx2 >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)idx2, (unsigned long)_count];
  }
  double temp = _values[idx1];
  _values[idx1] = _values[idx2];
  _values[idx2] = temp;
}

@end

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
  return [[(GPBBoolArray *)[self alloc] initWithValues:&value count:1] autorelease];
}

+ (instancetype)arrayWithValueArray:(GPBBoolArray *)array {
  return [[(GPBBoolArray *)[self alloc] initWithValueArray:array] autorelease];
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

- (instancetype)initWithValues:(const BOOL[])values count:(NSUInteger)count {
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
                    format:@"Failed to allocate %lu bytes", (unsigned long)(count * sizeof(BOOL))];
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
  NSAssert(!_autocreator, @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  free(_values);
  [super dealloc];
}

- (BOOL)isEqual:(id)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBBoolArray class]]) {
    return NO;
  }
  GPBBoolArray *otherArray = other;
  return (_count == otherArray->_count &&
          memcmp(_values, otherArray->_values, (_count * sizeof(BOOL))) == 0);
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

- (void)enumerateValuesWithBlock:(void(NS_NOESCAPE ^)(BOOL value, NSUInteger idx, BOOL *stop))
                                     block {
  [self enumerateValuesWithOptions:(NSEnumerationOptions)0 usingBlock:block];
}

- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void(NS_NOESCAPE ^)(BOOL value, NSUInteger idx, BOOL *stop))
                                       block {
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
  }
  return _values[index];
}

- (void)internalResizeToCapacity:(NSUInteger)newCapacity {
  _values = reallocf(_values, newCapacity * sizeof(BOOL));
  if (_values == NULL) {
    _capacity = 0;
    _count = 0;
    [NSException
         raise:NSMallocException
        format:@"Failed to allocate %lu bytes", (unsigned long)(newCapacity * sizeof(BOOL))];
  }
  _capacity = newCapacity;
}

- (void)addValue:(BOOL)value {
  [self addValues:&value count:1];
}

- (void)addValues:(const BOOL[])values count:(NSUInteger)count {
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count + 1];
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
  }
  _values[index] = value;
}

- (void)addValuesFromArray:(GPBBoolArray *)array {
  [self addValues:array->_values count:array->_count];
}

- (void)removeValueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
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

- (void)exchangeValueAtIndex:(NSUInteger)idx1 withValueAtIndex:(NSUInteger)idx2 {
  if (idx1 >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)idx1, (unsigned long)_count];
  }
  if (idx2 >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)idx2, (unsigned long)_count];
  }
  BOOL temp = _values[idx1];
  _values[idx1] = _values[idx2];
  _values[idx2] = temp;
}

@end
// @@protoc_objc_autogen_end

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

+ (instancetype)arrayWithValidationFunction:(GPBEnumValidationFunc)func rawValue:(int32_t)value {
  return [[[self alloc] initWithValidationFunction:func rawValues:&value count:1] autorelease];
}

+ (instancetype)arrayWithValueArray:(GPBEnumArray *)array {
  return [[(GPBEnumArray *)[self alloc] initWithValueArray:array] autorelease];
}

+ (instancetype)arrayWithValidationFunction:(GPBEnumValidationFunc)func capacity:(NSUInteger)count {
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
                                 rawValues:(const int32_t[])values
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
        [NSException
             raise:NSMallocException
            format:@"Failed to allocate %lu bytes", (unsigned long)(count * sizeof(int32_t))];
      }
    }
  }
  return self;
}

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func capacity:(NSUInteger)count {
  self = [self initWithValidationFunction:func];
  if (self && count) {
    [self internalResizeToCapacity:count];
  }
  return self;
}

- (instancetype)copyWithZone:(NSZone *)zone {
  return [[GPBEnumArray allocWithZone:zone] initWithValidationFunction:_validationFunc
                                                             rawValues:_values
                                                                 count:_count];
}

- (void)dealloc {
  NSAssert(!_autocreator, @"%@: Autocreator must be cleared before release, autocreator: %@",
           [self class], _autocreator);
  free(_values);
  [super dealloc];
}

- (BOOL)isEqual:(id)other {
  if (self == other) {
    return YES;
  }
  if (![other isKindOfClass:[GPBEnumArray class]]) {
    return NO;
  }
  GPBEnumArray *otherArray = other;
  return (_count == otherArray->_count &&
          memcmp(_values, otherArray->_values, (_count * sizeof(int32_t))) == 0);
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

- (void)enumerateRawValuesWithBlock:(void(NS_NOESCAPE ^)(int32_t value, NSUInteger idx, BOOL *stop))
                                        block {
  [self enumerateRawValuesWithOptions:(NSEnumerationOptions)0 usingBlock:block];
}

- (void)enumerateRawValuesWithOptions:(NSEnumerationOptions)opts
                           usingBlock:(void(NS_NOESCAPE ^)(int32_t value, NSUInteger idx,
                                                           BOOL *stop))block {
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
  }
  int32_t result = _values[index];
  if (!_validationFunc(result)) {
    result = kGPBUnrecognizedEnumeratorValue;
  }
  return result;
}

- (int32_t)rawValueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
  }
  return _values[index];
}

- (void)enumerateValuesWithBlock:(void(NS_NOESCAPE ^)(int32_t value, NSUInteger idx, BOOL *stop))
                                     block {
  [self enumerateValuesWithOptions:(NSEnumerationOptions)0 usingBlock:block];
}

- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void(NS_NOESCAPE ^)(int32_t value, NSUInteger idx, BOOL *stop))
                                       block {
  // NSEnumerationConcurrent isn't currently supported (and Apple's docs say that is ok).
  if (!_values || _count == 0) {
    return;
  }
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

- (void)internalResizeToCapacity:(NSUInteger)newCapacity {
  _values = reallocf(_values, newCapacity * sizeof(int32_t));
  if (_values == NULL) {
    _capacity = 0;
    _count = 0;
    [NSException
         raise:NSMallocException
        format:@"Failed to allocate %lu bytes", (unsigned long)(newCapacity * sizeof(int32_t))];
  }
  _capacity = newCapacity;
}

- (void)addRawValue:(int32_t)value {
  [self addRawValues:&value count:1];
}

- (void)addRawValues:(const int32_t[])values count:(NSUInteger)count {
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count + 1];
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
  }
  _values[index] = value;
}

- (void)addRawValuesFromArray:(GPBEnumArray *)array {
  [self addRawValues:array->_values count:array->_count];
}

- (void)removeValueAtIndex:(NSUInteger)index {
  if (index >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
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

- (void)exchangeValueAtIndex:(NSUInteger)idx1 withValueAtIndex:(NSUInteger)idx2 {
  if (idx1 >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)idx1, (unsigned long)_count];
  }
  if (idx2 >= _count) {
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)idx2, (unsigned long)_count];
  }
  int32_t temp = _values[idx1];
  _values[idx1] = _values[idx2];
  _values[idx2] = temp;
}

- (void)addValue:(int32_t)value {
  [self addValues:&value count:1];
}

- (void)addValues:(const int32_t[])values count:(NSUInteger)count {
  if (values == NULL || count == 0) return;
  GPBEnumValidationFunc func = _validationFunc;
  for (NSUInteger i = 0; i < count; ++i) {
    if (!func(values[i])) {
      [NSException raise:NSInvalidArgumentException
                  format:@"%@: Attempt to set an unknown enum value (%d)", [self class], values[i]];
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count + 1];
  }
  if (!_validationFunc(value)) {
    [NSException raise:NSInvalidArgumentException
                format:@"%@: Attempt to set an unknown enum value (%d)", [self class], value];
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
    [NSException
         raise:NSRangeException
        format:@"Index (%lu) beyond bounds (%lu)", (unsigned long)index, (unsigned long)_count];
  }
  if (!_validationFunc(value)) {
    [NSException raise:NSInvalidArgumentException
                format:@"%@: Attempt to set an unknown enum value (%d)", [self class], value];
  }
  _values[index] = value;
}

@end

#pragma mark - NSArray Subclass

@implementation GPBAutocreatedArray {
  NSMutableArray *_array;
}

- (void)dealloc {
  NSAssert(!_autocreator, @"%@: Autocreator must be cleared before release, autocreator: %@",
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
    return [[NSMutableArray allocWithZone:zone] init];
  }
  return [_array copyWithZone:zone];
}

- (id)mutableCopyWithZone:(NSZone *)zone {
  if (_array == nil) {
    return [[NSMutableArray allocWithZone:zone] init];
  }
  return [_array mutableCopyWithZone:zone];
}

- (NSUInteger)countByEnumeratingWithState:(NSFastEnumerationState *)state
                                  objects:(id __unsafe_unretained[])buffer
                                    count:(NSUInteger)len {
  return [_array countByEnumeratingWithState:state objects:buffer count:len];
}

- (void)enumerateObjectsUsingBlock:(void(NS_NOESCAPE ^)(id obj, NSUInteger idx, BOOL *stop))block {
  [_array enumerateObjectsUsingBlock:block];
}

- (void)enumerateObjectsWithOptions:(NSEnumerationOptions)opts
                         usingBlock:(void(NS_NOESCAPE ^)(id obj, NSUInteger idx, BOOL *stop))block {
  [_array enumerateObjectsWithOptions:opts usingBlock:block];
}

@end

#pragma clang diagnostic pop
