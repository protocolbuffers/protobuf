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

// The index of the extension registration Mach-O section of one loaded image.
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
  _Atomic(struct GPBExtensionSectionImageIndex *) next;
} GPBExtensionSectionImageIndex;

// Singly linked list of the published image indexes in the order the images
// were indexed (load order), oldest first, so lookups walking from the head
// return the entry of the first loaded image when an extension is linked into
// more than one image ("first one wins"). A new index is appended at the tail
// with a compare-and-swap (release) on the last `next` link; lookups load the
// links with acquire semantics.
struct GPBExtensionSectionIndexList {
  _Atomic(GPBExtensionSectionImageIndex *) head;
};

// The list of the global registry.
static GPBExtensionSectionIndexList gExtensionSectionIndexList = {NULL};

// The states of the global registry's indexing of the extension sections.
typedef enum {
  // No lookup has happened yet; no section has been read.
  GPBExtensionSectionIndexingStateNotStarted = 0,
  // One thread is inside _dyld_register_func_for_add_image(), indexing the
  // already loaded images. The published indexes may be incomplete.
  GPBExtensionSectionIndexingStateRegistering = 1,
  // The already loaded images are indexed, and images loaded later are indexed
  // by the dyld add-image callback before their initializers run.
  GPBExtensionSectionIndexingStateIndexed = 2,
} GPBExtensionSectionIndexingState;

static _Atomic(int) gExtensionSectionIndexingState = GPBExtensionSectionIndexingStateNotStarted;

// Declared in GPBDescriptor_PackagePrivate.h for unit tests
// (GPBGlobalExtensionRegistryProcessTest.m).
BOOL GPBGlobalExtensionRegistryHasStartedIndexing(void) {
  return atomic_load(&gExtensionSectionIndexingState) != GPBExtensionSectionIndexingStateNotStarted
             ? YES
             : NO;
}

// Returns the extension section of the image, or NULL if the image has no
// (non-empty) section. `entryCount` receives the number of entries.
static GPBExtensionSectionEntry *GPBExtensionSectionOfImage(const struct mach_header *header,
                                                            size_t *entryCount) {
  unsigned long sectionSize = 0;
  uint8_t *sectionData =
      getsectiondata((const GPBMachHeader *)header, GPB_EXTENSION_SECTION_SEGMENT,
                     GPB_EXTENSION_SECTION_NAME, &sectionSize);
  if (sectionData == NULL || sectionSize < sizeof(GPBExtensionSectionEntry)) {
    *entryCount = 0;
    return NULL;
  }
  *entryCount = sectionSize / sizeof(GPBExtensionSectionEntry);
  return (GPBExtensionSectionEntry *)sectionData;
}

// Returns the extended class of a section entry, or Nil for a malformed entry.
static Class GPBExtendedClassOfSectionEntry(GPBExtensionSectionEntry *entry) {
  // Every generated GPBExtensionSectionEntry has a non-NULL description and
  // non-Nil extendedClass.clazz; assert in debug builds and guard defensively
  // if assertions are disabled.
  GPBExtensionDescription *description = entry->description;
  NSCAssert(description != NULL, @"Internal error: section entry missing description.");
  if (description == NULL) {
    return Nil;
  }
  Class extendedClass = description->extendedClass.clazz;
  NSCAssert(extendedClass != Nil, @"Internal error: extension %s missing extendedClass.",
            description->singletonName ? description->singletonName : "<unknown>");
  return extendedClass;
}

// Builds the index of the section entries of one image. Returns NULL if no
// entry is valid. Only reads the static entries and takes no lock.
static GPBExtensionSectionImageIndex *GPBCreateExtensionSectionImageIndex(
    GPBExtensionSectionEntry *entries, size_t entryCount) {
  CFMutableDictionaryRef classMap =
      CFDictionaryCreateMutable(kCFAllocatorDefault, 0, NULL, &kCFTypeDictionaryValueCallBacks);
  for (size_t entryIndex = 0; entryIndex < entryCount; ++entryIndex) {
    GPBExtensionSectionEntry *entry = &entries[entryIndex];
    Class extendedClass = GPBExtendedClassOfSectionEntry(entry);
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
    ssize_t key = entry->description->fieldNumber;
    // The index is per image, so this only catches two entries in the same
    // image. The same generated extension can't be linked twice into one image
    // (duplicate symbol), so this means two different extensions use the same
    // field number on the same message. Entries of the same extension in
    // different images are handled at lookup time (first loaded image wins).
    NSCAssert(CFDictionaryGetValue(fieldMap, (const void *)key) == NULL,
              @"Duplicate extension section entry for %@ field %td.",
              NSStringFromClass(extendedClass), key);
    CFDictionarySetValue(fieldMap, (const void *)key, entry);
  }
  // Not expected: a non-empty section only ends up with an empty map if every
  // entry is malformed (asserted above in debug builds). Guards release builds
  // against publishing an empty index.
  if (CFDictionaryGetCount(classMap) == 0) {
    CFRelease(classMap);
    return NULL;
  }

  GPBExtensionSectionImageIndex *imageIndex = malloc(sizeof(GPBExtensionSectionImageIndex));
  if (imageIndex == NULL) {
    abort();
  }
  imageIndex->classMap = classMap;
  atomic_init(&imageIndex->next, NULL);
  return imageIndex;
}

