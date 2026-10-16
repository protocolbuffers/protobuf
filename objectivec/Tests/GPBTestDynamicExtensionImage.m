// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Source of a dynamic library that GPBGlobalExtensionRegistryProcessTest.m
// loads with dlopen() to check that +[GPBExtensionRegistry
// globalExtensionRegistry] finds extensions from images loaded after its first
// lookup.
//
// The library is NOT linked against the protobuf runtime or the test protos
// (that would duplicate their classes). The runtime functions and the
// TestAllExtensions class are resolved from the test bundle at load time
// (`-undefined dynamic_lookup`).

#import "GPBBootstrap.h"
#import "GPBDescriptor_PackagePrivate.h"
#import "GPBUtilities_PackagePrivate.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"

GPBObjCClassDeclaration(TestAllExtensions);

// Must match kDynamicImageExtensionFieldNumber in
// GPBGlobalExtensionRegistryProcessTest.m.
static GPBExtensionDescription gDynamicImageExtensionDescription = {
    .defaultValue.valueInt32 = 0,
    .singletonName = "GPBTestDynamicExtensionImage_extension",
    .extendedClass.clazz = GPBObjCClass(TestAllExtensions),
    .messageOrGroupClass.clazz = Nil,
    .enumDescriptorFunc = NULL,
    .fieldNumber = 30101,
    .dataType = GPBDataTypeInt32,
    .options = GPBExtensionNone,
};

// The same form the generator emits with the
// `experimental_global_extension_registry` option.
GPB_EXTENSION_REGISTRY_SECTION_ATTR static GPBExtensionSectionEntry gDynamicImageExtensionEntry = {
    .description = &gDynamicImageExtensionDescription,
    .descriptor = nil,
};

#pragma clang diagnostic pop

// Exported for GPBGlobalExtensionRegistryProcessTest.m, which looks them up
// with dlsym().
__attribute__((visibility("default"))) GPBExtensionDescriptor *
GPBTestDynamicExtensionImageExtension(void);
__attribute__((visibility("default"))) GPBExtensionSectionEntry *
GPBTestDynamicExtensionImageExtensionEntry(void);

// The extension's accessor, like a generated extension function.
GPBExtensionDescriptor *GPBTestDynamicExtensionImageExtension(void) {
  return GPBResolveExtensionSectionEntry(&gDynamicImageExtensionEntry,
                                         &GOOGLE_PROTOBUF_OBJC_EXPECTED_GENCODE_VERSION_40311);
}

// The section entry, so the test can check whether its descriptor was created.
GPBExtensionSectionEntry *GPBTestDynamicExtensionImageExtensionEntry(void) {
  return &gDynamicImageExtensionEntry;
}
