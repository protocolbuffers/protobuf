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

@interface MessageClassNameTests : GPBTestCase
@end

// clang-format off

// -------------------------------------------------------------------------------------------------
//
// This is extracted from generated code with the 30007 format for the following proto:
//
//  syntax = "proto2";
//
//  enum Enum30007 {
//    FOO = 0;
//    BAR = 1;
//  }
//
//  message Message30007 {
//    message SubMessage {
//      optional int32 x = 1;
//    }
//    enum SubEnum {
//      SUB_FOO = 0;
//      SUB_BAR = 1;
//    }
//    optional string name = 1;
//    optional Enum30007 value = 2;
//    repeated string names = 11;
//    repeated Enum30007 values = 12;
//    optional SubMessage sub_m = 20;
//    optional SubEnum sub_e = 21;
//    extensions 100 to max;
//  }
//
//  extend Message30007 {
//    optional Message30007 other_m = 100;
//    optional Enum30007 other_e = 101;
//    repeated Message30007 other_ms = 110;
//    repeated Enum30007 other_es = 111;
//  }
//
// -------------------------------------------------------------------------------------------------

@class Message30007_SubMessage;

NS_ASSUME_NONNULL_BEGIN

typedef GPB_ENUM(Enum30007) {
  Enum30007_Foo = 0,
  Enum30007_Bar = 1,
};

GPBEnumDescriptor *Enum30007_EnumDescriptor(void);
BOOL Enum30007_IsValidValue(int32_t value);

typedef GPB_ENUM(Message30007_SubEnum) {
  Message30007_SubEnum_SubFoo = 0,
  Message30007_SubEnum_SubBar = 1,
};

GPBEnumDescriptor *Message30007_SubEnum_EnumDescriptor(void);

BOOL Message30007_SubEnum_IsValidValue(int32_t value);

GPB_FINAL @interface Test30007Root : GPBRootObject
@end

@interface Test30007Root (DynamicMethods)
+ (GPBExtensionDescriptor *)otherM;
+ (GPBExtensionDescriptor *)otherE;
+ (GPBExtensionDescriptor *)otherMs;
+ (GPBExtensionDescriptor *)otherEs;
@end

typedef GPB_ENUM(Message30007_FieldNumber) {
  Message30007_FieldNumber_Name = 1,
  Message30007_FieldNumber_Value = 2,
  Message30007_FieldNumber_NamesArray = 11,
  Message30007_FieldNumber_ValuesArray = 12,
  Message30007_FieldNumber_SubM = 20,
  Message30007_FieldNumber_SubE = 21,
};

GPB_FINAL @interface Message30007 : GPBMessage

@property(nonatomic, readwrite, copy, null_resettable) NSString *name;
@property(nonatomic, readwrite) BOOL hasName;

@property(nonatomic, readwrite) Enum30007 value;
@property(nonatomic, readwrite) BOOL hasValue;

@property(nonatomic, readwrite, strong, null_resettable) NSMutableArray<NSString*> *namesArray;
@property(nonatomic, readonly) NSUInteger namesArray_Count;

// |valuesArray| contains |Enum30007|
@property(nonatomic, readwrite, strong, null_resettable) GPBEnumArray *valuesArray;
@property(nonatomic, readonly) NSUInteger valuesArray_Count;

@property(nonatomic, readwrite, strong, null_resettable) Message30007_SubMessage *subM;
@property(nonatomic, readwrite) BOOL hasSubM;

@property(nonatomic, readwrite) Message30007_SubEnum subE;
@property(nonatomic, readwrite) BOOL hasSubE;

@end

typedef GPB_ENUM(Message30007_SubMessage_FieldNumber) {
  Message30007_SubMessage_FieldNumber_X = 1,
};

GPB_FINAL @interface Message30007_SubMessage : GPBMessage

@property(nonatomic, readwrite) int32_t x;
@property(nonatomic, readwrite) BOOL hasX;

@end

NS_ASSUME_NONNULL_END

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"