// Appends a fully built index at the tail of `list`, so that earlier loaded
// images stay ahead of later ones. In practice dyld invokes add-image callbacks
// one at a time, so writers do not race and only readers run concurrently.
// That serialization is not documented, so the append claims the NULL `next`
// link of the last index with a compare-and-swap (uncontended, so it succeeds
// on the first try) rather than relying on it. The list is short (one index
// per image with extensions), so walking it on each append is cheap.
static void GPBAppendExtensionSectionImageIndex(GPBExtensionSectionIndexList *list,
                                                GPBExtensionSectionImageIndex *imageIndex) {
  _Atomic(GPBExtensionSectionImageIndex *) *link = &list->head;
  while (true) {
    GPBExtensionSectionImageIndex *next = atomic_load_explicit(link, memory_order_acquire);
    if (next != NULL) {
      link = &next->next;
      continue;
    }
    if (atomic_compare_exchange_strong_explicit(link, &next, imageIndex, memory_order_release,
                                                memory_order_acquire)) {
      return;
    }
    // Another append claimed `link` first; `next` now holds its index.
    link = &next->next;
  }
}

// Looks up (extended class, field number) in the published image indexes of
// `list`, oldest image first, so the first loaded image wins.
static GPBExtensionSectionEntry *GPBFindExtensionSectionEntryInIndexList(
    GPBExtensionSectionIndexList *list, Class messageClass, ssize_t fieldNumber) {
  for (GPBExtensionSectionImageIndex *imageIndex =
           atomic_load_explicit(&list->head, memory_order_acquire);
       imageIndex != NULL;
       imageIndex = atomic_load_explicit(&imageIndex->next, memory_order_acquire)) {
    CFDictionaryRef fieldMap = CFDictionaryGetValue(imageIndex->classMap, messageClass);
    if (fieldMap == NULL) {
      continue;
    }
    GPBExtensionSectionEntry *entry =
        (GPBExtensionSectionEntry *)CFDictionaryGetValue(fieldMap, (const void *)fieldNumber);
    if (entry != NULL) {
      return entry;
    }
  }
  return NULL;
}

// dyld add-image callback. Called synchronously for every already loaded image
// from within _dyld_register_func_for_add_image(), and later for every newly
// loaded image (on the thread loading it, before the image's initializers have
// run). Only reads the static section data and takes no lock, so it is safe to
// run while dyld holds its own locks.
static void GPBIndexExtensionSectionOfImage(const struct mach_header *header,
                                            __unused intptr_t vmaddrSlide) {
  size_t entryCount = 0;
  GPBExtensionSectionEntry *entries = GPBExtensionSectionOfImage(header, &entryCount);
  if (entries == NULL) {
    return;
  }
  GPBExtensionSectionImageIndex *imageIndex =
      GPBCreateExtensionSectionImageIndex(entries, entryCount);
  if (imageIndex != NULL) {
    GPBAppendExtensionSectionImageIndex(&gExtensionSectionIndexList, imageIndex);
  }
}

#pragma mark - Test hooks (declared in GPBDescriptor_PackagePrivate.h)

GPBExtensionSectionIndexList *GPBExtensionSectionIndexListCreateForTesting(void) {
  GPBExtensionSectionIndexList *list = calloc(1, sizeof(GPBExtensionSectionIndexList));
  if (list == NULL) {
    abort();
  }
  atomic_init(&list->head, NULL);
  return list;
}

