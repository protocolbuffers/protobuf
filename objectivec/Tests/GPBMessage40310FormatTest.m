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

#if GOOGLE_PROTOBUF_OBJC_MIN_SUPPORTED_VERSION > 40310
#error "Time to remove this test."
#else

@interface Message40310FormatTest : GPBTestCase
@end

// clang-format off
// NOLINTBEGIN

// -------------------------------------------------------------------------------------------------
//
// This is extracted from generated code with the 40310 format for the following proto:
//
//  syntax = "proto2";
//
//  enum Enum40310 {
//    FOO = 0;
//    BAR = 1;
//  }
//
//  message Message40310 {
//    message SubMessage {
//      optional int32 x = 1;
//    }
//    enum SubEnum {
//      SUB_FOO = 0;
//      SUB_BAR = 1;
//    }
//    optional string name = 1;
//    optional Enum40310 value = 2;
//    repeated string names = 11;
//    repeated Enum40310 values = 12;
//    optional SubMessage sub_m = 20;
//    optional SubEnum sub_e = 21;
//    extensions 100 to max;
//  }
//
//  extend Message40310 {
//    optional Message40310 other_m = 100;
//    optional Enum40310 other_e = 101;
//    repeated Message40310 other_ms = 110;
//    repeated Enum40310 other_es = 111;
//  }
//
// -------------------------------------------------------------------------------------------------

@class Message40310_SubMessage;

NS_ASSUME_NONNULL_BEGIN

typedef GPB_ENUM(Enum40310) {
  Enum40310_Foo = 0,
  Enum40310_Bar = 1,
};

GPBEnumDescriptor *Enum40310_EnumDescriptor(void);
BOOL Enum40310_IsValidValue(int32_t value);

typedef GPB_ENUM(Message40310_SubEnum) {
  Message40310_SubEnum_SubFoo = 0,
  Message40310_SubEnum_SubBar = 1,
};

GPBEnumDescriptor *Message40310_SubEnum_EnumDescriptor(void);

BOOL Message40310_SubEnum_IsValidValue(int32_t value);

GPB_FINAL @interface Test40310Root : GPBRootObject
@end

@interface Test40310Root (DynamicMethods)
+ (GPBExtensionDescriptor *)otherM;
+ (GPBExtensionDescriptor *)otherE;
+ (GPBExtensionDescriptor *)otherMs;
+ (GPBExtensionDescriptor *)otherEs;
@end

typedef GPB_ENUM(Message40310_FieldNumber) {
  Message40310_FieldNumber_Name = 1,
  Message40310_FieldNumber_Value = 2,
  Message40310_FieldNumber_NamesArray = 11,
  Message40310_FieldNumber_ValuesArray = 12,
  Message40310_FieldNumber_SubM = 20,
  Message40310_FieldNumber_SubE = 21,
};

GPB_FINAL @interface Message40310 : GPBMessage

@property(nonatomic, readwrite, copy, null_resettable) NSString *name;
@property(nonatomic, readwrite) BOOL hasName;

@property(nonatomic, readwrite) Enum40310 value;
@property(nonatomic, readwrite) BOOL hasValue;

@property(nonatomic, readwrite, strong, null_resettable) NSMutableArray<NSString*> *namesArray;
@property(nonatomic, readonly) NSUInteger namesArray_Count;

@property(nonatomic, readwrite, strong, null_resettable) GPBEnumArray *valuesArray;
@property(nonatomic, readonly) NSUInteger valuesArray_Count;

@property(nonatomic, readwrite, strong, null_resettable) Message40310_SubMessage *subM;
@property(nonatomic, readwrite) BOOL hasSubM;

@property(nonatomic, readwrite) Message40310_SubEnum subE;
@property(nonatomic, readwrite) BOOL hasSubE;

@end

typedef GPB_ENUM(Message40310_SubMessage_FieldNumber) {
  Message40310_SubMessage_FieldNumber_X = 1,
};

GPB_FINAL @interface Message40310_SubMessage : GPBMessage

@property(nonatomic, readwrite) int32_t x;
@property(nonatomic, readwrite) BOOL hasX;

@end

NS_ASSUME_NONNULL_END

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"

GPBObjCClassDeclaration(Message40310);
GPBObjCClassDeclaration(Message40310_SubMessage);

@implementation Test40310Root

