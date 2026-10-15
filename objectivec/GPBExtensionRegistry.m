// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBExtensionRegistry.h"

#import <mach-o/dyld.h>
#import <mach-o/getsect.h>
#import <stdatomic.h>
#import <stdlib.h>

#import "GPBBootstrap.h"
#import "GPBDescriptor.h"
#import "GPBDescriptor_PackagePrivate.h"

#pragma mark - Global extension registry

// Pointer-width-appropriate Mach-O header type for getsectiondata().
#if __LP64__
typedef struct mach_header_64 GPBMachHeader;
#else
typedef struct mach_header GPBMachHeader;
#endif

// The index of the "__DATA,__gpb_ext_reg" section of one loaded Mach-O image.
//
// `classMap` maps `Class` (extended message class) -> CFDictionaryRef of
// (ssize_t fieldNumber -> GPBExtensionSectionEntry *). Only raw pointers to the
// static section entries are stored, so building the index creates no
// GPBExtensionDescriptor objects and sends no Objective-C messages.
//
// An index is fully built before it is published and is never modified or
// freed after that (images with Objective-C classes are never unloaded), so
// lookups read published indexes without taking a lock.
typedef struct GPBExtensionSectionImageIndex {
  CFDictionaryRef classMap;
  struct GPBExtensionSectionImageIndex *next;
} GPBExtensionSectionImageIndex;

// Singly linked list of the published image indexes, newest first. A new
// index is pushed with a compare-and-swap (release); lookups load the head
// with acquire semantics and walk the list.
static _Atomic(GPBExtensionSectionImageIndex *) gExtensionSectionImageIndexes = NULL;

// Set when the first lookup starts reading the sections. Only used by tests.
static atomic_bool gGlobalExtensionRegistryStartedIndexing = false;

BOOL GPBGlobalExtensionRegistryHasStartedIndexing(void) {
  return atomic_load(&gGlobalExtensionRegistryStartedIndexing) ? YES : NO;
}

// dyld add-image callback. Called synchronously for every already loaded image
// from within _dyld_register_func_for_add_image(), and later for every newly
// loaded image (on the thread loading it, before the image's initializers have
// run). Only reads the static section data.
static void GPBIndexExtensionSectionOfImage(const struct mach_header *header,
                                            __unused intptr_t vmaddrSlide) {
  unsigned long sectionSize = 0;
  uint8_t *sectionData =
      getsectiondata((const GPBMachHeader *)header, GPB_EXTENSION_SECTION_SEGMENT,
                     GPB_EXTENSION_SECTION_NAME, &sectionSize);
  if (sectionData == NULL || sectionSize < sizeof(GPBExtensionSectionEntry)) {
    return;
  }
  GPBExtensionSectionEntry *entries = (GPBExtensionSectionEntry *)sectionData;
  size_t entryCount = sectionSize / sizeof(GPBExtensionSectionEntry);

  CFMutableDictionaryRef classMap =
      CFDictionaryCreateMutable(kCFAllocatorDefault, 0, NULL, &kCFTypeDictionaryValueCallBacks);
  for (size_t entryIndex = 0; entryIndex < entryCount; ++entryIndex) {
    GPBExtensionSectionEntry *entry = &entries[entryIndex];
    GPBExtensionDescription *description = entry->description;
    if (description == NULL) {
      continue;
    }
    Class extendedClass = description->extendedClass.clazz;
    if (extendedClass == Nil) {
      continue;
    }
    CFMutableDictionaryRef fieldMap =
        (CFMutableDictionaryRef)CFDictionaryGetValue(classMap, extendedClass);
    if (fieldMap == NULL) {
      fieldMap = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, NULL, NULL);
      CFDictionarySetValue(classMap, extendedClass, fieldMap);
      CFRelease(fieldMap);
    }
    ssize_t key = description->fieldNumber;
    CFDictionarySetValue(fieldMap, (const void *)key, entry);
  }
  if (CFDictionaryGetCount(classMap) == 0) {
    CFRelease(classMap);
    return;
  }

  GPBExtensionSectionImageIndex *imageIndex = malloc(sizeof(GPBExtensionSectionImageIndex));
  if (imageIndex == NULL) {
    abort();
  }
  imageIndex->classMap = classMap;
  imageIndex->next = atomic_load_explicit(&gExtensionSectionImageIndexes, memory_order_relaxed);
  while (!atomic_compare_exchange_weak_explicit(&gExtensionSectionImageIndexes, &imageIndex->next,
                                                imageIndex, memory_order_release,
                                                memory_order_relaxed)) {
    // `imageIndex->next` was updated to the current head; retry.
  }
}

// Starts indexing the extension sections the first time it is called. When it
// returns, every image that was loaded before the first call has been indexed.
static void GPBStartIndexingExtensionSections(void) {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    atomic_store(&gGlobalExtensionRegistryStartedIndexing, true);
    _dyld_register_func_for_add_image(&GPBIndexExtensionSectionOfImage);
  });
}

