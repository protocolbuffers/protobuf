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

#import "GPBDescriptor_PackagePrivate.h"

#import <objc/runtime.h>

#import "GPBMessage_PackagePrivate.h"
#import "GPBUtilities_PackagePrivate.h"
#import "GPBWireFormat.h"

@interface GPBDescriptor ()
- (instancetype)initWithClass:(Class)messageClass
                  messageName:(NSString *)messageName
              fileDescription:(GPBFileDescription *)fileDescription
                       fields:(NSArray *)fields
                  storageSize:(uint32_t)storage
                   wireFormat:(BOOL)wireFormat;
@end

@interface GPBFieldDescriptor ()
// Single initializer
// description has to be long lived, it is held as a raw pointer.
- (instancetype)initWithFieldDescription:(void *)description
                         descriptorFlags:(GPBDescriptorInitializationFlags)descriptorFlags;

@end

@interface GPBEnumDescriptor ()
- (instancetype)initWithName:(NSString *)name
                  valueNames:(const char *)valueNames
                      values:(const int32_t *)values
                       count:(uint32_t)valueCount
                enumVerifier:(GPBEnumValidationFunc)enumVerifier
                       flags:(GPBEnumDescriptorInitializationFlags)flags;
@end

// Direct access is use for speed, to avoid even internally declaring things
// read/write, etc. The warning is enabled in the project to ensure code calling
// protos can turn on -Wdirect-ivar-access without issues.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdirect-ivar-access"

// The addresses of these variables are used as keys for objc_getAssociatedObject.
static const char kTextFormatExtraValueKey = 0;
static const char kParentClassValueKey = 0;
static const char kClassNameSuffixKey = 0;
static const char kFileDescriptorCacheKey = 0;

// Utility function to generate selectors on the fly.
static SEL SelFromStrings(const char *prefix, const char *middle, const char *suffix,
                          BOOL takesArg) {
  if (prefix == NULL && suffix == NULL && !takesArg) {
    return sel_getUid(middle);
  }
  const size_t prefixLen = prefix != NULL ? strlen(prefix) : 0;
  const size_t middleLen = strlen(middle);
  const size_t suffixLen = suffix != NULL ? strlen(suffix) : 0;
  size_t totalLen = prefixLen + middleLen + suffixLen + 1;  // include space for null on end.
  if (takesArg) {
    totalLen += 1;
  }
  char buffer[totalLen];
  if (prefix != NULL) {
    memcpy(buffer, prefix, prefixLen);
    memcpy(buffer + prefixLen, middle, middleLen);
    buffer[prefixLen] = (char)toupper(buffer[prefixLen]);
  } else {
    memcpy(buffer, middle, middleLen);
  }
  if (suffix != NULL) {
    memcpy(buffer + prefixLen + middleLen, suffix, suffixLen);
  }
  if (takesArg) {
    buffer[totalLen - 2] = ':';
  }
  // Always null terminate it.
  buffer[totalLen - 1] = 0;

  SEL result = sel_getUid(buffer);
  return result;
}

static NSArray *NewFieldsArrayForHasIndex(int hasIndex, NSArray *allMessageFields)
    __attribute__((ns_returns_retained));

static NSArray *NewFieldsArrayForHasIndex(int hasIndex, NSArray *allMessageFields) {
  NSMutableArray *result = [[NSMutableArray alloc] init];
  for (GPBFieldDescriptor *fieldDesc in allMessageFields) {
    if (fieldDesc->description_->hasIndex == hasIndex) {
      [result addObject:fieldDesc];
    }
  }
  return result;
}

@implementation GPBDescriptor {
  Class messageClass_;
  NSString *messageName_;
  const GPBFileDescription *fileDescription_;
  BOOL wireFormat_;
}

@synthesize messageClass = messageClass_;
@synthesize fields = fields_;
@synthesize oneofs = oneofs_;
@synthesize extensionRanges = extensionRanges_;
@synthesize extensionRangesCount = extensionRangesCount_;
@synthesize wireFormat = wireFormat_;

+ (instancetype)allocDescriptorForClass:(Class)messageClass
                            messageName:(NSString *)messageName
                        fileDescription:(GPBFileDescription *)fileDescription
                                 fields:(void *)fieldDescriptions
                             fieldCount:(uint32_t)fieldCount
                            storageSize:(uint32_t)storageSize
                                  flags:(GPBDescriptorInitializationFlags)flags {
  // Compute the unknown flags by this version of the runtime and then check the passed in flags
  // (from the generated code) to detect when sources from a newer version are being used with an
  // older runtime.
  GPBDescriptorInitializationFlags unknownFlags =
      ~(GPBDescriptorInitializationFlag_FieldsWithDefault |
        GPBDescriptorInitializationFlag_WireFormat | GPBDescriptorInitializationFlag_UsesClassRefs |
        GPBDescriptorInitializationFlag_Proto3OptionalKnown |
        GPBDescriptorInitializationFlag_ClosedEnumSupportKnown);
  if ((flags & unknownFlags) != 0) {
    GPBRuntimeMatchFailure();
  }

#if defined(DEBUG) && DEBUG
  NSAssert((flags & GPBDescriptorInitializationFlag_UsesClassRefs) != 0,
           @"Internal error: all fields should have class refs");
  NSAssert((flags & GPBDescriptorInitializationFlag_Proto3OptionalKnown) != 0,
           @"Internal error: proto3 optional should be known");
  NSAssert((flags & GPBDescriptorInitializationFlag_ClosedEnumSupportKnown) != 0,
           @"Internal error: close enum should be known");

  // `messageName` and `fileDescription` should both be set or both be unset depending on if this is
  // being called from current code generation or legacy code generation.
  NSAssert((messageName == nil) == (fileDescription == NULL),
           @"name and fileDescription should always be provided together");
#endif

  NSMutableArray *fields =
      (fieldCount ? [[NSMutableArray alloc] initWithCapacity:fieldCount] : nil);
  BOOL fieldsIncludeDefault = (flags & GPBDescriptorInitializationFlag_FieldsWithDefault) != 0;

  void *desc;
  GPBFieldFlags mergedFieldFlags = GPBFieldNone;
  for (uint32_t i = 0; i < fieldCount; ++i) {
    // Need correctly typed pointer for array indexing below to work.
    if (fieldsIncludeDefault) {
      desc = &(((GPBMessageFieldDescriptionWithDefault *)fieldDescriptions)[i]);
      mergedFieldFlags |=
          (((GPBMessageFieldDescriptionWithDefault *)fieldDescriptions)[i]).core.flags;
    } else {
      desc = &(((GPBMessageFieldDescription *)fieldDescriptions)[i]);
      mergedFieldFlags |= (((GPBMessageFieldDescription *)fieldDescriptions)[i]).flags;
    }
    GPBFieldDescriptor *fieldDescriptor =
        [[GPBFieldDescriptor alloc] initWithFieldDescription:desc descriptorFlags:flags];
    [fields addObject:fieldDescriptor];
    [fieldDescriptor release];
  }
  // No real value in checking all the fields individually, just check the combined flags at the
  // end.
  GPBFieldFlags unknownFieldFlags =
      ~(GPBFieldRequired | GPBFieldRepeated | GPBFieldPacked | GPBFieldOptional |
        GPBFieldHasDefaultValue | GPBFieldClearHasIvarOnZero | GPBFieldTextFormatNameCustom |
        GPBFieldHasEnumDescriptor | GPBFieldMapKeyMask | GPBFieldClosedEnum);
  if ((mergedFieldFlags & unknownFieldFlags) != 0) {
    GPBRuntimeMatchFailure();
  }

  BOOL wireFormat = (flags & GPBDescriptorInitializationFlag_WireFormat) != 0;
  GPBDescriptor *descriptor = [[self alloc] initWithClass:messageClass
                                              messageName:messageName
                                          fileDescription:fileDescription
                                                   fields:fields
                                              storageSize:storageSize
                                               wireFormat:wireFormat];
  [fields release];
  return descriptor;
}

