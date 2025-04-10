// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This header is private to the ProtobolBuffers library and must NOT be
// included by any sources outside this library. The contents of this file are
// subject to change at any time without notice.

#import "GPBDescriptor.h"
#import "GPBWireFormat.h"

// Describes attributes of the field.
typedef NS_OPTIONS(uint16_t, GPBFieldFlags) {
  GPBFieldNone = 0,
  // These map to standard protobuf concepts.
  GPBFieldRequired = 1 << 0,
  GPBFieldRepeated = 1 << 1,
  GPBFieldPacked = 1 << 2,
  GPBFieldOptional __attribute__((deprecated)) = 1 << 3,
  GPBFieldHasDefaultValue = 1 << 4,

  // Indicate that the field should "clear" when set to zero value. This is the
  // proto3 non optional behavior for singular data (ints, data, string, enum)
  // fields.
  GPBFieldClearHasIvarOnZero = 1 << 5,
  // Indicates the field needs custom handling for the TextFormat name, if not
  // set, the name can be derived from the ObjC name.
  GPBFieldTextFormatNameCustom = 1 << 6,

  // Legacy Flag. This is set for all enum fields in the older generated code but should no longer
  // be used in new generated code. It can not be reused until the 30007 generated format is no
  // longer supported.
  GPBFieldHasEnumDescriptor = 1 << 7,

  // These are not standard protobuf concepts, they are specific to the
  // Objective C runtime.

  // These bits are used to mark the field as a map and what the key
  // type is.
  GPBFieldMapKeyMask = 0xF << 8,
  GPBFieldMapKeyInt32 = 1 << 8,
  GPBFieldMapKeyInt64 = 2 << 8,
  GPBFieldMapKeyUInt32 = 3 << 8,
  GPBFieldMapKeyUInt64 = 4 << 8,
  GPBFieldMapKeySInt32 = 5 << 8,
  GPBFieldMapKeySInt64 = 6 << 8,
  GPBFieldMapKeyFixed32 = 7 << 8,
  GPBFieldMapKeyFixed64 = 8 << 8,
  GPBFieldMapKeySFixed32 = 9 << 8,
  GPBFieldMapKeySFixed64 = 10 << 8,
  GPBFieldMapKeyBool = 11 << 8,
  GPBFieldMapKeyString = 12 << 8,

  // Legacy Flag. This is set for as needed enum fields in the older generated code but should no
  // longer be used in new generated code. It can not be reused until the 30007 generated format is
  // no longer supported.
  GPBFieldClosedEnum = 1 << 12,
};

// NOTE: The structures defined here have their members ordered to minimize
// their size. This directly impacts the size of apps since these exist per
// field/extension.

// This is the current version of GPBFileDescription. It must maintain field alignment with the
// previous structure.
typedef struct GPBFilePackageAndPrefix {
  // The proto package for the file.
  const char *package;
  // The objc_class_prefix option if present.
  const char *prefix;
} GPBFilePackageAndPrefix;

// This is used by older code generation.
typedef struct GPBFileDescription {
  // The proto package for the file.
  const char *package;
  // The objc_class_prefix option if present.
  const char *prefix;
  // The file's proto syntax.
  GPBFileSyntax syntax;
} GPBFileDescription;

// Fetches an EnumDescriptor.
typedef GPBEnumDescriptor *(*GPBEnumDescriptorFunc)(void);

