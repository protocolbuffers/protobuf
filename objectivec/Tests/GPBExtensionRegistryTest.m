// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBTestUtilities.h"

#import <dispatch/dispatch.h>

#import "GPBCodedOutputStream.h"
#import "GPBDescriptor_PackagePrivate.h"
#import "GPBExtensionRegistry.h"
#import "GPBUtilities_PackagePrivate.h"
#import "objectivec/Tests/Unittest.pbobjc.h"

// Hand written "__DATA,__gpb_ext_reg" section entries, in the same form the
// generator emits with the `experimental_global_extension_registry` option.
// They make the +globalExtensionRegistry tests independent of how the test
// protos were generated. The field numbers are not used by any extension in the
// test protos, and each entry is used by one test only, so a test controls
// whether the entry's descriptor has been created yet.

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"

GPBObjCClassDeclaration(TestAllExtensions);

#define GPB_TEST_HAND_WRITTEN_INT32_EXTENSION(ENTRY_NAME, FIELD_NUMBER)              \
  static GPBExtensionDescription ENTRY_NAME##Description = {                         \
      .defaultValue.valueInt32 = 0,                                                  \
      .singletonName = "GPBExtensionRegistryTest_" #ENTRY_NAME,                      \
      .extendedClass.clazz = GPBObjCClass(TestAllExtensions),                        \
      .messageOrGroupClass.clazz = Nil,                                              \
      .enumDescriptorFunc = NULL,                                                    \
      .fieldNumber = FIELD_NUMBER,                                                   \
      .dataType = GPBDataTypeInt32,                                                  \
      .options = GPBExtensionNone,                                                   \
  };                                                                                 \
  GPB_EXTENSION_REGISTRY_SECTION_ATTR static GPBExtensionSectionEntry ENTRY_NAME = { \
      .description = &ENTRY_NAME##Description,                                       \
      .descriptor = nil,                                                             \
  }

// Enum constants, so they can be used in the static initializers.
enum {
  kLookupExtensionFieldNumber = 30001,
  kConcurrentExtensionFieldNumber = 30002,
  kParseExtensionFieldNumber = 30003,
  kMergeExtensionFieldNumber = 30004,
  kOverrideExtensionFieldNumber = 30005,
  kUnusedExtensionFieldNumber = 30099,
};

GPB_TEST_HAND_WRITTEN_INT32_EXTENSION(gLookupExtensionEntry, kLookupExtensionFieldNumber);
GPB_TEST_HAND_WRITTEN_INT32_EXTENSION(gConcurrentExtensionEntry, kConcurrentExtensionFieldNumber);
GPB_TEST_HAND_WRITTEN_INT32_EXTENSION(gParseExtensionEntry, kParseExtensionFieldNumber);
GPB_TEST_HAND_WRITTEN_INT32_EXTENSION(gMergeExtensionEntry, kMergeExtensionFieldNumber);
GPB_TEST_HAND_WRITTEN_INT32_EXTENSION(gOverrideExtensionEntry, kOverrideExtensionFieldNumber);

#undef GPB_TEST_HAND_WRITTEN_INT32_EXTENSION

// Same extended class and field number as gOverrideExtensionEntry, but not in
// the section, for an extension that is added explicitly to a registry.
static GPBExtensionDescription gOverrideOwnExtensionDescription = {
    .defaultValue.valueInt32 = 0,
    .singletonName = "GPBExtensionRegistryTest_overrideOwnExtension",
    .extendedClass.clazz = GPBObjCClass(TestAllExtensions),
    .messageOrGroupClass.clazz = Nil,
    .enumDescriptorFunc = NULL,
    .fieldNumber = kOverrideExtensionFieldNumber,
    .dataType = GPBDataTypeInt32,
    .options = GPBExtensionNone,
};

#pragma clang diagnostic pop

// What a generated extension function does: resolve the entry's descriptor.
static GPBExtensionDescriptor *ResolveHandWrittenEntry(GPBExtensionSectionEntry *entry) {
  return GPBResolveExtensionSectionEntry(entry,
                                         &GOOGLE_PROTOBUF_OBJC_EXPECTED_GENCODE_VERSION_40311);
}

// A registry implementation that is neither a GPBExtensionRegistry nor the
// global registry.
@interface GPBExtensionRegistryTestOtherImplementation : NSObject <GPBExtensionRegistry>
@end

@implementation GPBExtensionRegistryTestOtherImplementation
- (GPBExtensionDescriptor *)extensionForDescriptor:(GPBDescriptor *)descriptor
                                       fieldNumber:(NSInteger)fieldNumber {
  return nil;
}
@end

