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

#import <Foundation/Foundation.h>

#import "GPBTypes.h"

// These classes are used for repeated fields of basic data types. They are used because
// they perform better than boxing into NSNumbers in NSArrays.

// Note: These are not meant to be subclassed.

//%PDDM-EXPAND DECLARE_ARRAYS()
// This block of code is generated, do not edit it directly.

#pragma mark - Int32

@interface GPBInt32Array : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)array;
+ (instancetype)arrayWithValue:(int32_t)value;
+ (instancetype)arrayWithValueArray:(GPBInt32Array *)array;
+ (instancetype)arrayWithCapacity:(NSUInteger)count;

// Initializes the array, copying the values.
- (instancetype)initWithValues:(const int32_t [])values
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithValueArray:(GPBInt32Array *)array;
- (instancetype)initWithCapacity:(NSUInteger)count;

- (int32_t)valueAtIndex:(NSUInteger)index;

- (void)enumerateValuesWithBlock:(void (^)(int32_t value, NSUInteger idx, BOOL *stop))block;
- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void (^)(int32_t value, NSUInteger idx, BOOL *stop))block;

- (void)addValue:(int32_t)value;
- (void)addValues:(const int32_t [])values count:(NSUInteger)count;
- (void)addValuesFromArray:(GPBInt32Array *)array;

- (void)insertValue:(int32_t)value atIndex:(NSUInteger)index;

- (void)replaceValueAtIndex:(NSUInteger)index withValue:(int32_t)value;

- (void)removeValueAtIndex:(NSUInteger)index;
- (void)removeAll;

- (void)exchangeValueAtIndex:(NSUInteger)idx1
            withValueAtIndex:(NSUInteger)idx2;

@end

#pragma mark - UInt32

@interface GPBUInt32Array : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)array;
+ (instancetype)arrayWithValue:(uint32_t)value;
+ (instancetype)arrayWithValueArray:(GPBUInt32Array *)array;
+ (instancetype)arrayWithCapacity:(NSUInteger)count;

// Initializes the array, copying the values.
- (instancetype)initWithValues:(const uint32_t [])values
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithValueArray:(GPBUInt32Array *)array;
- (instancetype)initWithCapacity:(NSUInteger)count;

- (uint32_t)valueAtIndex:(NSUInteger)index;

- (void)enumerateValuesWithBlock:(void (^)(uint32_t value, NSUInteger idx, BOOL *stop))block;
- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void (^)(uint32_t value, NSUInteger idx, BOOL *stop))block;

- (void)addValue:(uint32_t)value;
- (void)addValues:(const uint32_t [])values count:(NSUInteger)count;
- (void)addValuesFromArray:(GPBUInt32Array *)array;

- (void)insertValue:(uint32_t)value atIndex:(NSUInteger)index;

- (void)replaceValueAtIndex:(NSUInteger)index withValue:(uint32_t)value;

- (void)removeValueAtIndex:(NSUInteger)index;
- (void)removeAll;

- (void)exchangeValueAtIndex:(NSUInteger)idx1
            withValueAtIndex:(NSUInteger)idx2;

@end

#pragma mark - Int64

@interface GPBInt64Array : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)array;
+ (instancetype)arrayWithValue:(int64_t)value;
+ (instancetype)arrayWithValueArray:(GPBInt64Array *)array;
+ (instancetype)arrayWithCapacity:(NSUInteger)count;

// Initializes the array, copying the values.
- (instancetype)initWithValues:(const int64_t [])values
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithValueArray:(GPBInt64Array *)array;
- (instancetype)initWithCapacity:(NSUInteger)count;

- (int64_t)valueAtIndex:(NSUInteger)index;

- (void)enumerateValuesWithBlock:(void (^)(int64_t value, NSUInteger idx, BOOL *stop))block;
- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void (^)(int64_t value, NSUInteger idx, BOOL *stop))block;

- (void)addValue:(int64_t)value;
- (void)addValues:(const int64_t [])values count:(NSUInteger)count;
- (void)addValuesFromArray:(GPBInt64Array *)array;

