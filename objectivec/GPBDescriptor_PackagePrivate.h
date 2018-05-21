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
#import "GPBWireFormat.h"

// Describes attributes of the field.
typedef NS_OPTIONS(uint16_t, GPBFieldFlags) {
  GPBFieldNone            = 0,
  // These map to standard protobuf concepts.
  GPBFieldRequired        = 1 << 0,
  GPBFieldRepeated        = 1 << 1,
  GPBFieldPacked          = 1 << 2,
  GPBFieldOptional        = 1 << 3,
  GPBFieldHasDefaultValue = 1 << 4,

  // Indicates the field needs custom handling for the TextFormat name, if not
  // set, the name can be derived from the ObjC name.
  GPBFieldTextFormatNameCustom = 1 << 6,
  // Indicates the field has an enum descriptor.
  GPBFieldHasEnumDescriptor = 1 << 7,

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
};

// NOTE: The structures defined here have their members ordered to minimize
// their size. This directly impacts the size of apps since these exist per
// field/extension.

// Describes a single field in a protobuf as it is represented as an ivar.
typedef struct GPBMessageFieldDescription {
  // Name of ivar.
  const char *name;
  union {
    const char *className;  // Name for message class.
    // For enums only: If EnumDescriptors are compiled in, it will be that,
    // otherwise it will be the verifier.
    GPBEnumDescriptorFunc enumDescFunc;
    GPBEnumValidationFunc enumVerifier;
  } dataTypeSpecific;
  // The field number for the ivar.
  uint32_t number;
  // The index (in bits) into _has_storage_.
  //   >= 0: the bit to use for a value being set.
  //   = GPBNoHasBit(INT32_MAX): no storage used.
  //   < 0: in a oneOf, use a full int32 to record the field active.
  int32_t hasIndex;
  // Offset of the variable into it's structure struct.
  uint32_t offset;
  // Field flags. Use accessor functions below.
  GPBFieldFlags flags;
  // Data type of the ivar.
  GPBDataType dataType;
} GPBMessageFieldDescription;

// Fields in messages defined in a 'proto2' syntax file can provide a default
// value. This struct provides the default along with the field info.
typedef struct GPBMessageFieldDescriptionWithDefault {
  // Default value for the ivar.
  GPBGenericValue defaultValue;

  GPBMessageFieldDescription core;
} GPBMessageFieldDescriptionWithDefault;

// Describes attributes of the extension.
typedef NS_OPTIONS(uint8_t, GPBExtensionOptions) {
  GPBExtensionNone          = 0,
  // These map to standard protobuf concepts.
  GPBExtensionRepeated      = 1 << 0,
  GPBExtensionPacked        = 1 << 1,
  GPBExtensionSetWireFormat = 1 << 2,
};

// An extension
typedef struct GPBExtensionDescription {
  GPBGenericValue defaultValue;
  const char *singletonName;
  const char *extendedClass;
  const char *messageOrGroupClassName;
  GPBEnumDescriptorFunc enumDescriptorFunc;
  int32_t fieldNumber;
  GPBDataType dataType;
  GPBExtensionOptions options;
} GPBExtensionDescription;

typedef NS_OPTIONS(uint32_t, GPBDescriptorInitializationFlags) {
  GPBDescriptorInitializationFlag_None              = 0,
  GPBDescriptorInitializationFlag_FieldsWithDefault = 1 << 0,
  GPBDescriptorInitializationFlag_WireFormat        = 1 << 1,
};

@interface GPBDescriptor () {
 @package
  NSArray *fields_;
  NSArray *oneofs_;
  uint32_t storageSize_;
}

// fieldDescriptions have to be long lived, they are held as raw pointers.
+ (instancetype)
    allocDescriptorForClass:(Class)messageClass
                  rootClass:(Class)rootClass
                       file:(GPBFileDescriptor *)file
                     fields:(void *)fieldDescriptions
                 fieldCount:(uint32_t)fieldCount
                storageSize:(uint32_t)storageSize
                      flags:(GPBDescriptorInitializationFlags)flags;

- (instancetype)initWithClass:(Class)messageClass
                         file:(GPBFileDescriptor *)file
                       fields:(NSArray *)fields
                  storageSize:(uint32_t)storage
                   wireFormat:(BOOL)wireFormat;

// Called right after init to provide extra information to avoid init having
// an explosion of args. These pointers are recorded, so they are expected
// to live for the lifetime of the app.
- (void)setupOneofs:(const char **)oneofNames
              count:(uint32_t)count
      firstHasIndex:(int32_t)firstHasIndex;
- (void)setupExtraTextInfo:(const char *)extraTextFormatInfo;
- (void)setupExtensionRanges:(const GPBExtensionRange *)ranges count:(int32_t)count;
- (void)setupContainingMessageClassName:(const char *)msgClassName;
- (void)setupMessageClassNameSuffix:(NSString *)suffix;

@end

@interface GPBFileDescriptor ()
- (instancetype)initWithPackage:(NSString *)package
                     objcPrefix:(NSString *)objcPrefix
                         syntax:(GPBFileSyntax)syntax;
- (instancetype)initWithPackage:(NSString *)package
                         syntax:(GPBFileSyntax)syntax;
@end

@interface GPBOneofDescriptor () {
 @package
  const char *name_;
  NSArray *fields_;
  SEL caseSel_;
}
// name must be long lived.
- (instancetype)initWithName:(const char *)name fields:(NSArray *)fields;
@end