GPBObjCClassDeclaration(Message30007);
GPBObjCClassDeclaration(Message30007_SubMessage);

@implementation Test30007Root

+ (GPBExtensionRegistry*)extensionRegistry {
  // This is called by +initialize so there is no need to worry
  // about thread safety and initialization of registry.
  static GPBExtensionRegistry* registry = nil;
  if (!registry) {
    GPB_DEBUG_CHECK_RUNTIME_VERSIONS();
    registry = [[GPBExtensionRegistry alloc] init];
    static GPBExtensionDescription descriptions[] = {
      {
        .defaultValue.valueMessage = nil,
        .singletonName = GPBStringifySymbol(Test30007Root) "_otherM",
        .extendedClass.clazz = GPBObjCClass(Message30007),
        .messageOrGroupClass.clazz = GPBObjCClass(Message30007),
        .enumDescriptorFunc = NULL,
        .fieldNumber = 100,
        .dataType = GPBDataTypeMessage,
        .options = GPBExtensionNone,
      },
      {
        .defaultValue.valueEnum = Enum30007_Foo,
        .singletonName = GPBStringifySymbol(Test30007Root) "_otherE",
        .extendedClass.clazz = GPBObjCClass(Message30007),
        .messageOrGroupClass.clazz = Nil,
        .enumDescriptorFunc = Enum30007_EnumDescriptor,
        .fieldNumber = 101,
        .dataType = GPBDataTypeEnum,
        .options = GPBExtensionNone,
      },
      {
        .defaultValue.valueMessage = nil,
        .singletonName = GPBStringifySymbol(Test30007Root) "_otherMs",
        .extendedClass.clazz = GPBObjCClass(Message30007),
        .messageOrGroupClass.clazz = GPBObjCClass(Message30007),
        .enumDescriptorFunc = NULL,
        .fieldNumber = 110,
        .dataType = GPBDataTypeMessage,
        .options = GPBExtensionRepeated,
      },
      {
        .defaultValue.valueMessage = nil,
        .singletonName = GPBStringifySymbol(Test30007Root) "_otherEs",
        .extendedClass.clazz = GPBObjCClass(Message30007),
        .messageOrGroupClass.clazz = Nil,
        .enumDescriptorFunc = Enum30007_EnumDescriptor,
        .fieldNumber = 111,
        .dataType = GPBDataTypeEnum,
        .options = GPBExtensionRepeated,
      },
    };
    for (size_t i = 0; i < sizeof(descriptions) / sizeof(descriptions[0]); ++i) {
      GPBExtensionDescriptor *extension =
          [[GPBExtensionDescriptor alloc] initWithExtensionDescription:&descriptions[i]
                                                         usesClassRefs:YES];
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

static GPBFileDescription Test30007Root_FileDescription = {
  .package = NULL,
  .prefix = NULL,
  .syntax = GPBFileSyntaxProto2
};

GPBEnumDescriptor *Enum30007_EnumDescriptor(void) {
  static _Atomic(GPBEnumDescriptor*) descriptor = nil;
  if (!descriptor) {
    GPB_DEBUG_CHECK_RUNTIME_VERSIONS();
    static const char *valueNames =
        "Foo\000Bar\000";
    static const int32_t values[] = {
        Enum30007_Foo,
        Enum30007_Bar,
    };
    GPBEnumDescriptor *worker =
        [GPBEnumDescriptor allocDescriptorForName:GPBNSStringifySymbol(Enum30007)
                                       valueNames:valueNames
                                           values:values
                                            count:(uint32_t)(sizeof(values) / sizeof(int32_t))
                                     enumVerifier:Enum30007_IsValidValue
                                            flags:GPBEnumDescriptorInitializationFlag_IsClosed];
    GPBEnumDescriptor *expected = nil;
    if (!atomic_compare_exchange_strong(&descriptor, &expected, worker)) {
      [worker release];
    }
  }
  return descriptor;
}

BOOL Enum30007_IsValidValue(int32_t value__) {
  switch (value__) {
    case Enum30007_Foo:
    case Enum30007_Bar:
      return YES;
    default:
      return NO;
  }
}

GPBEnumDescriptor *Message30007_SubEnum_EnumDescriptor(void) {
  static _Atomic(GPBEnumDescriptor*) descriptor = nil;
  if (!descriptor) {
    GPB_DEBUG_CHECK_RUNTIME_VERSIONS();
    static const char *valueNames =
        "SubFoo\000SubBar\000";
    static const int32_t values[] = {
        Message30007_SubEnum_SubFoo,
        Message30007_SubEnum_SubBar,
    };
    GPBEnumDescriptor *worker =
        [GPBEnumDescriptor allocDescriptorForName:GPBNSStringifySymbol(Message30007_SubEnum)
                                       valueNames:valueNames
                                           values:values
                                            count:(uint32_t)(sizeof(values) / sizeof(int32_t))
                                     enumVerifier:Message30007_SubEnum_IsValidValue
                                            flags:GPBEnumDescriptorInitializationFlag_IsClosed];
    GPBEnumDescriptor *expected = nil;
    if (!atomic_compare_exchange_strong(&descriptor, &expected, worker)) {
      [worker release];
    }
  }
  return descriptor;
}

BOOL Message30007_SubEnum_IsValidValue(int32_t value__) {
  switch (value__) {
    case Message30007_SubEnum_SubFoo:
    case Message30007_SubEnum_SubBar:
      return YES;
    default:
      return NO;
  }
}

@implementation Message30007

@dynamic hasName, name;
@dynamic hasValue, value;
@dynamic namesArray, namesArray_Count;
@dynamic valuesArray, valuesArray_Count;
@dynamic hasSubM, subM;
@dynamic hasSubE, subE;

typedef struct Message30007__storage_ {
  uint32_t _has_storage_[1];
  Enum30007 value;
  Message30007_SubEnum subE;
  NSString *name;
  NSMutableArray *namesArray;
  GPBEnumArray *valuesArray;
  Message30007_SubMessage *subM;
} Message30007__storage_;

+ (GPBDescriptor *)descriptor {
  static GPBDescriptor *descriptor = nil;
  if (!descriptor) {
    GPB_DEBUG_CHECK_RUNTIME_VERSIONS();
    static GPBMessageFieldDescription fields[] = {
      {
        .name = "name",
        .dataTypeSpecific.clazz = Nil,
        .number = Message30007_FieldNumber_Name,
        .hasIndex = 0,
        .offset = (uint32_t)offsetof(Message30007__storage_, name),
        .flags = GPBFieldOptional,
        .dataType = GPBDataTypeString,
      },
      {
        .name = "value",
        .dataTypeSpecific.enumDescFunc = Enum30007_EnumDescriptor,
        .number = Message30007_FieldNumber_Value,
        .hasIndex = 1,
        .offset = (uint32_t)offsetof(Message30007__storage_, value),
        .flags = (GPBFieldFlags)(GPBFieldOptional | GPBFieldHasEnumDescriptor | GPBFieldClosedEnum),
        .dataType = GPBDataTypeEnum,
      },
      {
        .name = "namesArray",
        .dataTypeSpecific.clazz = Nil,
        .number = Message30007_FieldNumber_NamesArray,
        .hasIndex = GPBNoHasBit,
        .offset = (uint32_t)offsetof(Message30007__storage_, namesArray),
        .flags = GPBFieldRepeated,
        .dataType = GPBDataTypeString,
      },
      {
        .name = "valuesArray",
        .dataTypeSpecific.enumDescFunc = Enum30007_EnumDescriptor,
        .number = Message30007_FieldNumber_ValuesArray,
        .hasIndex = GPBNoHasBit,
        .offset = (uint32_t)offsetof(Message30007__storage_, valuesArray),
        .flags = (GPBFieldFlags)(GPBFieldRepeated | GPBFieldHasEnumDescriptor | GPBFieldClosedEnum),
        .dataType = GPBDataTypeEnum,
      },
      {
        .name = "subM",
        .dataTypeSpecific.clazz = GPBObjCClass(Message30007_SubMessage),
        .number = Message30007_FieldNumber_SubM,
        .hasIndex = 2,
        .offset = (uint32_t)offsetof(Message30007__storage_, subM),
        .flags = GPBFieldOptional,
        .dataType = GPBDataTypeMessage,
      },
      {
        .name = "subE",
        .dataTypeSpecific.enumDescFunc = Message30007_SubEnum_EnumDescriptor,
        .number = Message30007_FieldNumber_SubE,
        .hasIndex = 3,
        .offset = (uint32_t)offsetof(Message30007__storage_, subE),
        .flags = (GPBFieldFlags)(GPBFieldOptional | GPBFieldHasEnumDescriptor | GPBFieldClosedEnum),
        .dataType = GPBDataTypeEnum,
      },
    };
    GPBDescriptor *localDescriptor =
        [GPBDescriptor allocDescriptorForClass:GPBObjCClass(Message30007)
                                   messageName:@"Message30007"
                               fileDescription:&Test30007Root_FileDescription
                                        fields:fields
                                    fieldCount:(uint32_t)(sizeof(fields) / sizeof(GPBMessageFieldDescription))
                                   storageSize:sizeof(Message30007__storage_)
                                         flags:(GPBDescriptorInitializationFlags)(GPBDescriptorInitializationFlag_UsesClassRefs | GPBDescriptorInitializationFlag_Proto3OptionalKnown | GPBDescriptorInitializationFlag_ClosedEnumSupportKnown)];
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

@implementation Message30007_SubMessage

@dynamic hasX, x;

typedef struct Message30007_SubMessage__storage_ {
  uint32_t _has_storage_[1];
  int32_t x;
} Message30007_SubMessage__storage_;

+ (GPBDescriptor *)descriptor {
  static GPBDescriptor *descriptor = nil;
  if (!descriptor) {
    GPB_DEBUG_CHECK_RUNTIME_VERSIONS();
    static GPBMessageFieldDescription fields[] = {
      {
        .name = "x",
        .dataTypeSpecific.clazz = Nil,
        .number = Message30007_SubMessage_FieldNumber_X,
        .hasIndex = 0,
        .offset = (uint32_t)offsetof(Message30007_SubMessage__storage_, x),
        .flags = GPBFieldOptional,
        .dataType = GPBDataTypeInt32,
      },
    };
    GPBDescriptor *localDescriptor =
        [GPBDescriptor allocDescriptorForClass:GPBObjCClass(Message30007_SubMessage)
                                   messageName:@"SubMessage"
                               fileDescription:&Test30007Root_FileDescription
                                        fields:fields
                                    fieldCount:(uint32_t)(sizeof(fields) / sizeof(GPBMessageFieldDescription))
                                   storageSize:sizeof(Message30007_SubMessage__storage_)
                                         flags:(GPBDescriptorInitializationFlags)(GPBDescriptorInitializationFlag_UsesClassRefs | GPBDescriptorInitializationFlag_Proto3OptionalKnown | GPBDescriptorInitializationFlag_ClosedEnumSupportKnown)];
    [localDescriptor setupContainingMessageClass:GPBObjCClass(Message30007)];
    #if defined(DEBUG) && DEBUG
      NSAssert(descriptor == nil, @"Startup recursed!");
    #endif  // DEBUG
    descriptor = localDescriptor;
  }
  return descriptor;
}

@end

#pragma clang diagnostic pop

// clang-format on

// -------------------------------------------------------------------------------------------------

@implementation MessageClassNameTests

- (void)test30007MessageFormat {
  // This doesn't test everything, just exists to ensure the code compiles/links and seems
  // to work.
  Message30007 *message = [Message30007 message];
  message.name = @"foo";
  message.subM.x = 123;
}

@end
