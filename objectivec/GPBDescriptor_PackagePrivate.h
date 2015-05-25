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

// This header is private to the ProtobolBuffers library and must NOT be
// included by any sources outside this library. The contents of this file are
// subject to change at any time without notice.

#import "GPBDescriptor.h"

// Describes attributes of the field.
typedef NS_OPTIONS(uint32_t, GPBFieldFlags) {
  // These map to standard protobuf concepts.
  GPBFieldRequired        = 1 << 0,
  GPBFieldRepeated        = 1 << 1,
  GPBFieldPacked          = 1 << 2,
  GPBFieldOptional        = 1 << 3,
  GPBFieldHasDefaultValue = 1 << 4,

  // These are not standard protobuf concepts, they are specific to the
  // Objective C runtime.

  // These bits are used to mark the field as a map and what the key
  // type is.
  GPBFieldMapKeyMask     = 0xF << 8,
  GPBFieldMapKeyInt32    =  1 << 8,
  GPBFieldMapKeyInt64    =  2 << 8,
  GPBFieldMapKeyUInt32   =  3 << 8,
  GPBFieldMapKeyUInt64   =  4 << 8,
  GPBFieldMapKeySInt32   =  5 << 8,
  GPBFieldMapKeySInt64   =  6 << 8,
  GPBFieldMapKeyFixed32  =  7 << 8,
  GPBFieldMapKeyFixed64  =  8 << 8,
  GPBFieldMapKeySFixed32 =  9 << 8,
  GPBFieldMapKeySFixed64 = 10 << 8,
  GPBFieldMapKeyBool     = 11 << 8,
  GPBFieldMapKeyString   = 12 << 8,

  // Indicates the field needs custom handling for the TextFormat name, if not
  // set, the name can be derived from the ObjC name.
  GPBFieldTextFormatNameCustom = 1 << 16,
  // Indicates the field has an enum descriptor.
  // TODO(thomasvl): Output the CPP check to use descFunc or validator based
  // on final compile.  This will then get added based on that.
  GPBFieldHasEnumDescriptor = 1 << 17,
};

// Describes a single field in a protobuf as it is represented as an ivar.
typedef struct GPBMessageFieldDescription {
  // Name of ivar.
  const char *name;
  // The field number for the ivar.
  uint32_t number;
  // The index (in bits) into _has_storage_.
  //   > 0: the bit to use for a value being set.
  //   = 0: no storage used.
  //   < 0: in a oneOf, use a full int32 to record the field active.
  int32_t hasIndex;
  // Field flags. Use accessor functions below.
  GPBFieldFlags flags;
  // Type of the ivar.
  GPBType type;
  // Offset of the variable into it's structure struct.
  size_t offset;
  // FieldOptions protobuf, serialized as string.
  const char *fieldOptions;

  GPBValue defaultValue;  // Default value for the ivar.
  union {
    const char *className;  // Name for message class.
    // For enums only: If EnumDescriptors are compiled in, it will be that,
    // otherwise it will be the verifier.
    GPBEnumDescriptorFunc enumDescFunc;
    GPBEnumValidationFunc enumVerifier;
  } typeSpecific;
} GPBMessageFieldDescription;

// Describes a oneof.
typedef struct GPBMessageOneofDescription {
  // Name of this enum oneof.
  const char *name;
  // The index of this oneof in the has_storage.
  int32_t index;
} GPBMessageOneofDescription;

// Describes an enum type defined in a .proto file.
typedef struct GPBMessageEnumDescription {
  GPBEnumDescriptorFunc enumDescriptorFunc;
} GPBMessageEnumDescription;

// Describes an individual enum constant of a particular type.
typedef struct GPBMessageEnumValueDescription {
  // Name of this enum constant.
  const char *name;
  // Numeric value of this enum constant.
  int32_t number;
} GPBMessageEnumValueDescription;

// Describes attributes of the extension.
typedef NS_OPTIONS(uint32_t, GPBExtensionOptions) {
  // These map to standard protobuf concepts.
  GPBExtensionRepeated      = 1 << 0,
  GPBExtensionPacked        = 1 << 1,
  GPBExtensionSetWireFormat = 1 << 2,
};

// An extension
typedef struct GPBExtensionDescription {
  const char *singletonName;
  GPBType type;
  const char *extendedClass;
  int32_t fieldNumber;
  GPBValue defaultValue;
  const char *messageOrGroupClassName;
  GPBExtensionOptions options;
  GPBEnumDescriptorFunc enumDescriptorFunc;
} GPBExtensionDescription;

@interface GPBDescriptor () {
 @package
  NSArray *fields_;
  NSArray *oneofs_;
  size_t storageSize_;
}

// fieldDescriptions, enumDescriptions, rangeDescriptions, and
// extraTextFormatInfo have to be long lived, they are held as raw pointers.
+ (instancetype)
    allocDescriptorForClass:(Class)messageClass
                  rootClass:(Class)rootClass
                       file:(GPBFileDescriptor *)file
                     fields:(GPBMessageFieldDescription *)fieldDescriptions
                 fieldCount:(NSUInteger)fieldCount
                     oneofs:(GPBMessageOneofDescription *)oneofDescriptions
                 oneofCount:(NSUInteger)oneofCount
                      enums:(GPBMessageEnumDescription *)enumDescriptions
                  enumCount:(NSUInteger)enumCount
                     ranges:(const GPBExtensionRange *)ranges
                 rangeCount:(NSUInteger)rangeCount
                storageSize:(size_t)storageSize
                 wireFormat:(BOOL)wireFormat;
