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
// section before its first lookup, and to load a dynamic library after that
// first lookup.

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
};

typedef GPBExtensionDescriptor *(*GPBTestExtensionFunction)(void);
typedef GPBExtensionSectionEntry *(*GPBTestExtensionEntryFunction)(void);

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

  // 2. The first lookup on the global registry indexes the loaded images. The
  // dynamic image is not loaded yet.
  XCTAssertNil([globalRegistry extensionForDescriptor:extendedDescriptor
                                          fieldNumber:kDynamicImageExtensionFieldNumber]);
  XCTAssertTrue(GPBGlobalExtensionRegistryHasStartedIndexing());

  // 3. Load the dynamic image. It resolves the protobuf runtime and
  // TestAllExtensions from this test bundle through the flat namespace, so
  // first make the (already loaded) test bundle's symbols globally visible.
  NSBundle *testBundle = [NSBundle bundleForClass:[self class]];
  NSString *testBundleExecutablePath = testBundle.executablePath;
  XCTAssertNotNil(testBundleExecutablePath);
  void *testBundleHandle = dlopen(testBundleExecutablePath.fileSystemRepresentation,
                                  RTLD_NOW | RTLD_NOLOAD | RTLD_GLOBAL);
  XCTAssertTrue(testBundleHandle != NULL, @"%s", dlerror());
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

  // 4. The global registry now finds the image's extension: the instance its
  // accessor returns.
  GPBExtensionDescriptor *fromGlobalRegistry =
      [globalRegistry extensionForDescriptor:extendedDescriptor
                                 fieldNumber:kDynamicImageExtensionFieldNumber];
  XCTAssertNotNil(fromGlobalRegistry);
  XCTAssertTrue(entry->descriptor == fromGlobalRegistry);    // ptr equality
  XCTAssertTrue(extensionFunction() == fromGlobalRegistry);  // ptr equality
  XCTAssertTrue(fromGlobalRegistry.containingMessageClass == [TestAllExtensions class]);
  XCTAssertEqual(fromGlobalRegistry.fieldNumber, (uint32_t)kDynamicImageExtensionFieldNumber);
}

@end
