// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import <stdatomic.h>

#import "GPBDescriptor_PackagePrivate.h"
#import "GPBExtensionRegistry.h"
#import "GPBMessage.h"
#import "GPBProtocolBuffers_RuntimeSupport.h"
#import "GPBRootObject_PackagePrivate.h"
#import "GPBTestUtilities.h"

// This is a fake version, so the ptr check will fail.
static const int32_t FAKE_GOOGLE_PROTOBUF_OBJC_EXPECTED_GENCODE_VERSION_40311 = 40311;

@interface MessageBadVersionFormatTest : GPBTestCase
@end

// clang-format off
// NOLINTBEGIN

// -------------------------------------------------------------------------------------------------
//
// This is extracted from generated code with the 40311 format but then edited so the pass in
// unknown values for the version support, the original proto was as follows:
//
//  syntax = "proto2";
//
//  enum EnumBadVersion {
//    FOO = 0;
//    BAR = 1;
//  }
//
//  message MessageBadVersion {
//    optional EnumBadVersion value = 1;
//    extensions 100 to max;
//  }
//
//  extend MessageBadVersion {
//    optional MessageBadVersion other_m = 100;
//    optional EnumBadVersion other_e = 101;
//  }
//
// -------------------------------------------------------------------------------------------------

NS_ASSUME_NONNULL_BEGIN

typedef GPB_ENUM(EnumBadVersion) {
  EnumBadVersion_Foo = 0,
  EnumBadVersion_Bar = 1,
};

GPBEnumDescriptor *EnumBadVersion_EnumDescriptor(void);

BOOL EnumBadVersion_IsValidValue(int32_t value);

GPB_FINAL @interface TestBadVersionRoot : GPBRootObject
@end

@interface TestBadVersionRoot (DynamicMethods)
+ (GPBExtensionDescriptor *)otherM;
+ (GPBExtensionDescriptor *)otherE;
@end

typedef GPB_ENUM(MessageBadVersion_FieldNumber) {
  MessageBadVersion_FieldNumber_Value = 1,
};

GPB_FINAL @interface MessageBadVersion : GPBMessage

@property(nonatomic, readwrite) EnumBadVersion value;
@property(nonatomic, readwrite) BOOL hasValue;

@end

NS_ASSUME_NONNULL_END

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"

GPBObjCClassDeclaration(MessageBadVersion);

@implementation TestBadVersionRoot

+ (GPBExtensionRegistry*)extensionRegistry {
  // This is called by +initialize so there is no need to worry
  // about thread safety and initialization of registry.
  static GPBExtensionRegistry* registry = nil;
  if (!registry) {
    registry = [[GPBExtensionRegistry alloc] init];
    static GPBExtensionDescription descriptions[] = {
      {
        .defaultValue.valueMessage = nil,
        .singletonName = GPBStringifySymbol(TestBadVersionRoot) "_otherM",
        .extendedClass.clazz = GPBObjCClass(MessageBadVersion),
        .messageOrGroupClass.clazz = GPBObjCClass(MessageBadVersion),
        .enumDescriptorFunc = NULL,
        .fieldNumber = 100,
        .dataType = GPBDataTypeMessage,
        .options = GPBExtensionNone,
      },
      {
        .defaultValue.valueEnum = EnumBadVersion_Foo,
        .singletonName = GPBStringifySymbol(TestBadVersionRoot) "_otherE",
        .extendedClass.clazz = GPBObjCClass(MessageBadVersion),
        .messageOrGroupClass.clazz = Nil,
        .enumDescriptorFunc = EnumBadVersion_EnumDescriptor,
        .fieldNumber = 101,
        .dataType = GPBDataTypeEnum,
        .options = GPBExtensionNone,
      },
    };
    for (size_t i = 0; i < sizeof(descriptions) / sizeof(descriptions[0]); ++i) {
      GPBExtensionDescriptor *extension =
          [[GPBExtensionDescriptor alloc] initWithExtensionDescription:&descriptions[i]
                                                        runtimeSupport:&FAKE_GOOGLE_PROTOBUF_OBJC_EXPECTED_GENCODE_VERSION_40311];
      [registry addExtension:extension];
      [self globallyRegisterExtension:extension];
      [extension release];
    }
    // None of the imports (direct or indirect) defined extensions, so no need to add
    // them to this registry.
  }
  return registry;
}

@end