@interface GPBFieldDescriptor () {
 @package
  GPBMessageFieldDescription *description_;
  GPB_UNSAFE_UNRETAINED GPBOneofDescriptor *containingOneof_;

  SEL getSel_;
  SEL setSel_;
  SEL hasOrCountSel_;  // *Count for map<>/repeated fields, has* otherwise.
  SEL setHasSel_;
}

// Single initializer
// description has to be long lived, it is held as a raw pointer.
- (instancetype)initWithFieldDescription:(void *)description
                         includesDefault:(BOOL)includesDefault
                                  syntax:(GPBFileSyntax)syntax;
@end

@interface GPBEnumDescriptor ()
// valueNames, values and extraTextFormatInfo have to be long lived, they are
// held as raw pointers.
+ (instancetype)
    allocDescriptorForName:(NSString *)name
                valueNames:(const char *)valueNames
                    values:(const int32_t *)values
                     count:(uint32_t)valueCount
              enumVerifier:(GPBEnumValidationFunc)enumVerifier;
+ (instancetype)
    allocDescriptorForName:(NSString *)name
                valueNames:(const char *)valueNames
                    values:(const int32_t *)values
                     count:(uint32_t)valueCount
              enumVerifier:(GPBEnumValidationFunc)enumVerifier
       extraTextFormatInfo:(const char *)extraTextFormatInfo;

- (instancetype)initWithName:(NSString *)name
                  valueNames:(const char *)valueNames
                      values:(const int32_t *)values
                       count:(uint32_t)valueCount
                enumVerifier:(GPBEnumValidationFunc)enumVerifier;
@end

@interface GPBExtensionDescriptor () {
 @package
  GPBExtensionDescription *description_;
}
@property(nonatomic, readonly) GPBWireFormat wireType;

// For repeated extensions, alternateWireType is the wireType with the opposite
// value for the packable property.  i.e. - if the extension was marked packed
// it would be the wire type for unpacked; if the extension was marked unpacked,
// it would be the wire type for packed.
@property(nonatomic, readonly) GPBWireFormat alternateWireType;

// description has to be long lived, it is held as a raw pointer.
- (instancetype)initWithExtensionDescription:
    (GPBExtensionDescription *)description;
- (NSComparisonResult)compareByFieldNumber:(GPBExtensionDescriptor *)other;
@end

CF_EXTERN_C_BEGIN

// Direct access is use for speed, to avoid even internally declaring things
// read/write, etc. The warning is enabled in the project to ensure code calling
// protos can turn on -Wdirect-ivar-access without issues.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdirect-ivar-access"

GPB_INLINE BOOL GPBFieldIsMapOrArray(GPBFieldDescriptor *field) {
  return (field->description_->flags &
          (GPBFieldRepeated | GPBFieldMapKeyMask)) != 0;
}

GPB_INLINE GPBDataType GPBGetFieldDataType(GPBFieldDescriptor *field) {
  return field->description_->dataType;
}

GPB_INLINE int32_t GPBFieldHasIndex(GPBFieldDescriptor *field) {
  return field->description_->hasIndex;
}

GPB_INLINE uint32_t GPBFieldNumber(GPBFieldDescriptor *field) {
  return field->description_->number;
}

#pragma clang diagnostic pop

uint32_t GPBFieldTag(GPBFieldDescriptor *self);

// For repeated fields, alternateWireType is the wireType with the opposite
// value for the packable property.  i.e. - if the field was marked packed it
// would be the wire type for unpacked; if the field was marked unpacked, it
// would be the wire type for packed.
uint32_t GPBFieldAlternateTag(GPBFieldDescriptor *self);

GPB_INLINE BOOL GPBHasPreservingUnknownEnumSemantics(GPBFileSyntax syntax) {
  return syntax == GPBFileSyntaxProto3;
}

GPB_INLINE BOOL GPBExtensionIsRepeated(GPBExtensionDescription *description) {
  return (description->options & GPBExtensionRepeated) != 0;
}

GPB_INLINE BOOL GPBExtensionIsPacked(GPBExtensionDescription *description) {
  return (description->options & GPBExtensionPacked) != 0;
}

GPB_INLINE BOOL GPBExtensionIsWireFormat(GPBExtensionDescription *description) {
  return (description->options & GPBExtensionSetWireFormat) != 0;
}

// Helper for compile time assets.
#ifndef GPBInternalCompileAssert
  #if __has_feature(c_static_assert) || __has_extension(c_static_assert)
    #define GPBInternalCompileAssert(test, msg) _Static_assert((test), #msg)
  #else
    // Pre-Xcode 7 support.
    #define GPBInternalCompileAssertSymbolInner(line, msg) GPBInternalCompileAssert ## line ## __ ## msg
    #define GPBInternalCompileAssertSymbol(line, msg) GPBInternalCompileAssertSymbolInner(line, msg)
    #define GPBInternalCompileAssert(test, msg) \
        typedef char GPBInternalCompileAssertSymbol(__LINE__, msg) [ ((test) ? 1 : -1) ]
  #endif  // __has_feature(c_static_assert) || __has_extension(c_static_assert)
#endif // GPBInternalCompileAssert

// Sanity check that there isn't padding between the field description
// structures with and without a default.
GPBInternalCompileAssert(sizeof(GPBMessageFieldDescriptionWithDefault) ==
                         (sizeof(GPBGenericValue) +
                          sizeof(GPBMessageFieldDescription)),
                         DescriptionsWithDefault_different_size_than_expected);

CF_EXTERN_C_END