+ (GPBExtensionRegistry*)extensionRegistry {
  // This is called by +initialize so there is no need to worry
  // about thread safety and initialization of registry.
  static GPBExtensionRegistry* registry = nil;
  if (!registry) {
    registry = [[GPBExtensionRegistry alloc] init];
    static GPBExtensionDescription descriptions[] = {
      {
        .defaultValue.valueMessage = nil,
        .singletonName = GPBStringifySymbol(Test40310Root) "_otherM",
        .extendedClass.clazz = GPBObjCClass(Message40310),
        .messageOrGroupClass.clazz = GPBObjCClass(Message40310),
        .enumDescriptorFunc = NULL,
        .fieldNumber = 100,
        .dataType = GPBDataTypeMessage,
        .options = GPBExtensionNone,
      },
      {
        .defaultValue.valueEnum = Enum40310_Foo,
        .singletonName = GPBStringifySymbol(Test40310Root) "_otherE",
        .extendedClass.clazz = GPBObjCClass(Message40310),
        .messageOrGroupClass.clazz = Nil,
        .enumDescriptorFunc = Enum40310_EnumDescriptor,
        .fieldNumber = 101,
        .dataType = GPBDataTypeEnum,
        .options = GPBExtensionNone,
      },
      {
        .defaultValue.valueMessage = nil,
        .singletonName = GPBStringifySymbol(Test40310Root) "_otherMs",
        .extendedClass.clazz = GPBObjCClass(Message40310),
        .messageOrGroupClass.clazz = GPBObjCClass(Message40310),
        .enumDescriptorFunc = NULL,
        .fieldNumber = 110,
        .dataType = GPBDataTypeMessage,
        .options = GPBExtensionRepeated,
      },
      {
        .defaultValue.valueMessage = nil,
        .singletonName = GPBStringifySymbol(Test40310Root) "_otherEs",
        .extendedClass.clazz = GPBObjCClass(Message40310),
        .messageOrGroupClass.clazz = Nil,
        .enumDescriptorFunc = Enum40310_EnumDescriptor,
        .fieldNumber = 111,
        .dataType = GPBDataTypeEnum,
        .options = GPBExtensionRepeated,
      },
    };
    for (size_t i = 0; i < sizeof(descriptions) / sizeof(descriptions[0]); ++i) {
      GPBExtensionDescriptor *extension =
          [[GPBExtensionDescriptor alloc] initWithExtensionDescription:&descriptions[i]
                                                        runtimeSupport:&GOOGLE_PROTOBUF_OBJC_EXPECTED_GENCODE_VERSION_40310];
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

static GPBFilePackageAndPrefix Test40310Root_FileDescription = {
  .package = NULL,
  .prefix = NULL
};

GPBEnumDescriptor *Enum40310_EnumDescriptor(void) {
  static _Atomic(GPBEnumDescriptor*) descriptor = nil;
  if (!descriptor) {
    static const char *valueNames =
        "Foo\000Bar\000";
    static const int32_t values[] = {
        Enum40310_Foo,
        Enum40310_Bar,
    };
    GPBEnumDescriptor *worker =
        [GPBEnumDescriptor allocDescriptorForName:GPBNSStringifySymbol(Enum40310)
                                   runtimeSupport:&GOOGLE_PROTOBUF_OBJC_EXPECTED_GENCODE_VERSION_40310
                                       valueNames:valueNames
                                           values:values
                                            count:(uint32_t)(sizeof(values) / sizeof(int32_t))
                                     enumVerifier:Enum40310_IsValidValue
                                            flags:GPBEnumDescriptorInitializationFlag_IsClosed];
    GPBEnumDescriptor *expected = nil;
    if (!atomic_compare_exchange_strong(&descriptor, &expected, worker)) {
      [worker release];
    }
  }
  return descriptor;
}

BOOL Enum40310_IsValidValue(int32_t value__) {
  switch (value__) {
    case Enum40310_Foo:
    case Enum40310_Bar:
      return YES;
    default:
      return NO;
  }
}

GPBEnumDescriptor *Message40310_SubEnum_EnumDescriptor(void) {
  static _Atomic(GPBEnumDescriptor*) descriptor = nil;
  if (!descriptor) {
    static const char *valueNames =
        "SubFoo\000SubBar\000";
    static const int32_t values[] = {
        Message40310_SubEnum_SubFoo,
        Message40310_SubEnum_SubBar,
    };
    GPBEnumDescriptor *worker =
        [GPBEnumDescriptor allocDescriptorForName:GPBNSStringifySymbol(Message40310_SubEnum)
                                   runtimeSupport:&GOOGLE_PROTOBUF_OBJC_EXPECTED_GENCODE_VERSION_40310
                                       valueNames:valueNames
                                           values:values
                                            count:(uint32_t)(sizeof(values) / sizeof(int32_t))
                                     enumVerifier:Message40310_SubEnum_IsValidValue
                                            flags:GPBEnumDescriptorInitializationFlag_IsClosed];
    GPBEnumDescriptor *expected = nil;
    if (!atomic_compare_exchange_strong(&descriptor, &expected, worker)) {
      [worker release];
    }
  }
  return descriptor;
}

BOOL Message40310_SubEnum_IsValidValue(int32_t value__) {
  switch (value__) {
    case Message40310_SubEnum_SubFoo:
    case Message40310_SubEnum_SubBar:
      return YES;
    default:
      return NO;
  }
}

@implementation Message40310

@dynamic hasName, name;
@dynamic hasValue, value;
@dynamic namesArray, namesArray_Count;
@dynamic valuesArray, valuesArray_Count;
@dynamic hasSubM, subM;
@dynamic hasSubE, subE;

typedef struct Message40310__storage_ {
  uint32_t _has_storage_[1];
  Enum40310 value;
  Message40310_SubEnum subE;
  NSString *name;
  NSMutableArray *namesArray;
  GPBEnumArray *valuesArray;
  Message40310_SubMessage *subM;
} Message40310__storage_;

+ (GPBDescriptor *)descriptor {
  static GPBDescriptor *descriptor = nil;
  if (!descriptor) {
    static GPBMessageFieldDescription fields[] = {
      {
        .name = "name",
        .dataTypeSpecific.clazz = Nil,
        .number = Message40310_FieldNumber_Name,
        .hasIndex = 0,
        .offset = (uint32_t)offsetof(Message40310__storage_, name),
        .flags = GPBFieldOptional,
        .dataType = GPBDataTypeString,
      },
      {
        .name = "value",
        .dataTypeSpecific.enumDescFunc = Enum40310_EnumDescriptor,
        .number = Message40310_FieldNumber_Value,
        .hasIndex = 1,
        .offset = (uint32_t)offsetof(Message40310__storage_, value),
        .flags = GPBFieldOptional,
        .dataType = GPBDataTypeEnum,
      },
      {
        .name = "namesArray",
        .dataTypeSpecific.clazz = Nil,
        .number = Message40310_FieldNumber_NamesArray,
        .hasIndex = GPBNoHasBit,
        .offset = (uint32_t)offsetof(Message40310__storage_, namesArray),
        .flags = GPBFieldRepeated,
        .dataType = GPBDataTypeString,
      },
      {
        .name = "valuesArray",
        .dataTypeSpecific.enumDescFunc = Enum40310_EnumDescriptor,
        .number = Message40310_FieldNumber_ValuesArray,
        .hasIndex = GPBNoHasBit,
        .offset = (uint32_t)offsetof(Message40310__storage_, valuesArray),
        .flags = GPBFieldRepeated,
        .dataType = GPBDataTypeEnum,
      },
      {
        .name = "subM",
        .dataTypeSpecific.clazz = GPBObjCClass(Message40310_SubMessage),
        .number = Message40310_FieldNumber_SubM,
        .hasIndex = 2,
        .offset = (uint32_t)offsetof(Message40310__storage_, subM),
        .flags = GPBFieldOptional,
        .dataType = GPBDataTypeMessage,
      },
      {
        .name = "subE",
        .dataTypeSpecific.enumDescFunc = Message40310_SubEnum_EnumDescriptor,
        .number = Message40310_FieldNumber_SubE,
        .hasIndex = 3,
        .offset = (uint32_t)offsetof(Message40310__storage_, subE),
        .flags = GPBFieldOptional,
        .dataType = GPBDataTypeEnum,
      },
    };
    GPBDescriptor *localDescriptor =
        [GPBDescriptor allocDescriptorForClass:GPBObjCClass(Message40310)
                                   messageName:@"Message40310"
                                runtimeSupport:&GOOGLE_PROTOBUF_OBJC_EXPECTED_GENCODE_VERSION_40310
                               fileDescription:&Test40310Root_FileDescription
                                        fields:fields
                                    fieldCount:(uint32_t)(sizeof(fields) / sizeof(GPBMessageFieldDescription))
                                   storageSize:sizeof(Message40310__storage_)
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

@implementation Message40310_SubMessage

@dynamic hasX, x;

typedef struct Message40310_SubMessage__storage_ {
  uint32_t _has_storage_[1];
  int32_t x;
} Message40310_SubMessage__storage_;

// This method is threadsafe because it is initially called
// in +initialize for each subclass.
+ (GPBDescriptor *)descriptor {
  static GPBDescriptor *descriptor = nil;
  if (!descriptor) {
    static GPBMessageFieldDescription fields[] = {
      {
        .name = "x",
        .dataTypeSpecific.clazz = Nil,
        .number = Message40310_SubMessage_FieldNumber_X,
        .hasIndex = 0,
        .offset = (uint32_t)offsetof(Message40310_SubMessage__storage_, x),
        .flags = GPBFieldOptional,
        .dataType = GPBDataTypeInt32,
      },
    };
    GPBDescriptor *localDescriptor =
        [GPBDescriptor allocDescriptorForClass:GPBObjCClass(Message40310_SubMessage)
                                   messageName:@"SubMessage"
                                runtimeSupport:&GOOGLE_PROTOBUF_OBJC_EXPECTED_GENCODE_VERSION_40310
                               fileDescription:&Test40310Root_FileDescription
                                        fields:fields
                                    fieldCount:(uint32_t)(sizeof(fields) / sizeof(GPBMessageFieldDescription))
                                   storageSize:sizeof(Message40310_SubMessage__storage_)
                                         flags:GPBDescriptorInitializationFlag_None];
    [localDescriptor setupContainingMessageClass:GPBObjCClass(Message40310)];
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

@implementation Message40310FormatTest

- (void)test40310MessageFormat {
  // This doesn't test everything, just exists to ensure the code compiles/links and seems
  // to work.
  Message40310 *message = [Message40310 message];
  message.name = @"foo";
  message.subM.x = 123;
}

@end

#endif  // GOOGLE_PROTOBUF_OBJC_MIN_SUPPORTED_VERSION > 40310