void GPBExtensionSectionIndexListAppendForTesting(GPBExtensionSectionIndexList *list,
                                                  GPBExtensionSectionEntry *entries,
                                                  size_t entryCount) {
  GPBExtensionSectionImageIndex *imageIndex =
      GPBCreateExtensionSectionImageIndex(entries, entryCount);
  if (imageIndex != NULL) {
    GPBAppendExtensionSectionImageIndex(list, imageIndex);
  }
}

GPBExtensionSectionEntry *GPBExtensionSectionIndexListFindForTesting(
    GPBExtensionSectionIndexList *list, Class extendedClass, ssize_t fieldNumber) {
  return GPBFindExtensionSectionEntryInIndexList(list, extendedClass, fieldNumber);
}

size_t GPBExtensionSectionIndexListCountForTesting(GPBExtensionSectionIndexList *list) {
  size_t count = 0;
  for (GPBExtensionSectionImageIndex *imageIndex =
           atomic_load_explicit(&list->head, memory_order_acquire);
       imageIndex != NULL;
       imageIndex = atomic_load_explicit(&imageIndex->next, memory_order_acquire)) {
    ++count;
  }
  return count;
}

void GPBExtensionSectionIndexListDestroyForTesting(GPBExtensionSectionIndexList *list) {
  GPBExtensionSectionImageIndex *imageIndex =
      atomic_load_explicit(&list->head, memory_order_acquire);
  while (imageIndex != NULL) {
    GPBExtensionSectionImageIndex *next =
        atomic_load_explicit(&imageIndex->next, memory_order_acquire);
    CFRelease(imageIndex->classMap);
    free(imageIndex);
    imageIndex = next;
  }
  free(list);
}

// Starts indexing the extension sections the first time it is called, and
// returns the current GPBExtensionSectionIndexingState.
//
// Why non-winning threads cannot simply spin/wait for registration to finish
// (AB-BA deadlock with dlopen()):
// - Thread A calls GPBStartIndexingExtensionSections(), wins the CAS below,
//   and calls _dyld_register_func_for_add_image(), which internally acquires
//   dyld's global API/image lock before invoking the callback for already
//   loaded images.
// - Concurrently, Thread B is inside dlopen() — which already holds dyld's
//   lock while running the newly loaded image's +load / constructor
//   initializers — and that initializer calls -extensionForDescriptor:fieldNumber:
//   on the global registry.
// - If Thread B spun or blocked waiting for Thread A to finish
//   _dyld_register_func_for_add_image(), Thread A would be blocked waiting for
//   Thread B's dyld lock while Thread B spun waiting for Thread A.
// Instead, Thread A registers the callback without holding any lock of this
// file, and if Thread B arrives while registration is in progress
// (GPBExtensionSectionIndexingStateRegistering), Thread B scans the currently
// loaded images directly via GPBFindExtensionSectionEntryInLoadedImages()
// without waiting for Thread A.
static GPBExtensionSectionIndexingState GPBStartIndexingExtensionSections(void) {
  int state = atomic_load_explicit(&gExtensionSectionIndexingState, memory_order_acquire);
  if (state != GPBExtensionSectionIndexingStateNotStarted) {
    return (GPBExtensionSectionIndexingState)state;
  }
  int expected = GPBExtensionSectionIndexingStateNotStarted;
  if (!atomic_compare_exchange_strong_explicit(&gExtensionSectionIndexingState, &expected,
                                               GPBExtensionSectionIndexingStateRegistering,
                                               memory_order_acq_rel, memory_order_acquire)) {
    // Another thread started the registration (and may have finished it).
    return (GPBExtensionSectionIndexingState)expected;
  }
  // Per <mach-o/dyld.h>, _dyld_register_func_for_add_image() synchronously
  // invokes the callback on the calling thread for every Mach-O image
  // currently loaded in the process before returning, and subsequently
  // invokes it as any new images are dynamically loaded (e.g. via dlopen()).
  _dyld_register_func_for_add_image(&GPBIndexExtensionSectionOfImage);
  atomic_store_explicit(&gExtensionSectionIndexingState, GPBExtensionSectionIndexingStateIndexed,
                        memory_order_release);
  return GPBExtensionSectionIndexingStateIndexed;
}

// Looks up (extended class, field number) in the global registry's published
// image indexes, oldest image first, so the first loaded image wins.
static GPBExtensionSectionEntry *GPBFindExtensionSectionEntryInIndexes(Class messageClass,
                                                                       ssize_t fieldNumber) {
  return GPBFindExtensionSectionEntryInIndexList(&gExtensionSectionIndexList, messageClass,
                                                 fieldNumber);
}

