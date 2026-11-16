// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import <Foundation/Foundation.h>

#import "GPBDictionary.h"

#import "GPBCodedInputStream.h"
#import "GPBCodedOutputStream.h"
#import "GPBDescriptor.h"
#import "GPBExtensionRegistry.h"

@protocol GPBDictionaryInternalsProtocol
- (size_t)computeSerializedSizeAsField:(GPBFieldDescriptor *)field;
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)outputStream
                         asField:(GPBFieldDescriptor *)field;
- (void)setGPBGenericValue:(GPBGenericValue *)value forGPBGenericValueKey:(GPBGenericValue *)key;
- (void)enumerateForTextFormat:(void (^)(id keyObj, id valueObj))block;
@end

// @@protoc_objc_autogen: GPBDictionary_PackagePrivate.h:private_interfaces
// This block of code is generated, do not edit it directly.

@interface GPBUInt32UInt32Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBUInt32Int32Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBUInt32UInt64Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBUInt32Int64Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBUInt32BoolDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBUInt32FloatDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBUInt32DoubleDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBUInt32EnumDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
- (NSData *)serializedDataForUnknownValue:(int32_t)value
                                   forKey:(GPBGenericValue *)key
                              keyDataType:(GPBDataType)keyDataType;
@end

@interface GPBUInt32ObjectDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
- (BOOL)isInitialized;
- (instancetype)deepCopyWithZone:(NSZone *)zone __attribute__((ns_returns_retained));
@end

@interface GPBInt32UInt32Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBInt32Int32Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBInt32UInt64Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBInt32Int64Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBInt32BoolDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBInt32FloatDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBInt32DoubleDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBInt32EnumDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
- (NSData *)serializedDataForUnknownValue:(int32_t)value
                                   forKey:(GPBGenericValue *)key
                              keyDataType:(GPBDataType)keyDataType;
@end

@interface GPBInt32ObjectDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
- (BOOL)isInitialized;
- (instancetype)deepCopyWithZone:(NSZone *)zone __attribute__((ns_returns_retained));
@end

@interface GPBUInt64UInt32Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBUInt64Int32Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBUInt64UInt64Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBUInt64Int64Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBUInt64BoolDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBUInt64FloatDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBUInt64DoubleDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBUInt64EnumDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
- (NSData *)serializedDataForUnknownValue:(int32_t)value
                                   forKey:(GPBGenericValue *)key
                              keyDataType:(GPBDataType)keyDataType;
@end

@interface GPBUInt64ObjectDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
- (BOOL)isInitialized;
- (instancetype)deepCopyWithZone:(NSZone *)zone __attribute__((ns_returns_retained));
@end

@interface GPBInt64UInt32Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBInt64Int32Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBInt64UInt64Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBInt64Int64Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBInt64BoolDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBInt64FloatDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBInt64DoubleDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBInt64EnumDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
- (NSData *)serializedDataForUnknownValue:(int32_t)value
                                   forKey:(GPBGenericValue *)key
                              keyDataType:(GPBDataType)keyDataType;
@end

@interface GPBInt64ObjectDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
- (BOOL)isInitialized;
- (instancetype)deepCopyWithZone:(NSZone *)zone __attribute__((ns_returns_retained));
@end

@interface GPBBoolUInt32Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBBoolInt32Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBBoolUInt64Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBBoolInt64Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBBoolBoolDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBBoolFloatDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBBoolDoubleDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBBoolEnumDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
- (NSData *)serializedDataForUnknownValue:(int32_t)value
                                   forKey:(GPBGenericValue *)key
                              keyDataType:(GPBDataType)keyDataType;
@end

@interface GPBBoolObjectDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
- (BOOL)isInitialized;
- (instancetype)deepCopyWithZone:(NSZone *)zone __attribute__((ns_returns_retained));
@end

@interface GPBStringUInt32Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBStringInt32Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBStringUInt64Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBStringInt64Dictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBStringBoolDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBStringFloatDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBStringDoubleDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

@interface GPBStringEnumDictionary () <GPBDictionaryInternalsProtocol> {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
- (NSData *)serializedDataForUnknownValue:(int32_t)value
                                   forKey:(GPBGenericValue *)key
                              keyDataType:(GPBDataType)keyDataType;
@end
// @@protoc_objc_autogen_end

#pragma mark - NSDictionary Subclass

@interface GPBAutocreatedDictionary : NSMutableDictionary {
 @package
  GPB_UNSAFE_UNRETAINED GPBMessage *_autocreator;
}
@end

#pragma mark - Helpers

CF_EXTERN_C_BEGIN

// Helper to compute size when an NSDictionary is used for the map instead
// of a custom type.
size_t GPBDictionaryComputeSizeInternalHelper(NSDictionary *dict, GPBFieldDescriptor *field);

// Helper to write out when an NSDictionary is used for the map instead
// of a custom type.
void GPBDictionaryWriteToStreamInternalHelper(GPBCodedOutputStream *outputStream,
                                              NSDictionary *dict, GPBFieldDescriptor *field);

// Helper to check message initialization when an NSDictionary is used for
// the map instead of a custom type.
BOOL GPBDictionaryIsInitializedInternalHelper(NSDictionary *dict, GPBFieldDescriptor *field);

// Helper to read a map instead.
void GPBDictionaryReadEntry(id mapDictionary, GPBCodedInputStream *stream,
                            id<GPBExtensionRegistry> registry, GPBFieldDescriptor *field,
                            GPBMessage *parentMessage);

CF_EXTERN_C_END