static GPBFilePackageAndPrefix TestBadVersionRoot_FileDescription = {
  .package = NULL,
  .prefix = NULL
};

GPBEnumDescriptor *EnumBadVersion_EnumDescriptor(void) {
  static _Atomic(GPBEnumDescriptor*) descriptor = nil;
  if (!descriptor) {
    static const char *valueNames =
        "Foo\000Bar\000";
    static const int32_t values[] = {
        EnumBadVersion_Foo,
        EnumBadVersion_Bar,
    };
    GPBEnumDescriptor *worker =
        [GPBEnumDescriptor allocDescriptorForName:GPBNSStringifySymbol(EnumBadVersion)
                                   runtimeSupport:&FAKE_GOOGLE_PROTOBUF_OBJC_EXPECTED_GENCODE_VERSION_40311
                                       valueNames:valueNames
                                           values:values
                                            count:(uint32_t)(sizeof(values) / sizeof(int32_t))
                                     enumVerifier:EnumBadVersion_IsValidValue
                                            flags:GPBEnumDescriptorInitializationFlag_IsClosed];
    GPBEnumDescriptor *expected = nil;
    if (!atomic_compare_exchange_strong(&descriptor, &expected, worker)) {
      [worker release];
    }
  }
  return descriptor;
}

BOOL EnumBadVersion_IsValidValue(int32_t value__) {
  switch (value__) {
    case EnumBadVersion_Foo:
    case EnumBadVersion_Bar:
      return YES;
    default:
      return NO;
  }
}

#pragma mark - MessageBadVersion

@implementation MessageBadVersion

@dynamic hasValue, value;

typedef struct MessageBadVersion__storage_ {
  uint32_t _has_storage_[1];
  EnumBadVersion value;
} MessageBadVersion__storage_;

+ (GPBDescriptor *)descriptor {
  static GPBDescriptor *descriptor = nil;
  if (!descriptor) {
    static GPBMessageFieldDescription fields[] = {
      {
        .name = "value",
        .dataTypeSpecific.enumDescFunc = EnumBadVersion_EnumDescriptor,
        .number = MessageBadVersion_FieldNumber_Value,
        .hasIndex = 0,
        .offset = (uint32_t)offsetof(MessageBadVersion__storage_, value),
        .flags = GPBFieldNone,
        .dataType = GPBDataTypeEnum,
      },
    };
    GPBDescriptor *localDescriptor =
        [GPBDescriptor allocDescriptorForClass:GPBObjCClass(MessageBadVersion)
                                   messageName:@"MessageBadVersion"
                                runtimeSupport:&FAKE_GOOGLE_PROTOBUF_OBJC_EXPECTED_GENCODE_VERSION_40311
                               fileDescription:&TestBadVersionRoot_FileDescription
                                        fields:fields
                                    fieldCount:(uint32_t)(sizeof(fields) / sizeof(GPBMessageFieldDescription))
                                   storageSize:sizeof(MessageBadVersion__storage_)
                                         flags:GPBDescriptorInitializationFlag_None];
    static const GPBExtensionRange ranges[] = {
      { .start = 100, .end = 536870912 },
    };
    [localDescriptor setupExtensionRanges:ranges
                                    count:(uint32_t)(sizeof(ranges) / sizeof(GPBExtensionRange))];
    #if defined(DEBUG) && DEBUG
      NSAssert(descriptor == nil, @"Startup recursed!");
    #endif  // DEBUG
    descriptor = localDescriptor;
  }
  return descriptor;
}

@end

#pragma clang diagnostic pop

// NOLINTEND
// clang-format on

// -------------------------------------------------------------------------------------------------

@implementation MessageBadVersionFormatTest

- (void)testMessageBadVersionFormat {
  // Calling each one should try to start it up and result in a throw for an unknown version marker.
  // Mostly this shouldn't happen as the symbol should be coming out of the runtime library so
  // things should result in a link error before getting to the runtime check; this is just an added
  // safety check.
  XCTAssertThrowsSpecificNamed(EnumBadVersion_EnumDescriptor(), NSException,
                               NSInternalInconsistencyException);

  XCTAssertThrowsSpecificNamed([TestBadVersionRoot otherM], NSException,
                               NSInternalInconsistencyException);

  XCTAssertThrowsSpecificNamed([MessageBadVersion class], NSException,
                               NSInternalInconsistencyException);
}

@end
