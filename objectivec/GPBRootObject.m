// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#import "GPBRootObject.h"
#import "GPBRootObject_PackagePrivate.h"

#import <CoreFoundation/CoreFoundation.h>
#import <objc/runtime.h>
#import <os/lock.h>

#import "GPBDescriptor.h"
#import "GPBExtensionRegistry.h"
#import "GPBUtilities.h"
#import "GPBUtilities_PackagePrivate.h"

@interface GPBExtensionDescriptor (GPBRootObject)
// Get singletonName as a c string.
- (const char *)singletonNameC;
@end

// We need some object to conform to the MessageSignatureProtocol to make sure
// the selectors in it are recorded in our Objective C runtime information.
// GPBMessage is arguably the more "obvious" choice, but given that all messages
// inherit from GPBMessage, conflicts seem likely, so we are using GPBRootObject
// instead.
@interface GPBRootObject () <GPBMessageSignatureProtocol>
@end

@implementation GPBRootObject

// Taken from http://www.burtleburtle.net/bob/hash/doobs.html
// Public Domain
static uint32_t jenkins_one_at_a_time_hash(const char *key) {
  uint32_t hash = 0;
  for (uint32_t i = 0; key[i] != '\0'; ++i) {
    hash += key[i];
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }
  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);
  return hash;
}

// Key methods for our custom CFDictionary.
// Note that the dictionary lasts for the lifetime of our app, so no need
// to worry about deallocation. All of the items are added to it at
// startup, and so the keys don't need to be retained/released.
// Keys are NULL terminated char *.
static const void *GPBRootExtensionKeyRetain(__unused CFAllocatorRef allocator, const void *value) {
  return value;
}

static void GPBRootExtensionKeyRelease(__unused CFAllocatorRef allocator,
                                       __unused const void *value) {}

static CFStringRef GPBRootExtensionCopyKeyDescription(const void *value) {
  const char *key = (const char *)value;
  return CFStringCreateWithCString(kCFAllocatorDefault, key, kCFStringEncodingUTF8);
}

static Boolean GPBRootExtensionKeyEqual(const void *value1, const void *value2) {
  const char *key1 = (const char *)value1;
  const char *key2 = (const char *)value2;
  return strcmp(key1, key2) == 0;
}

static CFHashCode GPBRootExtensionKeyHash(const void *value) {
  const char *key = (const char *)value;
  return jenkins_one_at_a_time_hash(key);
}

// Long ago, this was an OSSpinLock, but then it came to light that there were issues for that on
// iOS:
//   http://mjtsai.com/blog/2015/12/16/osspinlock-is-unsafe/
//   https://lists.swift.org/pipermail/swift-dev/Week-of-Mon-20151214/000372.html
// It was changed to a dispatch_semaphore_t, but that has potential for priority inversion issues.
// The minOS versions are now high enough that os_unfair_lock can be used, and should provide
// all the support we need. For more information in the concurrency/locking space see:
//   https://gist.github.com/tclementdev/6af616354912b0347cdf6db159c37057
//   https://developer.apple.com/library/archive/documentation/Performance/Conceptual/EnergyGuide-iOS/PrioritizeWorkWithQoS.html
//   https://developer.apple.com/videos/play/wwdc2017/706/
static os_unfair_lock gExtensionSingletonDictionaryLock = OS_UNFAIR_LOCK_INIT;
static CFMutableDictionaryRef gExtensionSingletonDictionary = NULL;
static GPBExtensionRegistry *gDefaultExtensionRegistry = NULL;

+ (void)initialize {
  // Ensure the global is started up.
  if (!gExtensionSingletonDictionary) {
    CFDictionaryKeyCallBacks keyCallBacks = {
        // See description above for reason for using custom dictionary.
        0,
        GPBRootExtensionKeyRetain,
        GPBRootExtensionKeyRelease,
        GPBRootExtensionCopyKeyDescription,
        GPBRootExtensionKeyEqual,
        GPBRootExtensionKeyHash,
    };
    gExtensionSingletonDictionary = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &keyCallBacks,
                                                              &kCFTypeDictionaryValueCallBacks);
    gDefaultExtensionRegistry = [[GPBExtensionRegistry alloc] init];
  }

  if ([self superclass] == [GPBRootObject class]) {
    // This is here to start up all the per file "Root" subclasses.
    // This must be done in initialize to enforce thread safety of start up of
    // the protocol buffer library.
    [self extensionRegistry];
  }
}

