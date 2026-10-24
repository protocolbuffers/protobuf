// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBTestUtilities.h"

#import <dispatch/dispatch.h>
#import <stdatomic.h>

#import "GPBCodedOutputStream.h"
#import "GPBDescriptor_PackagePrivate.h"
#import "GPBExtensionRegistry.h"
#import "GPBTestExtensionLinkShards.h"
#import "GPBUtilities_PackagePrivate.h"
#import "objectivec/Tests/Unittest.pbobjc.h"

// Hand written extension section entries (GPB_EXTENSION_SECTION_SEGMENT,
// GPB_EXTENSION_SECTION_NAME), in the same form the generator emits with the
// `experimental_global_extension_registry` option.
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
  kUnusedExtensionFieldNumber = 30099,
};

GPB_TEST_HAND_WRITTEN_INT32_EXTENSION(gLookupExtensionEntry, kLookupExtensionFieldNumber);
GPB_TEST_HAND_WRITTEN_INT32_EXTENSION(gConcurrentExtensionEntry, kConcurrentExtensionFieldNumber);
GPB_TEST_HAND_WRITTEN_INT32_EXTENSION(gParseExtensionEntry, kParseExtensionFieldNumber);

#undef GPB_TEST_HAND_WRITTEN_INT32_EXTENSION

#pragma clang diagnostic pop

// What a generated extension function does: resolve the entry's descriptor.
static GPBExtensionDescriptor *ResolveHandWrittenEntry(GPBExtensionSectionEntry *entry) {
  return GPBResolveExtensionSectionEntry(entry,
                                         &GOOGLE_PROTOBUF_OBJC_EXPECTED_GENCODE_VERSION_40311);
}

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

- (void)testGlobalExtensionRegistryOnlyFindsLinkedShards {
  id<GPBExtensionRegistry> globalRegistry = [GPBExtensionRegistry globalExtensionRegistry];
  GPBDescriptor *extendedDescriptor = [TestAllExtensions descriptor];
  // GPBTestExtensionLinkShardReferenced.m is referenced (here), so its object
  // file is linked and its entry is found.
  GPBExtensionDescriptor *referenced =
      [globalRegistry extensionForDescriptor:extendedDescriptor
                                 fieldNumber:kGPBTestExtensionLinkShardReferencedFieldNumber];
  XCTAssertNotNil(referenced);
  XCTAssertTrue(referenced == GPBTestExtensionLinkShardReferencedExtension());  // ptr equality
  // GPBTestExtensionLinkShardUnreferenced.m is in the same (not always linked)
  // static library, but nothing references it, so its object file and its
  // entry are not linked.
  XCTAssertNil([globalRegistry
      extensionForDescriptor:extendedDescriptor
                 fieldNumber:kGPBTestExtensionLinkShardUnreferencedFieldNumber]);
}

// Only runs when the test protos were generated with the
// `experimental_global_extension_registry` option in C function extension mode.
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

#pragma mark - Extension section index list

// Fake image sections for the index list tests: heap-allocated entries that
// are never put in a Mach-O section, so they never reach the global registry.
// Each fake image has `kFakeImageSharedFieldNumber` (present in every image, so
// lookups of it show which image wins) and one field number unique to it.
enum {
  kFakeImageSharedFieldNumber = 40000,
  kFakeImageFirstUniqueFieldNumber = 40001,
  kFakeImageEntriesPerImage = 2,
};

typedef struct {
  GPBExtensionDescription descriptions[kFakeImageEntriesPerImage];
  GPBExtensionSectionEntry entries[kFakeImageEntriesPerImage];
} GPBTestFakeImageSection;

static GPBTestFakeImageSection *CreateFakeImageSections(size_t imageCount) {
  GPBTestFakeImageSection *sections = calloc(imageCount, sizeof(GPBTestFakeImageSection));
  for (size_t imageIndex = 0; imageIndex < imageCount; ++imageIndex) {
    GPBTestFakeImageSection *section = &sections[imageIndex];
    int32_t fieldNumbers[kFakeImageEntriesPerImage] = {
        kFakeImageSharedFieldNumber,
        (int32_t)(kFakeImageFirstUniqueFieldNumber + imageIndex),
    };
    for (size_t entryIndex = 0; entryIndex < kFakeImageEntriesPerImage; ++entryIndex) {
      section->descriptions[entryIndex] = (GPBExtensionDescription){
          .singletonName = "GPBExtensionRegistryTest_fakeImageExtension",
          .extendedClass.clazz = [TestAllExtensions class],
          .fieldNumber = fieldNumbers[entryIndex],
          .dataType = GPBDataTypeInt32,
          .options = GPBExtensionNone,
      };
      section->entries[entryIndex].description = &section->descriptions[entryIndex];
    }
  }
  return sections;
}