@interface GPBExtensionRegistryTest : GPBTestCase
@end

@implementation GPBExtensionRegistryTest

- (void)testBasics {
  GPBExtensionRegistry *reg = [[[GPBExtensionRegistry alloc] init] autorelease];
  XCTAssertNotNil(reg);

  XCTAssertNil([reg extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1]);
  XCTAssertNil([reg extensionForDescriptor:[TestAllTypes descriptor] fieldNumber:1]);

#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  [reg addExtension:Objc_Protobuf_Tests_extension_OptionalInt32Extension()];
  [reg addExtension:Objc_Protobuf_Tests_extension_PackedInt64Extension()];

  XCTAssertTrue([reg extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                Objc_Protobuf_Tests_extension_OptionalInt32Extension());  // ptr equality
  XCTAssertNil([reg extensionForDescriptor:[TestAllTypes descriptor] fieldNumber:1]);
  XCTAssertTrue([reg extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91] ==
                Objc_Protobuf_Tests_extension_PackedInt64Extension());  // ptr equality
#else
  [reg addExtension:[UnittestRoot optionalInt32Extension]];
  [reg addExtension:[UnittestRoot packedInt64Extension]];

  XCTAssertTrue([reg extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                [UnittestRoot optionalInt32Extension]);  // ptr equality
  XCTAssertNil([reg extensionForDescriptor:[TestAllTypes descriptor] fieldNumber:1]);
  XCTAssertTrue([reg extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91] ==
                [UnittestRoot packedInt64Extension]);  // ptr equality
#endif
}

- (void)testCopy {
  GPBExtensionRegistry *reg1 = [[[GPBExtensionRegistry alloc] init] autorelease];
#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  [reg1 addExtension:Objc_Protobuf_Tests_extension_OptionalInt32Extension()];
#else
  [reg1 addExtension:[UnittestRoot optionalInt32Extension]];
#endif

  GPBExtensionRegistry *reg2 = [[reg1 copy] autorelease];
  XCTAssertNotNil(reg2);

#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  XCTAssertTrue([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                Objc_Protobuf_Tests_extension_OptionalInt32Extension());  // ptr equality
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                Objc_Protobuf_Tests_extension_OptionalInt32Extension());  // ptr equality
#else
  XCTAssertTrue([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                [UnittestRoot optionalInt32Extension]);  // ptr equality
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                [UnittestRoot optionalInt32Extension]);  // ptr equality
#endif

  // Message class that had registered extension(s) at the -copy time.

#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  [reg1 addExtension:Objc_Protobuf_Tests_extension_OptionalBoolExtension()];
  [reg2 addExtension:Objc_Protobuf_Tests_extension_OptionalStringExtension()];

  XCTAssertTrue([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13] ==
                Objc_Protobuf_Tests_extension_OptionalBoolExtension());  // ptr equality
  XCTAssertNil([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14]);
  XCTAssertNil([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13]);
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14] ==
                Objc_Protobuf_Tests_extension_OptionalStringExtension());  // ptr equality
#else
  [reg1 addExtension:[UnittestRoot optionalBoolExtension]];
  [reg2 addExtension:[UnittestRoot optionalStringExtension]];

  XCTAssertTrue([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13] ==
                [UnittestRoot optionalBoolExtension]);  // ptr equality
  XCTAssertNil([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14]);
  XCTAssertNil([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13]);
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14] ==
                [UnittestRoot optionalStringExtension]);  // ptr equality
#endif

  // Message class that did not have any registered extensions at the -copy time.

#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  [reg1 addExtension:Objc_Protobuf_Tests_extension_PackedInt64Extension()];
  [reg2 addExtension:Objc_Protobuf_Tests_extension_PackedSint32Extension()];

  XCTAssertTrue([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91] ==
                Objc_Protobuf_Tests_extension_PackedInt64Extension());  // ptr equality
  XCTAssertNil([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94]);
  XCTAssertNil([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91]);
  XCTAssertTrue([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94] ==
                Objc_Protobuf_Tests_extension_PackedSint32Extension());  // ptr equality
#else
  [reg1 addExtension:[UnittestRoot packedInt64Extension]];
  [reg2 addExtension:[UnittestRoot packedSint32Extension]];

  XCTAssertTrue([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91] ==
                [UnittestRoot packedInt64Extension]);  // ptr equality
  XCTAssertNil([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94]);
  XCTAssertNil([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91]);
  XCTAssertTrue([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94] ==
                [UnittestRoot packedSint32Extension]);  // ptr equality
