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

#import "GPBDictionary.h"

@class GPBCodedInputStream;
@class GPBCodedOutputStream;
@class GPBExtensionRegistry;
@class GPBFieldDescriptor;

//%PDDM-DEFINE DICTIONARY_PRIV_INTERFACES_FOR_POD_KEY(KEY_NAME)
//%DICTIONARY_POD_PRIV_INTERFACES_FOR_KEY(KEY_NAME)
//%DICTIONARY_PRIVATE_INTERFACES(KEY_NAME, Object, Object)
//%PDDM-DEFINE DICTIONARY_POD_PRIV_INTERFACES_FOR_KEY(KEY_NAME)
//%DICTIONARY_PRIVATE_INTERFACES(KEY_NAME, UInt32, Basic)
//%DICTIONARY_PRIVATE_INTERFACES(KEY_NAME, Int32, Basic)
//%DICTIONARY_PRIVATE_INTERFACES(KEY_NAME, UInt64, Basic)
//%DICTIONARY_PRIVATE_INTERFACES(KEY_NAME, Int64, Basic)
//%DICTIONARY_PRIVATE_INTERFACES(KEY_NAME, Bool, Basic)
//%DICTIONARY_PRIVATE_INTERFACES(KEY_NAME, Float, Basic)
//%DICTIONARY_PRIVATE_INTERFACES(KEY_NAME, Double, Basic)
//%DICTIONARY_PRIVATE_INTERFACES(KEY_NAME, Enum, Enum)

//%PDDM-DEFINE DICTIONARY_PRIVATE_INTERFACES(KEY_NAME, VALUE_NAME, HELPER)
//%@interface GPB##KEY_NAME##VALUE_NAME##Dictionary ()
//%- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
//%- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
//%                         asField:(GPBFieldDescriptor *)field;
//%- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
//%- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
//%EXTRA_DICTIONARY_PRIVATE_INTERFACES_##HELPER()@end
//%

//%PDDM-DEFINE EXTRA_DICTIONARY_PRIVATE_INTERFACES_Basic()
// Empty
//%PDDM-DEFINE EXTRA_DICTIONARY_PRIVATE_INTERFACES_Object()
//%- (BOOL)isInitialized;
//%- (instancetype)deepCopyWithZone:(NSZone *)zone
//%    __attribute__((ns_returns_retained));
//%
//%PDDM-DEFINE EXTRA_DICTIONARY_PRIVATE_INTERFACES_Enum()
//%- (NSData *)serializedDataForUnknownValue:(int32_t)value
//%                                   forKey:(GPBValue *)key
//%                                  keyType:(GPBType)keyType;
//%

//%PDDM-EXPAND DICTIONARY_PRIV_INTERFACES_FOR_POD_KEY(UInt32)
// This block of code is generated, do not edit it directly.

@interface GPBUInt32UInt32Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBUInt32Int32Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBUInt32UInt64Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBUInt32Int64Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBUInt32BoolDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBUInt32FloatDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBUInt32DoubleDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBUInt32EnumDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
- (NSData *)serializedDataForUnknownValue:(int32_t)value
                                   forKey:(GPBValue *)key
                                  keyType:(GPBType)keyType;
@end

@interface GPBUInt32ObjectDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
- (BOOL)isInitialized;
- (instancetype)deepCopyWithZone:(NSZone *)zone
    __attribute__((ns_returns_retained));
@end

//%PDDM-EXPAND DICTIONARY_PRIV_INTERFACES_FOR_POD_KEY(Int32)
// This block of code is generated, do not edit it directly.

@interface GPBInt32UInt32Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBInt32Int32Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBInt32UInt64Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBInt32Int64Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBInt32BoolDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBInt32FloatDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBInt32DoubleDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBInt32EnumDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
- (NSData *)serializedDataForUnknownValue:(int32_t)value
                                   forKey:(GPBValue *)key
                                  keyType:(GPBType)keyType;
@end