- (void)insertValue:(int64_t)value atIndex:(NSUInteger)index;

- (void)replaceValueAtIndex:(NSUInteger)index withValue:(int64_t)value;

- (void)removeValueAtIndex:(NSUInteger)index;
- (void)removeAll;

- (void)exchangeValueAtIndex:(NSUInteger)idx1
            withValueAtIndex:(NSUInteger)idx2;

@end

#pragma mark - UInt64

@interface GPBUInt64Array : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)array;
+ (instancetype)arrayWithValue:(uint64_t)value;
+ (instancetype)arrayWithValueArray:(GPBUInt64Array *)array;
+ (instancetype)arrayWithCapacity:(NSUInteger)count;

// Initializes the array, copying the values.
- (instancetype)initWithValues:(const uint64_t [])values
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithValueArray:(GPBUInt64Array *)array;
- (instancetype)initWithCapacity:(NSUInteger)count;

- (uint64_t)valueAtIndex:(NSUInteger)index;

- (void)enumerateValuesWithBlock:(void (^)(uint64_t value, NSUInteger idx, BOOL *stop))block;
- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void (^)(uint64_t value, NSUInteger idx, BOOL *stop))block;

- (void)addValue:(uint64_t)value;
- (void)addValues:(const uint64_t [])values count:(NSUInteger)count;
- (void)addValuesFromArray:(GPBUInt64Array *)array;

- (void)insertValue:(uint64_t)value atIndex:(NSUInteger)index;

- (void)replaceValueAtIndex:(NSUInteger)index withValue:(uint64_t)value;

- (void)removeValueAtIndex:(NSUInteger)index;
- (void)removeAll;

- (void)exchangeValueAtIndex:(NSUInteger)idx1
            withValueAtIndex:(NSUInteger)idx2;

@end

#pragma mark - Float

@interface GPBFloatArray : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)array;
+ (instancetype)arrayWithValue:(float)value;
+ (instancetype)arrayWithValueArray:(GPBFloatArray *)array;
+ (instancetype)arrayWithCapacity:(NSUInteger)count;

// Initializes the array, copying the values.
- (instancetype)initWithValues:(const float [])values
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithValueArray:(GPBFloatArray *)array;
- (instancetype)initWithCapacity:(NSUInteger)count;

- (float)valueAtIndex:(NSUInteger)index;

- (void)enumerateValuesWithBlock:(void (^)(float value, NSUInteger idx, BOOL *stop))block;
- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void (^)(float value, NSUInteger idx, BOOL *stop))block;

- (void)addValue:(float)value;
- (void)addValues:(const float [])values count:(NSUInteger)count;
- (void)addValuesFromArray:(GPBFloatArray *)array;

- (void)insertValue:(float)value atIndex:(NSUInteger)index;

- (void)replaceValueAtIndex:(NSUInteger)index withValue:(float)value;

- (void)removeValueAtIndex:(NSUInteger)index;
- (void)removeAll;

- (void)exchangeValueAtIndex:(NSUInteger)idx1
            withValueAtIndex:(NSUInteger)idx2;

@end

#pragma mark - Double

@interface GPBDoubleArray : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)array;
+ (instancetype)arrayWithValue:(double)value;
+ (instancetype)arrayWithValueArray:(GPBDoubleArray *)array;
+ (instancetype)arrayWithCapacity:(NSUInteger)count;

// Initializes the array, copying the values.
- (instancetype)initWithValues:(const double [])values
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithValueArray:(GPBDoubleArray *)array;
- (instancetype)initWithCapacity:(NSUInteger)count;

- (double)valueAtIndex:(NSUInteger)index;

- (void)enumerateValuesWithBlock:(void (^)(double value, NSUInteger idx, BOOL *stop))block;
- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void (^)(double value, NSUInteger idx, BOOL *stop))block;

- (void)addValue:(double)value;
- (void)addValues:(const double [])values count:(NSUInteger)count;
- (void)addValuesFromArray:(GPBDoubleArray *)array;