#endif
}

- (void)testAddExtensions {
  GPBExtensionRegistry *reg1 = [[[GPBExtensionRegistry alloc] init] autorelease];
#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  [reg1 addExtension:Objc_Protobuf_Tests_extension_OptionalInt32Extension()];
#else
  [reg1 addExtension:[UnittestRoot optionalInt32Extension]];
#endif

  GPBExtensionRegistry *reg2 = [[[GPBExtensionRegistry alloc] init] autorelease];

  XCTAssertNil([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1]);

  [reg2 addExtensions:reg1];

#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                Objc_Protobuf_Tests_extension_OptionalInt32Extension());  // ptr equality
#else
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1] ==
                [UnittestRoot optionalInt32Extension]);  // ptr equality
#endif

  // Confirm adding to the first doesn't add to the second.

#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  [reg1 addExtension:Objc_Protobuf_Tests_extension_OptionalBoolExtension()];
  [reg1 addExtension:Objc_Protobuf_Tests_extension_PackedInt64Extension()];

  XCTAssertTrue([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13] ==
                Objc_Protobuf_Tests_extension_OptionalBoolExtension());  // ptr equality
  XCTAssertTrue([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91] ==
                Objc_Protobuf_Tests_extension_PackedInt64Extension());  // ptr equality
#else
  [reg1 addExtension:[UnittestRoot optionalBoolExtension]];
  [reg1 addExtension:[UnittestRoot packedInt64Extension]];

  XCTAssertTrue([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13] ==
                [UnittestRoot optionalBoolExtension]);  // ptr equality
  XCTAssertTrue([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91] ==
                [UnittestRoot packedInt64Extension]);  // ptr equality
#endif
  XCTAssertNil([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13]);
  XCTAssertNil([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:91]);

  // Confirm adding to the second doesn't add to the first.

#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  [reg2 addExtension:Objc_Protobuf_Tests_extension_OptionalStringExtension()];
  [reg2 addExtension:Objc_Protobuf_Tests_extension_PackedSint32Extension()];
#else
  [reg2 addExtension:[UnittestRoot optionalStringExtension]];
  [reg2 addExtension:[UnittestRoot packedSint32Extension]];
#endif

  XCTAssertNil([reg1 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14]);
  XCTAssertNil([reg1 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94]);
#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14] ==
                Objc_Protobuf_Tests_extension_OptionalStringExtension());  // ptr equality
  XCTAssertTrue([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94] ==
                Objc_Protobuf_Tests_extension_PackedSint32Extension());  // ptr equality
#else
  XCTAssertTrue([reg2 extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:14] ==
                [UnittestRoot optionalStringExtension]);  // ptr equality
  XCTAssertTrue([reg2 extensionForDescriptor:[TestPackedExtensions descriptor] fieldNumber:94] ==
                [UnittestRoot packedSint32Extension]);  // ptr equality
#endif
}

#pragma mark - +globalExtensionRegistry

- (void)testGlobalExtensionRegistryIsSingleton {
  id<GPBExtensionRegistry> globalRegistry = [GPBExtensionRegistry globalExtensionRegistry];
  XCTAssertNotNil(globalRegistry);
  XCTAssertTrue([GPBExtensionRegistry globalExtensionRegistry] == globalRegistry);  // ptr equality
  XCTAssertTrue([globalRegistry conformsToProtocol:@protocol(GPBExtensionRegistry)]);
  // Immutable, so it is not a (mutable) GPBExtensionRegistry.
  XCTAssertFalse([(NSObject *)globalRegistry isKindOfClass:[GPBExtensionRegistry class]]);

  const size_t kIterationCount = 256;
  id<GPBExtensionRegistry> *concurrentResults =
      calloc(kIterationCount, sizeof(id<GPBExtensionRegistry>));
  dispatch_apply(kIterationCount, dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0),
                 ^(size_t iteration) {
                   concurrentResults[iteration] = [GPBExtensionRegistry globalExtensionRegistry];
                 });
  for (size_t iteration = 0; iteration < kIterationCount; ++iteration) {
    XCTAssertTrue(concurrentResults[iteration] == globalRegistry);  // ptr equality
  }
  free(concurrentResults);
}