- (void)testExtensionSectionIndexListFirstAppendedImageWins {
  const size_t kImageCount = 32;
  GPBTestFakeImageSection *sections = CreateFakeImageSections(kImageCount);
  GPBExtensionSectionIndexList *list = GPBExtensionSectionIndexListCreateForTesting();
  Class extendedClass = [TestAllExtensions class];

  XCTAssertEqual(GPBExtensionSectionIndexListCountForTesting(list), (size_t)0);
  XCTAssertTrue(GPBExtensionSectionIndexListFindForTesting(list, extendedClass,
                                                           kFakeImageSharedFieldNumber) == NULL);

  for (size_t imageIndex = 0; imageIndex < kImageCount; ++imageIndex) {
    GPBExtensionSectionIndexListAppendForTesting(list, sections[imageIndex].entries,
                                                 kFakeImageEntriesPerImage);
    // The first image's entry keeps winning as later images are appended.
    XCTAssertTrue(GPBExtensionSectionIndexListFindForTesting(list, extendedClass,
                                                             kFakeImageSharedFieldNumber) ==
                  &sections[0].entries[0]);  // ptr equality
  }
  // An image without entries adds no index.
  GPBExtensionSectionIndexListAppendForTesting(list, sections[0].entries, 0);
  XCTAssertEqual(GPBExtensionSectionIndexListCountForTesting(list), kImageCount);

  for (size_t imageIndex = 0; imageIndex < kImageCount; ++imageIndex) {
    XCTAssertTrue(GPBExtensionSectionIndexListFindForTesting(
                      list, extendedClass, kFakeImageFirstUniqueFieldNumber + imageIndex) ==
                  &sections[imageIndex].entries[1]);  // ptr equality
  }
  XCTAssertTrue(GPBExtensionSectionIndexListFindForTesting(
                    list, extendedClass, kFakeImageFirstUniqueFieldNumber + kImageCount) == NULL);
  XCTAssertTrue(GPBExtensionSectionIndexListFindForTesting(list, [TestAllTypes class],
                                                           kFakeImageSharedFieldNumber) == NULL);

  GPBExtensionSectionIndexListDestroyForTesting(list);
  free(sections);
}

// Appends from many threads at once (the contended compare-and-swap path,
// which dyld's serialized callbacks never exercise) while other threads look
// up entries, and checks that no index is lost or seen partially built and
// that the winner of a duplicated key never changes once observed.
- (void)testExtensionSectionIndexListConcurrentAppendsAndLookups {
  const size_t kImageCount = 256;
  const size_t kLookupsPerReader = 2000;
  GPBTestFakeImageSection *sections = CreateFakeImageSections(kImageCount);
  GPBExtensionSectionIndexList *list = GPBExtensionSectionIndexListCreateForTesting();
  Class extendedClass = [TestAllExtensions class];

  // Shared-key results seen by each reader (NULL until some index is
  // published), checked after all threads finish.
  GPBExtensionSectionEntry **sharedKeyResults =
      calloc(kImageCount * kLookupsPerReader, sizeof(GPBExtensionSectionEntry *));
  _Atomic(int) *badUniqueKeyResults = calloc(1, sizeof(_Atomic(int)));

  // Even iterations append one image each; odd iterations are readers.
  dispatch_apply(kImageCount * 2, dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0),
                 ^(size_t iteration) {
                   size_t imageIndex = iteration / 2;
                   if (iteration % 2 == 0) {
                     GPBExtensionSectionIndexListAppendForTesting(
                         list, sections[imageIndex].entries, kFakeImageEntriesPerImage);
                     return;
                   }
                   for (size_t lookup = 0; lookup < kLookupsPerReader; ++lookup) {
                     sharedKeyResults[imageIndex * kLookupsPerReader + lookup] =
                         GPBExtensionSectionIndexListFindForTesting(list, extendedClass,
                                                                    kFakeImageSharedFieldNumber);
                     size_t probedImage = (imageIndex + lookup) % kImageCount;
                     GPBExtensionSectionEntry *unique = GPBExtensionSectionIndexListFindForTesting(
                         list, extendedClass, kFakeImageFirstUniqueFieldNumber + probedImage);
                     // Either not published yet, or exactly that image's entry.
                     if (unique != NULL && unique != &sections[probedImage].entries[1]) {
                       atomic_fetch_add(badUniqueKeyResults, 1);
                     }
                   }
                 });

  XCTAssertEqual(atomic_load(badUniqueKeyResults), 0);
  free(badUniqueKeyResults);
  // Every image was appended exactly once.
  XCTAssertEqual(GPBExtensionSectionIndexListCountForTesting(list), kImageCount);
  for (size_t imageIndex = 0; imageIndex < kImageCount; ++imageIndex) {
    XCTAssertTrue(GPBExtensionSectionIndexListFindForTesting(
                      list, extendedClass, kFakeImageFirstUniqueFieldNumber + imageIndex) ==
                  &sections[imageIndex].entries[1]);  // ptr equality
  }
  // The first published image won the shared key, and no reader ever saw a
  // different winner.
  GPBExtensionSectionEntry *winner =
      GPBExtensionSectionIndexListFindForTesting(list, extendedClass, kFakeImageSharedFieldNumber);
  XCTAssertTrue(winner != NULL);
  size_t mismatchedSharedKeyResults = 0;
  for (size_t index = 0; index < kImageCount * kLookupsPerReader; ++index) {
    if (sharedKeyResults[index] != NULL && sharedKeyResults[index] != winner) {
      ++mismatchedSharedKeyResults;
    }
  }
  XCTAssertEqual(mismatchedSharedKeyResults, (size_t)0);

  free(sharedKeyResults);
  GPBExtensionSectionIndexListDestroyForTesting(list);
  free(sections);
}

@end