- (void)insertValue:(double)value atIndex:(NSUInteger)index;

- (void)replaceValueAtIndex:(NSUInteger)index withValue:(double)value;

- (void)removeValueAtIndex:(NSUInteger)index;
- (void)removeAll;

- (void)exchangeValueAtIndex:(NSUInteger)idx1
            withValueAtIndex:(NSUInteger)idx2;

@end

#pragma mark - Bool

@interface GPBBoolArray : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)array;
+ (instancetype)arrayWithValue:(BOOL)value;
+ (instancetype)arrayWithValueArray:(GPBBoolArray *)array;
+ (instancetype)arrayWithCapacity:(NSUInteger)count;

// Initializes the array, copying the values.
- (instancetype)initWithValues:(const BOOL [])values
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithValueArray:(GPBBoolArray *)array;
- (instancetype)initWithCapacity:(NSUInteger)count;

- (BOOL)valueAtIndex:(NSUInteger)index;

- (void)enumerateValuesWithBlock:(void (^)(BOOL value, NSUInteger idx, BOOL *stop))block;
- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void (^)(BOOL value, NSUInteger idx, BOOL *stop))block;

- (void)addValue:(BOOL)value;
- (void)addValues:(const BOOL [])values count:(NSUInteger)count;
- (void)addValuesFromArray:(GPBBoolArray *)array;

- (void)insertValue:(BOOL)value atIndex:(NSUInteger)index;

- (void)replaceValueAtIndex:(NSUInteger)index withValue:(BOOL)value;

- (void)removeValueAtIndex:(NSUInteger)index;
- (void)removeAll;

- (void)exchangeValueAtIndex:(NSUInteger)idx1
            withValueAtIndex:(NSUInteger)idx2;

@end

#pragma mark - Enum

@interface GPBEnumArray : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;
@property(nonatomic, readonly) GPBEnumValidationFunc validationFunc;

+ (instancetype)array;
+ (instancetype)arrayWithValidationFunction:(GPBEnumValidationFunc)func;
+ (instancetype)arrayWithValidationFunction:(GPBEnumValidationFunc)func
                                   rawValue:(int32_t)value;
+ (instancetype)arrayWithValueArray:(GPBEnumArray *)array;
+ (instancetype)arrayWithValidationFunction:(GPBEnumValidationFunc)func
                                   capacity:(NSUInteger)count;

- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func;

// Initializes the array, copying the values.
- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func
                                 rawValues:(const int32_t [])values
                                     count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithValueArray:(GPBEnumArray *)array;
- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func
                                  capacity:(NSUInteger)count;

// These will return kGPBUnrecognizedEnumeratorValue if the value at index is not a
// valid enumerator as defined by validationFunc. If the actual value is
// desired, use "raw" version of the method.

- (int32_t)valueAtIndex:(NSUInteger)index;

- (void)enumerateValuesWithBlock:(void (^)(int32_t value, NSUInteger idx, BOOL *stop))block;
- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
                        usingBlock:(void (^)(int32_t value, NSUInteger idx, BOOL *stop))block;

// These methods bypass the validationFunc to provide access to values that were not
// known at the time the binary was compiled.

- (int32_t)rawValueAtIndex:(NSUInteger)index;

- (void)enumerateRawValuesWithBlock:(void (^)(int32_t value, NSUInteger idx, BOOL *stop))block;
- (void)enumerateRawValuesWithOptions:(NSEnumerationOptions)opts
                           usingBlock:(void (^)(int32_t value, NSUInteger idx, BOOL *stop))block;

// If value is not a valid enumerator as defined by validationFunc, these
// methods will assert in debug, and will log in release and assign the value
// to the default value. Use the rawValue methods below to assign non enumerator
// values.

- (void)addValue:(int32_t)value;
- (void)addValues:(const int32_t [])values count:(NSUInteger)count;

- (void)insertValue:(int32_t)value atIndex:(NSUInteger)index;

