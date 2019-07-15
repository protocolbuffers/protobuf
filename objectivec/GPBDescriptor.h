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

@class GPBEnumDescriptor;
@class GPBFieldDescriptor;
@class GPBFileDescriptor;
@class GPBOneofDescriptor;

NS_ASSUME_NONNULL_BEGIN

/** Syntax used in the proto file. */
typedef NS_ENUM(uint8_t, GPBFileSyntax) {
  /** Unknown syntax. */
  GPBFileSyntaxUnknown = 0,
  /** Proto2 syntax. */
  GPBFileSyntaxProto2 = 2,
  /** Proto3 syntax. */
  GPBFileSyntaxProto3 = 3,
};

/** Type of proto field. */
typedef NS_ENUM(uint8_t, GPBFieldType) {
  /** Optional/required field. Only valid for proto2 fields. */
  GPBFieldTypeSingle,
  /** Repeated field. */
  GPBFieldTypeRepeated,
  /** Map field. */
  GPBFieldTypeMap,
};

/**
 * Describes a proto message.
 **/
@interface GPBDescriptor : NSObject<NSCopying>

/** Name of the message. */
@property(nonatomic, readonly, copy) NSString *name;
/** Fields declared in the message. */
@property(nonatomic, readonly, strong, nullable) NSArray<GPBFieldDescriptor*> *fields;
/** Oneofs declared in the message. */
@property(nonatomic, readonly, strong, nullable) NSArray<GPBOneofDescriptor*> *oneofs;
/** Extension range declared for the message. */
@property(nonatomic, readonly, nullable) const GPBExtensionRange *extensionRanges;
/** Number of extension ranges declared for the message. */
@property(nonatomic, readonly) uint32_t extensionRangesCount;
/** Descriptor for the file where the message was defined. */
@property(nonatomic, readonly) GPBFileDescriptor *file;

/** Whether the message is in wire format or not. */
@property(nonatomic, readonly, getter=isWireFormat) BOOL wireFormat;
/** The class of this message. */
@property(nonatomic, readonly) Class messageClass;
/** Containing message descriptor if this message is nested, or nil otherwise. */
@property(readonly, nullable) GPBDescriptor *containingType;
/**
 * Fully qualified name for this message (package.message). Can be nil if the
 * value is unable to be computed.
 */
@property(readonly, nullable) NSString *fullName;

/**
 * Gets the field for the given number.
 *
 * @param fieldNumber The number for the field to get.
 *
 * @return The field descriptor for the given number, or nil if not found.
 **/
- (nullable GPBFieldDescriptor *)fieldWithNumber:(uint32_t)fieldNumber;

/**
 * Gets the field for the given name.
 *
 * @param name The name for the field to get.
 *
 * @return The field descriptor for the given name, or nil if not found.
 **/
- (nullable GPBFieldDescriptor *)fieldWithName:(NSString *)name;

/**
 * Gets the oneof for the given name.
 *
 * @param name The name for the oneof to get.
 *
 * @return The oneof descriptor for the given name, or nil if not found.
 **/
- (nullable GPBOneofDescriptor *)oneofWithName:(NSString *)name;

@end

/**
 * Describes a proto file.
 **/
@interface GPBFileDescriptor : NSObject

/** The package declared in the proto file. */
@property(nonatomic, readonly, copy) NSString *package;
/** The objc prefix declared in the proto file. */
@property(nonatomic, readonly, copy, nullable) NSString *objcPrefix;
/** The syntax of the proto file. */
@property(nonatomic, readonly) GPBFileSyntax syntax;

@end

/**
 * Describes a oneof field.
 **/
@interface GPBOneofDescriptor : NSObject
/** Name of the oneof field. */
@property(nonatomic, readonly) NSString *name;
/** Fields declared in the oneof. */
@property(nonatomic, readonly) NSArray<GPBFieldDescriptor*> *fields;

/**
 * Gets the field for the given number.
 *
 * @param fieldNumber The number for the field to get.
 *
 * @return The field descriptor for the given number, or nil if not found.
 **/
- (nullable GPBFieldDescriptor *)fieldWithNumber:(uint32_t)fieldNumber;

/**
 * Gets the field for the given name.
 *
 * @param name The name for the field to get.
 *
 * @return The field descriptor for the given name, or nil if not found.
 **/
- (nullable GPBFieldDescriptor *)fieldWithName:(NSString *)name;

@end

/**
 * Describes a proto field.
 **/
@interface GPBFieldDescriptor : NSObject