// Describes a single field in a protobuf as it is represented as an ivar.
typedef struct GPBMessageFieldDescription {
  // Name of ivar.
  // Note that we looked into using a SEL here instead (which really is just a C string)
  // but there is not a way to initialize an SEL with a constant (@selector is not constant) so the
  // additional code generated to initialize the value is actually bigger in size than just using a
  // C identifier for large apps.
  const char *name;
  union {
    Class clazz;                         // Class of the message.
    GPBEnumDescriptorFunc enumDescFunc;  // Function to get the enum descriptor.
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

// If a message uses fields where they provide default values that are non zero, then this
// struct is used to provide the values along with the field info.
typedef struct GPBMessageFieldDescriptionWithDefault {
  // Default value for the ivar.
  GPBGenericValue defaultValue;

  GPBMessageFieldDescription core;
} GPBMessageFieldDescriptionWithDefault;

// Describes attributes of the extension.
typedef NS_OPTIONS(uint8_t, GPBExtensionOptions) {
  GPBExtensionNone = 0,
  // These map to standard protobuf concepts.
  GPBExtensionRepeated = 1 << 0,
  GPBExtensionPacked = 1 << 1,

  // Legacy Flag. This was used by older versions of the runtime, but not it only needs the value
  // set on the message that has the option enabled.
  GPBExtensionSetWireFormat = 1 << 2,
};

// An extension
typedef struct GPBExtensionDescription {
  GPBGenericValue defaultValue;
  const char *singletonName;
  // Historically this had more than one entry, but the union name is referenced in the generated
  // code so it can't be removed without breaking compatibility.
  union {
    Class clazz;
  } extendedClass;
  // Historically this had more than one entry, but the union name is referenced in the generated
  // code so it can't be removed without breaking compatibility.
  union {
    Class clazz;
  } messageOrGroupClass;
  GPBEnumDescriptorFunc enumDescriptorFunc;
  int32_t fieldNumber;
  GPBDataType dataType;
  GPBExtensionOptions options;
} GPBExtensionDescription;

typedef NS_OPTIONS(uint32_t, GPBDescriptorInitializationFlags) {
  GPBDescriptorInitializationFlag_None = 0,
  GPBDescriptorInitializationFlag_FieldsWithDefault = 1 << 0,
  GPBDescriptorInitializationFlag_WireFormat = 1 << 1,

  // Legacy Flags. These are always set in older generated code but should not be used in new
  // generated code. The bits can not be reused until the 30007 generated format is no longer
  // supported.
  GPBDescriptorInitializationFlag_UsesClassRefs = 1 << 2,
  GPBDescriptorInitializationFlag_Proto3OptionalKnown = 1 << 3,
  GPBDescriptorInitializationFlag_ClosedEnumSupportKnown = 1 << 4,
};

@interface GPBDescriptor () {
 @package
  NSArray *fields_;
  NSArray *oneofs_;
  uint32_t storageSize_;
}

// fieldDescriptions and fileDescription have to be long lived, they are held as raw pointers.
+ (instancetype)allocDescriptorForClass:(Class)messageClass
                            messageName:(NSString *)messageName
                         runtimeSupport:(const int32_t *)runtimeSupport
                        fileDescription:(GPBFilePackageAndPrefix *)fileDescription
                                 fields:(void *)fieldDescriptions
                             fieldCount:(uint32_t)fieldCount
                            storageSize:(uint32_t)storageSize
                                  flags:(GPBDescriptorInitializationFlags)flags;
+ (instancetype)allocDescriptorForClass:(Class)messageClass
                            messageName:(NSString *)messageName
                        fileDescription:(GPBFileDescription *)fileDescription
                                 fields:(void *)fieldDescriptions
                             fieldCount:(uint32_t)fieldCount
                            storageSize:(uint32_t)storageSize
                                  flags:(GPBDescriptorInitializationFlags)flags;

// Called right after init to provide extra information to avoid init having
// an explosion of args. These pointers are recorded, so they are expected
// to live for the lifetime of the app.
- (void)setupOneofs:(const char **)oneofNames
              count:(uint32_t)count
      firstHasIndex:(int32_t)firstHasIndex;
- (void)setupExtraTextInfo:(const char *)extraTextFormatInfo;
- (void)setupExtensionRanges:(const GPBExtensionRange *)ranges count:(int32_t)count;
- (void)setupContainingMessageClass:(Class)msgClass;

@end

@interface GPBOneofDescriptor () {
 @package
  const char *name_;
  NSArray *fields_;
}
@end

@interface GPBFieldDescriptor () {
 @package
  GPBMessageFieldDescription *description_;
  GPB_UNSAFE_UNRETAINED GPBOneofDescriptor *containingOneof_;
}
@end

typedef NS_OPTIONS(uint32_t, GPBEnumDescriptorInitializationFlags) {
  GPBEnumDescriptorInitializationFlag_None = 0,

  // Available: 1 << 0

  // Marks this enum as a closed enum.
  GPBEnumDescriptorInitializationFlag_IsClosed = 1 << 1,
};

@interface GPBEnumDescriptor ()
// valueNames, values and extraTextFormatInfo have to be long lived, they are
// held as raw pointers.
+ (instancetype)allocDescriptorForName:(NSString *)name
                        runtimeSupport:(const int32_t *)runtimeSupport
                            valueNames:(const char *)valueNames
                                values:(const int32_t *)values
                                 count:(uint32_t)valueCount
                          enumVerifier:(GPBEnumValidationFunc)enumVerifier
                                 flags:(GPBEnumDescriptorInitializationFlags)flags;
+ (instancetype)allocDescriptorForName:(NSString *)name
                        runtimeSupport:(const int32_t *)runtimeSupport
                            valueNames:(const char *)valueNames
                                values:(const int32_t *)values
                                 count:(uint32_t)valueCount
                          enumVerifier:(GPBEnumValidationFunc)enumVerifier
                                 flags:(GPBEnumDescriptorInitializationFlags)flags
                   extraTextFormatInfo:(const char *)extraTextFormatInfo;
+ (instancetype)allocDescriptorForName:(NSString *)name
                            valueNames:(const char *)valueNames
                                values:(const int32_t *)values
                                 count:(uint32_t)valueCount
                          enumVerifier:(GPBEnumValidationFunc)enumVerifier
                                 flags:(GPBEnumDescriptorInitializationFlags)flags;
+ (instancetype)allocDescriptorForName:(NSString *)name
                            valueNames:(const char *)valueNames
                                values:(const int32_t *)values
                                 count:(uint32_t)valueCount
                          enumVerifier:(GPBEnumValidationFunc)enumVerifier
                                 flags:(GPBEnumDescriptorInitializationFlags)flags
                   extraTextFormatInfo:(const char *)extraTextFormatInfo;

- (BOOL)isOpenOrValidValue:(int32_t)value;
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
- (instancetype)initWithExtensionDescription:(GPBExtensionDescription *)desc
                              runtimeSupport:(const int32_t *)runtimeSupport;
- (instancetype)initWithExtensionDescription:(GPBExtensionDescription *)desc
                               usesClassRefs:(BOOL)usesClassRefs;

- (NSComparisonResult)compareByFieldNumber:(GPBExtensionDescriptor *)other;
@end

CF_EXTERN_C_BEGIN

// Direct access is use for speed, to avoid even internally declaring things
// read/write, etc. The warning is enabled in the project to ensure code calling
// protos can turn on -Wdirect-ivar-access without issues.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdirect-ivar-access"

GPB_INLINE BOOL GPBFieldIsMapOrArray(GPBFieldDescriptor *field) {
  return (field->description_->flags & (GPBFieldRepeated | GPBFieldMapKeyMask)) != 0;
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

GPB_INLINE BOOL GPBExtensionIsRepeated(GPBExtensionDescription *description) {
  return (description->options & GPBExtensionRepeated) != 0;
}

GPB_INLINE BOOL GPBExtensionIsPacked(GPBExtensionDescription *description) {
  return (description->options & GPBExtensionPacked) != 0;
}

// Helper for compile time assets.
#ifndef GPBInternalCompileAssert
#define GPBInternalCompileAssert(test, msg) _Static_assert((test), #msg)
#endif  // GPBInternalCompileAssert

// Sanity check that there isn't padding between the field description
// structures with and without a default.
GPBInternalCompileAssert(sizeof(GPBMessageFieldDescriptionWithDefault) ==
                             (sizeof(GPBGenericValue) + sizeof(GPBMessageFieldDescription)),
                         DescriptionsWithDefault_different_size_than_expected);

// Sanity check that the file description structures old and new work.
GPBInternalCompileAssert(offsetof(GPBFilePackageAndPrefix, package) ==
                             offsetof(GPBFileDescription, package),
                         FileDescription_package_offsets_dont_match);
GPBInternalCompileAssert(offsetof(GPBFilePackageAndPrefix, prefix) ==
                             offsetof(GPBFileDescription, prefix),
                         FileDescription_prefix_offsets_dont_match);

CF_EXTERN_C_END