- (void)replaceValueAtIndex:(NSUInteger)index withValue:(int32_t)value;

// These methods bypass the validationFunc to provide setting of values that were not
// known at the time the binary was compiled.

- (void)addRawValue:(int32_t)value;
- (void)addRawValuesFromArray:(GPBEnumArray *)array;
- (void)addRawValues:(const int32_t [])values count:(NSUInteger)count;

- (void)insertRawValue:(int32_t)value atIndex:(NSUInteger)index;

- (void)replaceValueAtIndex:(NSUInteger)index withRawValue:(int32_t)value;

// No validation applies to these methods.

- (void)removeValueAtIndex:(NSUInteger)index;
- (void)removeAll;

- (void)exchangeValueAtIndex:(NSUInteger)idx1
            withValueAtIndex:(NSUInteger)idx2;

@end

//%PDDM-EXPAND-END DECLARE_ARRAYS()

//%PDDM-DEFINE DECLARE_ARRAYS()
//%ARRAY_INTERFACE_SIMPLE(Int32, int32_t)
//%ARRAY_INTERFACE_SIMPLE(UInt32, uint32_t)
//%ARRAY_INTERFACE_SIMPLE(Int64, int64_t)
//%ARRAY_INTERFACE_SIMPLE(UInt64, uint64_t)
//%ARRAY_INTERFACE_SIMPLE(Float, float)
//%ARRAY_INTERFACE_SIMPLE(Double, double)
//%ARRAY_INTERFACE_SIMPLE(Bool, BOOL)
//%ARRAY_INTERFACE_ENUM(Enum, int32_t)

//
// The common case (everything but Enum)
//

//%PDDM-DEFINE ARRAY_INTERFACE_SIMPLE(NAME, TYPE)
//%#pragma mark - NAME
//%
//%@interface GPB##NAME##Array : NSObject <NSCopying>
//%
//%@property(nonatomic, readonly) NSUInteger count;
//%
//%+ (instancetype)array;
//%+ (instancetype)arrayWithValue:(TYPE)value;
//%+ (instancetype)arrayWithValueArray:(GPB##NAME##Array *)array;
//%+ (instancetype)arrayWithCapacity:(NSUInteger)count;
//%
//%// Initializes the array, copying the values.
//%- (instancetype)initWithValues:(const TYPE [])values
//%                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
//%- (instancetype)initWithValueArray:(GPB##NAME##Array *)array;
//%- (instancetype)initWithCapacity:(NSUInteger)count;
//%
//%ARRAY_IMMUTABLE_INTERFACE(NAME, TYPE, Basic)
//%
//%ARRAY_MUTABLE_INTERFACE(NAME, TYPE, Basic)
//%
//%@end
//%

//
// Macros specific to Enums (to tweak their interface).
//

