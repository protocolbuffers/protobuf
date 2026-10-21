// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// See GPBTestExtensionLinkShards.h. Referenced by GPBExtensionRegistryTest.m,
// so the linker loads this object file.

#import "GPBTestExtensionLinkShards.h"

#import "GPBBootstrap.h"
#import "GPBDescriptor_PackagePrivate.h"
#import "GPBUtilities_PackagePrivate.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"

GPBObjCClassDeclaration(TestAllExtensions);

static GPBExtensionDescription gReferencedShardExtensionDescription = {
    .defaultValue.valueInt32 = 0,
    .singletonName = "GPBTestExtensionLinkShardReferenced_extension",
    .extendedClass.clazz = GPBObjCClass(TestAllExtensions),
    .messageOrGroupClass.clazz = Nil,
    .enumDescriptorFunc = NULL,
    .fieldNumber = kGPBTestExtensionLinkShardReferencedFieldNumber,
    .dataType = GPBDataTypeInt32,
    .options = GPBExtensionNone,
};

GPB_EXTENSION_REGISTRY_SECTION_ATTR static GPBExtensionSectionEntry gReferencedShardExtensionEntry =
    {
        .description = &gReferencedShardExtensionDescription,
        .descriptor = nil,
    };

#pragma clang diagnostic pop

GPBExtensionDescriptor *GPBTestExtensionLinkShardReferencedExtension(void) {
  return GPBResolveExtensionSectionEntry(&gReferencedShardExtensionEntry,
                                         &GOOGLE_PROTOBUF_OBJC_EXPECTED_GENCODE_VERSION_40311);
}
