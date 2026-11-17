// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// The only test in its test targets (macos_ and ios_global_extension_registry_
// process_test), so it runs in a fresh process in which nothing has used
// +[GPBExtensionRegistry globalExtensionRegistry] yet. That makes it possible
// to check, deterministically, that the global registry reads no Mach-O
// section before its first lookup, that the first lookup does not deadlock
// with a lookup from an image initializer running inside dlopen(), and to load
// a dynamic library after that first lookup.

#import <XCTest/XCTest.h>
#import <dlfcn.h>

#import "GPBDescriptor_PackagePrivate.h"
#import "GPBExtensionRegistry.h"
#import "GPBUtilities_PackagePrivate.h"
#import "objectivec/Tests/Unittest.pbobjc.h"

// Enum constants, so they can be used in the static initializer.
enum {
  // The extension in libGPBTestDynamicExtensionImage.dylib
  // (GPBTestDynamicExtensionImage.m).
  kDynamicImageExtensionFieldNumber = 30101,
  // The extension in libGPBTestInitializerLookupImage.dylib
  // (GPBTestInitializerLookupImage.m).
  kInitializerImageExtensionFieldNumber = 30102,
};

// Seconds to wait for the first lookup started by the initializer of
// libGPBTestInitializerLookupImage.dylib.
static const int64_t kFirstLookupTimeoutInSeconds = 60;

typedef GPBExtensionDescriptor *(*GPBTestExtensionFunction)(void);
typedef GPBExtensionSectionEntry *(*GPBTestExtensionEntryFunction)(void);
typedef BOOL (*GPBTestBoolFunction)(void);
typedef BOOL (*GPBTestWaitForFirstLookupFunction)(int64_t timeoutInSeconds,
                                                  GPBExtensionDescriptor **firstLookupResult);

@interface GPBGlobalExtensionRegistryProcessTest : XCTestCase
@end

@implementation GPBGlobalExtensionRegistryProcessTest