@interface GPBInt32ObjectDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
- (BOOL)isInitialized;
- (instancetype)deepCopyWithZone:(NSZone *)zone
    __attribute__((ns_returns_retained));
@end

//%PDDM-EXPAND DICTIONARY_PRIV_INTERFACES_FOR_POD_KEY(UInt64)
// This block of code is generated, do not edit it directly.

@interface GPBUInt64UInt32Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBUInt64Int32Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBUInt64UInt64Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBUInt64Int64Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBUInt64BoolDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBUInt64FloatDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBUInt64DoubleDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBUInt64EnumDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
- (NSData *)serializedDataForUnknownValue:(int32_t)value
                                   forKey:(GPBValue *)key
                                  keyType:(GPBType)keyType;
@end

@interface GPBUInt64ObjectDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
- (BOOL)isInitialized;
- (instancetype)deepCopyWithZone:(NSZone *)zone
    __attribute__((ns_returns_retained));
@end

//%PDDM-EXPAND DICTIONARY_PRIV_INTERFACES_FOR_POD_KEY(Int64)
// This block of code is generated, do not edit it directly.

@interface GPBInt64UInt32Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBInt64Int32Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBInt64UInt64Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBInt64Int64Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBInt64BoolDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBInt64FloatDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBInt64DoubleDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBInt64EnumDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
- (NSData *)serializedDataForUnknownValue:(int32_t)value
                                   forKey:(GPBValue *)key
                                  keyType:(GPBType)keyType;
@end

@interface GPBInt64ObjectDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
- (BOOL)isInitialized;
- (instancetype)deepCopyWithZone:(NSZone *)zone
    __attribute__((ns_returns_retained));
@end

//%PDDM-EXPAND DICTIONARY_PRIV_INTERFACES_FOR_POD_KEY(Bool)
// This block of code is generated, do not edit it directly.

@interface GPBBoolUInt32Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBBoolInt32Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBBoolUInt64Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBBoolInt64Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBBoolBoolDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBBoolFloatDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBBoolDoubleDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBBoolEnumDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
- (NSData *)serializedDataForUnknownValue:(int32_t)value
                                   forKey:(GPBValue *)key
                                  keyType:(GPBType)keyType;
@end

@interface GPBBoolObjectDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
- (BOOL)isInitialized;
- (instancetype)deepCopyWithZone:(NSZone *)zone
    __attribute__((ns_returns_retained));
@end

//%PDDM-EXPAND DICTIONARY_POD_PRIV_INTERFACES_FOR_KEY(String)
// This block of code is generated, do not edit it directly.

@interface GPBStringUInt32Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBStringInt32Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBStringUInt64Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBStringInt64Dictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBStringBoolDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBStringFloatDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBStringDoubleDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

@interface GPBStringEnumDictionary ()
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBValue:(GPBValue *)value forGPBValueKey:(GPBValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
- (NSData *)serializedDataForUnknownValue:(int32_t)value
                                   forKey:(GPBValue *)key
                                  keyType:(GPBType)keyType;
@end

//%PDDM-EXPAND-END (6 expansions)

CF_EXTERN_C_BEGIN

// Helper to compute size when an NSDictionary is used for the map instead
// of a custom type.
size_t GPBDictionaryComputeSizeInternalHelper(NSDictionary *dict,
                                              GPBFieldDescriptor *field);

// Helper to write out when an NSDictionary is used for the map instead
// of a custom type.
void GPBDictionaryWriteToStreamInternalHelper(
    GPBCodedOutputStream *outputStream, NSDictionary *dict,
    GPBFieldDescriptor *field);

// Helper to check message initialization when an NSDictionary is used for
// the map instead of a custom type.
BOOL GPBDictionaryIsInitializedInternalHelper(NSDictionary *dict,
                                              GPBFieldDescriptor *field);

// Helper to read a map instead.
void GPBDictionaryReadEntry(id mapDictionary, GPBCodedInputStream *stream,
                            GPBExtensionRegistry *registry,
                            GPBFieldDescriptor *field,
                            GPBMessage *parentMessage);

CF_EXTERN_C_END
