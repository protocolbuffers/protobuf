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

// Note on naming: for the classes holding numeric values, a more natural
// naming of the method might be things like "-valueForKey:",
// "-setValue:forKey:"; etc. But those selectors are also defined by Key Value
// Coding (KVC) as categories on NSObject. So "overloading" the selectors with
// other meanings can cause warnings (based on compiler settings), but more
// importantly, some of those selector get called as KVC breaks up keypaths.
// So if those selectors are used, using KVC will compile cleanly, but could
// crash as it invokes those selectors with the wrong types of arguments.

NS_ASSUME_NONNULL_BEGIN

//%PDDM-EXPAND DECLARE_DICTIONARIES()
// This block of code is generated, do not edit it directly.

#pragma mark - UInt32 -> UInt32

@interface GPBUInt32UInt32Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithUInt32:(uint32_t)value
                              forKey:(uint32_t)key;
+ (instancetype)dictionaryWithUInt32s:(const uint32_t [])values
                              forKeys:(const uint32_t [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt32UInt32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithUInt32s:(const uint32_t [])values
                        forKeys:(const uint32_t [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt32UInt32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getUInt32:(nullable uint32_t *)value forKey:(uint32_t)key;

- (void)enumerateKeysAndUInt32sUsingBlock:
    (void (^)(uint32_t key, uint32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt32UInt32Dictionary *)otherDictionary;

- (void)setUInt32:(uint32_t)value forKey:(uint32_t)key;

- (void)removeUInt32ForKey:(uint32_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt32 -> Int32

@interface GPBUInt32Int32Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithInt32:(int32_t)value
                             forKey:(uint32_t)key;
+ (instancetype)dictionaryWithInt32s:(const int32_t [])values
                             forKeys:(const uint32_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt32Int32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithInt32s:(const int32_t [])values
                       forKeys:(const uint32_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt32Int32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getInt32:(nullable int32_t *)value forKey:(uint32_t)key;

- (void)enumerateKeysAndInt32sUsingBlock:
    (void (^)(uint32_t key, int32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt32Int32Dictionary *)otherDictionary;

- (void)setInt32:(int32_t)value forKey:(uint32_t)key;

- (void)removeInt32ForKey:(uint32_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt32 -> UInt64

@interface GPBUInt32UInt64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithUInt64:(uint64_t)value
                              forKey:(uint32_t)key;
+ (instancetype)dictionaryWithUInt64s:(const uint64_t [])values
                              forKeys:(const uint32_t [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt32UInt64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithUInt64s:(const uint64_t [])values
                        forKeys:(const uint32_t [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt32UInt64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getUInt64:(nullable uint64_t *)value forKey:(uint32_t)key;

- (void)enumerateKeysAndUInt64sUsingBlock:
    (void (^)(uint32_t key, uint64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt32UInt64Dictionary *)otherDictionary;

- (void)setUInt64:(uint64_t)value forKey:(uint32_t)key;

- (void)removeUInt64ForKey:(uint32_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt32 -> Int64

@interface GPBUInt32Int64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithInt64:(int64_t)value
                             forKey:(uint32_t)key;
+ (instancetype)dictionaryWithInt64s:(const int64_t [])values
                             forKeys:(const uint32_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt32Int64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithInt64s:(const int64_t [])values
                       forKeys:(const uint32_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt32Int64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getInt64:(nullable int64_t *)value forKey:(uint32_t)key;

- (void)enumerateKeysAndInt64sUsingBlock:
    (void (^)(uint32_t key, int64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt32Int64Dictionary *)otherDictionary;

- (void)setInt64:(int64_t)value forKey:(uint32_t)key;

- (void)removeInt64ForKey:(uint32_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt32 -> Bool

@interface GPBUInt32BoolDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithBool:(BOOL)value
                            forKey:(uint32_t)key;
+ (instancetype)dictionaryWithBools:(const BOOL [])values
                            forKeys:(const uint32_t [])keys
                              count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt32BoolDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithBools:(const BOOL [])values
                      forKeys:(const uint32_t [])keys
                        count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt32BoolDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getBool:(nullable BOOL *)value forKey:(uint32_t)key;

- (void)enumerateKeysAndBoolsUsingBlock:
    (void (^)(uint32_t key, BOOL value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt32BoolDictionary *)otherDictionary;

- (void)setBool:(BOOL)value forKey:(uint32_t)key;

- (void)removeBoolForKey:(uint32_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt32 -> Float

@interface GPBUInt32FloatDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithFloat:(float)value
                             forKey:(uint32_t)key;
+ (instancetype)dictionaryWithFloats:(const float [])values
                             forKeys:(const uint32_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt32FloatDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithFloats:(const float [])values
                       forKeys:(const uint32_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt32FloatDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getFloat:(nullable float *)value forKey:(uint32_t)key;

- (void)enumerateKeysAndFloatsUsingBlock:
    (void (^)(uint32_t key, float value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt32FloatDictionary *)otherDictionary;

- (void)setFloat:(float)value forKey:(uint32_t)key;

- (void)removeFloatForKey:(uint32_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt32 -> Double

@interface GPBUInt32DoubleDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithDouble:(double)value
                              forKey:(uint32_t)key;
+ (instancetype)dictionaryWithDoubles:(const double [])values
                              forKeys:(const uint32_t [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt32DoubleDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithDoubles:(const double [])values
                        forKeys:(const uint32_t [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt32DoubleDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getDouble:(nullable double *)value forKey:(uint32_t)key;

- (void)enumerateKeysAndDoublesUsingBlock:
    (void (^)(uint32_t key, double value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt32DoubleDictionary *)otherDictionary;

- (void)setDouble:(double)value forKey:(uint32_t)key;

- (void)removeDoubleForKey:(uint32_t)aKey;
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

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getEnum:(nullable int32_t *)value forKey:(uint32_t)key;

- (void)enumerateKeysAndEnumsUsingBlock:
    (void (^)(uint32_t key, int32_t value, BOOL *stop))block;

// These methods bypass the validationFunc to provide access to values that were not
// known at the time the binary was compiled.

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getRawValue:(nullable int32_t *)rawValue forKey:(uint32_t)key;

- (void)enumerateKeysAndRawValuesUsingBlock:
    (void (^)(uint32_t key, int32_t rawValue, BOOL *stop))block;

- (void)addRawEntriesFromDictionary:(GPBUInt32EnumDictionary *)otherDictionary;

// If value is not a valid enumerator as defined by validationFunc, these
// methods will assert in debug, and will log in release and assign the value
// to the default value. Use the rawValue methods below to assign non enumerator
// values.

- (void)setEnum:(int32_t)value forKey:(uint32_t)key;

// This method bypass the validationFunc to provide setting of values that were not
// known at the time the binary was compiled.
- (void)setRawValue:(int32_t)rawValue forKey:(uint32_t)key;

// No validation applies to these methods.

- (void)removeEnumForKey:(uint32_t)aKey;
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
+ (instancetype)dictionaryWithUInt32:(uint32_t)value
                              forKey:(int32_t)key;
+ (instancetype)dictionaryWithUInt32s:(const uint32_t [])values
                              forKeys:(const int32_t [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt32UInt32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithUInt32s:(const uint32_t [])values
                        forKeys:(const int32_t [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt32UInt32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getUInt32:(nullable uint32_t *)value forKey:(int32_t)key;

- (void)enumerateKeysAndUInt32sUsingBlock:
    (void (^)(int32_t key, uint32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt32UInt32Dictionary *)otherDictionary;

- (void)setUInt32:(uint32_t)value forKey:(int32_t)key;

- (void)removeUInt32ForKey:(int32_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int32 -> Int32

@interface GPBInt32Int32Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithInt32:(int32_t)value
                             forKey:(int32_t)key;
+ (instancetype)dictionaryWithInt32s:(const int32_t [])values
                             forKeys:(const int32_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt32Int32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithInt32s:(const int32_t [])values
                       forKeys:(const int32_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt32Int32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getInt32:(nullable int32_t *)value forKey:(int32_t)key;

- (void)enumerateKeysAndInt32sUsingBlock:
    (void (^)(int32_t key, int32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt32Int32Dictionary *)otherDictionary;

- (void)setInt32:(int32_t)value forKey:(int32_t)key;

- (void)removeInt32ForKey:(int32_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int32 -> UInt64

@interface GPBInt32UInt64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithUInt64:(uint64_t)value
                              forKey:(int32_t)key;
+ (instancetype)dictionaryWithUInt64s:(const uint64_t [])values
                              forKeys:(const int32_t [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt32UInt64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithUInt64s:(const uint64_t [])values
                        forKeys:(const int32_t [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt32UInt64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getUInt64:(nullable uint64_t *)value forKey:(int32_t)key;

- (void)enumerateKeysAndUInt64sUsingBlock:
    (void (^)(int32_t key, uint64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt32UInt64Dictionary *)otherDictionary;

- (void)setUInt64:(uint64_t)value forKey:(int32_t)key;

- (void)removeUInt64ForKey:(int32_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int32 -> Int64

@interface GPBInt32Int64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithInt64:(int64_t)value
                             forKey:(int32_t)key;
+ (instancetype)dictionaryWithInt64s:(const int64_t [])values
                             forKeys:(const int32_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt32Int64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithInt64s:(const int64_t [])values
                       forKeys:(const int32_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt32Int64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getInt64:(nullable int64_t *)value forKey:(int32_t)key;

- (void)enumerateKeysAndInt64sUsingBlock:
    (void (^)(int32_t key, int64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt32Int64Dictionary *)otherDictionary;

- (void)setInt64:(int64_t)value forKey:(int32_t)key;

- (void)removeInt64ForKey:(int32_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int32 -> Bool

@interface GPBInt32BoolDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithBool:(BOOL)value
                            forKey:(int32_t)key;
+ (instancetype)dictionaryWithBools:(const BOOL [])values
                            forKeys:(const int32_t [])keys
                              count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt32BoolDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithBools:(const BOOL [])values
                      forKeys:(const int32_t [])keys
                        count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt32BoolDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getBool:(nullable BOOL *)value forKey:(int32_t)key;

- (void)enumerateKeysAndBoolsUsingBlock:
    (void (^)(int32_t key, BOOL value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt32BoolDictionary *)otherDictionary;

- (void)setBool:(BOOL)value forKey:(int32_t)key;

- (void)removeBoolForKey:(int32_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int32 -> Float

@interface GPBInt32FloatDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithFloat:(float)value
                             forKey:(int32_t)key;
+ (instancetype)dictionaryWithFloats:(const float [])values
                             forKeys:(const int32_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt32FloatDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithFloats:(const float [])values
                       forKeys:(const int32_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt32FloatDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getFloat:(nullable float *)value forKey:(int32_t)key;

- (void)enumerateKeysAndFloatsUsingBlock:
    (void (^)(int32_t key, float value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt32FloatDictionary *)otherDictionary;

- (void)setFloat:(float)value forKey:(int32_t)key;

- (void)removeFloatForKey:(int32_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int32 -> Double

@interface GPBInt32DoubleDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithDouble:(double)value
                              forKey:(int32_t)key;
+ (instancetype)dictionaryWithDoubles:(const double [])values
                              forKeys:(const int32_t [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt32DoubleDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithDoubles:(const double [])values
                        forKeys:(const int32_t [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt32DoubleDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getDouble:(nullable double *)value forKey:(int32_t)key;

- (void)enumerateKeysAndDoublesUsingBlock:
    (void (^)(int32_t key, double value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt32DoubleDictionary *)otherDictionary;

- (void)setDouble:(double)value forKey:(int32_t)key;

- (void)removeDoubleForKey:(int32_t)aKey;
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

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getEnum:(nullable int32_t *)value forKey:(int32_t)key;

- (void)enumerateKeysAndEnumsUsingBlock:
    (void (^)(int32_t key, int32_t value, BOOL *stop))block;

// These methods bypass the validationFunc to provide access to values that were not
// known at the time the binary was compiled.

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getRawValue:(nullable int32_t *)rawValue forKey:(int32_t)key;

- (void)enumerateKeysAndRawValuesUsingBlock:
    (void (^)(int32_t key, int32_t rawValue, BOOL *stop))block;

- (void)addRawEntriesFromDictionary:(GPBInt32EnumDictionary *)otherDictionary;

// If value is not a valid enumerator as defined by validationFunc, these
// methods will assert in debug, and will log in release and assign the value
// to the default value. Use the rawValue methods below to assign non enumerator
// values.

- (void)setEnum:(int32_t)value forKey:(int32_t)key;

// This method bypass the validationFunc to provide setting of values that were not
// known at the time the binary was compiled.
- (void)setRawValue:(int32_t)rawValue forKey:(int32_t)key;

// No validation applies to these methods.

- (void)removeEnumForKey:(int32_t)aKey;
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
+ (instancetype)dictionaryWithUInt32:(uint32_t)value
                              forKey:(uint64_t)key;
+ (instancetype)dictionaryWithUInt32s:(const uint32_t [])values
                              forKeys:(const uint64_t [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt64UInt32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithUInt32s:(const uint32_t [])values
                        forKeys:(const uint64_t [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt64UInt32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getUInt32:(nullable uint32_t *)value forKey:(uint64_t)key;

- (void)enumerateKeysAndUInt32sUsingBlock:
    (void (^)(uint64_t key, uint32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt64UInt32Dictionary *)otherDictionary;

- (void)setUInt32:(uint32_t)value forKey:(uint64_t)key;

- (void)removeUInt32ForKey:(uint64_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt64 -> Int32

@interface GPBUInt64Int32Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithInt32:(int32_t)value
                             forKey:(uint64_t)key;
+ (instancetype)dictionaryWithInt32s:(const int32_t [])values
                             forKeys:(const uint64_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt64Int32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithInt32s:(const int32_t [])values
                       forKeys:(const uint64_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt64Int32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getInt32:(nullable int32_t *)value forKey:(uint64_t)key;

- (void)enumerateKeysAndInt32sUsingBlock:
    (void (^)(uint64_t key, int32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt64Int32Dictionary *)otherDictionary;

- (void)setInt32:(int32_t)value forKey:(uint64_t)key;

- (void)removeInt32ForKey:(uint64_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt64 -> UInt64

@interface GPBUInt64UInt64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithUInt64:(uint64_t)value
                              forKey:(uint64_t)key;
+ (instancetype)dictionaryWithUInt64s:(const uint64_t [])values
                              forKeys:(const uint64_t [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt64UInt64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithUInt64s:(const uint64_t [])values
                        forKeys:(const uint64_t [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt64UInt64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getUInt64:(nullable uint64_t *)value forKey:(uint64_t)key;

- (void)enumerateKeysAndUInt64sUsingBlock:
    (void (^)(uint64_t key, uint64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt64UInt64Dictionary *)otherDictionary;

- (void)setUInt64:(uint64_t)value forKey:(uint64_t)key;

- (void)removeUInt64ForKey:(uint64_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt64 -> Int64

@interface GPBUInt64Int64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithInt64:(int64_t)value
                             forKey:(uint64_t)key;
+ (instancetype)dictionaryWithInt64s:(const int64_t [])values
                             forKeys:(const uint64_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt64Int64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithInt64s:(const int64_t [])values
                       forKeys:(const uint64_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt64Int64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getInt64:(nullable int64_t *)value forKey:(uint64_t)key;

- (void)enumerateKeysAndInt64sUsingBlock:
    (void (^)(uint64_t key, int64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt64Int64Dictionary *)otherDictionary;

- (void)setInt64:(int64_t)value forKey:(uint64_t)key;

- (void)removeInt64ForKey:(uint64_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt64 -> Bool

@interface GPBUInt64BoolDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithBool:(BOOL)value
                            forKey:(uint64_t)key;
+ (instancetype)dictionaryWithBools:(const BOOL [])values
                            forKeys:(const uint64_t [])keys
                              count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt64BoolDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithBools:(const BOOL [])values
                      forKeys:(const uint64_t [])keys
                        count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt64BoolDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getBool:(nullable BOOL *)value forKey:(uint64_t)key;

- (void)enumerateKeysAndBoolsUsingBlock:
    (void (^)(uint64_t key, BOOL value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt64BoolDictionary *)otherDictionary;

- (void)setBool:(BOOL)value forKey:(uint64_t)key;

- (void)removeBoolForKey:(uint64_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt64 -> Float

@interface GPBUInt64FloatDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithFloat:(float)value
                             forKey:(uint64_t)key;
+ (instancetype)dictionaryWithFloats:(const float [])values
                             forKeys:(const uint64_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt64FloatDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithFloats:(const float [])values
                       forKeys:(const uint64_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt64FloatDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getFloat:(nullable float *)value forKey:(uint64_t)key;

- (void)enumerateKeysAndFloatsUsingBlock:
    (void (^)(uint64_t key, float value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt64FloatDictionary *)otherDictionary;

- (void)setFloat:(float)value forKey:(uint64_t)key;

- (void)removeFloatForKey:(uint64_t)aKey;
- (void)removeAll;

@end

#pragma mark - UInt64 -> Double

@interface GPBUInt64DoubleDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithDouble:(double)value
                              forKey:(uint64_t)key;
+ (instancetype)dictionaryWithDoubles:(const double [])values
                              forKeys:(const uint64_t [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBUInt64DoubleDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithDoubles:(const double [])values
                        forKeys:(const uint64_t [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBUInt64DoubleDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getDouble:(nullable double *)value forKey:(uint64_t)key;

- (void)enumerateKeysAndDoublesUsingBlock:
    (void (^)(uint64_t key, double value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBUInt64DoubleDictionary *)otherDictionary;

- (void)setDouble:(double)value forKey:(uint64_t)key;

- (void)removeDoubleForKey:(uint64_t)aKey;
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

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getEnum:(nullable int32_t *)value forKey:(uint64_t)key;

- (void)enumerateKeysAndEnumsUsingBlock:
    (void (^)(uint64_t key, int32_t value, BOOL *stop))block;

// These methods bypass the validationFunc to provide access to values that were not
// known at the time the binary was compiled.

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getRawValue:(nullable int32_t *)rawValue forKey:(uint64_t)key;

- (void)enumerateKeysAndRawValuesUsingBlock:
    (void (^)(uint64_t key, int32_t rawValue, BOOL *stop))block;

- (void)addRawEntriesFromDictionary:(GPBUInt64EnumDictionary *)otherDictionary;

// If value is not a valid enumerator as defined by validationFunc, these
// methods will assert in debug, and will log in release and assign the value
// to the default value. Use the rawValue methods below to assign non enumerator
// values.

- (void)setEnum:(int32_t)value forKey:(uint64_t)key;

// This method bypass the validationFunc to provide setting of values that were not
// known at the time the binary was compiled.
- (void)setRawValue:(int32_t)rawValue forKey:(uint64_t)key;

// No validation applies to these methods.

- (void)removeEnumForKey:(uint64_t)aKey;
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
+ (instancetype)dictionaryWithUInt32:(uint32_t)value
                              forKey:(int64_t)key;
+ (instancetype)dictionaryWithUInt32s:(const uint32_t [])values
                              forKeys:(const int64_t [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt64UInt32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithUInt32s:(const uint32_t [])values
                        forKeys:(const int64_t [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt64UInt32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getUInt32:(nullable uint32_t *)value forKey:(int64_t)key;

- (void)enumerateKeysAndUInt32sUsingBlock:
    (void (^)(int64_t key, uint32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt64UInt32Dictionary *)otherDictionary;

- (void)setUInt32:(uint32_t)value forKey:(int64_t)key;

- (void)removeUInt32ForKey:(int64_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int64 -> Int32

@interface GPBInt64Int32Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithInt32:(int32_t)value
                             forKey:(int64_t)key;
+ (instancetype)dictionaryWithInt32s:(const int32_t [])values
                             forKeys:(const int64_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt64Int32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithInt32s:(const int32_t [])values
                       forKeys:(const int64_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt64Int32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getInt32:(nullable int32_t *)value forKey:(int64_t)key;

- (void)enumerateKeysAndInt32sUsingBlock:
    (void (^)(int64_t key, int32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt64Int32Dictionary *)otherDictionary;

- (void)setInt32:(int32_t)value forKey:(int64_t)key;

- (void)removeInt32ForKey:(int64_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int64 -> UInt64

@interface GPBInt64UInt64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithUInt64:(uint64_t)value
                              forKey:(int64_t)key;
+ (instancetype)dictionaryWithUInt64s:(const uint64_t [])values
                              forKeys:(const int64_t [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt64UInt64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithUInt64s:(const uint64_t [])values
                        forKeys:(const int64_t [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt64UInt64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getUInt64:(nullable uint64_t *)value forKey:(int64_t)key;

- (void)enumerateKeysAndUInt64sUsingBlock:
    (void (^)(int64_t key, uint64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt64UInt64Dictionary *)otherDictionary;

- (void)setUInt64:(uint64_t)value forKey:(int64_t)key;

- (void)removeUInt64ForKey:(int64_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int64 -> Int64

@interface GPBInt64Int64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithInt64:(int64_t)value
                             forKey:(int64_t)key;
+ (instancetype)dictionaryWithInt64s:(const int64_t [])values
                             forKeys:(const int64_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt64Int64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithInt64s:(const int64_t [])values
                       forKeys:(const int64_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt64Int64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getInt64:(nullable int64_t *)value forKey:(int64_t)key;

- (void)enumerateKeysAndInt64sUsingBlock:
    (void (^)(int64_t key, int64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt64Int64Dictionary *)otherDictionary;

- (void)setInt64:(int64_t)value forKey:(int64_t)key;

- (void)removeInt64ForKey:(int64_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int64 -> Bool

@interface GPBInt64BoolDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithBool:(BOOL)value
                            forKey:(int64_t)key;
+ (instancetype)dictionaryWithBools:(const BOOL [])values
                            forKeys:(const int64_t [])keys
                              count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt64BoolDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithBools:(const BOOL [])values
                      forKeys:(const int64_t [])keys
                        count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt64BoolDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getBool:(nullable BOOL *)value forKey:(int64_t)key;

- (void)enumerateKeysAndBoolsUsingBlock:
    (void (^)(int64_t key, BOOL value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt64BoolDictionary *)otherDictionary;

- (void)setBool:(BOOL)value forKey:(int64_t)key;

- (void)removeBoolForKey:(int64_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int64 -> Float

@interface GPBInt64FloatDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithFloat:(float)value
                             forKey:(int64_t)key;
+ (instancetype)dictionaryWithFloats:(const float [])values
                             forKeys:(const int64_t [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt64FloatDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithFloats:(const float [])values
                       forKeys:(const int64_t [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt64FloatDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getFloat:(nullable float *)value forKey:(int64_t)key;

- (void)enumerateKeysAndFloatsUsingBlock:
    (void (^)(int64_t key, float value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt64FloatDictionary *)otherDictionary;

- (void)setFloat:(float)value forKey:(int64_t)key;

- (void)removeFloatForKey:(int64_t)aKey;
- (void)removeAll;

@end

#pragma mark - Int64 -> Double

@interface GPBInt64DoubleDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithDouble:(double)value
                              forKey:(int64_t)key;
+ (instancetype)dictionaryWithDoubles:(const double [])values
                              forKeys:(const int64_t [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBInt64DoubleDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithDoubles:(const double [])values
                        forKeys:(const int64_t [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBInt64DoubleDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getDouble:(nullable double *)value forKey:(int64_t)key;

- (void)enumerateKeysAndDoublesUsingBlock:
    (void (^)(int64_t key, double value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBInt64DoubleDictionary *)otherDictionary;

- (void)setDouble:(double)value forKey:(int64_t)key;

- (void)removeDoubleForKey:(int64_t)aKey;
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

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getEnum:(nullable int32_t *)value forKey:(int64_t)key;

- (void)enumerateKeysAndEnumsUsingBlock:
    (void (^)(int64_t key, int32_t value, BOOL *stop))block;

// These methods bypass the validationFunc to provide access to values that were not
// known at the time the binary was compiled.

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getRawValue:(nullable int32_t *)rawValue forKey:(int64_t)key;

- (void)enumerateKeysAndRawValuesUsingBlock:
    (void (^)(int64_t key, int32_t rawValue, BOOL *stop))block;

- (void)addRawEntriesFromDictionary:(GPBInt64EnumDictionary *)otherDictionary;

// If value is not a valid enumerator as defined by validationFunc, these
// methods will assert in debug, and will log in release and assign the value
// to the default value. Use the rawValue methods below to assign non enumerator
// values.

- (void)setEnum:(int32_t)value forKey:(int64_t)key;

// This method bypass the validationFunc to provide setting of values that were not
// known at the time the binary was compiled.
- (void)setRawValue:(int32_t)rawValue forKey:(int64_t)key;

// No validation applies to these methods.

- (void)removeEnumForKey:(int64_t)aKey;
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
+ (instancetype)dictionaryWithUInt32:(uint32_t)value
                              forKey:(BOOL)key;
+ (instancetype)dictionaryWithUInt32s:(const uint32_t [])values
                              forKeys:(const BOOL [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBBoolUInt32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithUInt32s:(const uint32_t [])values
                        forKeys:(const BOOL [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBBoolUInt32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getUInt32:(nullable uint32_t *)value forKey:(BOOL)key;

- (void)enumerateKeysAndUInt32sUsingBlock:
    (void (^)(BOOL key, uint32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBBoolUInt32Dictionary *)otherDictionary;

- (void)setUInt32:(uint32_t)value forKey:(BOOL)key;

- (void)removeUInt32ForKey:(BOOL)aKey;
- (void)removeAll;

@end

#pragma mark - Bool -> Int32

@interface GPBBoolInt32Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithInt32:(int32_t)value
                             forKey:(BOOL)key;
+ (instancetype)dictionaryWithInt32s:(const int32_t [])values
                             forKeys:(const BOOL [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBBoolInt32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithInt32s:(const int32_t [])values
                       forKeys:(const BOOL [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBBoolInt32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getInt32:(nullable int32_t *)value forKey:(BOOL)key;

- (void)enumerateKeysAndInt32sUsingBlock:
    (void (^)(BOOL key, int32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBBoolInt32Dictionary *)otherDictionary;

- (void)setInt32:(int32_t)value forKey:(BOOL)key;

- (void)removeInt32ForKey:(BOOL)aKey;
- (void)removeAll;

@end

#pragma mark - Bool -> UInt64

@interface GPBBoolUInt64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithUInt64:(uint64_t)value
                              forKey:(BOOL)key;
+ (instancetype)dictionaryWithUInt64s:(const uint64_t [])values
                              forKeys:(const BOOL [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBBoolUInt64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithUInt64s:(const uint64_t [])values
                        forKeys:(const BOOL [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBBoolUInt64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getUInt64:(nullable uint64_t *)value forKey:(BOOL)key;

- (void)enumerateKeysAndUInt64sUsingBlock:
    (void (^)(BOOL key, uint64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBBoolUInt64Dictionary *)otherDictionary;

- (void)setUInt64:(uint64_t)value forKey:(BOOL)key;

- (void)removeUInt64ForKey:(BOOL)aKey;
- (void)removeAll;

@end

#pragma mark - Bool -> Int64

@interface GPBBoolInt64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithInt64:(int64_t)value
                             forKey:(BOOL)key;
+ (instancetype)dictionaryWithInt64s:(const int64_t [])values
                             forKeys:(const BOOL [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBBoolInt64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithInt64s:(const int64_t [])values
                       forKeys:(const BOOL [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBBoolInt64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getInt64:(nullable int64_t *)value forKey:(BOOL)key;

- (void)enumerateKeysAndInt64sUsingBlock:
    (void (^)(BOOL key, int64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBBoolInt64Dictionary *)otherDictionary;

- (void)setInt64:(int64_t)value forKey:(BOOL)key;

- (void)removeInt64ForKey:(BOOL)aKey;
- (void)removeAll;

@end

#pragma mark - Bool -> Bool

@interface GPBBoolBoolDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithBool:(BOOL)value
                            forKey:(BOOL)key;
+ (instancetype)dictionaryWithBools:(const BOOL [])values
                            forKeys:(const BOOL [])keys
                              count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBBoolBoolDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithBools:(const BOOL [])values
                      forKeys:(const BOOL [])keys
                        count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBBoolBoolDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getBool:(nullable BOOL *)value forKey:(BOOL)key;

- (void)enumerateKeysAndBoolsUsingBlock:
    (void (^)(BOOL key, BOOL value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBBoolBoolDictionary *)otherDictionary;

- (void)setBool:(BOOL)value forKey:(BOOL)key;

- (void)removeBoolForKey:(BOOL)aKey;
- (void)removeAll;

@end

#pragma mark - Bool -> Float

@interface GPBBoolFloatDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithFloat:(float)value
                             forKey:(BOOL)key;
+ (instancetype)dictionaryWithFloats:(const float [])values
                             forKeys:(const BOOL [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBBoolFloatDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithFloats:(const float [])values
                       forKeys:(const BOOL [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBBoolFloatDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getFloat:(nullable float *)value forKey:(BOOL)key;

- (void)enumerateKeysAndFloatsUsingBlock:
    (void (^)(BOOL key, float value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBBoolFloatDictionary *)otherDictionary;

- (void)setFloat:(float)value forKey:(BOOL)key;

- (void)removeFloatForKey:(BOOL)aKey;
- (void)removeAll;

@end

#pragma mark - Bool -> Double

@interface GPBBoolDoubleDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithDouble:(double)value
                              forKey:(BOOL)key;
+ (instancetype)dictionaryWithDoubles:(const double [])values
                              forKeys:(const BOOL [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBBoolDoubleDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithDoubles:(const double [])values
                        forKeys:(const BOOL [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBBoolDoubleDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getDouble:(nullable double *)value forKey:(BOOL)key;

- (void)enumerateKeysAndDoublesUsingBlock:
    (void (^)(BOOL key, double value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBBoolDoubleDictionary *)otherDictionary;

- (void)setDouble:(double)value forKey:(BOOL)key;

- (void)removeDoubleForKey:(BOOL)aKey;
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

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getEnum:(nullable int32_t *)value forKey:(BOOL)key;

- (void)enumerateKeysAndEnumsUsingBlock:
    (void (^)(BOOL key, int32_t value, BOOL *stop))block;

// These methods bypass the validationFunc to provide access to values that were not
// known at the time the binary was compiled.

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getRawValue:(nullable int32_t *)rawValue forKey:(BOOL)key;

- (void)enumerateKeysAndRawValuesUsingBlock:
    (void (^)(BOOL key, int32_t rawValue, BOOL *stop))block;

- (void)addRawEntriesFromDictionary:(GPBBoolEnumDictionary *)otherDictionary;

// If value is not a valid enumerator as defined by validationFunc, these
// methods will assert in debug, and will log in release and assign the value
// to the default value. Use the rawValue methods below to assign non enumerator
// values.

- (void)setEnum:(int32_t)value forKey:(BOOL)key;

// This method bypass the validationFunc to provide setting of values that were not
// known at the time the binary was compiled.
- (void)setRawValue:(int32_t)rawValue forKey:(BOOL)key;

// No validation applies to these methods.

- (void)removeEnumForKey:(BOOL)aKey;
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
+ (instancetype)dictionaryWithUInt32:(uint32_t)value
                              forKey:(NSString *)key;
+ (instancetype)dictionaryWithUInt32s:(const uint32_t [])values
                              forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBStringUInt32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithUInt32s:(const uint32_t [])values
                        forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBStringUInt32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getUInt32:(nullable uint32_t *)value forKey:(NSString *)key;

- (void)enumerateKeysAndUInt32sUsingBlock:
    (void (^)(NSString *key, uint32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBStringUInt32Dictionary *)otherDictionary;

- (void)setUInt32:(uint32_t)value forKey:(NSString *)key;

- (void)removeUInt32ForKey:(NSString *)aKey;
- (void)removeAll;

@end

#pragma mark - String -> Int32

@interface GPBStringInt32Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithInt32:(int32_t)value
                             forKey:(NSString *)key;
+ (instancetype)dictionaryWithInt32s:(const int32_t [])values
                             forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBStringInt32Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithInt32s:(const int32_t [])values
                       forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBStringInt32Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getInt32:(nullable int32_t *)value forKey:(NSString *)key;

- (void)enumerateKeysAndInt32sUsingBlock:
    (void (^)(NSString *key, int32_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBStringInt32Dictionary *)otherDictionary;

- (void)setInt32:(int32_t)value forKey:(NSString *)key;

- (void)removeInt32ForKey:(NSString *)aKey;
- (void)removeAll;

@end

#pragma mark - String -> UInt64

@interface GPBStringUInt64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithUInt64:(uint64_t)value
                              forKey:(NSString *)key;
+ (instancetype)dictionaryWithUInt64s:(const uint64_t [])values
                              forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBStringUInt64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithUInt64s:(const uint64_t [])values
                        forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBStringUInt64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getUInt64:(nullable uint64_t *)value forKey:(NSString *)key;

- (void)enumerateKeysAndUInt64sUsingBlock:
    (void (^)(NSString *key, uint64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBStringUInt64Dictionary *)otherDictionary;

- (void)setUInt64:(uint64_t)value forKey:(NSString *)key;

- (void)removeUInt64ForKey:(NSString *)aKey;
- (void)removeAll;

@end

#pragma mark - String -> Int64

@interface GPBStringInt64Dictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithInt64:(int64_t)value
                             forKey:(NSString *)key;
+ (instancetype)dictionaryWithInt64s:(const int64_t [])values
                             forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBStringInt64Dictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithInt64s:(const int64_t [])values
                       forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBStringInt64Dictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getInt64:(nullable int64_t *)value forKey:(NSString *)key;

- (void)enumerateKeysAndInt64sUsingBlock:
    (void (^)(NSString *key, int64_t value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBStringInt64Dictionary *)otherDictionary;

- (void)setInt64:(int64_t)value forKey:(NSString *)key;

- (void)removeInt64ForKey:(NSString *)aKey;
- (void)removeAll;

@end

#pragma mark - String -> Bool

@interface GPBStringBoolDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithBool:(BOOL)value
                            forKey:(NSString *)key;
+ (instancetype)dictionaryWithBools:(const BOOL [])values
                            forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                              count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBStringBoolDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithBools:(const BOOL [])values
                      forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                        count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBStringBoolDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getBool:(nullable BOOL *)value forKey:(NSString *)key;

- (void)enumerateKeysAndBoolsUsingBlock:
    (void (^)(NSString *key, BOOL value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBStringBoolDictionary *)otherDictionary;

- (void)setBool:(BOOL)value forKey:(NSString *)key;

- (void)removeBoolForKey:(NSString *)aKey;
- (void)removeAll;

@end

#pragma mark - String -> Float

@interface GPBStringFloatDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithFloat:(float)value
                             forKey:(NSString *)key;
+ (instancetype)dictionaryWithFloats:(const float [])values
                             forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                               count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBStringFloatDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithFloats:(const float [])values
                       forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                         count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBStringFloatDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getFloat:(nullable float *)value forKey:(NSString *)key;

- (void)enumerateKeysAndFloatsUsingBlock:
    (void (^)(NSString *key, float value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBStringFloatDictionary *)otherDictionary;

- (void)setFloat:(float)value forKey:(NSString *)key;

- (void)removeFloatForKey:(NSString *)aKey;
- (void)removeAll;

@end

#pragma mark - String -> Double

@interface GPBStringDoubleDictionary : NSObject <NSCopying>

@property(nonatomic, readonly) NSUInteger count;

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithDouble:(double)value
                              forKey:(NSString *)key;
+ (instancetype)dictionaryWithDoubles:(const double [])values
                              forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                                count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(GPBStringDoubleDictionary *)dictionary;
+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;

- (instancetype)initWithDoubles:(const double [])values
                        forKeys:(const NSString * GPB_UNSAFE_UNRETAINED [])keys
                          count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithDictionary:(GPBStringDoubleDictionary *)dictionary;
- (instancetype)initWithCapacity:(NSUInteger)numItems;

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getDouble:(nullable double *)value forKey:(NSString *)key;

- (void)enumerateKeysAndDoublesUsingBlock:
    (void (^)(NSString *key, double value, BOOL *stop))block;

- (void)addEntriesFromDictionary:(GPBStringDoubleDictionary *)otherDictionary;

- (void)setDouble:(double)value forKey:(NSString *)key;

- (void)removeDoubleForKey:(NSString *)aKey;
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

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getEnum:(nullable int32_t *)value forKey:(NSString *)key;

- (void)enumerateKeysAndEnumsUsingBlock:
    (void (^)(NSString *key, int32_t value, BOOL *stop))block;

// These methods bypass the validationFunc to provide access to values that were not
// known at the time the binary was compiled.

// Returns YES/NO to indicate if the key was found or not, filling in the value
// only when the key was found.
- (BOOL)getRawValue:(nullable int32_t *)rawValue forKey:(NSString *)key;

- (void)enumerateKeysAndRawValuesUsingBlock:
    (void (^)(NSString *key, int32_t rawValue, BOOL *stop))block;

- (void)addRawEntriesFromDictionary:(GPBStringEnumDictionary *)otherDictionary;

// If value is not a valid enumerator as defined by validationFunc, these
// methods will assert in debug, and will log in release and assign the value
// to the default value. Use the rawValue methods below to assign non enumerator
// values.

- (void)setEnum:(int32_t)value forKey:(NSString *)key;

// This method bypass the validationFunc to provide setting of values that were not
// known at the time the binary was compiled.
- (void)setRawValue:(int32_t)rawValue forKey:(NSString *)key;

// No validation applies to these methods.

- (void)removeEnumForKey:(NSString *)aKey;
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
//%DICTIONARY_COMMON_INTERFACE(KEY_NAME, KEY_TYPE, KisP, KHELPER, VALUE_NAME, VALUE_TYPE, POD, VALUE_NAME, value)
//%PDDM-DEFINE DICTIONARY_POD_KEY_TO_OBJECT_INTERFACE(KEY_NAME, KEY_TYPE, VALUE_NAME, VALUE_TYPE)
//%DICTIONARY_COMMON_INTERFACE(KEY_NAME, KEY_TYPE, , POD, VALUE_NAME, VALUE_TYPE, OBJECT, Object, object)
//%PDDM-DEFINE VALUE_FOR_KEY_POD(KEY_TYPE, VALUE_TYPE, VNAME)
//%// Returns YES/NO to indicate if the key was found or not, filling in the value
//%// only when the key was found.
//%- (BOOL)get##VNAME##:(nullable VALUE_TYPE *)value forKey:(KEY_TYPE)key;
//%PDDM-DEFINE VALUE_FOR_KEY_OBJECT(KEY_TYPE, VALUE_TYPE, VNAME)
//%- (VALUE_TYPE)objectForKey:(KEY_TYPE)key;
//%PDDM-DEFINE VALUE_FOR_KEY_Enum(KEY_TYPE, VALUE_TYPE, VNAME)
//%VALUE_FOR_KEY_POD(KEY_TYPE, VALUE_TYPE, VNAME)
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
//%PDDM-DEFINE DICTIONARY_COMMON_INTERFACE(KEY_NAME, KEY_TYPE, KisP, KHELPER, VALUE_NAME, VALUE_TYPE, VHELPER, VNAME, VNAME_VAR)
//%#pragma mark - KEY_NAME -> VALUE_NAME
//%
//%@interface DICTIONARY_CLASS_DECL##VHELPER(KEY_NAME, VALUE_NAME, VALUE_TYPE) : NSObject <NSCopying>
//%
//%@property(nonatomic, readonly) NSUInteger count;
//%
//%+ (instancetype)dictionary;
//%+ (instancetype)dictionaryWith##VNAME##:(VALUE_TYPE)##VNAME_VAR
//%                       ##VNAME$S## forKey:(KEY_TYPE##KisP$S##KisP)key;
//%+ (instancetype)dictionaryWith##VNAME##s:(const VALUE_TYPE ARRAY_ARG_MODIFIER##VHELPER()[])##VNAME_VAR##s
//%                      ##VNAME$S##  forKeys:(const KEY_TYPE##KisP$S##KisP ARRAY_ARG_MODIFIER##KHELPER()[])keys
//%                      ##VNAME$S##    count:(NSUInteger)count;
//%+ (instancetype)dictionaryWithDictionary:(GPB##KEY_NAME##VALUE_NAME##Dictionary *)dictionary;
//%+ (instancetype)dictionaryWithCapacity:(NSUInteger)numItems;
//%
//%- (instancetype)initWith##VNAME##s:(const VALUE_TYPE ARRAY_ARG_MODIFIER##VHELPER()[])##VNAME_VAR##s
//%                ##VNAME$S##  forKeys:(const KEY_TYPE##KisP$S##KisP ARRAY_ARG_MODIFIER##KHELPER()[])keys
//%                ##VNAME$S##    count:(NSUInteger)count NS_DESIGNATED_INITIALIZER;
//%- (instancetype)initWithDictionary:(GPB##KEY_NAME##VALUE_NAME##Dictionary *)dictionary;
//%- (instancetype)initWithCapacity:(NSUInteger)numItems;
//%
//%DICTIONARY_IMMUTABLE_INTERFACE(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, VHELPER, VNAME, VNAME_VAR)
//%
//%- (void)addEntriesFromDictionary:(GPB##KEY_NAME##VALUE_NAME##Dictionary *)otherDictionary;
//%
//%DICTIONARY_MUTABLE_INTERFACE(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, VHELPER, VNAME, VNAME_VAR)
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
//%DICTIONARY_IMMUTABLE_INTERFACE(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, VHELPER, Enum, value)
//%
//%// These methods bypass the validationFunc to provide access to values that were not
//%// known at the time the binary was compiled.
//%
//%// Returns YES/NO to indicate if the key was found or not, filling in the value
//%// only when the key was found.
//%- (BOOL)getRawValue:(nullable VALUE_TYPE *)rawValue forKey:(KEY_TYPE##KisP$S##KisP)key;
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
//%DICTIONARY_MUTABLE_INTERFACE(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, VHELPER, Enum, value)
//%
//%@end
//%

//%PDDM-DEFINE DICTIONARY_IMMUTABLE_INTERFACE(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, VHELPER, VNAME, VNAME_VAR)
//%VALUE_FOR_KEY_##VHELPER(KEY_TYPE##KisP$S##KisP, VALUE_TYPE, VNAME)
//%
//%- (void)enumerateKeysAnd##VNAME##sUsingBlock:
//%    (void (^)(KEY_TYPE KisP##key, VALUE_TYPE VNAME_VAR, BOOL *stop))block;

//%PDDM-DEFINE DICTIONARY_MUTABLE_INTERFACE(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE, VHELPER, VNAME, VNAME_VAR)
//%- (void)set##VNAME##:(VALUE_TYPE)##VNAME_VAR forKey:(KEY_TYPE##KisP$S##KisP)key;
//%DICTIONARY_EXTRA_MUTABLE_METHODS_##VHELPER(KEY_NAME, KEY_TYPE, KisP, VALUE_NAME, VALUE_TYPE)
//%- (void)remove##VNAME##ForKey:(KEY_TYPE##KisP$S##KisP)aKey;
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