- (void)testGlobalExtensionRegistryReturnsSameDescriptorAsSectionEntry {
  id<GPBExtensionRegistry> globalRegistry = [GPBExtensionRegistry globalExtensionRegistry];
  XCTAssertTrue(gLookupExtensionEntry.descriptor == nil);

  GPBExtensionDescriptor *fromRegistry =
      [globalRegistry extensionForDescriptor:[TestAllExtensions descriptor]
                                 fieldNumber:kLookupExtensionFieldNumber];
  XCTAssertNotNil(fromRegistry);
  XCTAssertTrue(gLookupExtensionEntry.descriptor == fromRegistry);  // ptr equality
  XCTAssertEqual(fromRegistry.fieldNumber, (uint32_t)kLookupExtensionFieldNumber);
  XCTAssertTrue(fromRegistry.containingMessageClass == [TestAllExtensions class]);
  XCTAssertEqual(fromRegistry.dataType, GPBDataTypeInt32);

  // The generated function path returns the same instance.
  XCTAssertTrue(ResolveHandWrittenEntry(&gLookupExtensionEntry) == fromRegistry);  // ptr equality
  // And repeated lookups do too.
  XCTAssertTrue([globalRegistry extensionForDescriptor:[TestAllExtensions descriptor]
                                           fieldNumber:kLookupExtensionFieldNumber] ==
                fromRegistry);  // ptr equality
}

- (void)testGlobalExtensionRegistryReturnsNilForUnknownExtensions {
  id<GPBExtensionRegistry> globalRegistry = [GPBExtensionRegistry globalExtensionRegistry];
  // Known class, unknown field number.
  XCTAssertNil([globalRegistry extensionForDescriptor:[TestAllExtensions descriptor]
                                          fieldNumber:kUnusedExtensionFieldNumber]);
  // A field number that has an entry, but for a different class.
  XCTAssertNil([globalRegistry extensionForDescriptor:[TestAllTypes descriptor]
                                          fieldNumber:kLookupExtensionFieldNumber]);
}

- (void)testGlobalExtensionRegistryResolvesConcurrently {
  // Unresolved, so the concurrent lookups below race to create the descriptor.
  XCTAssertTrue(gConcurrentExtensionEntry.descriptor == nil);

  id<GPBExtensionRegistry> globalRegistry = [GPBExtensionRegistry globalExtensionRegistry];
  GPBDescriptor *extendedDescriptor = [TestAllExtensions descriptor];
  const size_t kIterationCount = 1000;
  GPBExtensionDescriptor **concurrentResults =
      calloc(kIterationCount, sizeof(GPBExtensionDescriptor *));
  dispatch_apply(
      kIterationCount, dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^(size_t iteration) {
        if (iteration % 2 == 0) {
          concurrentResults[iteration] =
              [globalRegistry extensionForDescriptor:extendedDescriptor
                                         fieldNumber:kConcurrentExtensionFieldNumber];
        } else {
          concurrentResults[iteration] = ResolveHandWrittenEntry(&gConcurrentExtensionEntry);
        }
      });

  GPBExtensionDescriptor *resolved = gConcurrentExtensionEntry.descriptor;
  XCTAssertNotNil(resolved);
  for (size_t iteration = 0; iteration < kIterationCount; ++iteration) {
    XCTAssertTrue(concurrentResults[iteration] == resolved);  // ptr equality
  }
  free(concurrentResults);
}