+ (GPBExtensionRegistry *)extensionRegistry {
  // Is overridden in all the subclasses that provide extensions to provide the
  // per class one.
  return gDefaultExtensionRegistry;
}

+ (void)globallyRegisterExtension:(GPBExtensionDescriptor *)field {
  const char *key = [field singletonNameC];
  os_unfair_lock_lock(&gExtensionSingletonDictionaryLock);
  CFDictionarySetValue(gExtensionSingletonDictionary, key, field);
  os_unfair_lock_unlock(&gExtensionSingletonDictionaryLock);
}

static id ExtensionForName(id self, SEL _cmd) {
  // Really fast way of doing "classname_selName".
  // This came up as a hotspot (creation of NSString *) when accessing a
  // lot of extensions.
  const char *selName = sel_getName(_cmd);
  if (selName[0] == '_') {
    return nil;  // Apple internal selector.
  }
  size_t selNameLen = 0;
  while (1) {
    char c = selName[selNameLen];
    if (c == '\0') {  // String end.
      break;
    }
    if (c == ':') {
      return nil;  // Selector took an arg, not one of the runtime methods.
    }
    ++selNameLen;
  }

  const char *className = class_getName(self);
  size_t classNameLen = strlen(className);
  char key[classNameLen + selNameLen + 2];
  memcpy(key, className, classNameLen);
  key[classNameLen] = '_';
  memcpy(&key[classNameLen + 1], selName, selNameLen);
  key[classNameLen + 1 + selNameLen] = '\0';

  // NOTE: Even though this method is called from another C function,
  // gExtensionSingletonDictionaryLock and gExtensionSingletonDictionary
  // will always be initialized. This is because this call flow is just to
  // lookup the Extension, meaning the code is calling an Extension class
  // message on a Message or Root class. This guarantees that the class was
  // initialized and Message classes ensure their Root was also initialized.
  NSAssert(gExtensionSingletonDictionary, @"Startup order broken!");

  os_unfair_lock_lock(&gExtensionSingletonDictionaryLock);
  id extension = (id)CFDictionaryGetValue(gExtensionSingletonDictionary, key);
  // We can't remove the key from the dictionary here (as an optimization),
  // two threads could have gone into +resolveClassMethod: for the same method,
  // and ended up here; there's no way to ensure both return YES without letting
  // both try to wire in the method.
  os_unfair_lock_unlock(&gExtensionSingletonDictionaryLock);
  return extension;
}

BOOL GPBResolveExtensionClassMethod(Class self, SEL sel) {
  // Another option would be to register the extensions with the class at
  // globallyRegisterExtension:
  // Timing the two solutions, this solution turned out to be much faster
  // and reduced startup time, and runtime memory.
  // The advantage to globallyRegisterExtension is that it would reduce the
  // size of the protos somewhat because the singletonNameC wouldn't need
  // to include the class name. For a class with a lot of extensions it
  // can add up. You could also significantly reduce the code complexity of this
  // file.
  id extension = ExtensionForName(self, sel);
  if (extension != nil) {
    const char *encoding = GPBMessageEncodingForSelector(@selector(getClassValue), NO);
    Class metaClass = objc_getMetaClass(class_getName(self));
    IMP imp = imp_implementationWithBlock(^(__unused id obj) {
      return extension;
    });
    BOOL methodAdded = class_addMethod(metaClass, sel, imp, encoding);
    // class_addMethod() is documented as also failing if the method was already
    // added; so we check if the method is already there and return success so
    // the method dispatch will still happen.  Why would it already be added?
    // Two threads could cause the same method to be bound at the same time,
    // but only one will actually bind it; the other still needs to return true
    // so things will dispatch.
    if (!methodAdded) {
      methodAdded = GPBClassHasSel(metaClass, sel);
    }
    return methodAdded;
  }
  return NO;
}

+ (BOOL)resolveClassMethod:(SEL)sel {
  if (GPBResolveExtensionClassMethod(self, sel)) {
    return YES;
  }
  return [super resolveClassMethod:sel];
}

@end
