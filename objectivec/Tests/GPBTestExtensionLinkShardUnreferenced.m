// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// See GPBTestExtensionLinkShards.h. Nothing references this file, so the
// linker does not load its object file and its section entry is not linked.

#import "GPBTestExtensionLinkShards.h"

#import "GPBBootstrap.h"
#import "GPBDescriptor_PackagePrivate.h"
#import "GPBUtilities_PackagePrivate.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"

GPBObjCClassDeclaration(TestAllExtensions);

static GPBExtensionDescription gUnreferencedShardExtensionDescription = {
    .defaultValue.valueInt32 = 0,
    .singletonName = "GPBTestExtensionLinkShardUnreferenced_extension",
    .extendedClass.clazz = GPBObjCClass(TestAllExtensions),
    .messageOrGroupClass.clazz = Nil,
    .enumDescriptorFunc = NULL,
    .fieldNumber = kGPBTestExtensionLinkShardUnreferencedFieldNumber,
    .dataType = GPBDataTypeInt32,
    .options = GPBExtensionNone,
};

GPB_EXTENSION_REGISTRY_SECTION_ATTR static GPBExtensionSectionEntry
    gUnreferencedShardExtensionEntry = {
        .description = &gUnreferencedShardExtensionDescription,
        .descriptor = nil,
    };

#pragma clang diagnostic pop

GPBExtensionDescriptor *GPBTestExtensionLinkShardUnreferencedExtension(void) {
  return GPBResolveExtensionSectionEntry(&gUnreferencedShardExtensionEntry,
                                         &GOOGLE_PROTOBUF_OBJC_EXPECTED_GENCODE_VERSION_40311);
}