// The immutable registry returned by +[GPBExtensionRegistry globalExtensionRegistry].
//
// Synchronization: lookups take no lock. The first lookup indexes the already
// loaded images inside a dispatch_once, so every lookup sees them. Images
// loaded later are indexed by the dyld callback on the loading thread and
// published with a release compare-and-swap; a lookup that runs at the same
// time as an image load may not see that image's extensions yet. Descriptor
// creation goes through GPBResolveExtensionSectionEntry() (atomic
// compare-and-swap), which the generated extension functions also use.
__attribute__((objc_subclassing_restricted))
@interface GPBGlobalSectionExtensionRegistry : NSObject<GPBExtensionRegistry>
@end

@implementation GPBGlobalSectionExtensionRegistry

- (GPBExtensionDescriptor *)extensionForDescriptor:(GPBDescriptor *)descriptor
                                       fieldNumber:(NSInteger)fieldNumber {
  GPBStartIndexingExtensionSections();
  Class messageClass = descriptor.messageClass;
  ssize_t key = fieldNumber;
  for (GPBExtensionSectionImageIndex *imageIndex =
           atomic_load_explicit(&gExtensionSectionImageIndexes, memory_order_acquire);
       imageIndex != NULL; imageIndex = imageIndex->next) {
    CFDictionaryRef fieldMap = CFDictionaryGetValue(imageIndex->classMap, messageClass);
    if (fieldMap == NULL) {
      continue;
    }
    GPBExtensionSectionEntry *entry =
        (GPBExtensionSectionEntry *)CFDictionaryGetValue(fieldMap, (const void *)key);
    if (entry != NULL) {
      return GPBResolveExtensionSectionEntry(entry,
                                             &GOOGLE_PROTOBUF_OBJC_EXPECTED_GENCODE_VERSION_40311);
    }
  }
  return nil;
}

@end

#pragma mark - GPBExtensionRegistry

// `globalRegistry_` is set (to the +globalExtensionRegistry singleton) when
// the global registry, or a registry that has it, is merged in with
// -addExtensions:. Lookups that miss `mutableClassMap_` are then forwarded to
// it, so merging does not read any Mach-O section, images loaded later stay
// visible, and explicitly added extensions take precedence.
@implementation GPBExtensionRegistry {
  CFMutableDictionaryRef mutableClassMap_;
  GPBGlobalSectionExtensionRegistry *globalRegistry_;
}

- (instancetype)init {
  if ((self = [super init])) {
    // IMPORTANT: In c_function mode, this may be called from any thread during a dispatch_once
    // scoped to each specific registry - i.e., other registries may be initializing at the same
    // time.

    // The keys are ObjC classes, so straight up ptr comparisons are fine.
    mutableClassMap_ =
        CFDictionaryCreateMutable(kCFAllocatorDefault, 0, NULL, &kCFTypeDictionaryValueCallBacks);
  }
  return self;
}

- (void)dealloc {
  CFRelease(mutableClassMap_);
  [globalRegistry_ release];
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
  if (result == nil && globalRegistry_ != nil) {
    result = [globalRegistry_ extensionForDescriptor:descriptor fieldNumber:fieldNumber];
  }
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

- (void)addExtensions:(id<GPBExtensionRegistry>)registry {
  if (registry == nil) {
    // In the case where there are no extensions just ignore.
    return;
  }
  GPBGlobalSectionExtensionRegistry *globalRegistry = nil;
  if ([registry isKindOfClass:[GPBGlobalSectionExtensionRegistry class]]) {
    // +globalExtensionRegistry: keep a reference instead of copying entries.
    globalRegistry = (GPBGlobalSectionExtensionRegistry *)registry;
  } else if ([registry isKindOfClass:[GPBExtensionRegistry class]]) {
    GPBExtensionRegistry *otherRegistry = (GPBExtensionRegistry *)registry;
    CFDictionaryApplyFunction(otherRegistry->mutableClassMap_, CopySubDictionary, mutableClassMap_);
    globalRegistry = otherRegistry->globalRegistry_;
  } else {
    // Any other object only conforms to the protocol, so there is nothing that
    // can be copied from it.
    [NSException raise:NSInvalidArgumentException
                format:@"-[GPBExtensionRegistry addExtensions:] requires a GPBExtensionRegistry or "
                       @"+[GPBExtensionRegistry globalExtensionRegistry], got a %@.",
                       [registry class]];
  }
  if (globalRegistry != nil && globalRegistry_ == nil) {
    // There is only one global registry, so once set this never changes.
    globalRegistry_ = [globalRegistry retain];
  }
}

+ (id<GPBExtensionRegistry>)globalExtensionRegistry {
  static GPBGlobalSectionExtensionRegistry *globalRegistry = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    // Only creates the object; the sections are read on the first lookup.
    globalRegistry = [[GPBGlobalSectionExtensionRegistry alloc] init];
  });
  return globalRegistry;
}

#pragma clang diagnostic pop

@end
