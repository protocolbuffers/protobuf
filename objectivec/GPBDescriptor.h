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

#import "GPBTypes.h"

@class GPBEnumDescriptor;
@class GPBFieldDescriptor;
@class GPBFieldOptions;
@class GPBFileDescriptor;
@class GPBOneofDescriptor;

typedef NS_ENUM(NSInteger, GPBFileSyntax) {
  GPBFileSyntaxUnknown = 0,
  GPBFileSyntaxProto2 = 2,
  GPBFileSyntaxProto3 = 3,
};

typedef NS_ENUM(NSInteger, GPBFieldType) {
  GPBFieldTypeSingle,    // optional/required
  GPBFieldTypeRepeated,  // repeated
  GPBFieldTypeMap,       // map<K,V>
};

@interface GPBDescriptor : NSObject<NSCopying>

@property(nonatomic, readonly, copy) NSString *name;
@property(nonatomic, readonly, strong) NSArray *fields;
@property(nonatomic, readonly, strong) NSArray *oneofs;
@property(nonatomic, readonly, strong) NSArray *enums;
@property(nonatomic, readonly) const GPBExtensionRange *extensionRanges;
@property(nonatomic, readonly) NSUInteger extensionRangesCount;
@property(nonatomic, readonly, assign) GPBFileDescriptor *file;

@property(nonatomic, readonly, getter=isWireFormat) BOOL wireFormat;
@property(nonatomic, readonly) Class messageClass;

- (GPBFieldDescriptor *)fieldWithNumber:(uint32_t)fieldNumber;
- (GPBFieldDescriptor *)fieldWithName:(NSString *)name;
- (GPBOneofDescriptor *)oneofWithName:(NSString *)name;
- (GPBEnumDescriptor *)enumWithName:(NSString *)name;

@end

@interface GPBFileDescriptor : NSObject

@property(nonatomic, readonly, copy) NSString *package;
@property(nonatomic, readonly) GPBFileSyntax syntax;

@end

@interface GPBOneofDescriptor : NSObject
@property(nonatomic, readonly) NSString *name;
@property(nonatomic, readonly) NSArray *fields;

- (GPBFieldDescriptor *)fieldWithNumber:(uint32_t)fieldNumber;
- (GPBFieldDescriptor *)fieldWithName:(NSString *)name;
@end

@interface GPBFieldDescriptor : NSObject

@property(nonatomic, readonly, copy) NSString *name;
@property(nonatomic, readonly) uint32_t number;
@property(nonatomic, readonly) GPBType type;
@property(nonatomic, readonly) BOOL hasDefaultValue;
@property(nonatomic, readonly) GPBValue defaultValue;
@property(nonatomic, readonly, getter=isRequired) BOOL required;
@property(nonatomic, readonly, getter=isOptional) BOOL optional;
@property(nonatomic, readonly) GPBFieldType fieldType;
// If it is a map, the value type is in -type.
@property(nonatomic, readonly) GPBType mapKeyType;
@property(nonatomic, readonly, getter=isPackable) BOOL packable;

@property(nonatomic, readonly, assign) GPBOneofDescriptor *containingOneof;

@property(nonatomic, readonly) GPBFieldOptions *fieldOptions;

// Message properties
@property(nonatomic, readonly, assign) Class msgClass;

// Enum properties
@property(nonatomic, readonly, strong) GPBEnumDescriptor *enumDescriptor;

- (BOOL)isValidEnumValue:(int32_t)value;

// For now, this will return nil if it doesn't know the name to use for
// TextFormat.
- (NSString *)textFormatName;

@end

@interface GPBEnumDescriptor : NSObject

@property(nonatomic, readonly, copy) NSString *name;
@property(nonatomic, readonly) GPBEnumValidationFunc enumVerifier;

- (NSString *)enumNameForValue:(int32_t)number;
- (BOOL)getValue:(int32_t *)outValue forEnumName:(NSString *)name;

- (NSString *)textFormatNameForValue:(int32_t)number;

@end

@interface GPBExtensionDescriptor : NSObject
@property(nonatomic, readonly) uint32_t fieldNumber;
@property(nonatomic, readonly) GPBType type;
@property(nonatomic, readonly, getter=isRepeated) BOOL repeated;
@property(nonatomic, readonly, getter=isPackable) BOOL packable;
@property(nonatomic, readonly, assign) Class msgClass;
@property(nonatomic, readonly) NSString *singletonName;
@property(nonatomic, readonly, strong) GPBEnumDescriptor *enumDescriptor;
@end