- (void)testGlobalExtensionRegistryParsesExtension {
  XCTAssertTrue(gParseExtensionEntry.descriptor == nil);

  NSOutputStream *rawOutput = [NSOutputStream outputStreamToMemory];
  GPBCodedOutputStream *output = [GPBCodedOutputStream streamWithOutputStream:rawOutput];
  [output writeInt32:kParseExtensionFieldNumber value:7];
  [output flush];
  NSData *wireData = [rawOutput propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
  XCTAssertNotNil(wireData);

  NSError *error = nil;
  TestAllExtensions *withoutRegistry = [TestAllExtensions parseFromData:wireData error:&error];
  XCTAssertNotNil(withoutRegistry);
  XCTAssertNil(error);
  XCTAssertEqual(withoutRegistry.extensionsCurrentlySet.count, (NSUInteger)0);
  // Parsing without a registry does not resolve the entry.
  XCTAssertTrue(gParseExtensionEntry.descriptor == nil);

  TestAllExtensions *message =
      [TestAllExtensions parseFromData:wireData
                     extensionRegistry:[GPBExtensionRegistry globalExtensionRegistry]
                                 error:&error];
  XCTAssertNotNil(message);
  XCTAssertNil(error);
  GPBExtensionDescriptor *resolved = gParseExtensionEntry.descriptor;
  XCTAssertNotNil(resolved);
  NSArray *extensionsSet = message.extensionsCurrentlySet;
  XCTAssertEqual(extensionsSet.count, (NSUInteger)1);
  XCTAssertTrue(extensionsSet.firstObject == resolved);  // ptr equality
  XCTAssertTrue([message hasExtension:resolved]);
  XCTAssertEqualObjects([message getExtension:resolved], @7);
  XCTAssertEqualObjects(message.data, wireData);
}

- (void)testAddExtensionsWithGlobalExtensionRegistry {
  GPBExtensionRegistry *registry = [[[GPBExtensionRegistry alloc] init] autorelease];
  [registry addExtensions:[GPBExtensionRegistry globalExtensionRegistry]];
#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  GPBExtensionDescriptor *ownExtension = Objc_Protobuf_Tests_extension_OptionalInt32Extension();
#else
  GPBExtensionDescriptor *ownExtension = [UnittestRoot optionalInt32Extension];
#endif
  [registry addExtension:ownExtension];

  // Explicitly added extension.
  XCTAssertTrue([registry extensionForDescriptor:[TestAllExtensions descriptor]
                                     fieldNumber:1] == ownExtension);  // ptr equality
  // Section entry, through the global registry.
  XCTAssertTrue(gMergeExtensionEntry.descriptor == nil);
  GPBExtensionDescriptor *fromSection =
      [registry extensionForDescriptor:[TestAllExtensions descriptor]
                           fieldNumber:kMergeExtensionFieldNumber];
  XCTAssertNotNil(fromSection);
  XCTAssertTrue(fromSection == gMergeExtensionEntry.descriptor);                 // ptr equality
  XCTAssertTrue(fromSection == ResolveHandWrittenEntry(&gMergeExtensionEntry));  // ptr equality
  XCTAssertTrue([[GPBExtensionRegistry globalExtensionRegistry]
                    extensionForDescriptor:[TestAllExtensions descriptor]
                               fieldNumber:kMergeExtensionFieldNumber] ==
                fromSection);  // ptr equality
  XCTAssertNil([registry extensionForDescriptor:[TestAllExtensions descriptor]
                                    fieldNumber:kUnusedExtensionFieldNumber]);

  // Adding it again (or adding it to a registry that has it) changes nothing.
  [registry addExtensions:[GPBExtensionRegistry globalExtensionRegistry]];
  XCTAssertTrue([registry extensionForDescriptor:[TestAllExtensions descriptor]
                                     fieldNumber:kMergeExtensionFieldNumber] ==
                fromSection);  // ptr equality
}

- (void)testAddExtensionsWithGlobalExtensionRegistryPrefersOwnExtensions {
  GPBExtensionDescriptor *ownExtension = [[[GPBExtensionDescriptor alloc]
      initWithExtensionDescription:&gOverrideOwnExtensionDescription
                    runtimeSupport:&GOOGLE_PROTOBUF_OBJC_EXPECTED_GENCODE_VERSION_40311]
      autorelease];
  GPBExtensionDescriptor *fromSection = ResolveHandWrittenEntry(&gOverrideExtensionEntry);
  XCTAssertTrue(ownExtension != fromSection);

  // Own extension added before or after the global registry wins.
  GPBExtensionRegistry *addedBefore = [[[GPBExtensionRegistry alloc] init] autorelease];
  [addedBefore addExtension:ownExtension];
  [addedBefore addExtensions:[GPBExtensionRegistry globalExtensionRegistry]];
  GPBExtensionRegistry *addedAfter = [[[GPBExtensionRegistry alloc] init] autorelease];
  [addedAfter addExtensions:[GPBExtensionRegistry globalExtensionRegistry]];
  [addedAfter addExtension:ownExtension];
  for (GPBExtensionRegistry *registry in @[ addedBefore, addedAfter ]) {
    XCTAssertTrue([registry extensionForDescriptor:[TestAllExtensions descriptor]
                                       fieldNumber:kOverrideExtensionFieldNumber] ==
                  ownExtension);  // ptr equality
  }
  // The global registry itself is unchanged.
  XCTAssertTrue([[GPBExtensionRegistry globalExtensionRegistry]
                    extensionForDescriptor:[TestAllExtensions descriptor]
                               fieldNumber:kOverrideExtensionFieldNumber] ==
                fromSection);  // ptr equality
}

- (void)testCopyAndMergeKeepGlobalExtensionRegistry {
  GPBExtensionRegistry *extended = [[[GPBExtensionRegistry alloc] init] autorelease];
  [extended addExtensions:[GPBExtensionRegistry globalExtensionRegistry]];
  GPBExtensionDescriptor *fromSection = ResolveHandWrittenEntry(&gMergeExtensionEntry);

  GPBExtensionRegistry *copied = [[extended copy] autorelease];
  GPBExtensionRegistry *merged = [[[GPBExtensionRegistry alloc] init] autorelease];
  [merged addExtensions:extended];
  for (GPBExtensionRegistry *registry in @[ copied, merged ]) {
    XCTAssertTrue([registry extensionForDescriptor:[TestAllExtensions descriptor]
                                       fieldNumber:kMergeExtensionFieldNumber] ==
                  fromSection);  // ptr equality
  }

  // Extending the copy does not change the original.
#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  GPBExtensionDescriptor *ownExtension = Objc_Protobuf_Tests_extension_OptionalBoolExtension();
#else
  GPBExtensionDescriptor *ownExtension = [UnittestRoot optionalBoolExtension];
#endif
  [copied addExtension:ownExtension];
  XCTAssertTrue([copied extensionForDescriptor:[TestAllExtensions descriptor]
                                   fieldNumber:13] == ownExtension);  // ptr equality
  // Without the generator option the test protos have no section entries for
  // optional_bool_extension, and with it the global registry returns the same
  // instance, so either way the original never returns anything else.
  GPBExtensionDescriptor *fromOriginal =
      [extended extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:13];
  XCTAssertTrue(fromOriginal == nil || fromOriginal == ownExtension);
}

- (void)testAddExtensionsRejectsOtherRegistryImplementations {
  GPBExtensionRegistry *registry = [[[GPBExtensionRegistry alloc] init] autorelease];
  id<GPBExtensionRegistry> otherImplementation =
      [[[GPBExtensionRegistryTestOtherImplementation alloc] init] autorelease];
  XCTAssertThrowsSpecificNamed([registry addExtensions:otherImplementation], NSException,
                               NSInvalidArgumentException);
}

// Only runs when the test protos were generated with the
// `experimental_global_extension_registry` option (e.g. with
// `--//tools/build_defs/objc/private:experimental_global_extension_registry=True`)
// in a C function extension mode.
- (void)testGlobalExtensionRegistryRoundTripsGeneratedExtensions {
  id<GPBExtensionRegistry> globalRegistry = [GPBExtensionRegistry globalExtensionRegistry];
  // optional_int32_extension = 1 in unittest.proto.
  GPBExtensionDescriptor *optionalInt32Extension =
      [globalRegistry extensionForDescriptor:[TestAllExtensions descriptor] fieldNumber:1];
  if (optionalInt32Extension == nil) {
    XCTSkip(@"The test protos were generated without section entries.");
  }
#if defined(GPB_UNITTEST_USE_C_FUNCTION_FOR_EXTENSIONS)
  XCTAssertTrue(optionalInt32Extension ==
                Objc_Protobuf_Tests_extension_OptionalInt32Extension());  // ptr equality
#endif

  NSError *error = nil;
  TestAllExtensions *allExtensions = [self allExtensionsSetRepeatedCount:kGPBDefaultRepeatCount];
  NSData *allExtensionsData = allExtensions.data;
  XCTAssertNotNil(allExtensionsData);
  TestAllExtensions *parsedAllExtensions = [TestAllExtensions parseFromData:allExtensionsData
                                                          extensionRegistry:globalRegistry
                                                                      error:&error];
  XCTAssertNotNil(parsedAllExtensions);
  XCTAssertNil(error);
  [self assertAllExtensionsSet:parsedAllExtensions repeatedCount:kGPBDefaultRepeatCount];
  XCTAssertEqualObjects(parsedAllExtensions, allExtensions);

  TestPackedExtensions *packedExtensions =
      [self packedExtensionsSetRepeatedCount:kGPBDefaultRepeatCount];
  NSData *packedExtensionsData = packedExtensions.data;
  XCTAssertNotNil(packedExtensionsData);
  TestPackedExtensions *parsedPackedExtensions =
      [TestPackedExtensions parseFromData:packedExtensionsData
                        extensionRegistry:globalRegistry
                                    error:&error];
  XCTAssertNotNil(parsedPackedExtensions);
  XCTAssertNil(error);
  [self assertPackedExtensionsSet:parsedPackedExtensions repeatedCount:kGPBDefaultRepeatCount];
  XCTAssertEqualObjects(parsedPackedExtensions, packedExtensions);
}

@end
