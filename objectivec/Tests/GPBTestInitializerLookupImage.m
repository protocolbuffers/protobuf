// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Source of a dynamic library that GPBGlobalExtensionRegistryProcessTest.m
// loads with dlopen() before anything has used +[GPBExtensionRegistry
// globalExtensionRegistry].
//
// Its initializer, which dyld runs inside dlopen() while holding its locks,
// starts the global registry's first lookup on another thread, waits until
// that lookup has started indexing (i.e. is about to call or is inside
// _dyld_register_func_for_add_image(), which needs dyld's locks), and then does
// a lookup itself. If that lookup waited for the first one (e.g. a
// dispatch_once around the dyld registration), the two threads would deadlock.
//
// Like GPBTestDynamicExtensionImage.m, the library is NOT linked against the
// protobuf runtime or the test protos; they are resolved from the test bundle
// at load time (`-undefined dynamic_lookup`).

#import <dispatch/dispatch.h>
#import <objc/runtime.h>
#import <unistd.h>

#import "GPBBootstrap.h"
#import "GPBDescriptor_PackagePrivate.h"
#import "GPBExtensionRegistry.h"
#import "GPBMessage.h"
#import "GPBUtilities_PackagePrivate.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"

GPBObjCClassDeclaration(TestAllExtensions);

// Must match kInitializerImageExtensionFieldNumber in
// GPBGlobalExtensionRegistryProcessTest.m.
static GPBExtensionDescription gInitializerImageExtensionDescription = {
    .defaultValue.valueInt32 = 0,
    .singletonName = "GPBTestInitializerLookupImage_extension",
    .extendedClass.clazz = GPBObjCClass(TestAllExtensions),
    .messageOrGroupClass.clazz = Nil,
    .enumDescriptorFunc = NULL,
    .fieldNumber = 30102,
    .dataType = GPBDataTypeInt32,
    .options = GPBExtensionNone,
};

GPB_EXTENSION_REGISTRY_SECTION_ATTR static GPBExtensionSectionEntry
    gInitializerImageExtensionEntry = {
        .description = &gInitializerImageExtensionDescription,
        .descriptor = nil,
    };

#pragma clang diagnostic pop

// A field number no extension uses; the first lookup's result must be nil.
static const NSInteger kInitializerImageUnusedFieldNumber = 30199;

// Written by the initializer and the first lookup's thread; read by the test
// after dlopen() returns (and after waiting for gFirstLookupDone).
static GPBExtensionDescriptor *gInitializerLookupResult = nil;
static GPBExtensionDescriptor *gFirstLookupResult = nil;
static BOOL gFirstLookupStartedBeforeInitializerLookup = NO;
static dispatch_semaphore_t gFirstLookupDone = NULL;

__attribute__((constructor)) static void GPBTestInitializerLookupImageInitializer(void) {
  Class extendedClass = objc_getClass("TestAllExtensions");
  GPBDescriptor *extendedDescriptor = [extendedClass descriptor];
  id<GPBExtensionRegistry> globalRegistry = [GPBExtensionRegistry globalExtensionRegistry];

  gFirstLookupDone = dispatch_semaphore_create(0);
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
    gFirstLookupResult = [globalRegistry extensionForDescriptor:extendedDescriptor
                                                    fieldNumber:kInitializerImageUnusedFieldNumber];
    dispatch_semaphore_signal(gFirstLookupDone);
  });

  // Wait (bounded) until the other thread's first lookup has started indexing.
  for (int attempt = 0; attempt < 5000; ++attempt) {
    if (GPBGlobalExtensionRegistryHasStartedIndexing()) {
      gFirstLookupStartedBeforeInitializerLookup = YES;
      break;
    }
    usleep(1000);
  }

  // Must not wait for the first lookup, which may be blocked on dyld's locks
  // held by this thread.
  gInitializerLookupResult =
      [globalRegistry extensionForDescriptor:extendedDescriptor
                                 fieldNumber:gInitializerImageExtensionDescription.fieldNumber];
}

// Exported for GPBGlobalExtensionRegistryProcessTest.m, which looks them up
// with dlsym().
__attribute__((visibility("default"))) GPBExtensionDescriptor *
GPBTestInitializerLookupImageExtension(void);
__attribute__((visibility("default"))) GPBExtensionDescriptor *
GPBTestInitializerLookupImageInitializerLookupResult(void);
__attribute__((visibility("default"))) BOOL
GPBTestInitializerLookupImageFirstLookupStartedBeforeInitializerLookup(void);
__attribute__((visibility("default"))) BOOL GPBTestInitializerLookupImageWaitForFirstLookup(
    int64_t timeoutInSeconds, GPBExtensionDescriptor **firstLookupResult);

// The extension's accessor, like a generated extension function.
GPBExtensionDescriptor *GPBTestInitializerLookupImageExtension(void) {
  return GPBResolveExtensionSectionEntry(&gInitializerImageExtensionEntry,
                                         &GOOGLE_PROTOBUF_OBJC_EXPECTED_GENCODE_VERSION_40311);
}

// What the lookup in the initializer returned.
GPBExtensionDescriptor *GPBTestInitializerLookupImageInitializerLookupResult(void) {
  return gInitializerLookupResult;
}

// Whether the first lookup had started indexing when the initializer's lookup
// ran (i.e. whether the potentially deadlocking interleaving was exercised).
BOOL GPBTestInitializerLookupImageFirstLookupStartedBeforeInitializerLookup(void) {
  return gFirstLookupStartedBeforeInitializerLookup;
}

// Waits for the first lookup's thread. Returns NO on timeout.
BOOL GPBTestInitializerLookupImageWaitForFirstLookup(int64_t timeoutInSeconds,
                                                     GPBExtensionDescriptor **firstLookupResult) {
  if (dispatch_semaphore_wait(gFirstLookupDone,
                              dispatch_time(DISPATCH_TIME_NOW, timeoutInSeconds * NSEC_PER_SEC)) !=
      0) {
    return NO;
  }
  *firstLookupResult = gFirstLookupResult;
  return YES;
}