// Fallback lookup that scans the extension section of each loaded image
// directly when another thread is currently inside
// _dyld_register_func_for_add_image()
// (GPBExtensionSectionIndexingStateRegistering). Spinning until that thread
// finishes would deadlock if the current thread is inside a dlopen() image
// initializer that holds dyld's lock; see GPBStartIndexingExtensionSections().
// Images are scanned in dyld's image order (load order), so, like the indexed
// lookup, the first loaded image wins.
//
// Despite the leading underscores, _dyld_image_count(),
// _dyld_get_image_header() and _dyld_register_func_for_add_image() are public,
// non-deprecated API declared in <mach-o/dyld.h>. The image iteration is not
// thread safe (another thread can add an image during the scan), so a NULL
// header is skipped. objc_addLoadImageFunc() (<objc/runtime.h>) is not used:
// it calls back with the ObjC runtime's lock held, so indexing could not touch
// Objective-C at all (e.g. the NSCAssert()/NSStringFromClass() diagnostics),
// and it is unavailable on the oldest tvOS the runtime supports (tvOS 13+).
// dladdr()-based lookups are not an option either, as they need an address
// inside each image.
//
// Unlike the dyld callbacks, this scan runs without dyld's lock on threads
// other than one inside dlopen(). It assumes no scanned image is unmapped
// during the scan: dyld never unloads images with Objective-C metadata (every
// image with an extension section has some) or images in the shared cache, so
// the remaining exposure is a dlclose() that really unloads a dylib with
// neither, concurrently with a lookup during the short window while another
// thread is still inside _dyld_register_func_for_add_image().
static GPBExtensionSectionEntry *GPBFindExtensionSectionEntryInLoadedImages(Class messageClass,
                                                                            ssize_t fieldNumber) {
  uint32_t imageCount = _dyld_image_count();
  for (uint32_t imageIndex = 0; imageIndex < imageCount; ++imageIndex) {
    const struct mach_header *header = _dyld_get_image_header(imageIndex);
    if (header == NULL) {
      // The image list changed during the scan.
      continue;
    }
    size_t entryCount = 0;
    GPBExtensionSectionEntry *entries = GPBExtensionSectionOfImage(header, &entryCount);
    for (size_t entryIndex = 0; entryIndex < entryCount; ++entryIndex) {
      GPBExtensionSectionEntry *entry = &entries[entryIndex];
      if (GPBExtendedClassOfSectionEntry(entry) == messageClass &&
          entry->description->fieldNumber == fieldNumber) {
        return entry;
      }
    }
  }
  return NULL;
}

// The immutable registry returned by +[GPBExtensionRegistry globalExtensionRegistry].
//
// Synchronization: lookups take no lock and never wait for another thread. The
// thread that does the first lookup indexes the already loaded images; lookups
// that run while it does so search the loaded images directly instead of
// waiting (see GPBStartIndexingExtensionSections()). Images loaded later are
// indexed by the dyld callback on the loading thread and published with a
// release compare-and-swap; a lookup that runs at the same time as an image
// load may not see that image's extensions yet. Descriptor creation goes
// through GPBResolveExtensionSectionEntry() (atomic compare-and-swap), which
// the generated extension functions also use.
__attribute__((objc_subclassing_restricted))
@interface GPBGlobalSectionExtensionRegistry : NSObject<GPBExtensionRegistry>
@end

@implementation GPBGlobalSectionExtensionRegistry

- (GPBExtensionDescriptor *)extensionForDescriptor:(GPBDescriptor *)descriptor
                                       fieldNumber:(NSInteger)fieldNumber {
  GPBExtensionSectionIndexingState state = GPBStartIndexingExtensionSections();
  Class messageClass = descriptor.messageClass;
  ssize_t key = fieldNumber;
  GPBExtensionSectionEntry *entry =
      (state == GPBExtensionSectionIndexingStateIndexed)
          ? GPBFindExtensionSectionEntryInIndexes(messageClass, key)
          : GPBFindExtensionSectionEntryInLoadedImages(messageClass, key);
  if (entry == NULL) {
    return nil;
  }
  return GPBResolveExtensionSectionEntryForGlobalRegistry(entry);
}

@end

#pragma mark - GPBExtensionRegistry

@implementation GPBExtensionRegistry {
  CFMutableDictionaryRef mutableClassMap_;
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