//%PDDM-DEFINE ARRAY_INTERFACE_ENUM(NAME, TYPE)
//%#pragma mark - NAME
//%
//%@interface GPB##NAME##Array : NSObject <NSCopying>
//%
//%@property(nonatomic, readonly) NSUInteger count;
//%@property(nonatomic, readonly) GPBEnumValidationFunc validationFunc;
//%
//%+ (instancetype)array;
//%+ (instancetype)arrayWithValidationFunction:(GPBEnumValidationFunc)func;
//%+ (instancetype)arrayWithValidationFunction:(GPBEnumValidationFunc)func
//%                                   rawValue:(TYPE)value;
//%+ (instancetype)arrayWithValueArray:(GPB##NAME##Array *)array;
//%+ (instancetype)arrayWithValidationFunction:(GPBEnumValidationFunc)func
//%                                   capacity:(NSUInteger)count;
//%
//%- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func;
//%
//%// Initializes the array, copying the values.
//%- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func
//%                                 rawValues:(const TYPE [])values
//%                                     count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
//%- (instancetype)initWithValueArray:(GPB##NAME##Array *)array;
//%- (instancetype)initWithValidationFunction:(GPBEnumValidationFunc)func
//%                                  capacity:(NSUInteger)count;
//%
//%// These will return kGPBUnrecognizedEnumeratorValue if the value at index is not a
//%// valid enumerator as defined by validationFunc. If the actual value is
//%// desired, use "raw" version of the method.
//%
//%ARRAY_IMMUTABLE_INTERFACE(NAME, TYPE, NAME)
//%
//%// These methods bypass the validationFunc to provide access to values that were not
//%// known at the time the binary was compiled.
//%
//%- (TYPE)rawValueAtIndex:(NSUInteger)index;
//%
//%- (void)enumerateRawValuesWithBlock:(void (^)(TYPE value, NSUInteger idx, BOOL *stop))block;
//%- (void)enumerateRawValuesWithOptions:(NSEnumerationOptions)opts
//%                           usingBlock:(void (^)(TYPE value, NSUInteger idx, BOOL *stop))block;
//%
//%// If value is not a valid enumerator as defined by validationFunc, these
//%// methods will assert in debug, and will log in release and assign the value
//%// to the default value. Use the rawValue methods below to assign non enumerator
//%// values.
//%
//%ARRAY_MUTABLE_INTERFACE(NAME, TYPE, NAME)
//%
//%@end
//%

//%PDDM-DEFINE ARRAY_IMMUTABLE_INTERFACE(NAME, TYPE, HELPER_NAME)
//%- (TYPE)valueAtIndex:(NSUInteger)index;
//%
//%- (void)enumerateValuesWithBlock:(void (^)(TYPE value, NSUInteger idx, BOOL *stop))block;
//%- (void)enumerateValuesWithOptions:(NSEnumerationOptions)opts
//%                        usingBlock:(void (^)(TYPE value, NSUInteger idx, BOOL *stop))block;

//%PDDM-DEFINE ARRAY_MUTABLE_INTERFACE(NAME, TYPE, HELPER_NAME)
//%- (void)addValue:(TYPE)value;
//%- (void)addValues:(const TYPE [])values count:(NSUInteger)count;
//%ARRAY_EXTRA_MUTABLE_METHODS1_##HELPER_NAME(NAME, TYPE)
//%- (void)insertValue:(TYPE)value atIndex:(NSUInteger)index;
//%
//%- (void)replaceValueAtIndex:(NSUInteger)index withValue:(TYPE)value;
//%ARRAY_EXTRA_MUTABLE_METHODS2_##HELPER_NAME(NAME, TYPE)
//%- (void)removeValueAtIndex:(NSUInteger)index;
//%- (void)removeAll;
//%
//%- (void)exchangeValueAtIndex:(NSUInteger)idx1
//%            withValueAtIndex:(NSUInteger)idx2;

//
// These are hooks invoked by the above to do insert as needed.
//

//%PDDM-DEFINE ARRAY_EXTRA_MUTABLE_METHODS1_Basic(NAME, TYPE)
//%- (void)addValuesFromArray:(GPB##NAME##Array *)array;
//%
//%PDDM-DEFINE ARRAY_EXTRA_MUTABLE_METHODS2_Basic(NAME, TYPE)
// Empty
//%PDDM-DEFINE ARRAY_EXTRA_MUTABLE_METHODS1_Enum(NAME, TYPE)
// Empty
//%PDDM-DEFINE ARRAY_EXTRA_MUTABLE_METHODS2_Enum(NAME, TYPE)
//%
//%// These methods bypass the validationFunc to provide setting of values that were not
//%// known at the time the binary was compiled.
//%
//%- (void)addRawValue:(TYPE)value;
//%- (void)addRawValuesFromArray:(GPB##NAME##Array *)array;
//%- (void)addRawValues:(const TYPE [])values count:(NSUInteger)count;
//%
//%- (void)insertRawValue:(TYPE)value atIndex:(NSUInteger)index;
//%
//%- (void)replaceValueAtIndex:(NSUInteger)index withRawValue:(TYPE)value;
//%
//%// No validation applies to these methods.
//%
