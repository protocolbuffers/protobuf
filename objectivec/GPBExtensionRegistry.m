// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBExtensionRegistry.h"

#import "GPBBootstrap.h"
#import "GPBDescriptor.h"

@implementation GPBExtensionRegistry {
  CFMutableDictionaryRef mutableClassMap_;
}

- (instancetype)init {
  if ((self = [super init])) {
    // The keys are ObjC classes, so straight up ptr comparisons are fine.
    mutableClassMap_ =
        CFDictionaryCreateMutable(kCFAllocatorDefault, 0, NULL, &kCFTypeDictionaryValueCallBacks);
  }
  return self;
}

- (void)dealloc {
  CFRelease(mutableClassMap_);
  [super dealloc];
}

// Direct access is use for speed, to avoid even internally declaring things
// read/write, etc. The warning is enabled in the project to ensure code calling
// protos can turn on -Wdirect-ivar-access without issues.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdirect-ivar-access"

- (instancetype)copyWithZone:(NSZone *)zone {
  GPBExtensionRegistry *result = [[[self class] allocWithZone:zone] init];
  [result addExtensions:self];
  return result;
}

- (void)addExtension:(GPBExtensionDescriptor *)extension {
  if (extension == nil) {
    return;
  }

  Class containingMessageClass = extension.containingMessageClass;
  CFMutableDictionaryRef extensionMap =
      (CFMutableDictionaryRef)CFDictionaryGetValue(mutableClassMap_, containingMessageClass);
  if (extensionMap == nil) {
    // Use a custom dictionary here because the keys are numbers and conversion
    // back and forth from NSNumber isn't worth the cost.
    extensionMap =
        CFDictionaryCreateMutable(kCFAllocatorDefault, 0, NULL, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(mutableClassMap_, containingMessageClass, extensionMap);
    CFRelease(extensionMap);
  }

  ssize_t key = extension.fieldNumber;
  CFDictionarySetValue(extensionMap, (const void *)key, extension);
}

- (GPBExtensionDescriptor *)extensionForDescriptor:(GPBDescriptor *)descriptor
                                       fieldNumber:(NSInteger)fieldNumber {
  Class messageClass = descriptor.messageClass;
  CFMutableDictionaryRef extensionMap =
      (CFMutableDictionaryRef)CFDictionaryGetValue(mutableClassMap_, messageClass);
  ssize_t key = fieldNumber;
  GPBExtensionDescriptor *result =
      (extensionMap ? CFDictionaryGetValue(extensionMap, (const void *)key) : nil);
  return result;
}

static void CopyKeyValue(const void *key, const void *value, void *context) {
  CFMutableDictionaryRef extensionMap = (CFMutableDictionaryRef)context;
  CFDictionarySetValue(extensionMap, key, value);
}

static void CopySubDictionary(const void *key, const void *value, void *context) {
  CFMutableDictionaryRef mutableClassMap = (CFMutableDictionaryRef)context;
  Class containingMessageClass = key;
  CFMutableDictionaryRef otherExtensionMap = (CFMutableDictionaryRef)value;

  CFMutableDictionaryRef extensionMap =
      (CFMutableDictionaryRef)CFDictionaryGetValue(mutableClassMap, containingMessageClass);
  if (extensionMap == nil) {
    extensionMap = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, otherExtensionMap);
    CFDictionarySetValue(mutableClassMap, containingMessageClass, extensionMap);
    CFRelease(extensionMap);
  } else {
    CFDictionaryApplyFunction(otherExtensionMap, CopyKeyValue, extensionMap);
  }
}

- (void)addExtensions:(GPBExtensionRegistry *)registry {
  if (registry == nil) {
    // In the case where there are no extensions just ignore.
    return;
  }
  CFDictionaryApplyFunction(registry->mutableClassMap_, CopySubDictionary, mutableClassMap_);
}

#pragma clang diagnostic pop

@end