+ (instancetype)
    allocDescriptorForClass:(Class)messageClass
                  rootClass:(Class)rootClass
                       file:(GPBFileDescriptor *)file
                     fields:(GPBMessageFieldDescription *)fieldDescriptions
                 fieldCount:(NSUInteger)fieldCount
                     oneofs:(GPBMessageOneofDescription *)oneofDescriptions
                 oneofCount:(NSUInteger)oneofCount
                      enums:(GPBMessageEnumDescription *)enumDescriptions
                  enumCount:(NSUInteger)enumCount
                     ranges:(const GPBExtensionRange *)ranges
                 rangeCount:(NSUInteger)rangeCount
                storageSize:(size_t)storageSize
                 wireFormat:(BOOL)wireFormat
        extraTextFormatInfo:(const char *)extraTextFormatInfo;

- (instancetype)initWithClass:(Class)messageClass
                         file:(GPBFileDescriptor *)file
                       fields:(NSArray *)fields
                       oneofs:(NSArray *)oneofs
                        enums:(NSArray *)enums
              extensionRanges:(const GPBExtensionRange *)ranges
         extensionRangesCount:(NSUInteger)rangeCount
                  storageSize:(size_t)storage
                   wireFormat:(BOOL)wireFormat;

@end

@interface GPBFileDescriptor ()
- (instancetype)initWithPackage:(NSString *)package
                         syntax:(GPBFileSyntax)syntax;
@end

@interface GPBOneofDescriptor () {
 @package
  GPBMessageOneofDescription *oneofDescription_;
  NSArray *fields_;

  SEL caseSel_;
}
- (instancetype)initWithOneofDescription:
                    (GPBMessageOneofDescription *)oneofDescription
                                  fields:(NSArray *)fields;
@end

@interface GPBFieldDescriptor () {
 @package
  GPBMessageFieldDescription *description_;
  GPB_UNSAFE_UNRETAINED GPBOneofDescriptor *containingOneof_;

  SEL getSel_;
  SEL setSel_;
  SEL hasSel_;
  SEL setHasSel_;
}

// Single initializer
// description has to be long lived, it is held as a raw pointer.
- (instancetype)initWithFieldDescription:
                    (GPBMessageFieldDescription *)description
                               rootClass:(Class)rootClass
                                  syntax:(GPBFileSyntax)syntax;
@end

@interface GPBEnumDescriptor ()
// valueDescriptions and extraTextFormatInfo have to be long lived, they are
// held as raw pointers.
+ (instancetype)
    allocDescriptorForName:(NSString *)name
                    values:(GPBMessageEnumValueDescription *)valueDescriptions
                valueCount:(NSUInteger)valueCount
              enumVerifier:(GPBEnumValidationFunc)enumVerifier;
+ (instancetype)
    allocDescriptorForName:(NSString *)name
                    values:(GPBMessageEnumValueDescription *)valueDescriptions
                valueCount:(NSUInteger)valueCount
              enumVerifier:(GPBEnumValidationFunc)enumVerifier
       extraTextFormatInfo:(const char *)extraTextFormatInfo;

- (instancetype)initWithName:(NSString *)name
                      values:(GPBMessageEnumValueDescription *)valueDescriptions
                  valueCount:(NSUInteger)valueCount
                enumVerifier:(GPBEnumValidationFunc)enumVerifier;
@end

@interface GPBExtensionDescriptor () {
 @package
  GPBExtensionDescription *description_;
}

// description has to be long lived, it is held as a raw pointer.
- (instancetype)initWithExtensionDescription:
        (GPBExtensionDescription *)description;
@end

CF_EXTERN_C_BEGIN

GPB_INLINE BOOL GPBFieldIsMapOrArray(GPBFieldDescriptor *field) {
  return (field->description_->flags &
          (GPBFieldRepeated | GPBFieldMapKeyMask)) != 0;
}

GPB_INLINE GPBType GPBGetFieldType(GPBFieldDescriptor *field) {
  return field->description_->type;
}

GPB_INLINE int32_t GPBFieldHasIndex(GPBFieldDescriptor *field) {
  return field->description_->hasIndex;
}

GPB_INLINE uint32_t GPBFieldNumber(GPBFieldDescriptor *field) {
  return field->description_->number;
}

uint32_t GPBFieldTag(GPBFieldDescriptor *self);

GPB_INLINE BOOL GPBPreserveUnknownFields(GPBFileSyntax syntax) {
  return syntax != GPBFileSyntaxProto3;
}

GPB_INLINE BOOL GPBHasPreservingUnknownEnumSemantics(GPBFileSyntax syntax) {
  return syntax == GPBFileSyntaxProto3;
}

CF_EXTERN_C_END