/** Name of the field. */
@property(nonatomic, readonly, copy) NSString *name;
/** Number associated with the field. */
@property(nonatomic, readonly) uint32_t number;
/** Data type contained in the field. */
@property(nonatomic, readonly) GPBDataType dataType;
/** Whether it has a default value or not. */
@property(nonatomic, readonly) BOOL hasDefaultValue;
/** Default value for the field. */
@property(nonatomic, readonly) GPBGenericValue defaultValue;
/** Whether this field is required. Only valid for proto2 fields. */
@property(nonatomic, readonly, getter=isRequired) BOOL required;
/** Whether this field is optional. */
@property(nonatomic, readonly, getter=isOptional) BOOL optional;
/** Type of field (single, repeated, map). */
@property(nonatomic, readonly) GPBFieldType fieldType;
/** Type of the key if the field is a map. The value's type is -fieldType. */
@property(nonatomic, readonly) GPBDataType mapKeyDataType;
/** Whether the field is packable. */
@property(nonatomic, readonly, getter=isPackable) BOOL packable;

/** The containing oneof if this field is part of one, nil otherwise. */
@property(nonatomic, readonly, nullable) GPBOneofDescriptor *containingOneof;

/** Class of the message if the field is of message type. */
@property(nonatomic, readonly, nullable) Class msgClass;

/** Descriptor for the enum if this field is an enum. */
@property(nonatomic, readonly, strong, nullable) GPBEnumDescriptor *enumDescriptor;

/**
 * Checks whether the given enum raw value is a valid enum value.
 *
 * @param value The raw enum value to check.
 *
 * @return YES if value is a valid enum raw value.
 **/
- (BOOL)isValidEnumValue:(int32_t)value;

/** @return Name for the text format, or nil if not known. */
- (nullable NSString *)textFormatName;

@end

/**
 * Describes a proto enum.
 **/
@interface GPBEnumDescriptor : NSObject

/** Name of the enum. */
@property(nonatomic, readonly, copy) NSString *name;
/** Function that validates that raw values are valid enum values. */
@property(nonatomic, readonly) GPBEnumValidationFunc enumVerifier;

/**
 * Returns the enum value name for the given raw enum.
 *
 * Note that there can be more than one name corresponding to a given value
 * if the allow_alias option is used.
 *
 * @param number The raw enum value.
 *
 * @return The first name that matches the enum value passed, or nil if not valid.
 **/
- (nullable NSString *)enumNameForValue:(int32_t)number;

/**
 * Gets the enum raw value for the given enum name.
 *
 * @param outValue A pointer where the value will be set.
 * @param name     The enum name for which to get the raw value.
 *
 * @return YES if a value was copied into the pointer, NO otherwise.
 **/
- (BOOL)getValue:(nullable int32_t *)outValue forEnumName:(NSString *)name;

/**
 * Returns the text format for the given raw enum value.
 *
 * @param number The raw enum value.
 *
 * @return The first text format name which matches the enum value, or nil if not valid.
 **/
- (nullable NSString *)textFormatNameForValue:(int32_t)number;

/**
 * Gets the enum raw value for the given text format name.
 *
 * @param outValue       A pointer where the value will be set.
 * @param textFormatName The text format name for which to get the raw value.
 *
 * @return YES if a value was copied into the pointer, NO otherwise.
 **/
- (BOOL)getValue:(nullable int32_t *)outValue forEnumTextFormatName:(NSString *)textFormatName;

/**
 * Gets the number of defined enum names.
 *
 * @return Count of the number of enum names, including any aliases.
 */
@property(nonatomic, readonly) uint32_t enumNameCount;

/**
 * Gets the enum name corresponding to the given index.
 *
 * @param index Index into the available names.  The defined range is from 0
 *              to self.enumNameCount - 1.
 *
 * @returns The enum name at the given index, or nil if the index is out of range.
 */
- (nullable NSString *)getEnumNameForIndex:(uint32_t)index;

/**
 * Gets the enum text format name corresponding to the given index.
 *
 * @param index Index into the available names.  The defined range is from 0
 *              to self.enumNameCount - 1.
 *
 * @returns The text format name at the given index, or nil if the index is out of range.
 */
- (nullable NSString *)getEnumTextFormatNameForIndex:(uint32_t)index;

@end

/**
 * Describes a proto extension.
 **/
@interface GPBExtensionDescriptor : NSObject<NSCopying>
/** Field number under which the extension is stored. */
@property(nonatomic, readonly) uint32_t fieldNumber;
/** The containing message class, i.e. the class extended by this extension. */
@property(nonatomic, readonly) Class containingMessageClass;
/** Data type contained in the extension. */
@property(nonatomic, readonly) GPBDataType dataType;
/** Whether the extension is repeated. */
@property(nonatomic, readonly, getter=isRepeated) BOOL repeated;
/** Whether the extension is packable. */
@property(nonatomic, readonly, getter=isPackable) BOOL packable;
/** The class of the message if the extension is of message type. */
@property(nonatomic, readonly) Class msgClass;
/** The singleton name for the extension. */
@property(nonatomic, readonly) NSString *singletonName;
/** The enum descriptor if the extension is of enum type. */
@property(nonatomic, readonly, strong, nullable) GPBEnumDescriptor *enumDescriptor;
/** The default value for the extension. */
@property(nonatomic, readonly, nullable) id defaultValue;

@end

NS_ASSUME_NONNULL_END
