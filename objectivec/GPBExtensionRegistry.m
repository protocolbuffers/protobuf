// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#import "GPBExtensionRegistry.h"

#import "GPBBootstrap.h"
#import "GPBDescriptor.h"

@implementation GPBExtensionRegistry {
  NSMutableDictionary *mutableClassMap_;
}

- (instancetype)init {
  if ((self = [super init])) {
    mutableClassMap_ = [[NSMutableDictionary alloc] init];
  }
  return self;
}

- (void)dealloc {
  [mutableClassMap_ release];
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
  CFMutableDictionaryRef extensionMap = (CFMutableDictionaryRef)
      [mutableClassMap_ objectForKey:containingMessageClass];
  if (extensionMap == nil) {
    // Use a custom dictionary here because the keys are numbers and conversion
    // back and forth from NSNumber isn't worth the cost.
    extensionMap = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, NULL,
                                             &kCFTypeDictionaryValueCallBacks);
    [mutableClassMap_ setObject:(id)extensionMap
                         forKey:(id<NSCopying>)containingMessageClass];
    CFRelease(extensionMap);
  }

  ssize_t key = extension.fieldNumber;
  CFDictionarySetValue(extensionMap, (const void *)key, extension);
}

- (GPBExtensionDescriptor *)extensionForDescriptor:(GPBDescriptor *)descriptor
                                       fieldNumber:(NSInteger)fieldNumber {
  Class messageClass = descriptor.messageClass;
  CFMutableDictionaryRef extensionMap = (CFMutableDictionaryRef)
      [mutableClassMap_ objectForKey:messageClass];
  ssize_t key = fieldNumber;
  GPBExtensionDescriptor *result =
      (extensionMap
       ? CFDictionaryGetValue(extensionMap, (const void *)key)
       : nil);
  return result;
}

static void CopyKeyValue(const void *key, const void *value, void *context) {
  CFMutableDictionaryRef extensionMap = (CFMutableDictionaryRef)context;
  CFDictionarySetValue(extensionMap, key, value);
}

- (void)addExtensions:(GPBExtensionRegistry *)registry {
  if (registry == nil) {
    // In the case where there are no extensions just ignore.
    return;
  }
  NSMutableDictionary *otherClassMap = registry->mutableClassMap_;
  [otherClassMap enumerateKeysAndObjectsUsingBlock:^(id key, id value, BOOL * stop) {
#pragma unused(stop)
    Class containingMessageClass = key;
    CFMutableDictionaryRef otherExtensionMap = (CFMutableDictionaryRef)value;

    CFMutableDictionaryRef extensionMap = (CFMutableDictionaryRef)
        [mutableClassMap_ objectForKey:containingMessageClass];
    if (extensionMap == nil) {
      extensionMap = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, otherExtensionMap);
      [mutableClassMap_ setObject:(id)extensionMap
                           forKey:(id<NSCopying>)containingMessageClass];
      CFRelease(extensionMap);
    } else {
      CFDictionaryApplyFunction(otherExtensionMap, CopyKeyValue, extensionMap);
    }
  }];
}

#pragma clang diagnostic pop

@end