// One test method, because the steps depend on the process wide state left by
// the previous ones.
- (void)testGlobalExtensionRegistryLifecycle {
  GPBDescriptor *extendedDescriptor = [TestAllExtensions descriptor];

  // 1. Getting the global registry reads no section.
  XCTAssertFalse(GPBGlobalExtensionRegistryHasStartedIndexing());
  id<GPBExtensionRegistry> globalRegistry = [GPBExtensionRegistry globalExtensionRegistry];
  XCTAssertFalse(GPBGlobalExtensionRegistryHasStartedIndexing());

  // The dynamic images resolve the protobuf runtime and TestAllExtensions from
  // this test bundle through the flat namespace, so first make the (already
  // loaded) test bundle's symbols globally visible.
  NSBundle *testBundle = [NSBundle bundleForClass:[self class]];
  NSString *testBundleExecutablePath = testBundle.executablePath;
  XCTAssertNotNil(testBundleExecutablePath);
  void *testBundleHandle = dlopen(testBundleExecutablePath.fileSystemRepresentation,
                                  RTLD_NOW | RTLD_NOLOAD | RTLD_GLOBAL);
  XCTAssertTrue(testBundleHandle != NULL, @"%s", dlerror());

  // 2. Load an image whose initializer (run by dyld inside dlopen()) starts the
  // global registry's first lookup on another thread and, once that lookup has
  // started indexing, does a lookup itself. Neither may wait for the other:
  // dlopen() returning (instead of hanging) is the main check.
  NSString *initializerImagePath = [testBundle pathForResource:@"libGPBTestInitializerLookupImage"
                                                        ofType:@"dylib"];
  XCTAssertNotNil(initializerImagePath);
  if (initializerImagePath == nil) {
    return;
  }
  // Never closed: images with Objective-C references are not unloaded.
  void *initializerImageHandle =
      dlopen(initializerImagePath.fileSystemRepresentation, RTLD_NOW | RTLD_LOCAL);
  XCTAssertTrue(initializerImageHandle != NULL, @"%s", dlerror());
  if (initializerImageHandle == NULL) {
    return;
  }
  GPBTestExtensionFunction initializerImageExtensionFunction = (GPBTestExtensionFunction)dlsym(
      initializerImageHandle, "GPBTestInitializerLookupImageExtension");
  GPBTestExtensionFunction initializerLookupResultFunction = (GPBTestExtensionFunction)dlsym(
      initializerImageHandle, "GPBTestInitializerLookupImageInitializerLookupResult");
  GPBTestBoolFunction firstLookupStartedFunction = (GPBTestBoolFunction)dlsym(
      initializerImageHandle,
      "GPBTestInitializerLookupImageFirstLookupStartedBeforeInitializerLookup");
  GPBTestWaitForFirstLookupFunction waitForFirstLookupFunction =
      (GPBTestWaitForFirstLookupFunction)dlsym(initializerImageHandle,
                                               "GPBTestInitializerLookupImageWaitForFirstLookup");
  XCTAssertTrue(initializerImageExtensionFunction != NULL);
  XCTAssertTrue(initializerLookupResultFunction != NULL);
  XCTAssertTrue(firstLookupStartedFunction != NULL);
  XCTAssertTrue(waitForFirstLookupFunction != NULL);
  if (initializerImageExtensionFunction == NULL || initializerLookupResultFunction == NULL ||
      firstLookupStartedFunction == NULL || waitForFirstLookupFunction == NULL) {
    return;
  }
  // The interleaving that used to deadlock was exercised.
  XCTAssertTrue(firstLookupStartedFunction());
  GPBExtensionDescriptor *firstLookupResult = nil;
  XCTAssertTrue(waitForFirstLookupFunction(kFirstLookupTimeoutInSeconds, &firstLookupResult));
  XCTAssertNil(firstLookupResult);
  XCTAssertTrue(GPBGlobalExtensionRegistryHasStartedIndexing());
  // The lookup in the initializer found the extension of the image being
  // loaded: the instance its accessor returns.
  GPBExtensionDescriptor *fromInitializerLookup = initializerLookupResultFunction();
  XCTAssertNotNil(fromInitializerLookup);
  XCTAssertTrue(initializerImageExtensionFunction() == fromInitializerLookup);  // ptr equality
  XCTAssertTrue([globalRegistry extensionForDescriptor:extendedDescriptor
                                           fieldNumber:kInitializerImageExtensionFieldNumber] ==
                fromInitializerLookup);  // ptr equality

  // 3. The dynamic image is not loaded yet.
  XCTAssertNil([globalRegistry extensionForDescriptor:extendedDescriptor
                                          fieldNumber:kDynamicImageExtensionFieldNumber]);

  // 4. Load the dynamic image after the global registry's first lookup.
  NSString *imagePath = [testBundle pathForResource:@"libGPBTestDynamicExtensionImage"
                                             ofType:@"dylib"];
  XCTAssertNotNil(imagePath);
  if (imagePath == nil) {
    return;
  }
  // Never closed: images with Objective-C references are not unloaded.
  void *imageHandle = dlopen(imagePath.fileSystemRepresentation, RTLD_NOW | RTLD_LOCAL);
  XCTAssertTrue(imageHandle != NULL, @"%s", dlerror());
  if (imageHandle == NULL) {
    return;
  }
  GPBTestExtensionEntryFunction entryFunction = (GPBTestExtensionEntryFunction)dlsym(
      imageHandle, "GPBTestDynamicExtensionImageExtensionEntry");
  GPBTestExtensionFunction extensionFunction =
      (GPBTestExtensionFunction)dlsym(imageHandle, "GPBTestDynamicExtensionImageExtension");
  XCTAssertTrue(entryFunction != NULL);
  XCTAssertTrue(extensionFunction != NULL);
  if (entryFunction == NULL || extensionFunction == NULL) {
    return;
  }
  GPBExtensionSectionEntry *entry = entryFunction();
  // Loading (and indexing) the image created no descriptor.
  XCTAssertTrue(entry->descriptor == nil);

  // 5. The global registry now finds the image's extension: the instance its
  // accessor returns.
  GPBExtensionDescriptor *fromGlobalRegistry =
      [globalRegistry extensionForDescriptor:extendedDescriptor
                                 fieldNumber:kDynamicImageExtensionFieldNumber];
  XCTAssertNotNil(fromGlobalRegistry);
  XCTAssertTrue(entry->descriptor == fromGlobalRegistry);    // ptr equality
  XCTAssertTrue(extensionFunction() == fromGlobalRegistry);  // ptr equality
  XCTAssertTrue(fromGlobalRegistry.containingMessageClass == [TestAllExtensions class]);
  XCTAssertEqual(fromGlobalRegistry.fieldNumber, (uint32_t)kDynamicImageExtensionFieldNumber);

  // 6. The dynamic image also has an entry for the initializer image's
  // extension. The initializer image was loaded first, so the global registry
  // still returns its descriptor (first loaded image wins) and never creates a
  // descriptor for the later image's duplicate entry.
  GPBTestExtensionEntryFunction duplicateEntryFunction = (GPBTestExtensionEntryFunction)dlsym(
      imageHandle, "GPBTestDynamicExtensionImageDuplicateExtensionEntry");
  XCTAssertTrue(duplicateEntryFunction != NULL);
  if (duplicateEntryFunction == NULL) {
    return;
  }
  XCTAssertTrue([globalRegistry extensionForDescriptor:extendedDescriptor
                                           fieldNumber:kInitializerImageExtensionFieldNumber] ==
                fromInitializerLookup);  // ptr equality
  XCTAssertTrue(duplicateEntryFunction()->descriptor == nil);
}

@end
