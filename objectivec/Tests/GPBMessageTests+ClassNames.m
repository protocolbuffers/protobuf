// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBTestUtilities.h"

#import <objc/runtime.h>

#import "GPBDescriptor_PackagePrivate.h"
#import "GPBExtensionRegistry.h"
#import "GPBMessage.h"
#import "GPBRootObject_PackagePrivate.h"

// Support classes for tests using old class name (vs classrefs) interfaces.
GPB_FINAL @interface MessageLackingClazzRoot : GPBRootObject
@end

@interface MessageLackingClazzRoot (DynamicMethods)
+ (GPBExtensionDescriptor *)ext1;
@end

GPB_FINAL @interface MessageLackingClazz : GPBMessage
@property(copy, nonatomic) NSString *foo;
@end

@implementation MessageLackingClazz

@dynamic foo;

typedef struct MessageLackingClazz_storage_ {
  uint32_t _has_storage_[1];
  NSString *foo;
} MessageLackingClazz_storage_;

+ (GPBDescriptor *)descriptor {
  static GPBDescriptor *descriptor = nil;
  if (!descriptor) {
    static GPBMessageFieldDescription fields[] = {
        {
            .name = "foo",
            .dataTypeSpecific.className = "NSString",
            .number = 1,
            .hasIndex = 0,
            .offset = (uint32_t)offsetof(MessageLackingClazz_storage_, foo),
            .flags = (GPBFieldFlags)(GPBFieldOptional),
            .dataType = GPBDataTypeMessage,
        },
    };
    GPBFileDescriptor *desc =
        [[[GPBFileDescriptor alloc] initWithPackage:@"test"
                                         objcPrefix:@"TEST"
                                             syntax:GPBFileSyntaxProto3] autorelease];

    // GPBDescriptorInitializationFlag_UsesClassRefs intentionally not set here
    descriptor = [GPBDescriptor
        allocDescriptorForClass:[MessageLackingClazz class]
                      rootClass:[MessageLackingClazzRoot class]
                           file:desc
                         fields:fields
                     fieldCount:(uint32_t)(sizeof(fields) / sizeof(GPBMessageFieldDescription))
                    storageSize:sizeof(MessageLackingClazz_storage_)
                          flags:GPBDescriptorInitializationFlag_None];
    [descriptor setupContainingMessageClassName:"MessageLackingClazz"];
  }
  return descriptor;
}
@end

@implementation MessageLackingClazzRoot

+ (GPBExtensionRegistry *)extensionRegistry {
  // This is called by +initialize so there is no need to worry
  // about thread safety and initialization of registry.
  static GPBExtensionRegistry *registry = nil;
  if (!registry) {
    registry = [[GPBExtensionRegistry alloc] init];
    static GPBExtensionDescription descriptions[] = {
        {
            .defaultValue.valueMessage = NULL,
            .singletonName = "MessageLackingClazzRoot_ext1",
            .extendedClass.name = "MessageLackingClazz",
            .messageOrGroupClass.name = "MessageLackingClazz",
            .enumDescriptorFunc = NULL,
            .fieldNumber = 1,
            .dataType = GPBDataTypeMessage,
            // GPBExtensionUsesClazz Intentionally not set
            .options = 0,
        },
    };
    for (size_t i = 0; i < sizeof(descriptions) / sizeof(descriptions[0]); ++i) {
      // Intentionall using `-initWithExtensionDescription:` and not `
      // -initWithExtensionDescription:usesClassRefs:` to test backwards
      // compatibility
      GPBExtensionDescriptor *extension =
          [[GPBExtensionDescriptor alloc] initWithExtensionDescription:&descriptions[i]];
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

@interface MessageClassNameTests : GPBTestCase
@end

@implementation MessageClassNameTests

- (void)testClassNameSupported {
  // This tests backwards compatibility to make sure we support older sources
  // that use class names instead of references.
  GPBDescriptor *desc = [MessageLackingClazz descriptor];
  GPBFieldDescriptor *fieldDesc = [desc fieldWithName:@"foo"];
  XCTAssertEqualObjects(fieldDesc.msgClass, [NSString class]);
}

- (void)testSetupContainingMessageClassNameSupported {
  // This tests backwards compatibility to make sure we support older sources
  // that use class names instead of references.
  GPBDescriptor *desc = [MessageLackingClazz descriptor];
  GPBDescriptor *container = [desc containingType];
  XCTAssertEqualObjects(container.messageClass, [MessageLackingClazz class]);
}

- (void)testExtensionsNameSupported {
  // This tests backwards compatibility to make sure we support older sources
  // that use class names instead of references.
  GPBExtensionDescriptor *desc = [MessageLackingClazzRoot ext1];
  Class containerClass = [desc containingMessageClass];
  XCTAssertEqualObjects(containerClass, [MessageLackingClazz class]);
  Class msgClass = [desc msgClass];
  XCTAssertEqualObjects(msgClass, [MessageLackingClazz class]);
}

@end
