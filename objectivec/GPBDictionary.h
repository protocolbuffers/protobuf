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

#import "GPBRuntimeTypes.h"

// These classes are used for map fields of basic data types. They are used because
// they perform better than boxing into NSNumbers in NSDictionaries.

// Note: These are not meant to be subclassed.

NS_ASSUME_NONNULL_BEGIN

//%PDDM-EXPAND DECLARE_DICTIONARIES()
// This block of code is generated, do not edit it directly.

#pragma mark - UInt32 -> UInt32

@interface GPBUInt32UInt32Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(uint32_t)value
                             forKey:(uint32_t)key;
+ (instancetype)dictionaryWithValues:(const uint32_t [])values
                             forKeys:(const uint32_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt32UInt32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const uint32_t [])values
                       forKeys:(const uint32_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt32UInt32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(uint32_t)key value:(nullable uint32_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint32_t key, uint32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt32UInt32Dictionary *)otherDictionary;

- (void)setValue:(uint32_t)value forKey:(uint32_t)key;

- (void)removeValueForKey:(uint32_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt32 -> Int32

@interface GPBUInt32Int32Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(int32_t)value
                             forKey:(uint32_t)key;
+ (instancetype)dictionaryWithValues:(const int32_t [])values
                             forKeys:(const uint32_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt32Int32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const int32_t [])values
                       forKeys:(const uint32_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt32Int32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(uint32_t)key value:(nullable int32_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint32_t key, int32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt32Int32Dictionary *)otherDictionary;

- (void)setValue:(int32_t)value forKey:(uint32_t)key;

- (void)removeValueForKey:(uint32_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt32 -> UInt64

@interface GPBUInt32UInt64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(uint64_t)value
                             forKey:(uint32_t)key;
+ (instancetype)dictionaryWithValues:(const uint64_t [])values
                             forKeys:(const uint32_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt32UInt64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const uint64_t [])values
                       forKeys:(const uint32_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt32UInt64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(uint32_t)key value:(nullable uint64_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint32_t key, uint64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt32UInt64Dictionary *)otherDictionary;

- (void)setValue:(uint64_t)value forKey:(uint32_t)key;

- (void)removeValueForKey:(uint32_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt32 -> Int64

@interface GPBUInt32Int64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(int64_t)value
                             forKey:(uint32_t)key;
+ (instancetype)dictionaryWithValues:(const int64_t [])values
                             forKeys:(const uint32_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt32Int64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const int64_t [])values
                       forKeys:(const uint32_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt32Int64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(uint32_t)key value:(nullable int64_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint32_t key, int64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt32Int64Dictionary *)otherDictionary;

- (void)setValue:(int64_t)value forKey:(uint32_t)key;

- (void)removeValueForKey:(uint32_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt32 -> Bool

@interface GPBUInt32BoolDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(BOOL)value
                             forKey:(uint32_t)key;
+ (instancetype)dictionaryWithValues:(const BOOL [])values
                             forKeys:(const uint32_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt32BoolDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const BOOL [])values
                       forKeys:(const uint32_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt32BoolDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(uint32_t)key value:(nullable BOOL *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint32_t key, BOOL value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt32BoolDictionary *)otherDictionary;

- (void)setValue:(BOOL)value forKey:(uint32_t)key;

- (void)removeValueForKey:(uint32_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt32 -> Float

@interface GPBUInt32FloatDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(float)value
                             forKey:(uint32_t)key;
+ (instancetype)dictionaryWithValues:(const float [])values
                             forKeys:(const uint32_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt32FloatDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const float [])values
                       forKeys:(const uint32_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt32FloatDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(uint32_t)key value:(nullable float *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint32_t key, float value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt32FloatDictionary *)otherDictionary;

- (void)setValue:(float)value forKey:(uint32_t)key;

- (void)removeValueForKey:(uint32_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt32 -> Double

@interface GPBUInt32DoubleDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(double)value
                             forKey:(uint32_t)key;
+ (instancetype)dictionaryWithValues:(const double [])values
                             forKeys:(const uint32_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt32DoubleDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const double [])values
                       forKeys:(const uint32_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt32DoubleDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(uint32_t)key value:(nullable double *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint32_t key, double value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt32DoubleDictionary *)otherDictionary;

- (void)setValue:(double)value forKey:(uint32_t)key;

- (void)removeValueForKey:(uint32_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt32 -> Enum

@interface GPBUInt32EnumDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;
@property(nonatomic, readonly) GPBEnumValidationFunc validationFunc;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                        rawValue:(int32_t)rawValue
                                          forKey:(uint32_t)key;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                       rawValues:(const int32_t [])values
                                         forKeys:(const uint32_t [])keys
                                           count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt32EnumDictionary *)dictionary;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                        capacity:(NSUInteger)numItems;

- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func;
- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                 rawValues:(const int32_t [])values
                                   forKeys:(const uint32_t [])keys
                                     count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt32EnumDictionary *)dictionary;
- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                  capacity:(NSUInteger)numItems;

// These will return kGPBUnrecognizedEnumeratorValue if the value for the key
// is not a valid enumerator as defined by validationFunc. If the actual value is
// desired, use "raw" version of the method.

- (BOOL)valueForKey:(uint32_t)key value:(nullable int32_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint32_t key, int32_t value, BOOL *stop))block;

// These methods bypass the validationFunc to provide access to values that were not
// known at the time the binary was compiled.

- (BOOL)valueForKey:(uint32_t)key rawValue:(nullable int32_t *)rawValue;

- (void)enumerateKeysAndRawValuesUsingBlock:
    (void (^)(uint32_t key, int32_t rawValue, BOOL *stop))block;

- (void)addRawEntriesFromDictionary:(GPBUInt32EnumDictionary *)otherDictionary;

// If value is not a valid enumerator as defined by validationFunc, these
// methods will assert in debug, and will log in release and assign the value
// to the default value. Use the rawValue methods below to assign non enumerator
// values.

- (void)setValue:(int32_t)value forKey:(uint32_t)key;

// This method bypass the validationFunc to provide setting of values that were not
// known at the time the binary was compiled.
- (void)setRawValue:(int32_t)rawValue forKey:(uint32_t)key;

// No validation applies to these methods.

- (void)removeValueForKey:(uint32_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt32 -> Object

@interface GPBUInt32ObjectDictionary<__covariant ObjectType> : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithObject:(ObjectType)object
                              forKey:(uint32_t)key;
+ (instancetype)dictionaryWithObjects:(const ObjectType GPB_UNSAFE_UNRETAINED [])objects
                              forKeys:(const uint32_t [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt32ObjectDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithObjects:(const ObjectType GPB_UNSAFE_UNRETAINED [])objects
                        forKeys:(const uint32_t [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt32ObjectDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (ObjectType)objectForKey:(uint32_t)key;

- (void)enumerateKeysAndObjectsUsingBlock:
    (void (^)(uint32_t key, ObjectType object, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt32ObjectDictionary *)otherDictionary;

- (void)setObject:(ObjectType)object forKey:(uint32_t)key;

- (void)removeObjectForKey:(uint32_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int32 -> UInt32

@interface GPBInt32UInt32Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(uint32_t)value
                             forKey:(int32_t)key;
+ (instancetype)dictionaryWithValues:(const uint32_t [])values
                             forKeys:(const int32_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt32UInt32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const uint32_t [])values
                       forKeys:(const int32_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt32UInt32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(int32_t)key value:(nullable uint32_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int32_t key, uint32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt32UInt32Dictionary *)otherDictionary;

- (void)setValue:(uint32_t)value forKey:(int32_t)key;

- (void)removeValueForKey:(int32_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int32 -> Int32

@interface GPBInt32Int32Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(int32_t)value
                             forKey:(int32_t)key;
+ (instancetype)dictionaryWithValues:(const int32_t [])values
                             forKeys:(const int32_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt32Int32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const int32_t [])values
                       forKeys:(const int32_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt32Int32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(int32_t)key value:(nullable int32_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int32_t key, int32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt32Int32Dictionary *)otherDictionary;

- (void)setValue:(int32_t)value forKey:(int32_t)key;

- (void)removeValueForKey:(int32_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int32 -> UInt64

@interface GPBInt32UInt64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(uint64_t)value
                             forKey:(int32_t)key;
+ (instancetype)dictionaryWithValues:(const uint64_t [])values
                             forKeys:(const int32_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt32UInt64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const uint64_t [])values
                       forKeys:(const int32_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt32UInt64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(int32_t)key value:(nullable uint64_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int32_t key, uint64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt32UInt64Dictionary *)otherDictionary;

- (void)setValue:(uint64_t)value forKey:(int32_t)key;

- (void)removeValueForKey:(int32_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int32 -> Int64

@interface GPBInt32Int64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(int64_t)value
                             forKey:(int32_t)key;
+ (instancetype)dictionaryWithValues:(const int64_t [])values
                             forKeys:(const int32_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt32Int64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const int64_t [])values
                       forKeys:(const int32_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt32Int64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(int32_t)key value:(nullable int64_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int32_t key, int64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt32Int64Dictionary *)otherDictionary;

- (void)setValue:(int64_t)value forKey:(int32_t)key;

- (void)removeValueForKey:(int32_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int32 -> Bool

@interface GPBInt32BoolDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(BOOL)value
                             forKey:(int32_t)key;
+ (instancetype)dictionaryWithValues:(const BOOL [])values
                             forKeys:(const int32_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt32BoolDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const BOOL [])values
                       forKeys:(const int32_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt32BoolDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(int32_t)key value:(nullable BOOL *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int32_t key, BOOL value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt32BoolDictionary *)otherDictionary;

- (void)setValue:(BOOL)value forKey:(int32_t)key;

- (void)removeValueForKey:(int32_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int32 -> Float

@interface GPBInt32FloatDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(float)value
                             forKey:(int32_t)key;
+ (instancetype)dictionaryWithValues:(const float [])values
                             forKeys:(const int32_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt32FloatDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const float [])values
                       forKeys:(const int32_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt32FloatDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(int32_t)key value:(nullable float *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int32_t key, float value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt32FloatDictionary *)otherDictionary;

- (void)setValue:(float)value forKey:(int32_t)key;

- (void)removeValueForKey:(int32_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int32 -> Double

@interface GPBInt32DoubleDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(double)value
                             forKey:(int32_t)key;
+ (instancetype)dictionaryWithValues:(const double [])values
                             forKeys:(const int32_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt32DoubleDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const double [])values
                       forKeys:(const int32_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt32DoubleDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(int32_t)key value:(nullable double *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int32_t key, double value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt32DoubleDictionary *)otherDictionary;

- (void)setValue:(double)value forKey:(int32_t)key;

- (void)removeValueForKey:(int32_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int32 -> Enum

@interface GPBInt32EnumDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;
@property(nonatomic, readonly) GPBEnumValidationFunc validationFunc;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                        rawValue:(int32_t)rawValue
                                          forKey:(int32_t)key;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                       rawValues:(const int32_t [])values
                                         forKeys:(const int32_t [])keys
                                           count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt32EnumDictionary *)dictionary;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                        capacity:(NSUInteger)numItems;

- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func;
- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                 rawValues:(const int32_t [])values
                                   forKeys:(const int32_t [])keys
                                     count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt32EnumDictionary *)dictionary;
- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                  capacity:(NSUInteger)numItems;

// These will return kGPBUnrecognizedEnumeratorValue if the value for the key
// is not a valid enumerator as defined by validationFunc. If the actual value is
// desired, use "raw" version of the method.

- (BOOL)valueForKey:(int32_t)key value:(nullable int32_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int32_t key, int32_t value, BOOL *stop))block;

// These methods bypass the validationFunc to provide access to values that were not
// known at the time the binary was compiled.

- (BOOL)valueForKey:(int32_t)key rawValue:(nullable int32_t *)rawValue;

- (void)enumerateKeysAndRawValuesUsingBlock:
    (void (^)(int32_t key, int32_t rawValue, BOOL *stop))block;

- (void)addRawEntriesFromDictionary:(GPBInt32EnumDictionary *)otherDictionary;

// If value is not a valid enumerator as defined by validationFunc, these
// methods will assert in debug, and will log in release and assign the value
// to the default value. Use the rawValue methods below to assign non enumerator
// values.

- (void)setValue:(int32_t)value forKey:(int32_t)key;

// This method bypass the validationFunc to provide setting of values that were not
// known at the time the binary was compiled.
- (void)setRawValue:(int32_t)rawValue forKey:(int32_t)key;

// No validation applies to these methods.

- (void)removeValueForKey:(int32_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int32 -> Object

@interface GPBInt32ObjectDictionary<__covariant ObjectType> : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithObject:(ObjectType)object
                              forKey:(int32_t)key;
+ (instancetype)dictionaryWithObjects:(const ObjectType GPB_UNSAFE_UNRETAINED [])objects
                              forKeys:(const int32_t [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt32ObjectDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithObjects:(const ObjectType GPB_UNSAFE_UNRETAINED [])objects
                        forKeys:(const int32_t [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt32ObjectDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (ObjectType)objectForKey:(int32_t)key;

- (void)enumerateKeysAndObjectsUsingBlock:
    (void (^)(int32_t key, ObjectType object, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt32ObjectDictionary *)otherDictionary;

- (void)setObject:(ObjectType)object forKey:(int32_t)key;

- (void)removeObjectForKey:(int32_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt64 -> UInt32

@interface GPBUInt64UInt32Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(uint32_t)value
                             forKey:(uint64_t)key;
+ (instancetype)dictionaryWithValues:(const uint32_t [])values
                             forKeys:(const uint64_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt64UInt32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const uint32_t [])values
                       forKeys:(const uint64_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt64UInt32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(uint64_t)key value:(nullable uint32_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint64_t key, uint32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt64UInt32Dictionary *)otherDictionary;

- (void)setValue:(uint32_t)value forKey:(uint64_t)key;

- (void)removeValueForKey:(uint64_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt64 -> Int32

@interface GPBUInt64Int32Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(int32_t)value
                             forKey:(uint64_t)key;
+ (instancetype)dictionaryWithValues:(const int32_t [])values
                             forKeys:(const uint64_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt64Int32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const int32_t [])values
                       forKeys:(const uint64_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt64Int32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(uint64_t)key value:(nullable int32_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint64_t key, int32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt64Int32Dictionary *)otherDictionary;

- (void)setValue:(int32_t)value forKey:(uint64_t)key;

- (void)removeValueForKey:(uint64_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt64 -> UInt64

@interface GPBUInt64UInt64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(uint64_t)value
                             forKey:(uint64_t)key;
+ (instancetype)dictionaryWithValues:(const uint64_t [])values
                             forKeys:(const uint64_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt64UInt64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const uint64_t [])values
                       forKeys:(const uint64_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt64UInt64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(uint64_t)key value:(nullable uint64_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint64_t key, uint64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt64UInt64Dictionary *)otherDictionary;

- (void)setValue:(uint64_t)value forKey:(uint64_t)key;

- (void)removeValueForKey:(uint64_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt64 -> Int64

@interface GPBUInt64Int64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(int64_t)value
                             forKey:(uint64_t)key;
+ (instancetype)dictionaryWithValues:(const int64_t [])values
                             forKeys:(const uint64_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt64Int64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const int64_t [])values
                       forKeys:(const uint64_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt64Int64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(uint64_t)key value:(nullable int64_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint64_t key, int64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt64Int64Dictionary *)otherDictionary;

- (void)setValue:(int64_t)value forKey:(uint64_t)key;

- (void)removeValueForKey:(uint64_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt64 -> Bool

@interface GPBUInt64BoolDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(BOOL)value
                             forKey:(uint64_t)key;
+ (instancetype)dictionaryWithValues:(const BOOL [])values
                             forKeys:(const uint64_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt64BoolDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const BOOL [])values
                       forKeys:(const uint64_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt64BoolDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(uint64_t)key value:(nullable BOOL *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint64_t key, BOOL value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt64BoolDictionary *)otherDictionary;

- (void)setValue:(BOOL)value forKey:(uint64_t)key;

- (void)removeValueForKey:(uint64_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt64 -> Float

@interface GPBUInt64FloatDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(float)value
                             forKey:(uint64_t)key;
+ (instancetype)dictionaryWithValues:(const float [])values
                             forKeys:(const uint64_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt64FloatDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const float [])values
                       forKeys:(const uint64_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt64FloatDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(uint64_t)key value:(nullable float *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint64_t key, float value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt64FloatDictionary *)otherDictionary;

- (void)setValue:(float)value forKey:(uint64_t)key;

- (void)removeValueForKey:(uint64_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt64 -> Double

@interface GPBUInt64DoubleDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(double)value
                             forKey:(uint64_t)key;
+ (instancetype)dictionaryWithValues:(const double [])values
                             forKeys:(const uint64_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt64DoubleDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const double [])values
                       forKeys:(const uint64_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt64DoubleDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(uint64_t)key value:(nullable double *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint64_t key, double value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt64DoubleDictionary *)otherDictionary;

- (void)setValue:(double)value forKey:(uint64_t)key;

- (void)removeValueForKey:(uint64_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt64 -> Enum

@interface GPBUInt64EnumDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;
@property(nonatomic, readonly) GPBEnumValidationFunc validationFunc;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                        rawValue:(int32_t)rawValue
                                          forKey:(uint64_t)key;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                       rawValues:(const int32_t [])values
                                         forKeys:(const uint64_t [])keys
                                           count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt64EnumDictionary *)dictionary;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                        capacity:(NSUInteger)numItems;

- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func;
- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                 rawValues:(const int32_t [])values
                                   forKeys:(const uint64_t [])keys
                                     count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt64EnumDictionary *)dictionary;
- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                  capacity:(NSUInteger)numItems;

// These will return kGPBUnrecognizedEnumeratorValue if the value for the key
// is not a valid enumerator as defined by validationFunc. If the actual value is
// desired, use "raw" version of the method.

- (BOOL)valueForKey:(uint64_t)key value:(nullable int32_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(uint64_t key, int32_t value, BOOL *stop))block;

// These methods bypass the validationFunc to provide access to values that were not
// known at the time the binary was compiled.

- (BOOL)valueForKey:(uint64_t)key rawValue:(nullable int32_t *)rawValue;

- (void)enumerateKeysAndRawValuesUsingBlock:
    (void (^)(uint64_t key, int32_t rawValue, BOOL *stop))block;

- (void)addRawEntriesFromDictionary:(GPBUInt64EnumDictionary *)otherDictionary;

// If value is not a valid enumerator as defined by validationFunc, these
// methods will assert in debug, and will log in release and assign the value
// to the default value. Use the rawValue methods below to assign non enumerator
// values.

- (void)setValue:(int32_t)value forKey:(uint64_t)key;

// This method bypass the validationFunc to provide setting of values that were not
// known at the time the binary was compiled.
- (void)setRawValue:(int32_t)rawValue forKey:(uint64_t)key;

// No validation applies to these methods.

- (void)removeValueForKey:(uint64_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt64 -> Object

@interface GPBUInt64ObjectDictionary<__covariant ObjectType> : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithObject:(ObjectType)object
                              forKey:(uint64_t)key;
+ (instancetype)dictionaryWithObjects:(const ObjectType GPB_UNSAFE_UNRETAINED [])objects
                              forKeys:(const uint64_t [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt64ObjectDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithObjects:(const ObjectType GPB_UNSAFE_UNRETAINED [])objects
                        forKeys:(const uint64_t [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt64ObjectDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (ObjectType)objectForKey:(uint64_t)key;

- (void)enumerateKeysAndObjectsUsingBlock:
    (void (^)(uint64_t key, ObjectType object, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt64ObjectDictionary *)otherDictionary;

- (void)setObject:(ObjectType)object forKey:(uint64_t)key;

- (void)removeObjectForKey:(uint64_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int64 -> UInt32

@interface GPBInt64UInt32Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(uint32_t)value
                             forKey:(int64_t)key;
+ (instancetype)dictionaryWithValues:(const uint32_t [])values
                             forKeys:(const int64_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt64UInt32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const uint32_t [])values
                       forKeys:(const int64_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt64UInt32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(int64_t)key value:(nullable uint32_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int64_t key, uint32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt64UInt32Dictionary *)otherDictionary;

- (void)setValue:(uint32_t)value forKey:(int64_t)key;

- (void)removeValueForKey:(int64_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int64 -> Int32

@interface GPBInt64Int32Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(int32_t)value
                             forKey:(int64_t)key;
+ (instancetype)dictionaryWithValues:(const int32_t [])values
                             forKeys:(const int64_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt64Int32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const int32_t [])values
                       forKeys:(const int64_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt64Int32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(int64_t)key value:(nullable int32_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int64_t key, int32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt64Int32Dictionary *)otherDictionary;

- (void)setValue:(int32_t)value forKey:(int64_t)key;

- (void)removeValueForKey:(int64_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int64 -> UInt64

@interface GPBInt64UInt64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(uint64_t)value
                             forKey:(int64_t)key;
+ (instancetype)dictionaryWithValues:(const uint64_t [])values
                             forKeys:(const int64_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt64UInt64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const uint64_t [])values
                       forKeys:(const int64_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt64UInt64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(int64_t)key value:(nullable uint64_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int64_t key, uint64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt64UInt64Dictionary *)otherDictionary;

- (void)setValue:(uint64_t)value forKey:(int64_t)key;

- (void)removeValueForKey:(int64_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int64 -> Int64

@interface GPBInt64Int64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(int64_t)value
                             forKey:(int64_t)key;
+ (instancetype)dictionaryWithValues:(const int64_t [])values
                             forKeys:(const int64_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt64Int64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const int64_t [])values
                       forKeys:(const int64_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt64Int64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(int64_t)key value:(nullable int64_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int64_t key, int64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt64Int64Dictionary *)otherDictionary;

- (void)setValue:(int64_t)value forKey:(int64_t)key;

- (void)removeValueForKey:(int64_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int64 -> Bool

@interface GPBInt64BoolDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(BOOL)value
                             forKey:(int64_t)key;
+ (instancetype)dictionaryWithValues:(const BOOL [])values
                             forKeys:(const int64_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt64BoolDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const BOOL [])values
                       forKeys:(const int64_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt64BoolDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(int64_t)key value:(nullable BOOL *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int64_t key, BOOL value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt64BoolDictionary *)otherDictionary;

- (void)setValue:(BOOL)value forKey:(int64_t)key;

- (void)removeValueForKey:(int64_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int64 -> Float

@interface GPBInt64FloatDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(float)value
                             forKey:(int64_t)key;
+ (instancetype)dictionaryWithValues:(const float [])values
                             forKeys:(const int64_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt64FloatDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const float [])values
                       forKeys:(const int64_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt64FloatDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(int64_t)key value:(nullable float *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int64_t key, float value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt64FloatDictionary *)otherDictionary;

- (void)setValue:(float)value forKey:(int64_t)key;

- (void)removeValueForKey:(int64_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int64 -> Double

@interface GPBInt64DoubleDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(double)value
                             forKey:(int64_t)key;
+ (instancetype)dictionaryWithValues:(const double [])values
                             forKeys:(const int64_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt64DoubleDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const double [])values
                       forKeys:(const int64_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt64DoubleDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(int64_t)key value:(nullable double *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int64_t key, double value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt64DoubleDictionary *)otherDictionary;

- (void)setValue:(double)value forKey:(int64_t)key;

- (void)removeValueForKey:(int64_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int64 -> Enum

@interface GPBInt64EnumDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;
@property(nonatomic, readonly) GPBEnumValidationFunc validationFunc;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                        rawValue:(int32_t)rawValue
                                          forKey:(int64_t)key;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                       rawValues:(const int32_t [])values
                                         forKeys:(const int64_t [])keys
                                           count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt64EnumDictionary *)dictionary;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                        capacity:(NSUInteger)numItems;

- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func;
- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                 rawValues:(const int32_t [])values
                                   forKeys:(const int64_t [])keys
                                     count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt64EnumDictionary *)dictionary;
- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                  capacity:(NSUInteger)numItems;

// These will return kGPBUnrecognizedEnumeratorValue if the value for the key
// is not a valid enumerator as defined by validationFunc. If the actual value is
// desired, use "raw" version of the method.

- (BOOL)valueForKey:(int64_t)key value:(nullable int32_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(int64_t key, int32_t value, BOOL *stop))block;

// These methods bypass the validationFunc to provide access to values that were not
// known at the time the binary was compiled.

- (BOOL)valueForKey:(int64_t)key rawValue:(nullable int32_t *)rawValue;

- (void)enumerateKeysAndRawValuesUsingBlock:
    (void (^)(int64_t key, int32_t rawValue, BOOL *stop))block;

- (void)addRawEntriesFromDictionary:(GPBInt64EnumDictionary *)otherDictionary;

// If value is not a valid enumerator as defined by validationFunc, these
// methods will assert in debug, and will log in release and assign the value
// to the default value. Use the rawValue methods below to assign non enumerator
// values.

- (void)setValue:(int32_t)value forKey:(int64_t)key;

// This method bypass the validationFunc to provide setting of values that were not
// known at the time the binary was compiled.
- (void)setRawValue:(int32_t)rawValue forKey:(int64_t)key;

// No validation applies to these methods.

- (void)removeValueForKey:(int64_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int64 -> Object

@interface GPBInt64ObjectDictionary<__covariant ObjectType> : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithObject:(ObjectType)object
                              forKey:(int64_t)key;
+ (instancetype)dictionaryWithObjects:(const ObjectType GPB_UNSAFE_UNRETAINED [])objects
                              forKeys:(const int64_t [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt64ObjectDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithObjects:(const ObjectType GPB_UNSAFE_UNRETAINED [])objects
                        forKeys:(const int64_t [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt64ObjectDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (ObjectType)objectForKey:(int64_t)key;

- (void)enumerateKeysAndObjectsUsingBlock:
    (void (^)(int64_t key, ObjectType object, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt64ObjectDictionary *)otherDictionary;

- (void)setObject:(ObjectType)object forKey:(int64_t)key;

- (void)removeObjectForKey:(int64_t)aKey;
- (void)removeAll;

@end

#pragma mark - Bool -> UInt32

@interface GPBBoolUInt32Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(uint32_t)value
                             forKey:(BOOL)key;
+ (instancetype)dictionaryWithValues:(const uint32_t [])values
                             forKeys:(const BOOL [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBBoolUInt32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const uint32_t [])values
                       forKeys:(const BOOL [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBBoolUInt32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(BOOL)key value:(nullable uint32_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(BOOL key, uint32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBBoolUInt32Dictionary *)otherDictionary;

- (void)setValue:(uint32_t)value forKey:(BOOL)key;

- (void)removeValueForKey:(BOOL)aKey;
- (void)removeAll;

@end

#pragma mark - Bool -> Int32

@interface GPBBoolInt32Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(int32_t)value
                             forKey:(BOOL)key;
+ (instancetype)dictionaryWithValues:(const int32_t [])values
                             forKeys:(const BOOL [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBBoolInt32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const int32_t [])values
                       forKeys:(const BOOL [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBBoolInt32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(BOOL)key value:(nullable int32_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(BOOL key, int32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBBoolInt32Dictionary *)otherDictionary;

- (void)setValue:(int32_t)value forKey:(BOOL)key;

- (void)removeValueForKey:(BOOL)aKey;
- (void)removeAll;

@end

#pragma mark - Bool -> UInt64

@interface GPBBoolUInt64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(uint64_t)value
                             forKey:(BOOL)key;
+ (instancetype)dictionaryWithValues:(const uint64_t [])values
                             forKeys:(const BOOL [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBBoolUInt64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const uint64_t [])values
                       forKeys:(const BOOL [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBBoolUInt64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(BOOL)key value:(nullable uint64_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(BOOL key, uint64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBBoolUInt64Dictionary *)otherDictionary;

- (void)setValue:(uint64_t)value forKey:(BOOL)key;

- (void)removeValueForKey:(BOOL)aKey;
- (void)removeAll;

@end

#pragma mark - Bool -> Int64

@interface GPBBoolInt64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(int64_t)value
                             forKey:(BOOL)key;
+ (instancetype)dictionaryWithValues:(const int64_t [])values
                             forKeys:(const BOOL [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBBoolInt64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const int64_t [])values
                       forKeys:(const BOOL [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBBoolInt64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(BOOL)key value:(nullable int64_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(BOOL key, int64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBBoolInt64Dictionary *)otherDictionary;

- (void)setValue:(int64_t)value forKey:(BOOL)key;

- (void)removeValueForKey:(BOOL)aKey;
- (void)removeAll;

@end

#pragma mark - Bool -> Bool

@interface GPBBoolBoolDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(BOOL)value
                             forKey:(BOOL)key;
+ (instancetype)dictionaryWithValues:(const BOOL [])values
                             forKeys:(const BOOL [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBBoolBoolDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const BOOL [])values
                       forKeys:(const BOOL [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBBoolBoolDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(BOOL)key value:(nullable BOOL *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(BOOL key, BOOL value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBBoolBoolDictionary *)otherDictionary;

- (void)setValue:(BOOL)value forKey:(BOOL)key;

- (void)removeValueForKey:(BOOL)aKey;
- (void)removeAll;

@end

#pragma mark - Bool -> Float

@interface GPBBoolFloatDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(float)value
                             forKey:(BOOL)key;
+ (instancetype)dictionaryWithValues:(const float [])values
                             forKeys:(const BOOL [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBBoolFloatDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const float [])values
                       forKeys:(const BOOL [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBBoolFloatDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(BOOL)key value:(nullable float *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(BOOL key, float value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBBoolFloatDictionary *)otherDictionary;

- (void)setValue:(float)value forKey:(BOOL)key;

- (void)removeValueForKey:(BOOL)aKey;
- (void)removeAll;

@end

#pragma mark - Bool -> Double

@interface GPBBoolDoubleDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(double)value
                             forKey:(BOOL)key;
+ (instancetype)dictionaryWithValues:(const double [])values
                             forKeys:(const BOOL [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBBoolDoubleDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const double [])values
                       forKeys:(const BOOL [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBBoolDoubleDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(BOOL)key value:(nullable double *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(BOOL key, double value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBBoolDoubleDictionary *)otherDictionary;

- (void)setValue:(double)value forKey:(BOOL)key;

- (void)removeValueForKey:(BOOL)aKey;
- (void)removeAll;

@end

#pragma mark - Bool -> Enum

@interface GPBBoolEnumDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;
@property(nonatomic, readonly) GPBEnumValidationFunc validationFunc;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                        rawValue:(int32_t)rawValue
                                          forKey:(BOOL)key;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                       rawValues:(const int32_t [])values
                                         forKeys:(const BOOL [])keys
                                           count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBBoolEnumDictionary *)dictionary;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                        capacity:(NSUInteger)numItems;

- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func;
- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                 rawValues:(const int32_t [])values
                                   forKeys:(const BOOL [])keys
                                     count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBBoolEnumDictionary *)dictionary;
- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                  capacity:(NSUInteger)numItems;

// These will return kGPBUnrecognizedEnumeratorValue if the value for the key
// is not a valid enumerator as defined by validationFunc. If the actual value is
// desired, use "raw" version of the method.

- (BOOL)valueForKey:(BOOL)key value:(nullable int32_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(BOOL key, int32_t value, BOOL *stop))block;

// These methods bypass the validationFunc to provide access to values that were not
// known at the time the binary was compiled.

- (BOOL)valueForKey:(BOOL)key rawValue:(nullable int32_t *)rawValue;

- (void)enumerateKeysAndRawValuesUsingBlock:
    (void (^)(BOOL key, int32_t rawValue, BOOL *stop))block;

- (void)addRawEntriesFromDictionary:(GPBBoolEnumDictionary *)otherDictionary;

// If value is not a valid enumerator as defined by validationFunc, these
// methods will assert in debug, and will log in release and assign the value
// to the default value. Use the rawValue methods below to assign non enumerator
// values.

- (void)setValue:(int32_t)value forKey:(BOOL)key;

// This method bypass the validationFunc to provide setting of values that were not
// known at the time the binary was compiled.
- (void)setRawValue:(int32_t)rawValue forKey:(BOOL)key;

// No validation applies to these methods.

- (void)removeValueForKey:(BOOL)aKey;
- (void)removeAll;

@end

#pragma mark - Bool -> Object

@interface GPBBoolObjectDictionary<__covariant ObjectType> : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithObject:(ObjectType)object
                              forKey:(BOOL)key;
+ (instancetype)dictionaryWithObjects:(const ObjectType GPB_UNSAFE_UNRETAINED [])objects
                              forKeys:(const BOOL [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBBoolObjectDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithObjects:(const ObjectType GPB_UNSAFE_UNRETAINED [])objects
                        forKeys:(const BOOL [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBBoolObjectDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (ObjectType)objectForKey:(BOOL)key;

- (void)enumerateKeysAndObjectsUsingBlock:
    (void (^)(BOOL key, ObjectType object, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBBoolObjectDictionary *)otherDictionary;

- (void)setObject:(ObjectType)object forKey:(BOOL)key;

- (void)removeObjectForKey:(BOOL)aKey;
- (void)removeAll;

@end

#pragma mark - String -> UInt32

@interface GPBStringUInt32Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(uint32_t)value
                             forKey:(NSString *)key;
+ (instancetype)dictionaryWithValues:(const uint32_t [])values
                             forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBStringUInt32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const uint32_t [])values
                       forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBStringUInt32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(NSString *)key value:(nullable uint32_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(NSString *key, uint32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBStringUInt32Dictionary *)otherDictionary;

- (void)setValue:(uint32_t)value forKey:(NSString *)key;

- (void)removeValueForKey:(NSString *)aKey;
- (void)removeAll;

@end

#pragma mark - String -> Int32

@interface GPBStringInt32Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(int32_t)value
                             forKey:(NSString *)key;
+ (instancetype)dictionaryWithValues:(const int32_t [])values
                             forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBStringInt32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const int32_t [])values
                       forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBStringInt32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(NSString *)key value:(nullable int32_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(NSString *key, int32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBStringInt32Dictionary *)otherDictionary;

- (void)setValue:(int32_t)value forKey:(NSString *)key;

- (void)removeValueForKey:(NSString *)aKey;
- (void)removeAll;

@end

#pragma mark - String -> UInt64

@interface GPBStringUInt64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(uint64_t)value
                             forKey:(NSString *)key;
+ (instancetype)dictionaryWithValues:(const uint64_t [])values
                             forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBStringUInt64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const uint64_t [])values
                       forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBStringUInt64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(NSString *)key value:(nullable uint64_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(NSString *key, uint64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBStringUInt64Dictionary *)otherDictionary;

- (void)setValue:(uint64_t)value forKey:(NSString *)key;

- (void)removeValueForKey:(NSString *)aKey;
- (void)removeAll;

@end

#pragma mark - String -> Int64

@interface GPBStringInt64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(int64_t)value
                             forKey:(NSString *)key;
+ (instancetype)dictionaryWithValues:(const int64_t [])values
                             forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBStringInt64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const int64_t [])values
                       forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBStringInt64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(NSString *)key value:(nullable int64_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(NSString *key, int64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBStringInt64Dictionary *)otherDictionary;

- (void)setValue:(int64_t)value forKey:(NSString *)key;

- (void)removeValueForKey:(NSString *)aKey;
- (void)removeAll;

@end

#pragma mark - String -> Bool

@interface GPBStringBoolDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(BOOL)value
                             forKey:(NSString *)key;
+ (instancetype)dictionaryWithValues:(const BOOL [])values
                             forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBStringBoolDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const BOOL [])values
                       forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBStringBoolDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(NSString *)key value:(nullable BOOL *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(NSString *key, BOOL value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBStringBoolDictionary *)otherDictionary;

- (void)setValue:(BOOL)value forKey:(NSString *)key;

- (void)removeValueForKey:(NSString *)aKey;
- (void)removeAll;

@end

#pragma mark - String -> Float

@interface GPBStringFloatDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(float)value
                             forKey:(NSString *)key;
+ (instancetype)dictionaryWithValues:(const float [])values
                             forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBStringFloatDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const float [])values
                       forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBStringFloatDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(NSString *)key value:(nullable float *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(NSString *key, float value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBStringFloatDictionary *)otherDictionary;

- (void)setValue:(float)value forKey:(NSString *)key;

- (void)removeValueForKey:(NSString *)aKey;
- (void)removeAll;

@end

#pragma mark - String -> Double

@interface GPBStringDoubleDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValue:(double)value
                             forKey:(NSString *)key;
+ (instancetype)dictionaryWithValues:(const double [])values
                             forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBStringDoubleDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithValues:(const double [])values
                       forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBStringDoubleDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

- (BOOL)valueForKey:(NSString *)key value:(nullable double *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(NSString *key, double value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBStringDoubleDictionary *)otherDictionary;

- (void)setValue:(double)value forKey:(NSString *)key;

- (void)removeValueForKey:(NSString *)aKey;
- (void)removeAll;

@end

#pragma mark - String -> Enum

@interface GPBStringEnumDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;
@property(nonatomic, readonly) GPBEnumValidationFunc validationFunc;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                        rawValue:(int32_t)rawValue
                                          forKey:(NSString *)key;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                       rawValues:(const int32_t [])values
                                         forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                                           count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBStringEnumDictionary *)dictionary;
+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                        capacity:(NSUInteger)numItems;

- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func;
- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                 rawValues:(const int32_t [])values
                                   forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                                     count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBStringEnumDictionary *)dictionary;
- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func
                                  capacity:(NSUInteger)numItems;

// These will return kGPBUnrecognizedEnumeratorValue if the value for the key
// is not a valid enumerator as defined by validationFunc. If the actual value is
// desired, use "raw" version of the method.

- (BOOL)valueForKey:(NSString *)key value:(nullable int32_t *)value;

- (void)enumerateKeysAndValuesUsingBlock:
    (void (^)(NSString *key, int32_t value, BOOL *stop))block;

// These methods bypass the validationFunc to provide access to values that were not
// known at the time the binary was compiled.

- (BOOL)valueForKey:(NSString *)key rawValue:(nullable int32_t *)rawValue;

- (void)enumerateKeysAndRawValuesUsingBlock:
    (void (^)(NSString *key, int32_t rawValue, BOOL *stop))block;

- (void)addRawEntriesFromDictionary:(GPBStringEnumDictionary *)otherDictionary;

// If value is not a valid enumerator as defined by validationFunc, these
// methods will assert in debug, and will log in release and assign the value
// to the default value. Use the rawValue methods below to assign non enumerator
// values.

- (void)setValue:(int32_t)value forKey:(NSString *)key;

// This method bypass the validationFunc to provide setting of values that were not
// known at the time the binary was compiled.
- (void)setRawValue:(int32_t)rawValue forKey:(NSString *)key;

// No validation applies to these methods.

- (void)removeValueForKey:(NSString *)aKey;
- (void)removeAll;

@end

//%PDDM-EXPAND-END DECLARE_DICTIONARIES()

NS_ASSUME_NONNULL_END

//%PDDM-DEFINE DECLARE_DICTIONARIES()
//%DICTIONARY_INTERFACES_FOR_POD_KEY(UInt32, uint32_t)
//%DICTIONARY_INTERFACES_FOR_POD_KEY(Int32, int32_t)
//%DICTIONARY_INTERFACES_FOR_POD_KEY(UInt64, uint64_t)
//%DICTIONARY_INTERFACES_FOR_POD_KEY(Int64, int64_t)
//%DICTIONARY_INTERFACES_FOR_POD_KEY(Bool, BOOL)
//%DICTIONARY_POD_INTERFACES_FOR_KEY(String, NSString, *, OBJECT)
//%PDDM-DEFINE DICTIONARY_INTERFACES_FOR_POD_KEY(KEY_NAME, KEY_TYPE)
//%DICTIONARY_POD_INTERFACES_FOR_KEY(KEY_NAME, KEY_TYPE, , POD)
//%DICTIONARY_POD_KEY_TO_OBJECT_INTERFACE(KEY_NAME, KEY_TYPE, Object, ObjectType)
//%PDDM-DEFINE DICTIONARY_POD_INTERFACES_FOR_KEY(KEY_NAME, KEY_TYPE, KisP, KHELPER)
//%DICTIONARY_KEY_TO_POD_INTERFACE(KEY_NAME, KEY_TYPE, KisP, KHELPER, UInt32, uint32_t)
//%DICTIONARY_KEY_TO_POD_INTERFACE(KEY_NAME, KEY_TYPE, KisP, KHELPER, Int32, int32_t)
//%DICTIONARY_KEY_TO_POD_INTERFACE(KEY_NAME, KEY_TYPE, KisP, KHELPER, UInt64, uint64_t)
//%DICTIONARY_KEY_TO_POD_INTERFACE(KEY_NAME, KEY_TYPE, KisP, KHELPER, Int64, int64_t)
//%DICTIONARY_KEY_TO_POD_INTERFACE(KEY_NAME, KEY_TYPE, KisP, KHELPER, Bool, BOOL)
//%DICTIONARY_KEY_TO_POD_INTERFACE(KEY_NAME, KEY_TYPE, KisP, KHELPER, Float, float)
//%DICTIONARY_KEY_TO_POD_INTERFACE(KEY_NAME, KEY_TYPE, KisP, KHELPER, Double, double)
//%DICTIONARY_KEY_TO_ENUM_INTERFACE(KEY_NAME, KEY_TYPE, KisP, KHELPER, Enum, int32_t)
//%PDDM-DEFINE DICTIONARY_KEY_TO_POD_INTERFACE(KEY_NAME, KEY_TYPE, KisP, KHELPER, VALUE_NAME, VALUE_TYPE)
//%DICTIONARY_COMMON_INTERFACE(KEY_NAME, KEY_TYPE, KisP, KHELPER, VALUE_NAME, VALUE_TYPE, POD, value)
//%PDDM-DEFINE DICTIONARY_POD_KEY_TO_OBJECT_INTERFACE(KEY_NAME, KEY_TYPE, VALUE_NAME, VALUE_TYPE)
//%DICTIONARY_COMMON_INTERFACE(KEY_NAME, KEY_TYPE, , POD, VALUE_NAME, VALUE_TYPE, OBJECT, object)
//%PDDM-DEFINE VALUE_FOR_KEY_POD(KEY_TYPE, VALUE_TYPE)
//%- (BOOL)valueForKey:(KEY_TYPE)key value:(nullable VALUE_TYPE *)value;
//%PDDM-DEFINE VALUE_FOR_KEY_OBJECT(KEY_TYPE, VALUE_TYPE)
//%- (VALUE_TYPE)objectForKey:(KEY_TYPE)key;
//%PDDM-DEFINE VALUE_FOR_KEY_Enum(KEY_TYPE, VALUE_TYPE)
//%VALUE_FOR_KEY_POD(KEY_TYPE, VALUE_TYPE)
//%PDDM-DEFINE ARRAY_ARG_MODIFIERPOD()
// Nothing
//%PDDM-DEFINE ARRAY_ARG_MODIFIEREnum()
// Nothing
//%PDDM-DEFINE ARRAY_ARG_MODIFIEROBJECT()
//%GPB_UNSAFE_UNRETAINED ##
//%PDDM-DEFINE DICTIONARY_CLASS_DECLPOD(KEY_NAME, VALUE_NAME, VALUE_TYPE)
//%GPB##KEY_NAME##VALUE_NAME##Dictionary
//%PDDM-DEFINE DICTIONARY_CLASS_DECLEnum(KEY_NAME, VALUE_NAME, VALUE_TYPE)
//%GPB##KEY_NAME##VALUE_NAME##Dictionary
//%PDDM-DEFINE DICTIONARY_CLASS_DECLOBJECT(KEY_NAME, VALUE_NAME, VALUE_TYPE)
//%GPB##KEY_NAME##VALUE_NAME##Dictionary<__covariant VALUE_TYPE>
//%PDDM-DEFINE DICTIONARY_COMMON_INTERFACE(KEY_NAME, KEY_TYPE, KisP, KHELPER, VALUE_NAME, VALUE_TYPE, VHELPER, VNAME)
//%#pragma mark - KEY_NAME -> VALUE_NAME
//%
//%@interface DICTIONARY_CLASS_DECL##VHELPER(KEY_NAME, VALUE_NAME, VALUE_TYPE) : NSObject <NSCopying>
//%
//%@property(nonatomic, readonly) NSUInteger count;
//%
//%+ (instancetype)dictionary;
//%+ (instancetype)dictionaryWith##VNAME$u##:(VALUE_TYPE)##VNAME
//%                       ##VNAME$S## forKey:(KEY_TYPE##KisP$S##KisP)key;
//%+ (instancetype)dictionaryWith##VNAME$u##s:(const VALUE_TYPE ARRAY_ARG_MODIFIER##VHELPER()[])##VNAME##s
//%                      ##VNAME$S##  forKeys:(const KEY_TYPE##KisP$S##KisP ARRAY_ARG_MODIFIER##KHELPER()[])keys
//%                      ##VNAME$S##    count:(NSUInteger)count;
//%+ (instancetype)dictionaryWithDictionary:(GPB##KEY_NAME##VALUE_NAME##Dictionary *)dictionary;
//%+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;
//%
//%- (instancetype)initWith##VNAME$u##s:(const VALUE_TYPE ARRAY_ARG_MODIFIER##VHELPER()[])##VNAME##s
//%                ##VNAME$S##  forKeys:(const KEY_TYPE##KisP$S##KisP ARRAY_ARG_MODIFIER##KHELPER()[])keys
//%                ##VNAME$S##    count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
//%- (instancetype)initWithDictionary:(GPB##KEY_NAME##VALUE_NAME##Dictionary *)dictionary;
//%- (instancetype)initWithCapacity:(NSUInteger)numItems;
//%
//%DICTIONARY_IMMUTABLE_INTERFACE(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, VHELPER, VNAME)
//%
//%- (void)addEntriesFromDictionary:(GPB##KEY_NAME##VALUE_NAME##Dictionary *)otherDictionary;
//%
//%DICTIONARY_MUTABLE_INTERFACE(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, VHELPER, VNAME)
//%
//%@end
//%

//%PDDM-DEFINE DICTIONARY_KEY_TO_ENUM_INTERFACE(KEY_NAME, KEY_TYPE, KisP, KHELPER, VALUE_NAME, VALUE_TYPE)
//%DICTIONARY_KEY_TO_ENUM_INTERFACE2(KEY_NAME, KEY_TYPE, KisP, KHELPER, VALUE_NAME, VALUE_TYPE, Enum)
//%PDDM-DEFINE DICTIONARY_KEY_TO_ENUM_INTERFACE2(KEY_NAME, KEY_TYPE, KisP, KHELPER, VALUE_NAME, VALUE_TYPE, VHELPER)
//%#pragma mark - KEY_NAME -> VALUE_NAME
//%
//%@interface GPB##KEY_NAME##VALUE_NAME##Dictionary : NSObject <NSCopying>
//%
//%@property(nonatomic, readonly) NSUInteger count;
//%@property(nonatomic, readonly) GPBEnumValidationFunc validationFunc;
//%
//%+ (instancetype)dictionary;
//%+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func;
//%+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
//%                                        rawValue:(VALUE_TYPE)rawValue
//%                                          forKey:(KEY_TYPE##KisP$S##KisP)key;
//%+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
//%                                       rawValues:(const VALUE_TYPE ARRAY_ARG_MODIFIER##VHELPER()[])values
//%                                         forKeys:(const KEY_TYPE##KisP$S##KisP ARRAY_ARG_MODIFIER##KHELPER()[])keys
//%                                           count:(NSUInteger)count;
//%+ (instancetype)dictionaryWithDictionary:(GPB##KEY_NAME##VALUE_NAME##Dictionary *)dictionary;
//%+ (instancetype)dictionaryWithValidationFunction:(nullable GPBEnumValidationFunc)func
//%                                        capacity:(NSUInteger)numItems;
//%
//%- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func;
//%- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func
//%                                 rawValues:(const VALUE_TYPE ARRAY_ARG_MODIFIER##VHELPER()[])values
//%                                   forKeys:(const KEY_TYPE##KisP$S##KisP ARRAY_ARG_MODIFIER##KHELPER()[])keys
//%                                     count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
//%- (instancetype)initWithDictionary:(GPB##KEY_NAME##VALUE_NAME##Dictionary *)dictionary;
//%- (instancetype)initWithValidationFunction:(nullable GPBEnumValidationFunc)func
//%                                  capacity:(NSUInteger)numItems;
//%
//%// These will return kGPBUnrecognizedEnumeratorValue if the value for the key
//%// is not a valid enumerator as defined by validationFunc. If the actual value is
//%// desired, use "raw" version of the method.
//%
//%DICTIONARY_IMMUTABLE_INTERFACE(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, VHELPER, value)
//%
//%// These methods bypass the validationFunc to provide access to values that were not
//%// known at the time the binary was compiled.
//%
//%- (BOOL)valueForKey:(KEY_TYPE##KisP$S##KisP)key rawValue:(nullable VALUE_TYPE *)rawValue;
//%
//%- (void)enumerateKeysAndRawValuesUsingBlock:
//%    (void (^)(KEY_TYPE KisP##key, VALUE_TYPE rawValue, BOOL *stop))block;
//%
//%- (void)addRawEntriesFromDictionary:(GPB##KEY_NAME##VALUE_NAME##Dictionary *)otherDictionary;
//%
//%// If value is not a valid enumerator as defined by validationFunc, these
//%// methods will assert in debug, and will log in release and assign the value
//%// to the default value. Use the rawValue methods below to assign non enumerator
//%// values.
//%
//%DICTIONARY_MUTABLE_INTERFACE(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, VHELPER, value)
//%
//%@end
//%

//%PDDM-DEFINE DICTIONARY_IMMUTABLE_INTERFACE(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, VHELPER, VNAME)
//%VALUE_FOR_KEY_##VHELPER(KEY_TYPE##KisP$S##KisP, VALUE_TYPE)
//%
//%- (void)enumerateKeysAnd##VNAME$u##sUsingBlock:
//%    (void (^)(KEY_TYPE KisP##key, VALUE_TYPE VNAME, BOOL *stop))block;

//%PDDM-DEFINE DICTIONARY_MUTABLE_INTERFACE(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, VHELPER, VNAME)
//%- (void)set##VNAME$u##:(VALUE_TYPE)##VNAME forKey:(KEY_TYPE##KisP$S##KisP)key;
//%DICTIONARY_EXTRA_MUTABLE_METHODS_##VHELPER(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE)
//%- (void)remove##VNAME$u##ForKey:(KEY_TYPE##KisP$S##KisP)aKey;
//%- (void)removeAll;

//%PDDM-DEFINE DICTIONARY_EXTRA_MUTABLE_METHODS_POD(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE)
// Empty
//%PDDM-DEFINE DICTIONARY_EXTRA_MUTABLE_METHODS_OBJECT(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE)
// Empty
//%PDDM-DEFINE DICTIONARY_EXTRA_MUTABLE_METHODS_Enum(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE)
//%
//%// This method bypass the validationFunc to provide setting of values that were not
//%// known at the time the binary was compiled.
//%- (void)setRawValue:(VALUE_TYPE)rawValue forKey:(KEY_TYPE##KisP$S##KisP)key;
//%
//%// No validation applies to these methods.
//%