+ (instancetype)allocDescriptorForClass:(Class)messageClass
                                   file:(GPBFileDescriptor *)file
                                 fields:(void *)fieldDescriptions
                             fieldCount:(uint32_t)fieldCount
                            storageSize:(uint32_t)storageSize
                                  flags:(GPBDescriptorInitializationFlags)flags {
  GPBInternalCompileAssert(GOOGLE_PROTOBUF_OBJC_MIN_SUPPORTED_VERSION <= 30006,
                           time_to_remove_this_old_version_shim);

  BOOL fixClassRefs = (flags & GPBDescriptorInitializationFlag_UsesClassRefs) == 0;
  GPBInternalCompileAssert(GOOGLE_PROTOBUF_OBJC_MIN_SUPPORTED_VERSION <= 30003,
                           time_to_remove_non_class_ref_support);

  BOOL fixProto3Optional = (flags & GPBDescriptorInitializationFlag_Proto3OptionalKnown) == 0;
  GPBInternalCompileAssert(GOOGLE_PROTOBUF_OBJC_MIN_SUPPORTED_VERSION <= 30004,
                           time_to_remove_proto3_optional_fallback);

  BOOL fixClosedEnums = (flags & GPBDescriptorInitializationFlag_ClosedEnumSupportKnown) == 0;
  GPBInternalCompileAssert(GOOGLE_PROTOBUF_OBJC_MIN_SUPPORTED_VERSION <= 30005,
                           time_to_remove_closed_enum_fallback);

  if (fixClassRefs || fixProto3Optional || fixClosedEnums) {
    BOOL fieldsIncludeDefault = (flags & GPBDescriptorInitializationFlag_FieldsWithDefault) != 0;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    GPBFileSyntax fileSyntax = file.syntax;
#pragma clang diagnostic pop

    for (uint32_t i = 0; i < fieldCount; ++i) {
      GPBMessageFieldDescription *coreDesc;
      if (fieldsIncludeDefault) {
        coreDesc = &((((GPBMessageFieldDescriptionWithDefault *)fieldDescriptions)[i]).core);
      } else {
        coreDesc = &(((GPBMessageFieldDescription *)fieldDescriptions)[i]);
      }

      if (fixClassRefs && GPBDataTypeIsMessage(coreDesc->dataType)) {
        const char *className = coreDesc->dataTypeSpecific.className;
        Class msgClass = objc_getClass(className);
        NSAssert(msgClass, @"Class %s not defined", className);
        coreDesc->dataTypeSpecific.clazz = msgClass;
      }

      if (fixProto3Optional) {
        // If it was...
        //  - proto3 syntax
        //  - not repeated/map
        //  - not in a oneof (negative has index)
        //  - not a message (the flag doesn't make sense for messages)
        BOOL clearOnZero = ((fileSyntax == GPBFileSyntaxProto3) &&
                            ((coreDesc->flags & (GPBFieldRepeated | GPBFieldMapKeyMask)) == 0) &&
                            (coreDesc->hasIndex >= 0) && !GPBDataTypeIsMessage(coreDesc->dataType));
        if (clearOnZero) {
          coreDesc->flags |= GPBFieldClearHasIvarOnZero;
        }
      }

      if (fixClosedEnums) {
        // NOTE: This isn't correct, it is using the syntax of the file that
        // declared the field, not the syntax of the file that declared the
        // enum; but for older generated code, that's all we have and that happens
        // to be what the runtime was doing (even though it was wrong). This is
        // only wrong in the rare cases an enum is declared in a proto3 syntax
        // file but used for a field in the proto2 syntax file.
        BOOL isClosedEnum =
            (coreDesc->dataType == GPBDataTypeEnum && fileSyntax != GPBFileSyntaxProto3);
        if (isClosedEnum) {
          coreDesc->flags |= GPBFieldClosedEnum;
        }
      }
    }
    flags |= (GPBDescriptorInitializationFlag_UsesClassRefs |
              GPBDescriptorInitializationFlag_Proto3OptionalKnown |
              GPBDescriptorInitializationFlag_ClosedEnumSupportKnown);
  }

  GPBDescriptor *result = [self allocDescriptorForClass:messageClass
                                            messageName:nil
                                        fileDescription:NULL
                                                 fields:fieldDescriptions
                                             fieldCount:fieldCount
                                            storageSize:storageSize
                                                  flags:flags];
  objc_setAssociatedObject(result, &kFileDescriptorCacheKey, file,
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  return result;
}

+ (instancetype)allocDescriptorForClass:(Class)messageClass
                              rootClass:(__unused Class)rootClass
                                   file:(GPBFileDescriptor *)file
                                 fields:(void *)fieldDescriptions
                             fieldCount:(uint32_t)fieldCount
                            storageSize:(uint32_t)storageSize
                                  flags:(GPBDescriptorInitializationFlags)flags {
  GPBInternalCompileAssert(GOOGLE_PROTOBUF_OBJC_MIN_SUPPORTED_VERSION <= 30006,
                           time_to_remove_this_old_version_shim);
  // The rootClass is no longer used, but it is passed as [ROOT class] to
  // ensure it was started up during initialization also when the message
  // scopes extensions.
  return [self allocDescriptorForClass:messageClass
                                  file:file
                                fields:fieldDescriptions
                            fieldCount:fieldCount
                           storageSize:storageSize
                                 flags:flags];
}

- (instancetype)initWithClass:(Class)messageClass
                  messageName:(NSString *)messageName
              fileDescription:(GPBFileDescription *)fileDescription
                       fields:(NSArray *)fields
                  storageSize:(uint32_t)storageSize
                   wireFormat:(BOOL)wireFormat {
  if ((self = [super init])) {
    messageClass_ = messageClass;
    messageName_ = [messageName copy];
    fileDescription_ = fileDescription;
    fields_ = [fields retain];
    storageSize_ = storageSize;
    wireFormat_ = wireFormat;
  }
  return self;
}

- (void)dealloc {
  [messageName_ release];
  [fields_ release];
  [oneofs_ release];
  [super dealloc];
}

// No need to provide -hash/-isEqual: as the instances are singletons and the
// default from NSObject is fine.
- (instancetype)copyWithZone:(__unused NSZone *)zone {
  // Immutable.
  return [self retain];
}

- (void)setupOneofs:(const char **)oneofNames
              count:(uint32_t)count
      firstHasIndex:(int32_t)firstHasIndex {
  NSCAssert(firstHasIndex < 0, @"Should always be <0");
  NSMutableArray *oneofs = [[NSMutableArray alloc] initWithCapacity:count];
  for (uint32_t i = 0, hasIndex = firstHasIndex; i < count; ++i, --hasIndex) {
    const char *name = oneofNames[i];
    NSArray *fieldsForOneof = NewFieldsArrayForHasIndex(hasIndex, fields_);
    NSCAssert(fieldsForOneof.count > 0, @"No fields for this oneof? (%s:%d)", name, hasIndex);
    GPBOneofDescriptor *oneofDescriptor = [[GPBOneofDescriptor alloc] initWithName:name
                                                                            fields:fieldsForOneof];
    [oneofs addObject:oneofDescriptor];
    [oneofDescriptor release];
    [fieldsForOneof release];
  }
  oneofs_ = oneofs;
}

- (void)setupExtraTextInfo:(const char *)extraTextFormatInfo {
  // Extra info is a compile time option, so skip the work if not needed.
  if (extraTextFormatInfo) {
    NSValue *extraInfoValue = [NSValue valueWithPointer:extraTextFormatInfo];
    for (GPBFieldDescriptor *fieldDescriptor in fields_) {
      if (fieldDescriptor->description_->flags & GPBFieldTextFormatNameCustom) {
        objc_setAssociatedObject(fieldDescriptor, &kTextFormatExtraValueKey, extraInfoValue,
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
      }
    }
  }
}

- (void)setupExtensionRanges:(const GPBExtensionRange *)ranges count:(int32_t)count {
  extensionRanges_ = ranges;
  extensionRangesCount_ = count;
}

- (void)setupContainingMessageClass:(Class)messageClass {
  objc_setAssociatedObject(self, &kParentClassValueKey, messageClass, OBJC_ASSOCIATION_ASSIGN);
}

- (void)setupContainingMessageClassName:(const char *)msgClassName {
  GPBInternalCompileAssert(GOOGLE_PROTOBUF_OBJC_MIN_SUPPORTED_VERSION <= 30003,
                           time_to_remove_this_old_version_shim);
  // Note: Only fetch the class here, can't send messages to it because
  // that could cause cycles back to this class within +initialize if
  // two messages have each other in fields (i.e. - they build a graph).
  Class clazz = objc_getClass(msgClassName);
  NSAssert(clazz, @"Class %s not defined", msgClassName);
  [self setupContainingMessageClass:clazz];
}

- (void)setupMessageClassNameSuffix:(NSString *)suffix {
  GPBInternalCompileAssert(GOOGLE_PROTOBUF_OBJC_MIN_SUPPORTED_VERSION <= 30007,
                           time_to_remove_this_old_version_shim);
  if (suffix.length) {
    objc_setAssociatedObject(self, &kClassNameSuffixKey, suffix, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }
}

- (NSString *)name {
  return NSStringFromClass(messageClass_);
}

- (GPBFileDescriptor *)file {
  @synchronized(self) {
    GPBFileDescriptor *result = objc_getAssociatedObject(self, &kFileDescriptorCacheKey);
    if (!result) {
#if defined(DEBUG) && DEBUG
      NSAssert(fileDescription_ != NULL, @"Internal error in generation/startup");
#endif
      // `package` and `prefix` can both be NULL if there wasn't one for the file.
      NSString *package = fileDescription_->package ? @(fileDescription_->package) : @"";
      if (fileDescription_->prefix) {
        result = [[GPBFileDescriptor alloc] initWithPackage:package
                                                 objcPrefix:@(fileDescription_->prefix)
                                                     syntax:fileDescription_->syntax];

      } else {
        result = [[GPBFileDescriptor alloc] initWithPackage:package
                                                     syntax:fileDescription_->syntax];
      }
      objc_setAssociatedObject(result, &kFileDescriptorCacheKey, result,
                               OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return result;
  }
}

- (GPBDescriptor *)containingType {
  Class parentClass = objc_getAssociatedObject(self, &kParentClassValueKey);
  return [parentClass descriptor];
}

- (NSString *)fullName {
  GPBDescriptor *parent = self.containingType;
  if (messageName_) {
    if (parent) {
      return [NSString stringWithFormat:@"%@.%@", parent.fullName, messageName_];
    }
    if (fileDescription_->package) {
      return [NSString stringWithFormat:@"%s.%@", fileDescription_->package, messageName_];
    }
    return messageName_;
  }

  GPBInternalCompileAssert(GOOGLE_PROTOBUF_OBJC_MIN_SUPPORTED_VERSION <= 30007,
                           time_to_remove_this_old_approach);
  // NOTE: When this code path is removed, this also means this api can't return nil any more but
  // that would be a breaking code change (not longer a Swift optional), so changing that will be
  // harder.

  NSString *className = NSStringFromClass(self.messageClass);
  GPBFileDescriptor *file = self.file;
  NSString *objcPrefix = file.objcPrefix;
  if (objcPrefix && ![className hasPrefix:objcPrefix]) {
    NSAssert(0, @"Class didn't have correct prefix? (%@ - %@)", className, objcPrefix);
    return nil;
  }

  NSString *name = nil;
  if (parent) {
    NSString *parentClassName = NSStringFromClass(parent.messageClass);
    // The generator will add _Class to avoid reserved words, drop it.
    NSString *suffix = objc_getAssociatedObject(parent, &kClassNameSuffixKey);
    if (suffix) {
      if (![parentClassName hasSuffix:suffix]) {
        NSAssert(0, @"ParentMessage class didn't have correct suffix? (%@ - %@)", className,
                 suffix);
        return nil;
      }
      parentClassName = [parentClassName substringToIndex:(parentClassName.length - suffix.length)];
    }
    NSString *parentPrefix = [parentClassName stringByAppendingString:@"_"];
    if (![className hasPrefix:parentPrefix]) {
      NSAssert(0, @"Class didn't have the correct parent name prefix? (%@ - %@)", parentPrefix,
               className);
      return nil;
    }
    name = [className substringFromIndex:parentPrefix.length];
  } else {
    name = [className substringFromIndex:objcPrefix.length];
  }

  // The generator will add _Class to avoid reserved words, drop it.
  NSString *suffix = objc_getAssociatedObject(self, &kClassNameSuffixKey);
  if (suffix) {
    if (![name hasSuffix:suffix]) {
      NSAssert(0, @"Message class didn't have correct suffix? (%@ - %@)", name, suffix);
      return nil;
    }
    name = [name substringToIndex:(name.length - suffix.length)];
  }

  NSString *prefix = (parent != nil ? parent.fullName : file.package);
  NSString *result;
  if (prefix.length > 0) {
    result = [NSString stringWithFormat:@"%@.%@", prefix, name];
  } else {
    result = name;
  }
  return result;
}

- (GPBFieldDescriptor *)fieldWithNumber:(uint32_t)fieldNumber {
  for (GPBFieldDescriptor *descriptor in fields_) {
    if (GPBFieldNumber(descriptor) == fieldNumber) {
      return descriptor;
    }
  }
  return nil;
}

- (GPBFieldDescriptor *)fieldWithName:(NSString *)name {
  for (GPBFieldDescriptor *descriptor in fields_) {
    if ([descriptor.name isEqual:name]) {
      return descriptor;
    }
  }
  return nil;
}

- (GPBOneofDescriptor *)oneofWithName:(NSString *)name {
  for (GPBOneofDescriptor *descriptor in oneofs_) {
    if ([descriptor.name isEqual:name]) {
      return descriptor;
    }
  }
  return nil;
}

@end

@implementation GPBFileDescriptor {
  NSString *package_;
  NSString *objcPrefix_;
  GPBFileSyntax syntax_;
}

@synthesize package = package_;
@synthesize objcPrefix = objcPrefix_;
@synthesize syntax = syntax_;

- (instancetype)initWithPackage:(NSString *)package
                     objcPrefix:(NSString *)objcPrefix
                         syntax:(GPBFileSyntax)syntax {
  self = [super init];
  if (self) {
    package_ = [package copy];
    objcPrefix_ = [objcPrefix copy];
    syntax_ = syntax;
  }
  return self;
}

- (instancetype)initWithPackage:(NSString *)package syntax:(GPBFileSyntax)syntax {
  self = [super init];
  if (self) {
    package_ = [package copy];
    syntax_ = syntax;
  }
  return self;
}

- (void)dealloc {
  [package_ release];
  [objcPrefix_ release];
  [super dealloc];
}

- (BOOL)isEqual:(id)other {
  if (other == self) {
    return YES;
  }
  if (![other isKindOfClass:[GPBFileDescriptor class]]) {
    return NO;
  }
  GPBFileDescriptor *otherFile = other;
  // objcPrefix can be nil, otherwise, straight up compare.
  return (syntax_ == otherFile->syntax_ && [package_ isEqual:otherFile->package_] &&
          (objcPrefix_ == otherFile->objcPrefix_ ||
           (otherFile->objcPrefix_ && [objcPrefix_ isEqual:otherFile->objcPrefix_])));
}

- (NSUInteger)hash {
  // The prefix is recommended to be the same for a given package, so just hash
  // the package.
  return [package_ hash];
}

- (instancetype)copyWithZone:(__unused NSZone *)zone {
  // Immutable.
  return [self retain];
}

@end

@implementation GPBOneofDescriptor

@synthesize fields = fields_;

- (instancetype)initWithName:(const char *)name fields:(NSArray *)fields {
  self = [super init];
  if (self) {
    name_ = name;
    fields_ = [fields retain];
    for (GPBFieldDescriptor *fieldDesc in fields) {
      fieldDesc->containingOneof_ = self;
    }

    caseSel_ = SelFromStrings(NULL, name, "OneOfCase", NO);
  }
  return self;
}

- (void)dealloc {
  [fields_ release];
  [super dealloc];
}

// No need to provide -hash/-isEqual: as the instances are singletons and the
// default from NSObject is fine.
- (instancetype)copyWithZone:(__unused NSZone *)zone {
  // Immutable.
  return [self retain];
}

- (NSString *)name {
  return (NSString *_Nonnull)@(name_);
}

- (GPBFieldDescriptor *)fieldWithNumber:(uint32_t)fieldNumber {
  for (GPBFieldDescriptor *descriptor in fields_) {
    if (GPBFieldNumber(descriptor) == fieldNumber) {
      return descriptor;
    }
  }
  return nil;
}

- (GPBFieldDescriptor *)fieldWithName:(NSString *)name {
  for (GPBFieldDescriptor *descriptor in fields_) {
    if ([descriptor.name isEqual:name]) {
      return descriptor;
    }
  }
  return nil;
}

@end

uint32_t GPBFieldTag(GPBFieldDescriptor *self) {
  GPBMessageFieldDescription *description = self->description_;
  GPBWireFormat format;
  if ((description->flags & GPBFieldMapKeyMask) != 0) {
    // Maps are repeated messages on the wire.
    format = GPBWireFormatForType(GPBDataTypeMessage, NO);
  } else {
    format =
        GPBWireFormatForType(description->dataType, ((description->flags & GPBFieldPacked) != 0));
  }
  return GPBWireFormatMakeTag(description->number, format);
}

uint32_t GPBFieldAlternateTag(GPBFieldDescriptor *self) {
  GPBMessageFieldDescription *description = self->description_;
  NSCAssert((description->flags & GPBFieldRepeated) != 0, @"Only valid on repeated fields");
  GPBWireFormat format =
      GPBWireFormatForType(description->dataType, ((description->flags & GPBFieldPacked) == 0));
  return GPBWireFormatMakeTag(description->number, format);
}

@implementation GPBFieldDescriptor {
  GPBGenericValue defaultValue_;

  // Message ivars
  Class msgClass_;

  // Enum ivars.
  GPBEnumDescriptor *enumDescriptor_;
}

@synthesize msgClass = msgClass_;
@synthesize containingOneof = containingOneof_;

- (instancetype)initWithFieldDescription:(void *)description
                         descriptorFlags:(GPBDescriptorInitializationFlags)descriptorFlags {
  if ((self = [super init])) {
    BOOL includesDefault =
        (descriptorFlags & GPBDescriptorInitializationFlag_FieldsWithDefault) != 0;
    GPBMessageFieldDescription *coreDesc;
    if (includesDefault) {
      coreDesc = &(((GPBMessageFieldDescriptionWithDefault *)description)->core);
    } else {
      coreDesc = description;
    }
    description_ = coreDesc;
    getSel_ = sel_getUid(coreDesc->name);
    setSel_ = SelFromStrings("set", coreDesc->name, NULL, YES);

    GPBDataType dataType = coreDesc->dataType;
    BOOL isMessage = GPBDataTypeIsMessage(dataType);
    BOOL isMapOrArray = GPBFieldIsMapOrArray(self);

    if (isMapOrArray) {
      // map<>/repeated fields get a *Count property (inplace of a has*) to
      // support checking if there are any entries without triggering
      // autocreation.
      hasOrCountSel_ = SelFromStrings(NULL, coreDesc->name, "_Count", NO);
    } else {
      // It is a single field; it gets has/setHas selectors if...
      //  - not in a oneof (negative has index)
      //  - not clearing on zero
      if ((coreDesc->hasIndex >= 0) && ((coreDesc->flags & GPBFieldClearHasIvarOnZero) == 0)) {
        hasOrCountSel_ = SelFromStrings("has", coreDesc->name, NULL, NO);
        setHasSel_ = SelFromStrings("setHas", coreDesc->name, NULL, YES);
      }
    }

    // Extra type specific data.
    if (isMessage) {
      // Note: Only fetch the class here, can't send messages to it because
      // that could cause cycles back to this class within +initialize if
      // two messages have each other in fields (i.e. - they build a graph).
      msgClass_ = coreDesc->dataTypeSpecific.clazz;
    } else if (dataType == GPBDataTypeEnum) {
      enumDescriptor_ = coreDesc->dataTypeSpecific.enumDescFunc();
#if defined(DEBUG) && DEBUG
      NSAssert((coreDesc->flags & GPBFieldHasEnumDescriptor) != 0,
               @"Field must have GPBFieldHasEnumDescriptor set");
#endif  // DEBUG
    }

    // Non map<>/repeated fields can have defaults in proto2 syntax.
    if (!isMapOrArray && includesDefault) {
      defaultValue_ = ((GPBMessageFieldDescriptionWithDefault *)description)->defaultValue;
      if (dataType == GPBDataTypeBytes) {
        // Data stored as a length prefixed (network byte order) c-string in
        // descriptor structure.
        const uint8_t *bytes = (const uint8_t *)defaultValue_.valueData;
        if (bytes) {
          uint32_t length;
          memcpy(&length, bytes, sizeof(length));
          length = ntohl(length);
          bytes += sizeof(length);
          defaultValue_.valueData = [[NSData alloc] initWithBytes:bytes length:length];
        }
      }
    }
  }
  return self;
}

- (void)dealloc {
  if (description_->dataType == GPBDataTypeBytes && !(description_->flags & GPBFieldRepeated)) {
    [defaultValue_.valueData release];
  }
  [super dealloc];
}

// No need to provide -hash/-isEqual: as the instances are singletons and the
// default from NSObject is fine.
- (instancetype)copyWithZone:(__unused NSZone *)zone {
  // Immutable.
  return [self retain];
}

- (GPBDataType)dataType {
  return description_->dataType;
}

- (BOOL)hasDefaultValue {
  return (description_->flags & GPBFieldHasDefaultValue) != 0;
}

- (uint32_t)number {
  return description_->number;
}

- (NSString *)name {
  return (NSString *_Nonnull)@(description_->name);
}

- (BOOL)isRequired {
  return (description_->flags & GPBFieldRequired) != 0;
}

- (BOOL)isOptional {
  return (description_->flags & GPBFieldOptional) != 0;
}

- (GPBFieldType)fieldType {
  GPBFieldFlags flags = description_->flags;
  if ((flags & GPBFieldRepeated) != 0) {
    return GPBFieldTypeRepeated;
  } else if ((flags & GPBFieldMapKeyMask) != 0) {
    return GPBFieldTypeMap;
  } else {
    return GPBFieldTypeSingle;
  }
}

- (GPBDataType)mapKeyDataType {
  switch (description_->flags & GPBFieldMapKeyMask) {
    case GPBFieldMapKeyInt32:
      return GPBDataTypeInt32;
    case GPBFieldMapKeyInt64:
      return GPBDataTypeInt64;
    case GPBFieldMapKeyUInt32:
      return GPBDataTypeUInt32;
    case GPBFieldMapKeyUInt64:
      return GPBDataTypeUInt64;
    case GPBFieldMapKeySInt32:
      return GPBDataTypeSInt32;
    case GPBFieldMapKeySInt64:
      return GPBDataTypeSInt64;
    case GPBFieldMapKeyFixed32:
      return GPBDataTypeFixed32;
    case GPBFieldMapKeyFixed64:
      return GPBDataTypeFixed64;
    case GPBFieldMapKeySFixed32:
      return GPBDataTypeSFixed32;
    case GPBFieldMapKeySFixed64:
      return GPBDataTypeSFixed64;
    case GPBFieldMapKeyBool:
      return GPBDataTypeBool;
    case GPBFieldMapKeyString:
      return GPBDataTypeString;

    default:
      NSAssert(0, @"Not a map type");
      return GPBDataTypeInt32;  // For lack of anything better.
  }
}

- (BOOL)isPackable {
  return (description_->flags & GPBFieldPacked) != 0;
}

- (BOOL)isValidEnumValue:(int32_t)value {
  NSAssert(description_->dataType == GPBDataTypeEnum, @"Field Must be of type GPBDataTypeEnum");
  return enumDescriptor_.enumVerifier(value);
}

- (GPBEnumDescriptor *)enumDescriptor {
  return enumDescriptor_;
}

- (GPBGenericValue)defaultValue {
  // Depends on the fact that defaultValue_ is initialized either to "0/nil" or
  // to an actual defaultValue in our initializer.
  GPBGenericValue value = defaultValue_;

  if (!(description_->flags & GPBFieldRepeated)) {
    // We special handle data and strings. If they are nil, we replace them
    // with empty string/empty data.
    GPBDataType type = description_->dataType;
    if (type == GPBDataTypeBytes && value.valueData == nil) {
      value.valueData = GPBEmptyNSData();
    } else if (type == GPBDataTypeString && value.valueString == nil) {
      value.valueString = @"";
    }
  }
  return value;
}

- (NSString *)textFormatName {
  if ((description_->flags & GPBFieldTextFormatNameCustom) != 0) {
    NSValue *extraInfoValue = objc_getAssociatedObject(self, &kTextFormatExtraValueKey);
    // Support can be left out at generation time.
    if (!extraInfoValue) {
      return nil;
    }
    const uint8_t *extraTextFormatInfo = [extraInfoValue pointerValue];
    return GPBDecodeTextFormatName(extraTextFormatInfo, GPBFieldNumber(self), self.name);
  }

  // The logic here has to match SetCommonFieldVariables() from
  // objectivec/field.cc in the proto compiler.
  NSString *name = self.name;
  NSUInteger len = [name length];

  // Remove the "_p" added to reserved names.
  if ([name hasSuffix:@"_p"]) {
    name = [name substringToIndex:(len - 2)];
    len = [name length];
  }

  // Remove "Array" from the end for repeated fields.
  if (((description_->flags & GPBFieldRepeated) != 0) && [name hasSuffix:@"Array"]) {
    name = [name substringToIndex:(len - 5)];
    len = [name length];
  }

  // Groups vs. other fields.
  if (description_->dataType == GPBDataTypeGroup) {
    // Just capitalize the first letter.
    unichar firstChar = [name characterAtIndex:0];
    if (firstChar >= 'a' && firstChar <= 'z') {
      NSString *firstCharString =
          [NSString stringWithFormat:@"%C", (unichar)(firstChar - 'a' + 'A')];
      NSString *result = [name stringByReplacingCharactersInRange:NSMakeRange(0, 1)
                                                       withString:firstCharString];
      return result;
    }
    return name;

  } else {
    // Undo the CamelCase.
    NSMutableString *result = [NSMutableString stringWithCapacity:len];
    for (uint32_t i = 0; i < len; i++) {
      unichar c = [name characterAtIndex:i];
      if (c >= 'A' && c <= 'Z') {
        if (i > 0) {
          [result appendFormat:@"_%C", (unichar)(c - 'A' + 'a')];
        } else {
          [result appendFormat:@"%C", c];
        }
      } else {
        [result appendFormat:@"%C", c];
      }
    }
    return result;
  }
}

@end

@implementation GPBEnumDescriptor {
  NSString *name_;
  // valueNames_ is a single c string with all of the value names appended
  // together, each null terminated.  -calcValueNameOffsets fills in
  // nameOffsets_ with the offsets to allow quicker access to the individual
  // names.
  const char *valueNames_;
  const int32_t *values_;
  GPBEnumValidationFunc enumVerifier_;
  const uint8_t *extraTextFormatInfo_;
  uint32_t *nameOffsets_;
  uint32_t valueCount_;
  uint32_t flags_;
}

@synthesize name = name_;
@synthesize enumVerifier = enumVerifier_;

+ (instancetype)allocDescriptorForName:(NSString *)name
                            valueNames:(const char *)valueNames
                                values:(const int32_t *)values
                                 count:(uint32_t)valueCount
                          enumVerifier:(GPBEnumValidationFunc)enumVerifier
                                 flags:(GPBEnumDescriptorInitializationFlags)flags {
  // Compute the unknown flags by this version of the runtime and then check the passed in flags
  // (from the generated code) to detect when sources from a newer version are being used with an
  // older runtime.
  GPBEnumDescriptorInitializationFlags unknownFlags =
      ~(GPBEnumDescriptorInitializationFlag_IsClosed);
  if ((flags & unknownFlags) != 0) {
    GPBRuntimeMatchFailure();
  }
  GPBEnumDescriptor *descriptor = [[self alloc] initWithName:name
                                                  valueNames:valueNames
                                                      values:values
                                                       count:valueCount
                                                enumVerifier:enumVerifier
                                                       flags:flags];
  return descriptor;
}

+ (instancetype)allocDescriptorForName:(NSString *)name
                            valueNames:(const char *)valueNames
                                values:(const int32_t *)values
                                 count:(uint32_t)valueCount
                          enumVerifier:(GPBEnumValidationFunc)enumVerifier
                                 flags:(GPBEnumDescriptorInitializationFlags)flags
                   extraTextFormatInfo:(const char *)extraTextFormatInfo {
  // Call the common case.
  GPBEnumDescriptor *descriptor = [self allocDescriptorForName:name
                                                    valueNames:valueNames
                                                        values:values
                                                         count:valueCount
                                                  enumVerifier:enumVerifier
                                                         flags:flags];
  // Set the extra info.
  descriptor->extraTextFormatInfo_ = (const uint8_t *)extraTextFormatInfo;
  return descriptor;
}

+ (instancetype)allocDescriptorForName:(NSString *)name
                            valueNames:(const char *)valueNames
                                values:(const int32_t *)values
                                 count:(uint32_t)valueCount
                          enumVerifier:(GPBEnumValidationFunc)enumVerifier {
  GPBInternalCompileAssert(GOOGLE_PROTOBUF_OBJC_MIN_SUPPORTED_VERSION <= 30005,
                           time_to_remove_this_old_version_shim);
  return [self allocDescriptorForName:name
                           valueNames:valueNames
                               values:values
                                count:valueCount
                         enumVerifier:enumVerifier
                                flags:GPBEnumDescriptorInitializationFlag_None];
}

+ (instancetype)allocDescriptorForName:(NSString *)name
                            valueNames:(const char *)valueNames
                                values:(const int32_t *)values
                                 count:(uint32_t)valueCount
                          enumVerifier:(GPBEnumValidationFunc)enumVerifier
                   extraTextFormatInfo:(const char *)extraTextFormatInfo {
  GPBInternalCompileAssert(GOOGLE_PROTOBUF_OBJC_MIN_SUPPORTED_VERSION <= 30005,
                           time_to_remove_this_old_version_shim);
  return [self allocDescriptorForName:name
                           valueNames:valueNames
                               values:values
                                count:valueCount
                         enumVerifier:enumVerifier
                                flags:GPBEnumDescriptorInitializationFlag_None
                  extraTextFormatInfo:extraTextFormatInfo];
}

- (instancetype)initWithName:(NSString *)name
                  valueNames:(const char *)valueNames
                      values:(const int32_t *)values
                       count:(uint32_t)valueCount
                enumVerifier:(GPBEnumValidationFunc)enumVerifier
                       flags:(GPBEnumDescriptorInitializationFlags)flags {
  if ((self = [super init])) {
    name_ = [name copy];
    valueNames_ = valueNames;
    values_ = values;
    valueCount_ = valueCount;
    enumVerifier_ = enumVerifier;
    flags_ = flags;
  }
  return self;
}

- (void)dealloc {
  [name_ release];
  if (nameOffsets_) free(nameOffsets_);
  [super dealloc];
}

// No need to provide -hash/-isEqual: as the instances are singletons and the
// default from NSObject is fine.
- (instancetype)copyWithZone:(__unused NSZone *)zone {
  // Immutable.
  return [self retain];
}

- (BOOL)isClosed {
  return (flags_ & GPBEnumDescriptorInitializationFlag_IsClosed) != 0;
}

- (void)calcValueNameOffsets {
  @synchronized(self) {
    if (nameOffsets_ != NULL) {
      return;
    }
    uint32_t *offsets = malloc(valueCount_ * sizeof(uint32_t));
    if (!offsets) return;
    const char *scan = valueNames_;
    for (uint32_t i = 0; i < valueCount_; ++i) {
      offsets[i] = (uint32_t)(scan - valueNames_);
      while (*scan != '\0') ++scan;
      ++scan;  // Step over the null.
    }
    nameOffsets_ = offsets;
  }
}

- (NSString *)enumNameForValue:(int32_t)number {
  for (uint32_t i = 0; i < valueCount_; ++i) {
    if (values_[i] == number) {
      return [self getEnumNameForIndex:i];
    }
  }
  return nil;
}

- (BOOL)getValue:(int32_t *)outValue forEnumName:(NSString *)name {
  // Must have the prefix.
  NSUInteger prefixLen = name_.length + 1;
  if ((name.length <= prefixLen) || ![name hasPrefix:name_] ||
      ([name characterAtIndex:prefixLen - 1] != '_')) {
    return NO;
  }

  // Skip over the prefix.
  const char *nameAsCStr = [name UTF8String];
  nameAsCStr += prefixLen;

  [self calcValueNameOffsets];
  if (nameOffsets_ == NULL) return NO;

  // Find it.
  for (uint32_t i = 0; i < valueCount_; ++i) {
    const char *valueName = valueNames_ + nameOffsets_[i];
    if (strcmp(nameAsCStr, valueName) == 0) {
      if (outValue) {
        *outValue = values_[i];
      }
      return YES;
    }
  }
  return NO;
}

- (BOOL)getValue:(int32_t *)outValue forEnumTextFormatName:(NSString *)textFormatName {
  [self calcValueNameOffsets];
  if (nameOffsets_ == NULL) return NO;

  for (uint32_t i = 0; i < valueCount_; ++i) {
    NSString *valueTextFormatName = [self getEnumTextFormatNameForIndex:i];
    if ([valueTextFormatName isEqual:textFormatName]) {
      if (outValue) {
        *outValue = values_[i];
      }
      return YES;
    }
  }
  return NO;
}

- (NSString *)textFormatNameForValue:(int32_t)number {
  // Find the EnumValue descriptor and its index.
  BOOL foundIt = NO;
  uint32_t valueDescriptorIndex;
  for (valueDescriptorIndex = 0; valueDescriptorIndex < valueCount_; ++valueDescriptorIndex) {
    if (values_[valueDescriptorIndex] == number) {
      foundIt = YES;
      break;
    }
  }

  if (!foundIt) {
    return nil;
  }
  return [self getEnumTextFormatNameForIndex:valueDescriptorIndex];
}

- (uint32_t)enumNameCount {
  return valueCount_;
}

- (NSString *)getEnumNameForIndex:(uint32_t)index {
  [self calcValueNameOffsets];
  if (nameOffsets_ == NULL) return nil;

  if (index >= valueCount_) {
    return nil;
  }
  const char *valueName = valueNames_ + nameOffsets_[index];
  NSString *fullName = [NSString stringWithFormat:@"%@_%s", name_, valueName];
  return fullName;
}

- (NSString *)getEnumTextFormatNameForIndex:(uint32_t)index {
  [self calcValueNameOffsets];
  if (nameOffsets_ == NULL) return nil;

  if (index >= valueCount_) {
    return nil;
  }
  NSString *result = nil;
  // Naming adds an underscore between enum name and value name, skip that also.
  const char *valueName = valueNames_ + nameOffsets_[index];
  NSString *shortName = @(valueName);

  // See if it is in the map of special format handling.
  if (extraTextFormatInfo_) {
    result = GPBDecodeTextFormatName(extraTextFormatInfo_, (int32_t)index, shortName);
  }
  // Logic here needs to match what objectivec/enum.cc does in the proto
  // compiler.
  if (result == nil) {
    NSUInteger len = [shortName length];
    NSMutableString *worker = [NSMutableString stringWithCapacity:len];
    for (NSUInteger i = 0; i < len; i++) {
      unichar c = [shortName characterAtIndex:i];
      if (i > 0 && c >= 'A' && c <= 'Z') {
        [worker appendString:@"_"];
      }
      [worker appendFormat:@"%c", toupper((char)c)];
    }
    result = worker;
  }
  return result;
}

@end

@implementation GPBExtensionDescriptor {
  GPBGenericValue defaultValue_;
}

- (instancetype)initWithExtensionDescription:(GPBExtensionDescription *)desc
                               usesClassRefs:(BOOL)usesClassRefs {
  // Compute the unknown options by this version of the runtime and then check the passed in
  // descriptor's options (from the generated code) to detect when sources from a newer version are
  // being used with an older runtime.
  GPBExtensionOptions unknownOptions =
      ~(GPBExtensionRepeated | GPBExtensionPacked | GPBExtensionSetWireFormat);
  if ((desc->options & unknownOptions) != 0) {
    GPBRuntimeMatchFailure();
  }

#if defined(DEBUG) && DEBUG
  NSAssert(usesClassRefs, @"Internal error: all extensions should have class refs");
#endif

  if ((self = [super init])) {
    description_ = desc;

    GPBDataType type = description_->dataType;
    if (type == GPBDataTypeBytes) {
      // Data stored as a length prefixed c-string in descriptor records.
      const uint8_t *bytes = (const uint8_t *)description_->defaultValue.valueData;
      if (bytes) {
        uint32_t length;
        memcpy(&length, bytes, sizeof(length));
        // The length is stored in network byte order.
        length = ntohl(length);
        bytes += sizeof(length);
        defaultValue_.valueData = [[NSData alloc] initWithBytes:bytes length:length];
      }
    } else if (type == GPBDataTypeMessage || type == GPBDataTypeGroup) {
      // The default is looked up in -defaultValue instead since extensions
      // aren't common, we avoid the hit startup hit and it avoids initialization
      // order issues.
    } else {
      defaultValue_ = description_->defaultValue;
    }
  }
  return self;
}

- (instancetype)initWithExtensionDescription:(GPBExtensionDescription *)desc {
  GPBInternalCompileAssert(GOOGLE_PROTOBUF_OBJC_MIN_SUPPORTED_VERSION <= 30003,
                           time_to_remove_this_old_version_shim);

  const char *className = desc->messageOrGroupClass.name;
  if (className) {
    Class clazz = objc_lookUpClass(className);
    NSAssert(clazz != Nil, @"Class %s not defined", className);
    desc->messageOrGroupClass.clazz = clazz;
  }

  const char *extendedClassName = desc->extendedClass.name;
  if (extendedClassName) {
    Class clazz = objc_lookUpClass(extendedClassName);
    NSAssert(clazz, @"Class %s not defined", extendedClassName);
    desc->extendedClass.clazz = clazz;
  }

  return [self initWithExtensionDescription:desc usesClassRefs:YES];
}

- (void)dealloc {
  if ((description_->dataType == GPBDataTypeBytes) && !GPBExtensionIsRepeated(description_)) {
    [defaultValue_.valueData release];
  }
  [super dealloc];
}

// No need to provide -hash/-isEqual: as the instances are singletons and the
// default from NSObject is fine.
- (instancetype)copyWithZone:(__unused NSZone *)zone {
  // Immutable.
  return [self retain];
}

- (NSString *)singletonName {
  return (NSString *_Nonnull)@(description_->singletonName);
}

- (const char *)singletonNameC {
  return description_->singletonName;
}

- (uint32_t)fieldNumber {
  return description_->fieldNumber;
}

- (GPBDataType)dataType {
  return description_->dataType;
}

- (GPBWireFormat)wireType {
  return GPBWireFormatForType(description_->dataType, GPBExtensionIsPacked(description_));
}

- (GPBWireFormat)alternateWireType {
  NSAssert(GPBExtensionIsRepeated(description_), @"Only valid on repeated extensions");
  return GPBWireFormatForType(description_->dataType, !GPBExtensionIsPacked(description_));
}

- (BOOL)isRepeated {
  return GPBExtensionIsRepeated(description_);
}

- (BOOL)isPackable {
  return GPBExtensionIsPacked(description_);
}

- (Class)msgClass {
  return description_->messageOrGroupClass.clazz;
}

- (Class)containingMessageClass {
  return description_->extendedClass.clazz;
}

- (GPBEnumDescriptor *)enumDescriptor {
  if (description_->dataType == GPBDataTypeEnum) {
    GPBEnumDescriptor *enumDescriptor = description_->enumDescriptorFunc();
    return enumDescriptor;
  }
  return nil;
}

- (id)defaultValue {
  if (GPBExtensionIsRepeated(description_)) {
    return nil;
  }

  switch (description_->dataType) {
    case GPBDataTypeBool:
      return @(defaultValue_.valueBool);
    case GPBDataTypeFloat:
      return @(defaultValue_.valueFloat);
    case GPBDataTypeDouble:
      return @(defaultValue_.valueDouble);
    case GPBDataTypeInt32:
    case GPBDataTypeSInt32:
    case GPBDataTypeEnum:
    case GPBDataTypeSFixed32:
      return @(defaultValue_.valueInt32);
    case GPBDataTypeInt64:
    case GPBDataTypeSInt64:
    case GPBDataTypeSFixed64:
      return @(defaultValue_.valueInt64);
    case GPBDataTypeUInt32:
    case GPBDataTypeFixed32:
      return @(defaultValue_.valueUInt32);
    case GPBDataTypeUInt64:
    case GPBDataTypeFixed64:
      return @(defaultValue_.valueUInt64);
    case GPBDataTypeBytes:
      // Like message fields, the default is zero length data.
      return (defaultValue_.valueData ? defaultValue_.valueData : GPBEmptyNSData());
    case GPBDataTypeString:
      // Like message fields, the default is zero length string.
      return (defaultValue_.valueString ? defaultValue_.valueString : @"");
    case GPBDataTypeGroup:
    case GPBDataTypeMessage:
      return nil;
  }
}

- (NSComparisonResult)compareByFieldNumber:(GPBExtensionDescriptor *)other {
  int32_t selfNumber = description_->fieldNumber;
  int32_t otherNumber = other->description_->fieldNumber;
  if (selfNumber < otherNumber) {
    return NSOrderedAscending;
  } else if (selfNumber == otherNumber) {
    return NSOrderedSame;
  } else {
    return NSOrderedDescending;
  }
}

@end

#pragma clang diagnostic pop
