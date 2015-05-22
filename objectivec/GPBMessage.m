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

#import "GPBMessage_PackagePrivate.h"

#import <objc/runtime.h>
#import <objc/message.h>

#import "GPBArray_PackagePrivate.h"
#import "GPBCodedInputStream_PackagePrivate.h"
#import "GPBCodedOutputStream.h"
#import "GPBDescriptor_PackagePrivate.h"
#import "GPBDictionary_PackagePrivate.h"
#import "GPBExtensionField_PackagePrivate.h"
#import "GPBExtensionRegistry.h"
#import "GPBRootObject_PackagePrivate.h"
#import "GPBUnknownFieldSet_PackagePrivate.h"
#import "GPBUtilities_PackagePrivate.h"

NSString *const GPBMessageErrorDomain =
    GPBNSStringifySymbol(GPBMessageErrorDomain);

#ifdef DEBUG
NSString *const GPBExceptionMessageKey =
    GPBNSStringifySymbol(GPBExceptionMessage);
#endif  // DEBUG

static NSString *const kGPBDataCoderKey = @"GPBData";

//
// PLEASE REMEMBER:
//
// This is the base class for *all* messages generated, so any selector defined,
// *public* or *private* could end up colliding with a proto message field. So
// avoid using selectors that could match a property, use C functions to hide
// them, etc.
//

@interface GPBMessage () {
 @package
  GPBUnknownFieldSet *unknownFields_;
  NSMutableDictionary *extensionMap_;
  NSMutableDictionary *autocreatedExtensionMap_;

  // If the object was autocreated, we remember the creator so that if we get
  // mutated, we can inform the creator to make our field visible.
  GPBMessage *autocreator_;
  GPBFieldDescriptor *autocreatorField_;
  GPBExtensionField *autocreatorExtension_;
}
@end

static id CreateArrayForField(GPBFieldDescriptor *field,
                              GPBMessage *autocreator)
    __attribute__((ns_returns_retained));
static id GetOrCreateArrayIvarWithField(GPBMessage *self,
                                        GPBFieldDescriptor *field,
                                        GPBFileSyntax syntax);
static id GetArrayIvarWithField(GPBMessage *self, GPBFieldDescriptor *field);
static id CreateMapForField(GPBFieldDescriptor *field,
                            GPBMessage *autocreator)
    __attribute__((ns_returns_retained));
static id GetOrCreateMapIvarWithField(GPBMessage *self,
                                      GPBFieldDescriptor *field,
                                      GPBFileSyntax syntax);
static id GetMapIvarWithField(GPBMessage *self, GPBFieldDescriptor *field);
static NSMutableDictionary *CloneExtensionMap(NSDictionary *extensionMap,
                                              NSZone *zone)
    __attribute__((ns_returns_retained));

static NSError *MessageError(NSInteger code, NSDictionary *userInfo) {
  return [NSError errorWithDomain:GPBMessageErrorDomain
                             code:code
                         userInfo:userInfo];
}

static NSError *MessageErrorWithReason(NSInteger code, NSString *reason) {
  NSDictionary *userInfo = nil;
  if ([reason length]) {
    userInfo = @{ @"Reason" : reason };
  }
  return MessageError(code, userInfo);
}


static void CheckExtension(GPBMessage *self, GPBExtensionField *extension) {
  if ([[self class] descriptor] != [extension containingType]) {
    [NSException
         raise:NSInvalidArgumentException
        format:@"Extension %@ used on wrong class (%@ instead of %@)",
               extension.descriptor.singletonName,
               [[self class] descriptor].name, [extension containingType].name];
  }
}

static NSMutableDictionary *CloneExtensionMap(NSDictionary *extensionMap,
                                              NSZone *zone) {
  if (extensionMap.count == 0) {
    return nil;
  }
  NSMutableDictionary *result = [[NSMutableDictionary allocWithZone:zone]
      initWithCapacity:extensionMap.count];

  for (GPBExtensionField *field in extensionMap) {
    id value = [extensionMap objectForKey:field];
    GPBExtensionDescriptor *fieldDescriptor = field.descriptor;
    BOOL isMessageExtension = GPBExtensionIsMessage(fieldDescriptor);

    if ([field isRepeated]) {
      if (isMessageExtension) {
        NSMutableArray *list =
            [[NSMutableArray alloc] initWithCapacity:[value count]];
        for (GPBMessage *listValue in value) {
          GPBMessage *copiedValue = [listValue copyWithZone:zone];
          [list addObject:copiedValue];
          [copiedValue release];
        }
        [result setObject:list forKey:field];
        [list release];
      } else {
        NSMutableArray *copiedValue = [value mutableCopyWithZone:zone];
        [result setObject:copiedValue forKey:field];
        [copiedValue release];
      }
    } else {
      if (isMessageExtension) {
        GPBMessage *copiedValue = [value copyWithZone:zone];
        [result setObject:copiedValue forKey:field];
        [copiedValue release];
      } else {
        [result setObject:value forKey:field];
      }
    }
  }

  return result;
}

static id CreateArrayForField(GPBFieldDescriptor *field,
                              GPBMessage *autocreator) {
  id result;
  GPBType fieldDataType = GPBGetFieldType(field);
  switch (fieldDataType) {
    case GPBTypeBool:
      result = [[GPBBoolArray alloc] init];
      break;
    case GPBTypeFixed32:
    case GPBTypeUInt32:
      result = [[GPBUInt32Array alloc] init];
      break;
    case GPBTypeInt32:
    case GPBTypeSFixed32:
    case GPBTypeSInt32:
      result = [[GPBInt32Array alloc] init];
      break;
    case GPBTypeFixed64:
    case GPBTypeUInt64:
      result = [[GPBUInt64Array alloc] init];
      break;
    case GPBTypeInt64:
    case GPBTypeSFixed64:
    case GPBTypeSInt64:
      result = [[GPBInt64Array alloc] init];
      break;
    case GPBTypeFloat:
      result = [[GPBFloatArray alloc] init];
      break;
    case GPBTypeDouble:
      result = [[GPBDoubleArray alloc] init];
      break;

    case GPBTypeEnum:
      result = [[GPBEnumArray alloc]
                  initWithValidationFunction:field.enumDescriptor.enumVerifier];
      break;

    case GPBTypeData:
    case GPBTypeGroup:
    case GPBTypeMessage:
    case GPBTypeString:
      if (autocreator) {
        result = [[GPBAutocreatedArray alloc] init];
      } else {
        result = [[NSMutableArray alloc] init];
      }
      break;
  }

  if (autocreator) {
    if (GPBTypeIsObject(fieldDataType)) {
      GPBAutocreatedArray *autoArray = result;
      autoArray->_autocreator =  autocreator;
    } else {
      GPBInt32Array *gpbArray = result;
      gpbArray->_autocreator = autocreator;
    }
  }

  return result;
}

static id CreateMapForField(GPBFieldDescriptor *field,
                            GPBMessage *autocreator) {
  id result;
  GPBType keyType = field.mapKeyType;
  GPBType valueType = GPBGetFieldType(field);
  switch (keyType) {
    case GPBTypeBool:
      switch (valueType) {
        case GPBTypeBool:
          result = [[GPBBoolBoolDictionary alloc] init];
          break;
        case GPBTypeFixed32:
        case GPBTypeUInt32:
          result = [[GPBBoolUInt32Dictionary alloc] init];
          break;
        case GPBTypeInt32:
        case GPBTypeSFixed32:
        case GPBTypeSInt32:
          result = [[GPBBoolInt32Dictionary alloc] init];
          break;
        case GPBTypeFixed64:
        case GPBTypeUInt64:
          result = [[GPBBoolUInt64Dictionary alloc] init];
          break;
        case GPBTypeInt64:
        case GPBTypeSFixed64:
        case GPBTypeSInt64:
          result = [[GPBBoolInt64Dictionary alloc] init];
          break;
        case GPBTypeFloat:
          result = [[GPBBoolFloatDictionary alloc] init];
          break;
        case GPBTypeDouble:
          result = [[GPBBoolDoubleDictionary alloc] init];
          break;
        case GPBTypeEnum:
          result = [[GPBBoolEnumDictionary alloc]
              initWithValidationFunction:field.enumDescriptor.enumVerifier];
          break;
        case GPBTypeData:
        case GPBTypeMessage:
        case GPBTypeString:
          result = [[GPBBoolObjectDictionary alloc] init];
          break;
        case GPBTypeGroup:
          NSCAssert(NO, @"shouldn't happen");
          return nil;
      }
      break;
    case GPBTypeFixed32:
    case GPBTypeUInt32:
      switch (valueType) {
        case GPBTypeBool:
          result = [[GPBUInt32BoolDictionary alloc] init];
          break;
        case GPBTypeFixed32:
        case GPBTypeUInt32:
          result = [[GPBUInt32UInt32Dictionary alloc] init];
          break;
        case GPBTypeInt32:
        case GPBTypeSFixed32:
        case GPBTypeSInt32:
          result = [[GPBUInt32Int32Dictionary alloc] init];
          break;
        case GPBTypeFixed64:
        case GPBTypeUInt64:
          result = [[GPBUInt32UInt64Dictionary alloc] init];
          break;
        case GPBTypeInt64:
        case GPBTypeSFixed64:
        case GPBTypeSInt64:
          result = [[GPBUInt32Int64Dictionary alloc] init];
          break;
        case GPBTypeFloat:
          result = [[GPBUInt32FloatDictionary alloc] init];
          break;
        case GPBTypeDouble:
          result = [[GPBUInt32DoubleDictionary alloc] init];
          break;
        case GPBTypeEnum:
          result = [[GPBUInt32EnumDictionary alloc]
              initWithValidationFunction:field.enumDescriptor.enumVerifier];
          break;
        case GPBTypeData:
        case GPBTypeMessage:
        case GPBTypeString:
          result = [[GPBUInt32ObjectDictionary alloc] init];
          break;
        case GPBTypeGroup:
          NSCAssert(NO, @"shouldn't happen");
          return nil;
      }
      break;
    case GPBTypeInt32:
    case GPBTypeSFixed32:
    case GPBTypeSInt32:
      switch (valueType) {
        case GPBTypeBool:
          result = [[GPBInt32BoolDictionary alloc] init];
          break;
        case GPBTypeFixed32:
        case GPBTypeUInt32:
          result = [[GPBInt32UInt32Dictionary alloc] init];
          break;
        case GPBTypeInt32:
        case GPBTypeSFixed32:
        case GPBTypeSInt32:
          result = [[GPBInt32Int32Dictionary alloc] init];
          break;
        case GPBTypeFixed64:
        case GPBTypeUInt64:
          result = [[GPBInt32UInt64Dictionary alloc] init];
          break;
        case GPBTypeInt64:
        case GPBTypeSFixed64:
        case GPBTypeSInt64:
          result = [[GPBInt32Int64Dictionary alloc] init];
          break;
        case GPBTypeFloat:
          result = [[GPBInt32FloatDictionary alloc] init];
          break;
        case GPBTypeDouble:
          result = [[GPBInt32DoubleDictionary alloc] init];
          break;
        case GPBTypeEnum:
          result = [[GPBInt32EnumDictionary alloc]
              initWithValidationFunction:field.enumDescriptor.enumVerifier];
          break;
        case GPBTypeData:
        case GPBTypeMessage:
        case GPBTypeString:
          result = [[GPBInt32ObjectDictionary alloc] init];
          break;
        case GPBTypeGroup:
          NSCAssert(NO, @"shouldn't happen");
          return nil;
      }
      break;
    case GPBTypeFixed64:
    case GPBTypeUInt64:
      switch (valueType) {
        case GPBTypeBool:
          result = [[GPBUInt64BoolDictionary alloc] init];
          break;
        case GPBTypeFixed32:
        case GPBTypeUInt32:
          result = [[GPBUInt64UInt32Dictionary alloc] init];
          break;
        case GPBTypeInt32:
        case GPBTypeSFixed32:
        case GPBTypeSInt32:
          result = [[GPBUInt64Int32Dictionary alloc] init];
          break;
        case GPBTypeFixed64:
        case GPBTypeUInt64:
          result = [[GPBUInt64UInt64Dictionary alloc] init];
          break;
        case GPBTypeInt64:
        case GPBTypeSFixed64:
        case GPBTypeSInt64:
          result = [[GPBUInt64Int64Dictionary alloc] init];
          break;
        case GPBTypeFloat:
          result = [[GPBUInt64FloatDictionary alloc] init];
          break;
        case GPBTypeDouble:
          result = [[GPBUInt64DoubleDictionary alloc] init];
          break;
        case GPBTypeEnum:
          result = [[GPBUInt64EnumDictionary alloc]
              initWithValidationFunction:field.enumDescriptor.enumVerifier];
          break;
        case GPBTypeData:
        case GPBTypeMessage:
        case GPBTypeString:
          result = [[GPBUInt64ObjectDictionary alloc] init];
          break;
        case GPBTypeGroup:
          NSCAssert(NO, @"shouldn't happen");
          return nil;
      }
      break;
    case GPBTypeInt64:
    case GPBTypeSFixed64:
    case GPBTypeSInt64:
      switch (valueType) {
        case GPBTypeBool:
          result = [[GPBInt64BoolDictionary alloc] init];
          break;
        case GPBTypeFixed32:
        case GPBTypeUInt32:
          result = [[GPBInt64UInt32Dictionary alloc] init];
          break;
        case GPBTypeInt32:
        case GPBTypeSFixed32:
        case GPBTypeSInt32:
          result = [[GPBInt64Int32Dictionary alloc] init];
          break;
        case GPBTypeFixed64:
        case GPBTypeUInt64:
          result = [[GPBInt64UInt64Dictionary alloc] init];
          break;
        case GPBTypeInt64:
        case GPBTypeSFixed64:
        case GPBTypeSInt64:
          result = [[GPBInt64Int64Dictionary alloc] init];
          break;
        case GPBTypeFloat:
          result = [[GPBInt64FloatDictionary alloc] init];
          break;
        case GPBTypeDouble:
          result = [[GPBInt64DoubleDictionary alloc] init];
          break;
        case GPBTypeEnum:
          result = [[GPBInt64EnumDictionary alloc]
              initWithValidationFunction:field.enumDescriptor.enumVerifier];
          break;
        case GPBTypeData:
        case GPBTypeMessage:
        case GPBTypeString:
          result = [[GPBInt64ObjectDictionary alloc] init];
          break;
        case GPBTypeGroup:
          NSCAssert(NO, @"shouldn't happen");
          return nil;
      }
      break;
    case GPBTypeString:
      switch (valueType) {
        case GPBTypeBool:
          result = [[GPBStringBoolDictionary alloc] init];
          break;
        case GPBTypeFixed32:
        case GPBTypeUInt32:
          result = [[GPBStringUInt32Dictionary alloc] init];
          break;
        case GPBTypeInt32:
        case GPBTypeSFixed32:
        case GPBTypeSInt32:
          result = [[GPBStringInt32Dictionary alloc] init];
          break;
        case GPBTypeFixed64:
        case GPBTypeUInt64:
          result = [[GPBStringUInt64Dictionary alloc] init];
          break;
        case GPBTypeInt64:
        case GPBTypeSFixed64:
        case GPBTypeSInt64:
          result = [[GPBStringInt64Dictionary alloc] init];
          break;
        case GPBTypeFloat:
          result = [[GPBStringFloatDictionary alloc] init];
          break;
        case GPBTypeDouble:
          result = [[GPBStringDoubleDictionary alloc] init];
          break;
        case GPBTypeEnum:
          result = [[GPBStringEnumDictionary alloc]
              initWithValidationFunction:field.enumDescriptor.enumVerifier];
          break;
        case GPBTypeData:
        case GPBTypeMessage:
        case GPBTypeString:
          if (autocreator) {
            result = [[GPBAutocreatedDictionary alloc] init];
          } else {
            result = [[NSMutableDictionary alloc] init];
          }
          break;
        case GPBTypeGroup:
          NSCAssert(NO, @"shouldn't happen");
          return nil;
      }
      break;

    case GPBTypeFloat:
    case GPBTypeDouble:
    case GPBTypeEnum:
    case GPBTypeData:
    case GPBTypeGroup:
    case GPBTypeMessage:
      NSCAssert(NO, @"shouldn't happen");
      return nil;
  }

  if (autocreator) {
    if ((keyType == GPBTypeString) && GPBTypeIsObject(valueType)) {
      GPBAutocreatedDictionary *autoDict = result;
      autoDict->_autocreator =  autocreator;
    } else {
      GPBInt32Int32Dictionary *gpbDict = result;
      gpbDict->_autocreator = autocreator;
    }
  }

  return result;
}

#if !defined(__clang_analyzer__)
// These functions are blocked from the analyzer because the analyzer sees the
// GPBSetRetainedObjectIvarWithFieldInternal() call as consuming the array/map,
// so use of the array/map after the call returns is flagged as a use after
// free.
// But GPBSetRetainedObjectIvarWithFieldInternal() is "consuming" the retain
// count be holding onto the object (it is transfering it), the object is
// still valid after returning from the call.  The other way to avoid this
// would be to add a -retain/-autorelease, but that would force every
// repeated/map field parsed into the autorelease pool which is both a memory
// and performance hit.

static id GetOrCreateArrayIvarWithField(GPBMessage *self,
                                        GPBFieldDescriptor *field,
                                        GPBFileSyntax syntax) {
  id array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
  if (!array) {
    // No lock needed, this is called from places expecting to mutate
    // so no threading protection is needed.
    array = CreateArrayForField(field, nil);
    GPBSetRetainedObjectIvarWithFieldInternal(self, field, array, syntax);
  }
  return array;
}

// This is like GPBGetObjectIvarWithField(), but for arrays, it should
// only be used to wire the method into the class.
static id GetArrayIvarWithField(GPBMessage *self, GPBFieldDescriptor *field) {
  id array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
  if (!array) {
    // Check again after getting the lock.
    OSSpinLockLock(&self->readOnlyMutex_);
    array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
    if (!array) {
      array = CreateArrayForField(field, self);
      GPBSetAutocreatedRetainedObjectIvarWithField(self, field, array);
    }
    OSSpinLockUnlock(&self->readOnlyMutex_);
  }
  return array;
}

static id GetOrCreateMapIvarWithField(GPBMessage *self,
                                      GPBFieldDescriptor *field,
                                      GPBFileSyntax syntax) {
  id dict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
  if (!dict) {
    // No lock needed, this is called from places expecting to mutate
    // so no threading protection is needed.
    dict = CreateMapForField(field, nil);
    GPBSetRetainedObjectIvarWithFieldInternal(self, field, dict, syntax);
  }
  return dict;
}

// This is like GPBGetObjectIvarWithField(), but for maps, it should
// only be used to wire the method into the class.
static id GetMapIvarWithField(GPBMessage *self, GPBFieldDescriptor *field) {
  id dict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
  if (!dict) {
    // Check again after getting the lock.
    OSSpinLockLock(&self->readOnlyMutex_);
    dict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
    if (!dict) {
      dict = CreateMapForField(field, self);
      GPBSetAutocreatedRetainedObjectIvarWithField(self, field, dict);
    }
    OSSpinLockUnlock(&self->readOnlyMutex_);
  }
  return dict;
}

#endif  // !defined(__clang_analyzer__)

GPBMessage *GPBCreateMessageWithAutocreator(Class msgClass,
                                            GPBMessage *autocreator,
                                            GPBFieldDescriptor *field) {
  GPBMessage *message = [[msgClass alloc] init];
  message->autocreator_ = autocreator;
  message->autocreatorField_ = [field retain];
  return message;
}

static GPBMessage *CreateMessageWithAutocreatorForExtension(
    Class msgClass, GPBMessage *autocreator, GPBExtensionField *extension)
    __attribute__((ns_returns_retained));

static GPBMessage *CreateMessageWithAutocreatorForExtension(
    Class msgClass, GPBMessage *autocreator, GPBExtensionField *extension) {
  GPBMessage *message = [[msgClass alloc] init];
  message->autocreator_ = autocreator;
  message->autocreatorExtension_ = [extension retain];
  return message;
}

BOOL GPBWasMessageAutocreatedBy(GPBMessage *message, GPBMessage *parent) {
  return (message->autocreator_ == parent);
}

void GPBBecomeVisibleToAutocreator(GPBMessage *self) {
  // Message objects that are implicitly created by accessing a message field
  // are initially not visible via the hasX selector. This method makes them
  // visible.
  if (self->autocreator_) {
    // This will recursively make all parent messages visible until it reaches a
    // super-creator that's visible.
    if (self->autocreatorField_) {
      GPBFileSyntax syntax = [self->autocreator_ descriptor].file.syntax;
      GPBSetObjectIvarWithFieldInternal(self->autocreator_,
                                        self->autocreatorField_, self, syntax);
    } else {
      [self->autocreator_ setExtension:self->autocreatorExtension_ value:self];
    }
  }
}

void GPBAutocreatedArrayModified(GPBMessage *self, id array) {
  // When one of our autocreated arrays adds elements, make it visible.
  GPBDescriptor *descriptor = [[self class] descriptor];
  for (GPBFieldDescriptor *field in descriptor->fields_) {
    if (field.fieldType == GPBFieldTypeRepeated) {
      id curArray = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
      if (curArray == array) {
        if (GPBFieldTypeIsObject(field)) {
          GPBAutocreatedArray *autoArray = array;
          autoArray->_autocreator = nil;
        } else {
          GPBInt32Array *gpbArray = array;
          gpbArray->_autocreator = nil;
        }
        GPBBecomeVisibleToAutocreator(self);
        return;
      }
    }
  }
  NSCAssert(NO, @"Unknown autocreated %@ for %@.", [array class], self);
}

void GPBAutocreatedDictionaryModified(GPBMessage *self, id dictionary) {
  // When one of our autocreated dicts adds elements, make it visible.
  GPBDescriptor *descriptor = [[self class] descriptor];
  for (GPBFieldDescriptor *field in descriptor->fields_) {
    if (field.fieldType == GPBFieldTypeMap) {
      id curDict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
      if (curDict == dictionary) {
        if ((field.mapKeyType == GPBTypeString) &&
            GPBFieldTypeIsObject(field)) {
          GPBAutocreatedDictionary *autoDict = dictionary;
          autoDict->_autocreator = nil;
        } else {
          GPBInt32Int32Dictionary *gpbDict = dictionary;
          gpbDict->_autocreator = nil;
        }
        GPBBecomeVisibleToAutocreator(self);
        return;
      }
    }
  }
  NSCAssert(NO, @"Unknown autocreated %@ for %@.", [dictionary class], self);
}

void GPBClearMessageAutocreator(GPBMessage *self) {
  if ((self == nil) || !self->autocreator_) {
    return;
  }

#if DEBUG && !defined(NS_BLOCK_ASSERTIONS)
  // Either the autocreator must have its "has" flag set to YES, or it must be
  // NO and not equal to ourselves.
  BOOL autocreatorHas =
      (self->autocreatorField_
           ? GPBGetHasIvarField(self->autocreator_, self->autocreatorField_)
           : [self->autocreator_ hasExtension:self->autocreatorExtension_]);
  GPBMessage *autocreatorFieldValue =
      (self->autocreatorField_
           ? GPBGetObjectIvarWithFieldNoAutocreate(self->autocreator_,
                                                   self->autocreatorField_)
           : [self->autocreator_->autocreatedExtensionMap_
                 objectForKey:self->autocreatorExtension_]);
  NSCAssert(autocreatorHas || autocreatorFieldValue != self,
            @"Cannot clear autocreator because it still refers to self, self: %@.",
            self);

#endif  // DEBUG && !defined(NS_BLOCK_ASSERTIONS)

  self->autocreator_ = nil;
  [self->autocreatorField_ release];
  self->autocreatorField_ = nil;
  [self->autocreatorExtension_ release];
  self->autocreatorExtension_ = nil;
}

static GPBUnknownFieldSet *GetOrMakeUnknownFields(GPBMessage *self) {
  if (!self->unknownFields_) {
    self->unknownFields_ = [[GPBUnknownFieldSet alloc] init];
    GPBBecomeVisibleToAutocreator(self);
  }
  return self->unknownFields_;
}

@implementation GPBMessage

+ (void)initialize {
  Class pbMessageClass = [GPBMessage class];
  if ([self class] == pbMessageClass) {
    // This is here to start up the "base" class descriptor.
    [self descriptor];
    // Message shares extension method resolving with GPBRootObject so insure
    // it is started up at the same time.
    (void)[GPBRootObject class];
  } else if ([self superclass] == pbMessageClass) {
    // This is here to start up all the "message" subclasses. Just needs to be
    // done for the messages, not any of the subclasses.
    // This must be done in initialize to enforce thread safety of start up of
    // the protocol buffer library.
    // Note: The generated code for -descriptor calls
    // +[GPBDescriptor allocDescriptorForClass:...], passing the GPBRootObject
    // subclass for the file.  That call chain is what ensures that *Root class
    // is started up to support extension resolution off the message class
    // (+resolveClassMethod: below) in a thread safe manner.
    [self descriptor];
  }
}

+ (instancetype)allocWithZone:(NSZone *)zone {
  // Override alloc to allocate our classes with the additional storage
  // required for the instance variables.
  GPBDescriptor *descriptor = [self descriptor];
  return NSAllocateObject(self, descriptor->storageSize_, zone);
}

+ (instancetype)alloc {
  return [self allocWithZone:nil];
}

+ (GPBDescriptor *)descriptor {
  // This is thread safe because it is called from +initialize.
  static GPBDescriptor *descriptor = NULL;
  static GPBFileDescriptor *fileDescriptor = NULL;
  if (!descriptor) {
    // Use a dummy file that marks it as proto2 syntax so when used generically
    // it supports unknowns/etc.
    fileDescriptor =
        [[GPBFileDescriptor alloc] initWithPackage:@"internal"
                                            syntax:GPBFileSyntaxProto2];

    descriptor = [GPBDescriptor allocDescriptorForClass:[GPBMessage class]
                                              rootClass:Nil
                                                   file:fileDescriptor
                                                 fields:NULL
                                             fieldCount:0
                                                 oneofs:NULL
                                             oneofCount:0
                                                  enums:NULL
                                              enumCount:0
                                                 ranges:NULL
                                             rangeCount:0
                                            storageSize:0
                                             wireFormat:NO];
  }
  return descriptor;
}

+ (instancetype)message {
  return [[[self alloc] init] autorelease];
}

- (instancetype)init {
  if ((self = [super init])) {
    messageStorage_ = (GPBMessage_StoragePtr)(
        ((uint8_t *)self) + class_getInstanceSize([self class]));

    readOnlyMutex_ = OS_SPINLOCK_INIT;
  }

  return self;
}

- (instancetype)initWithData:(NSData *)data error:(NSError **)errorPtr {
  return [self initWithData:data extensionRegistry:nil error:errorPtr];
}

- (instancetype)initWithData:(NSData *)data
           extensionRegistry:(GPBExtensionRegistry *)extensionRegistry
                       error:(NSError **)errorPtr {
  if ((self = [self init])) {
    @try {
      [self mergeFromData:data extensionRegistry:extensionRegistry];
    }
    @catch (NSException *exception) {
      [self release];
      self = nil;
      if (errorPtr) {
        *errorPtr = MessageErrorWithReason(GPBMessageErrorCodeMalformedData,
                                           exception.reason);
      }
    }
#ifdef DEBUG
    if (self && !self.initialized) {
      [self release];
      self = nil;
      if (errorPtr) {
        *errorPtr = MessageError(GPBMessageErrorCodeMissingRequiredField, nil);
      }
    }
#endif
  }
  return self;
}

- (instancetype)initWithCodedInputStream:(GPBCodedInputStream *)input
                       extensionRegistry:
                           (GPBExtensionRegistry *)extensionRegistry
                                   error:(NSError **)errorPtr {
  if ((self = [self init])) {
    @try {
      [self mergeFromCodedInputStream:input extensionRegistry:extensionRegistry];
    }
    @catch (NSException *exception) {
      [self release];
      self = nil;
      if (errorPtr) {
        *errorPtr = MessageErrorWithReason(GPBMessageErrorCodeMalformedData,
                                           exception.reason);
      }
    }
#ifdef DEBUG
    if (self && !self.initialized) {
      [self release];
      self = nil;
      if (errorPtr) {
        *errorPtr = MessageError(GPBMessageErrorCodeMissingRequiredField, nil);
      }
    }
#endif
  }
  return self;
}

- (void)dealloc {
  [self internalClear:NO];
  NSCAssert(!autocreator_, @"Autocreator was not cleared before dealloc.");
  [super dealloc];
}

- (void)copyFieldsInto:(GPBMessage *)message
                  zone:(NSZone *)zone
            descriptor:(GPBDescriptor *)descriptor {
  // Copy all the storage...
  memcpy(message->messageStorage_, messageStorage_, descriptor->storageSize_);

  GPBFileSyntax syntax = descriptor.file.syntax;

  // Loop over the fields doing fixup...
  for (GPBFieldDescriptor *field in descriptor->fields_) {
    if (GPBFieldIsMapOrArray(field)) {
      id value = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
      if (value) {
        // We need to copy the array/map, but the catch is for message fields,
        // we also need to ensure all the messages as those need copying also.
        id newValue;
        if (GPBFieldTypeIsMessage(field)) {
          if (field.fieldType == GPBFieldTypeRepeated) {
            NSArray *existingArray = (NSArray *)value;
            NSMutableArray *newArray =
                [[NSMutableArray alloc] initWithCapacity:existingArray.count];
            newValue = newArray;
            for (GPBMessage *msg in existingArray) {
              GPBMessage *copiedMsg = [msg copyWithZone:zone];
              [newArray addObject:copiedMsg];
              [copiedMsg release];
            }
          } else {
            if (field.mapKeyType == GPBTypeString) {
              // Map is an NSDictionary.
              NSDictionary *existingDict = value;
              NSMutableDictionary *newDict = [[NSMutableDictionary alloc]
                  initWithCapacity:existingDict.count];
              newValue = newDict;
              [existingDict enumerateKeysAndObjectsUsingBlock:^(NSString *key,
                                                                GPBMessage *msg,
                                                                BOOL *stop) {
#pragma unused(stop)
                GPBMessage *copiedMsg = [msg copyWithZone:zone];
                [newDict setObject:copiedMsg forKey:key];
                [copiedMsg release];
              }];
            } else {
              // Is one of the GPB*ObjectDictionary classes.  Type doesn't
              // matter, just need one to invoke the selector.
              GPBInt32ObjectDictionary *existingDict = value;
              newValue = [existingDict deepCopyWithZone:zone];
            }
          }
        } else {
          // Not messages (but is a map/array)...
          if (field.fieldType == GPBFieldTypeRepeated) {
            if (GPBFieldTypeIsObject(field)) {
              // NSArray
              newValue = [value mutableCopyWithZone:zone];
            } else {
              // GPB*Array
              newValue = [value copyWithZone:zone];
            }
          } else {
            if (field.mapKeyType == GPBTypeString) {
              // NSDictionary
              newValue = [value mutableCopyWithZone:zone];
            } else {
              // Is one of the GPB*Dictionary classes.  Type doesn't matter,
              // just need one to invoke the selector.
              GPBInt32Int32Dictionary *existingDict = value;
              newValue = [existingDict copyWithZone:zone];
            }
          }
        }
        // We retain here because the memcpy picked up the pointer value and
        // the next call to SetRetainedObject... will release the current value.
        [value retain];
        GPBSetRetainedObjectIvarWithFieldInternal(message, field, newValue,
                                                  syntax);
      }
    } else if (GPBFieldTypeIsMessage(field)) {
      // For object types, if we have a value, copy it.  If we don't,
      // zero it to remove the pointer to something that was autocreated
      // (and the ptr just got memcpyed).
      if (GPBGetHasIvarField(self, field)) {
        GPBMessage *value = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        GPBMessage *newValue = [value copyWithZone:zone];
        // We retain here because the memcpy picked up the pointer value and
        // the next call to SetRetainedObject... will release the current value.
        [value retain];
        GPBSetRetainedObjectIvarWithFieldInternal(message, field, newValue,
                                                  syntax);
      } else {
        uint8_t *storage = (uint8_t *)message->messageStorage_;
        id *typePtr = (id *)&storage[field->description_->offset];
        *typePtr = NULL;
      }
    } else if (GPBFieldTypeIsObject(field) && GPBGetHasIvarField(self, field)) {
      // A set string/data value (message picked off above), copy it.
      id value = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
      id newValue = [value copyWithZone:zone];
      // We retain here because the memcpy picked up the pointer value and
      // the next call to SetRetainedObject... will release the current value.
      [value retain];
      GPBSetRetainedObjectIvarWithFieldInternal(message, field, newValue,
                                                syntax);
    } else {
      // memcpy took care of the rest of the primative fields if they were set.
    }
  }  // for (field in descriptor->fields_)
}

- (id)copyWithZone:(NSZone *)zone {
  GPBDescriptor *descriptor = [self descriptor];
  GPBMessage *result = [[descriptor.messageClass allocWithZone:zone] init];

  [self copyFieldsInto:result zone:zone descriptor:descriptor];
  // Make immutable copies of the extra bits.
  result->unknownFields_ = [unknownFields_ copyWithZone:zone];
  result->extensionMap_ = CloneExtensionMap(extensionMap_, zone);
  return result;
}

- (void)clear {
  [self internalClear:YES];
}

- (void)internalClear:(BOOL)zeroStorage {
  GPBDescriptor *descriptor = [self descriptor];
  for (GPBFieldDescriptor *field in descriptor->fields_) {
    if (GPBFieldIsMapOrArray(field)) {
      id arrayOrMap = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
      if (arrayOrMap) {
        if (field.fieldType == GPBFieldTypeRepeated) {
          if (GPBFieldTypeIsObject(field)) {
            GPBAutocreatedArray *autoArray = arrayOrMap;
            if (autoArray->_autocreator == self) {
              autoArray->_autocreator = nil;
            }
          } else {
            // Type doesn't matter, it is a GPB*Array.
            GPBInt32Array *gpbArray = arrayOrMap;
            if (gpbArray->_autocreator == self) {
              gpbArray->_autocreator = nil;
            }
          }
        } else {
          if ((field.mapKeyType == GPBTypeString) &&
              GPBFieldTypeIsObject(field)) {
            GPBAutocreatedDictionary *autoDict = arrayOrMap;
            if (autoDict->_autocreator == self) {
              autoDict->_autocreator = nil;
            }
          } else {
            // Type doesn't matter, it is a GPB*Dictionary.
            GPBInt32Int32Dictionary *gpbDict = arrayOrMap;
            if (gpbDict->_autocreator == self) {
              gpbDict->_autocreator = nil;
            }
          }
        }
        [arrayOrMap release];
      }
    } else if (GPBFieldTypeIsMessage(field)) {
      GPBClearAutocreatedMessageIvarWithField(self, field);
      GPBMessage *value = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
      [value release];
    } else if (GPBFieldTypeIsObject(field) && GPBGetHasIvarField(self, field)) {
      id value = GPBGetObjectIvarWithField(self, field);
      [value release];
    }
  }

  // GPBClearMessageAutocreator() expects that its caller has already been
  // removed from autocreatedExtensionMap_ so we set to nil first.
  NSArray *autocreatedValues = [autocreatedExtensionMap_ allValues];
  [autocreatedExtensionMap_ release];
  autocreatedExtensionMap_ = nil;

  // Since we're clearing all of our extensions, make sure that we clear the
  // autocreator on any that we've created so they no longer refer to us.
  for (GPBMessage *value in autocreatedValues) {
    NSCAssert(GPBWasMessageAutocreatedBy(value, self),
              @"Autocreated extension does not refer back to self.");
    GPBClearMessageAutocreator(value);
  }

  [extensionMap_ release];
  extensionMap_ = nil;
  [unknownFields_ release];
  unknownFields_ = nil;

  // Note that clearing does not affect autocreator_. If we are being cleared
  // because of a dealloc, then autocreator_ should be nil anyway. If we are
  // being cleared because someone explicitly clears us, we don't want to
  // sever our relationship with our autocreator.

  if (zeroStorage) {
    memset(messageStorage_, 0, descriptor->storageSize_);
  }
}

- (BOOL)isInitialized {
  GPBDescriptor *descriptor = [self descriptor];
  for (GPBFieldDescriptor *field in descriptor->fields_) {
    if (field.isRequired) {
      if (!GPBGetHasIvarField(self, field)) {
        return NO;
      }
    }
    if (GPBFieldTypeIsMessage(field)) {
      GPBFieldType fieldType = field.fieldType;
      if (fieldType == GPBFieldTypeSingle) {
        if (field.isRequired) {
          GPBMessage *message = GPBGetMessageIvarWithField(self, field);
          if (!message.initialized) {
            return NO;
          }
        } else {
          NSAssert(field.isOptional,
                   @"%@: Single message field %@ not required or optional?",
                   [self class], field.name);
          if (GPBGetHasIvarField(self, field)) {
            GPBMessage *message = GPBGetMessageIvarWithField(self, field);
            if (!message.initialized) {
              return NO;
            }
          }
        }
      } else if (fieldType == GPBFieldTypeRepeated) {
        NSArray *array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        for (GPBMessage *message in array) {
          if (!message.initialized) {
            return NO;
          }
        }
      } else {  // fieldType == GPBFieldTypeMap
        if (field.mapKeyType == GPBTypeString) {
          NSDictionary *map =
              GPBGetObjectIvarWithFieldNoAutocreate(self, field);
          if (map && !GPBDictionaryIsInitializedInternalHelper(map, field)) {
            return NO;
          }
        } else {
          // Real type is GPB*ObjectDictionary, exact type doesn't matter.
          GPBInt32ObjectDictionary *map =
              GPBGetObjectIvarWithFieldNoAutocreate(self, field);
          if (map && ![map isInitialized]) {
            return NO;
          }
        }
      }
    }
  }

  __block BOOL result = YES;
  [extensionMap_
      enumerateKeysAndObjectsUsingBlock:^(GPBExtensionField *extension, id obj,
                                          BOOL *stop) {
        GPBExtensionDescriptor *extDesc = extension.descriptor;
        if (GPBExtensionIsMessage(extDesc)) {
          if (extDesc.isRepeated) {
            for (GPBMessage *msg in obj) {
              if (!msg.initialized) {
                result = NO;
                *stop = YES;
                break;
              }
            }
          } else {
            GPBMessage *asMsg = obj;
            if (!asMsg.initialized) {
              result = NO;
              *stop = YES;
            }
          }
        }
      }];
  return result;
}

- (GPBDescriptor *)descriptor {
  return [[self class] descriptor];
}

- (NSData *)data {
#ifdef DEBUG
  if (!self.initialized) {
    return nil;
  }
#endif
  NSMutableData *data = [NSMutableData dataWithLength:[self serializedSize]];
  GPBCodedOutputStream *stream =
      [[GPBCodedOutputStream alloc] initWithData:data];
  @try {
    [self writeToCodedOutputStream:stream];
  }
  @catch (NSException *exception) {
    // This really shouldn't happen. The only way writeToCodedOutputStream:
    // could throw is if something in the library has a bug and the
    // serializedSize was wrong.
#ifdef DEBUG
    NSLog(@"%@: Internal exception while building message data: %@",
          [self class], exception);
#endif
    data = nil;
  }
  [stream release];
  return data;
}

- (NSData *)delimitedData {
  size_t serializedSize = [self serializedSize];
  size_t varintSize = GPBComputeRawVarint32SizeForInteger(serializedSize);
  NSMutableData *data =
      [NSMutableData dataWithLength:(serializedSize + varintSize)];
  GPBCodedOutputStream *stream =
      [[GPBCodedOutputStream alloc] initWithData:data];
  @try {
    [self writeDelimitedToCodedOutputStream:stream];
  }
  @catch (NSException *exception) {
    // This really shouldn't happen.  The only way writeToCodedOutputStream:
    // could throw is if something in the library has a bug and the
    // serializedSize was wrong.
#ifdef DEBUG
    NSLog(@"%@: Internal exception while building message delimitedData: %@",
          [self class], exception);
#endif
    data = nil;
  }
  [stream release];
  return data;
}

- (void)writeToOutputStream:(NSOutputStream *)output {
  GPBCodedOutputStream *stream =
      [[GPBCodedOutputStream alloc] initWithOutputStream:output];
  [self writeToCodedOutputStream:stream];
  [stream release];
}

- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)output {
  GPBDescriptor *descriptor = [self descriptor];
  NSArray *fieldsArray = descriptor->fields_;
  NSUInteger fieldCount = fieldsArray.count;
  const GPBExtensionRange *extensionRanges = descriptor.extensionRanges;
  NSUInteger extensionRangesCount = descriptor.extensionRangesCount;
  for (NSUInteger i = 0, j = 0; i < fieldCount || j < extensionRangesCount;) {
    if (i == fieldCount) {
      [self writeExtensionsToCodedOutputStream:output
                                         range:extensionRanges[j++]];
    } else if (j == extensionRangesCount ||
               GPBFieldNumber(fieldsArray[i]) < extensionRanges[j].start) {
      [self writeField:fieldsArray[i++] toCodedOutputStream:output];
    } else {
      [self writeExtensionsToCodedOutputStream:output
                                         range:extensionRanges[j++]];
    }
  }
  if (descriptor.isWireFormat) {
    [unknownFields_ writeAsMessageSetTo:output];
  } else {
    [unknownFields_ writeToCodedOutputStream:output];
  }
}

- (void)writeDelimitedToOutputStream:(NSOutputStream *)output {
  GPBCodedOutputStream *codedOutput =
      [[GPBCodedOutputStream alloc] initWithOutputStream:output];
  [self writeDelimitedToCodedOutputStream:codedOutput];
  [codedOutput release];
}

- (void)writeDelimitedToCodedOutputStream:(GPBCodedOutputStream *)output {
  [output writeRawVarintSizeTAs32:[self serializedSize]];
  [self writeToCodedOutputStream:output];
}

- (void)writeField:(GPBFieldDescriptor *)field
    toCodedOutputStream:(GPBCodedOutputStream *)output {
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeSingle) {
    BOOL has = GPBGetHasIvarField(self, field);
    if (!has) {
      return;
    }
  }
  uint32_t fieldNumber = GPBFieldNumber(field);

//%PDDM-DEFINE FIELD_CASE(TYPE, REAL_TYPE)
//%FIELD_CASE_FULL(TYPE, REAL_TYPE, REAL_TYPE)
//%PDDM-DEFINE FIELD_CASE_FULL(TYPE, REAL_TYPE, ARRAY_TYPE)
//%    case GPBType##TYPE:
//%      if (fieldType == GPBFieldTypeRepeated) {
//%        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
//%        GPB##ARRAY_TYPE##Array *array =
//%            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
//%        [output write##TYPE##s:fieldNumber values:array tag:tag];
//%      } else if (fieldType == GPBFieldTypeSingle) {
//%        [output write##TYPE:fieldNumber
//%              TYPE$S  value:GPBGet##REAL_TYPE##IvarWithField(self, field)];
//%      } else {  // fieldType == GPBFieldTypeMap
//%        // Exact type here doesn't matter.
//%        GPBInt32##ARRAY_TYPE##Dictionary *dict =
//%            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
//%        [dict writeToCodedOutputStream:output asField:field];
//%      }
//%      break;
//%
//%PDDM-DEFINE FIELD_CASE2(TYPE)
//%    case GPBType##TYPE:
//%      if (fieldType == GPBFieldTypeRepeated) {
//%        NSArray *array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
//%        [output write##TYPE##s:fieldNumber values:array];
//%      } else if (fieldType == GPBFieldTypeSingle) {
//%        // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
//%        // again.
//%        [output write##TYPE:fieldNumber
//%              TYPE$S  value:GPBGetObjectIvarWithFieldNoAutocreate(self, field)];
//%      } else {  // fieldType == GPBFieldTypeMap
//%        // Exact type here doesn't matter.
//%        id dict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
//%        GPBType mapKeyType = field.mapKeyType;
//%        if (mapKeyType == GPBTypeString) {
//%          GPBDictionaryWriteToStreamInternalHelper(output, dict, field);
//%        } else {
//%          [dict writeToCodedOutputStream:output asField:field];
//%        }
//%      }
//%      break;
//%

  switch (GPBGetFieldType(field)) {

//%PDDM-EXPAND FIELD_CASE(Bool, Bool)
// This block of code is generated, do not edit it directly.

    case GPBTypeBool:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBBoolArray *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeBools:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeBool:fieldNumber
                    value:GPBGetBoolIvarWithField(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32BoolDictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(Fixed32, UInt32)
// This block of code is generated, do not edit it directly.

    case GPBTypeFixed32:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBUInt32Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeFixed32s:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeFixed32:fieldNumber
                       value:GPBGetUInt32IvarWithField(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32UInt32Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(SFixed32, Int32)
// This block of code is generated, do not edit it directly.

    case GPBTypeSFixed32:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBInt32Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeSFixed32s:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeSFixed32:fieldNumber
                        value:GPBGetInt32IvarWithField(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32Int32Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(Float, Float)
// This block of code is generated, do not edit it directly.

    case GPBTypeFloat:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBFloatArray *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeFloats:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeFloat:fieldNumber
                     value:GPBGetFloatIvarWithField(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32FloatDictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(Fixed64, UInt64)
// This block of code is generated, do not edit it directly.

    case GPBTypeFixed64:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBUInt64Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeFixed64s:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeFixed64:fieldNumber
                       value:GPBGetUInt64IvarWithField(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32UInt64Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(SFixed64, Int64)
// This block of code is generated, do not edit it directly.

    case GPBTypeSFixed64:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBInt64Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeSFixed64s:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeSFixed64:fieldNumber
                        value:GPBGetInt64IvarWithField(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32Int64Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(Double, Double)
// This block of code is generated, do not edit it directly.

    case GPBTypeDouble:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBDoubleArray *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeDoubles:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeDouble:fieldNumber
                      value:GPBGetDoubleIvarWithField(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32DoubleDictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(Int32, Int32)
// This block of code is generated, do not edit it directly.

    case GPBTypeInt32:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBInt32Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeInt32s:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeInt32:fieldNumber
                     value:GPBGetInt32IvarWithField(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32Int32Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(Int64, Int64)
// This block of code is generated, do not edit it directly.

    case GPBTypeInt64:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBInt64Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeInt64s:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeInt64:fieldNumber
                     value:GPBGetInt64IvarWithField(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32Int64Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(SInt32, Int32)
// This block of code is generated, do not edit it directly.

    case GPBTypeSInt32:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBInt32Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeSInt32s:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeSInt32:fieldNumber
                      value:GPBGetInt32IvarWithField(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32Int32Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(SInt64, Int64)
// This block of code is generated, do not edit it directly.

    case GPBTypeSInt64:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBInt64Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeSInt64s:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeSInt64:fieldNumber
                      value:GPBGetInt64IvarWithField(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32Int64Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(UInt32, UInt32)
// This block of code is generated, do not edit it directly.

    case GPBTypeUInt32:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBUInt32Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeUInt32s:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeUInt32:fieldNumber
                      value:GPBGetUInt32IvarWithField(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32UInt32Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(UInt64, UInt64)
// This block of code is generated, do not edit it directly.

    case GPBTypeUInt64:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBUInt64Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeUInt64s:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeUInt64:fieldNumber
                      value:GPBGetUInt64IvarWithField(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32UInt64Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE_FULL(Enum, Int32, Enum)
// This block of code is generated, do not edit it directly.

    case GPBTypeEnum:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBEnumArray *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeEnums:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeEnum:fieldNumber
                    value:GPBGetInt32IvarWithField(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32EnumDictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE2(Data)
// This block of code is generated, do not edit it directly.

    case GPBTypeData:
      if (fieldType == GPBFieldTypeRepeated) {
        NSArray *array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeDatas:fieldNumber values:array];
      } else if (fieldType == GPBFieldTypeSingle) {
        // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
        // again.
        [output writeData:fieldNumber
                    value:GPBGetObjectIvarWithFieldNoAutocreate(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        id dict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        GPBType mapKeyType = field.mapKeyType;
        if (mapKeyType == GPBTypeString) {
          GPBDictionaryWriteToStreamInternalHelper(output, dict, field);
        } else {
          [dict writeToCodedOutputStream:output asField:field];
        }
      }
      break;

//%PDDM-EXPAND FIELD_CASE2(String)
// This block of code is generated, do not edit it directly.

    case GPBTypeString:
      if (fieldType == GPBFieldTypeRepeated) {
        NSArray *array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeStrings:fieldNumber values:array];
      } else if (fieldType == GPBFieldTypeSingle) {
        // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
        // again.
        [output writeString:fieldNumber
                      value:GPBGetObjectIvarWithFieldNoAutocreate(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        id dict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        GPBType mapKeyType = field.mapKeyType;
        if (mapKeyType == GPBTypeString) {
          GPBDictionaryWriteToStreamInternalHelper(output, dict, field);
        } else {
          [dict writeToCodedOutputStream:output asField:field];
        }
      }
      break;

//%PDDM-EXPAND FIELD_CASE2(Message)
// This block of code is generated, do not edit it directly.

    case GPBTypeMessage:
      if (fieldType == GPBFieldTypeRepeated) {
        NSArray *array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeMessages:fieldNumber values:array];
      } else if (fieldType == GPBFieldTypeSingle) {
        // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
        // again.
        [output writeMessage:fieldNumber
                       value:GPBGetObjectIvarWithFieldNoAutocreate(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        id dict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        GPBType mapKeyType = field.mapKeyType;
        if (mapKeyType == GPBTypeString) {
          GPBDictionaryWriteToStreamInternalHelper(output, dict, field);
        } else {
          [dict writeToCodedOutputStream:output asField:field];
        }
      }
      break;

//%PDDM-EXPAND FIELD_CASE2(Group)
// This block of code is generated, do not edit it directly.

    case GPBTypeGroup:
      if (fieldType == GPBFieldTypeRepeated) {
        NSArray *array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeGroups:fieldNumber values:array];
      } else if (fieldType == GPBFieldTypeSingle) {
        // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
        // again.
        [output writeGroup:fieldNumber
                     value:GPBGetObjectIvarWithFieldNoAutocreate(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        id dict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        GPBType mapKeyType = field.mapKeyType;
        if (mapKeyType == GPBTypeString) {
          GPBDictionaryWriteToStreamInternalHelper(output, dict, field);
        } else {
          [dict writeToCodedOutputStream:output asField:field];
        }
      }
      break;

//%PDDM-EXPAND-END (18 expansions)
  }
}

#pragma mark - Extensions

- (id)getExtension:(GPBExtensionField *)extension {
  CheckExtension(self, extension);
  id value = [extensionMap_ objectForKey:extension];
  if (value != nil) {
    return value;
  }

  GPBExtensionDescriptor *extDesc = extension.descriptor;

  // No default for repeated.
  if (extDesc.isRepeated) {
    return nil;
  }
  // Non messages get their default.
  if (!GPBExtensionIsMessage(extDesc)) {
    return [extension defaultValue];
  }

  // Check for an autocreated value.
  OSSpinLockLock(&readOnlyMutex_);
  value = [autocreatedExtensionMap_ objectForKey:extension];
  if (!value) {
    // Auto create the message extensions to match normal fields.
    value = CreateMessageWithAutocreatorForExtension(extDesc.msgClass, self,
                                                     extension);

    if (autocreatedExtensionMap_ == nil) {
      autocreatedExtensionMap_ = [[NSMutableDictionary alloc] init];
    }

    // We can't simply call setExtension here because that would clear the new
    // value's autocreator.
    [autocreatedExtensionMap_ setObject:value forKey:extension];
    [value release];
  }

  OSSpinLockUnlock(&readOnlyMutex_);
  return value;
}

- (id)getExistingExtension:(GPBExtensionField *)extension {
  // This is an internal method so we don't need to call CheckExtension().
  return [extensionMap_ objectForKey:extension];
}

- (BOOL)hasExtension:(GPBExtensionField *)extension {
#if DEBUG
  CheckExtension(self, extension);
#endif  // DEBUG
  return nil != [extensionMap_ objectForKey:extension];
}

- (NSArray *)extensionsCurrentlySet {
  return [extensionMap_ allKeys];
}

- (void)writeExtensionsToCodedOutputStream:(GPBCodedOutputStream *)output
                                     range:(GPBExtensionRange)range {
  NSArray *sortedExtensions = [[extensionMap_ allKeys]
      sortedArrayUsingSelector:@selector(compareByFieldNumber:)];
  uint32_t start = range.start;
  uint32_t end = range.end;
  for (GPBExtensionField *extension in sortedExtensions) {
    uint32_t fieldNumber = [extension fieldNumber];
    if (fieldNumber >= start && fieldNumber < end) {
      id value = [extensionMap_ objectForKey:extension];
      [extension writeValue:value includingTagToCodedOutputStream:output];
    }
  }
}

- (NSArray *)sortedExtensionsInUse {
  return [[extensionMap_ allKeys]
      sortedArrayUsingSelector:@selector(compareByFieldNumber:)];
}

- (void)setExtension:(GPBExtensionField *)extension value:(id)value {
  if (!value) {
    [self clearExtension:extension];
    return;
  }

  CheckExtension(self, extension);

  if ([extension isRepeated]) {
    [NSException raise:NSInvalidArgumentException
                format:@"Must call addExtension() for repeated types."];
  }

  if (extensionMap_ == nil) {
    extensionMap_ = [[NSMutableDictionary alloc] init];
  }

  [extensionMap_ setObject:value forKey:extension];

  GPBExtensionDescriptor *descriptor = extension.descriptor;

  if (GPBExtensionIsMessage(descriptor) && !descriptor.isRepeated) {
    GPBMessage *autocreatedValue =
        [[autocreatedExtensionMap_ objectForKey:extension] retain];
    // Must remove from the map before calling GPBClearMessageAutocreator() so
    // that GPBClearMessageAutocreator() knows its safe to clear.
    [autocreatedExtensionMap_ removeObjectForKey:extension];
    GPBClearMessageAutocreator(autocreatedValue);
    [autocreatedValue release];
  }

  GPBBecomeVisibleToAutocreator(self);
}

- (void)addExtension:(GPBExtensionField *)extension value:(id)value {
  CheckExtension(self, extension);

  if (![extension isRepeated]) {
    [NSException raise:NSInvalidArgumentException
                format:@"Must call setExtension() for singular types."];
  }

  if (extensionMap_ == nil) {
    extensionMap_ = [[NSMutableDictionary alloc] init];
  }
  NSMutableArray *list = [extensionMap_ objectForKey:extension];
  if (list == nil) {
    list = [NSMutableArray array];
    [extensionMap_ setObject:list forKey:extension];
  }

  [list addObject:value];
  GPBBecomeVisibleToAutocreator(self);
}

- (void)setExtension:(GPBExtensionField *)extension
               index:(NSUInteger)idx
               value:(id)value {
  CheckExtension(self, extension);

  if (![extension isRepeated]) {
    [NSException raise:NSInvalidArgumentException
                format:@"Must call setExtension() for singular types."];
  }

  if (extensionMap_ == nil) {
    extensionMap_ = [[NSMutableDictionary alloc] init];
  }

  NSMutableArray *list = [extensionMap_ objectForKey:extension];

  [list replaceObjectAtIndex:idx withObject:value];
  GPBBecomeVisibleToAutocreator(self);
}

- (void)clearExtension:(GPBExtensionField *)extension {
  CheckExtension(self, extension);

  // Only become visible if there was actually a value to clear.
  if ([extensionMap_ objectForKey:extension]) {
    [extensionMap_ removeObjectForKey:extension];
    GPBBecomeVisibleToAutocreator(self);
  }
}

#pragma mark - mergeFrom

- (void)mergeFromData:(NSData *)data
    extensionRegistry:(GPBExtensionRegistry *)extensionRegistry {
  GPBCodedInputStream *input = [[GPBCodedInputStream alloc] initWithData:data];
  [self mergeFromCodedInputStream:input extensionRegistry:extensionRegistry];
  [input checkLastTagWas:0];
  [input release];
}

#pragma mark - mergeDelimitedFrom

- (void)mergeDelimitedFromCodedInputStream:(GPBCodedInputStream *)input
                         extensionRegistry:(GPBExtensionRegistry *)extensionRegistry {
  GPBCodedInputStreamState *state = &input->state_;
  if (GPBCodedInputStreamIsAtEnd(state)) {
    return;
  }
  NSData *data = GPBCodedInputStreamReadRetainedDataNoCopy(state);
  if (data == nil) {
    return;
  }
  [self mergeFromData:data extensionRegistry:extensionRegistry];
  [data release];
}

#pragma mark - Parse From Data Support

+ (instancetype)parseFromData:(NSData *)data error:(NSError **)errorPtr {
  return [self parseFromData:data extensionRegistry:nil error:errorPtr];
}

+ (instancetype)parseFromData:(NSData *)data
            extensionRegistry:(GPBExtensionRegistry *)extensionRegistry
                        error:(NSError **)errorPtr {
  return [[[self alloc] initWithData:data
                   extensionRegistry:extensionRegistry
                               error:errorPtr] autorelease];
}

+ (instancetype)parseFromCodedInputStream:(GPBCodedInputStream *)input
                        extensionRegistry:(GPBExtensionRegistry *)extensionRegistry
                                    error:(NSError **)errorPtr {
  return
      [[[self alloc] initWithCodedInputStream:input
                            extensionRegistry:extensionRegistry
                                        error:errorPtr] autorelease];
}

#pragma mark - Parse Delimited From Data Support

+ (instancetype)parseDelimitedFromCodedInputStream:(GPBCodedInputStream *)input
                                 extensionRegistry:
                                     (GPBExtensionRegistry *)extensionRegistry
                                             error:(NSError **)errorPtr {
  GPBMessage *message = [[[self alloc] init] autorelease];
  @try {
    [message mergeDelimitedFromCodedInputStream:input
                              extensionRegistry:extensionRegistry];
  }
  @catch (NSException *exception) {
    [message release];
    message = nil;
    if (errorPtr) {
      *errorPtr = MessageErrorWithReason(GPBMessageErrorCodeMalformedData,
                                         exception.reason);
    }
  }
#ifdef DEBUG
  if (message && !message.initialized) {
    [message release];
    message = nil;
    if (errorPtr) {
      *errorPtr = MessageError(GPBMessageErrorCodeMissingRequiredField, nil);
    }
  }
#endif
  return message;
}

#pragma mark - Unknown Field Support

- (GPBUnknownFieldSet *)unknownFields {
  return unknownFields_;
}

- (void)setUnknownFields:(GPBUnknownFieldSet *)unknownFields {
  if (unknownFields != unknownFields_) {
    [unknownFields_ release];
    unknownFields_ = [unknownFields copy];
    GPBBecomeVisibleToAutocreator(self);
  }
}

- (void)parseMessageSet:(GPBCodedInputStream *)input
      extensionRegistry:(GPBExtensionRegistry *)extensionRegistry {
  uint32_t typeId = 0;
  NSData *rawBytes = nil;
  GPBExtensionField *extension = nil;
  GPBCodedInputStreamState *state = &input->state_;
  while (true) {
    uint32_t tag = GPBCodedInputStreamReadTag(state);
    if (tag == 0) {
      break;
    }

    if (tag == GPBWireFormatMessageSetTypeIdTag) {
      typeId = GPBCodedInputStreamReadUInt32(state);
      if (typeId != 0) {
        extension = [extensionRegistry getExtension:[self descriptor]
                                        fieldNumber:typeId];
      }
    } else if (tag == GPBWireFormatMessageSetMessageTag) {
      rawBytes = [GPBCodedInputStreamReadRetainedDataNoCopy(state) autorelease];
    } else {
      if (![input skipField:tag]) {
        break;
      }
    }
  }

  [input checkLastTagWas:GPBWireFormatMessageSetItemEndTag];

  if (rawBytes != nil && typeId != 0) {
    if (extension != nil) {
      GPBCodedInputStream *newInput =
          [[GPBCodedInputStream alloc] initWithData:rawBytes];
      [extension mergeFromCodedInputStream:newInput
                         extensionRegistry:extensionRegistry
                                   message:self];
      [newInput release];
    } else {
      GPBUnknownFieldSet *unknownFields = GetOrMakeUnknownFields(self);
      [unknownFields mergeMessageSetMessage:typeId data:rawBytes];
    }
  }
}

- (BOOL)parseUnknownField:(GPBCodedInputStream *)input
        extensionRegistry:(GPBExtensionRegistry *)extensionRegistry
                      tag:(uint32_t)tag {
  GPBWireFormat wireType = GPBWireFormatGetTagWireType(tag);
  int32_t fieldNumber = GPBWireFormatGetTagFieldNumber(tag);

  GPBDescriptor *descriptor = [self descriptor];
  GPBExtensionField *extension =
      [extensionRegistry getExtension:descriptor fieldNumber:fieldNumber];

  if (extension == nil) {
    if (descriptor.wireFormat && GPBWireFormatMessageSetItemTag == tag) {
      [self parseMessageSet:input extensionRegistry:extensionRegistry];
      return YES;
    }
  } else {
    if ([extension wireType] == wireType) {
      [extension mergeFromCodedInputStream:input
                         extensionRegistry:extensionRegistry
                                   message:self];
      return YES;
    }
  }
  if ([GPBUnknownFieldSet isFieldTag:tag]) {
    GPBUnknownFieldSet *unknownFields = GetOrMakeUnknownFields(self);
    return [unknownFields mergeFieldFrom:tag input:input];
  } else {
    return NO;
  }
}

- (void)addUnknownMapEntry:(int32_t)fieldNum value:(NSData *)data {
  GPBUnknownFieldSet *unknownFields = GetOrMakeUnknownFields(self);
  [unknownFields addUnknownMapEntry:fieldNum value:data];
}

#pragma mark - MergeFromCodedInputStream Support

typedef struct MergeFromCodedInputStreamContext {
  GPBMessage *self;
  GPBCodedInputStream *stream;
  GPBMessage *result;
  GPBExtensionRegistry *registry;
  uint32_t tag;
  GPBFileSyntax syntax;
} MergeFromCodedInputStreamContext;

//%PDDM-DEFINE MERGE_FROM_CODED_INPUT_STREAM_POD_FUNC(NAME, TYPE, ARRAY_TYPE)
//%static BOOL DynamicMergeFromCodedInputStream##NAME(GPBFieldDescriptor *field,
//%                                           NAME$S  void *voidContext) {
//%  MergeFromCodedInputStreamContext *context =
//%      (MergeFromCodedInputStreamContext *)voidContext;
//%  GPBCodedInputStreamState *state = &context->stream->state_;
//%  GPBFieldType fieldType = field.fieldType;
//%  if (fieldType == GPBFieldTypeRepeated) {
//%    GPB##ARRAY_TYPE##Array *array =
//%        GetOrCreateArrayIvarWithField(context->result, field, context->syntax);
//%    if (field.isPackable) {
//%      int32_t length = GPBCodedInputStreamReadInt32(state);
//%      size_t limit = GPBCodedInputStreamPushLimit(state, length);
//%      while (GPBCodedInputStreamBytesUntilLimit(state) > 0) {
//%        TYPE val = GPBCodedInputStreamRead##NAME(state);
//%        [array addValue:val];
//%      }
//%      GPBCodedInputStreamPopLimit(state, limit);
//%    } else {
//%      TYPE val = GPBCodedInputStreamRead##NAME(state);
//%      [array addValue:val];
//%    }
//%  } else if (fieldType == GPBFieldTypeSingle) {
//%    TYPE val = GPBCodedInputStreamRead##NAME(state);
//%    GPBSet##ARRAY_TYPE##IvarWithFieldInternal(context->result, field, val,
//%           ARRAY_TYPE$S                     context->syntax);
//%  } else {  // fieldType == GPBFieldTypeMap
//%    // The exact type doesn't matter, just need to know it is a GPB*Dictionary.
//%    GPBInt32Int32Dictionary *map =
//%        GetOrCreateMapIvarWithField(context->result, field, context->syntax);
//%    [context->stream readMapEntry:map
//%                extensionRegistry:nil
//%                            field:field
//%                    parentMessage:context->result];
//%  }
//%  return NO;
//%}
//%
//%PDDM-DEFINE MERGE_FROM_CODED_INPUT_STREAM_OBJ_FUNC(NAME)
//%static BOOL DynamicMergeFromCodedInputStream##NAME(GPBFieldDescriptor *field,
//%                                           NAME$S  void *voidContext) {
//%  MergeFromCodedInputStreamContext *context = voidContext;
//%  GPBCodedInputStreamState *state = &context->stream->state_;
//%  GPBFieldType fieldType = field.fieldType;
//%  if (fieldType == GPBFieldTypeMap) {
//%    // GPB*Dictionary or NSDictionary, exact type doesn't matter at this point.
//%    id map =
//%        GetOrCreateMapIvarWithField(context->result, field, context->syntax);
//%    [context->stream readMapEntry:map
//%                extensionRegistry:nil
//%                            field:field
//%                    parentMessage:context->result];
//%  } else {
//%    id val = GPBCodedInputStreamReadRetained##NAME(state);
//%    if (fieldType == GPBFieldTypeRepeated) {
//%      NSMutableArray *array =
//%          GetOrCreateArrayIvarWithField(context->result, field, context->syntax);
//%      [array addObject:val];
//%      [val release];
//%    } else {  // fieldType == GPBFieldTypeSingle
//%      GPBSetRetainedObjectIvarWithFieldInternal(context->result, field, val,
//%                                                context->syntax);
//%    }
//%  }
//%  return NO;
//%}
//%

static BOOL DynamicMergeFromCodedInputStreamGroup(GPBFieldDescriptor *field,
                                                  void *voidContext) {
  MergeFromCodedInputStreamContext *context =
      (MergeFromCodedInputStreamContext *)voidContext;
  int number = GPBFieldNumber(field);
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBMessage *message = [[field.msgClass alloc] init];
    NSMutableArray *array =
        GetOrCreateArrayIvarWithField(context->result, field, context->syntax);
    [context->stream readGroup:number
                       message:message
             extensionRegistry:context->registry];
    [array addObject:message];
    [message release];
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL has = GPBGetHasIvarField(context->result, field);
    if (has) {
      // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
      // again.
      GPBMessage *message =
          GPBGetObjectIvarWithFieldNoAutocreate(context->result, field);
      [context->stream readGroup:number
                         message:message
               extensionRegistry:context->registry];
    } else {
      GPBMessage *message = [[field.msgClass alloc] init];
      [context->stream readGroup:number
                         message:message
               extensionRegistry:context->registry];
      GPBSetRetainedObjectIvarWithFieldInternal(context->result, field, message,
                                                context->syntax);
    }
  } else {  // fieldType == GPBFieldTypeMap
    NSCAssert(NO, @"Shouldn't happen");
    return YES;
  }
  return NO;
}

static BOOL DynamicMergeFromCodedInputStreamMessage(GPBFieldDescriptor *field,
                                                    void *voidContext) {
  MergeFromCodedInputStreamContext *context =
      (MergeFromCodedInputStreamContext *)voidContext;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBMessage *message = [[field.msgClass alloc] init];
    NSMutableArray *array =
        GetOrCreateArrayIvarWithField(context->result, field, context->syntax);
    [context->stream readMessage:message extensionRegistry:context->registry];
    [array addObject:message];
    [message release];
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL has = GPBGetHasIvarField(context->result, field);
    if (has) {
      // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
      // again.
      GPBMessage *message =
          GPBGetObjectIvarWithFieldNoAutocreate(context->result, field);
      [context->stream readMessage:message extensionRegistry:context->registry];
    } else {
      GPBMessage *message = [[field.msgClass alloc] init];
      [context->stream readMessage:message extensionRegistry:context->registry];
      GPBSetRetainedObjectIvarWithFieldInternal(context->result, field, message,
                                                context->syntax);
    }
  } else {  // fieldType == GPBFieldTypeMap
    // GPB*Dictionary or NSDictionary, exact type doesn't matter.
    id map =
        GetOrCreateMapIvarWithField(context->result, field, context->syntax);
    [context->stream readMapEntry:map
                extensionRegistry:context->registry
                            field:field
                    parentMessage:context->result];
  }
  return NO;
}

static BOOL DynamicMergeFromCodedInputStreamEnum(GPBFieldDescriptor *field,
                                                 void *voidContext) {
  MergeFromCodedInputStreamContext *context =
      (MergeFromCodedInputStreamContext *)voidContext;
  int number = GPBFieldNumber(field);
  BOOL hasPreservingUnknownEnumSemantics =
      GPBHasPreservingUnknownEnumSemantics(context->syntax);
  GPBCodedInputStreamState *state = &context->stream->state_;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBEnumArray *array =
        GetOrCreateArrayIvarWithField(context->result, field, context->syntax);
    if (field.isPackable) {
      int32_t length = GPBCodedInputStreamReadInt32(state);
      size_t limit = GPBCodedInputStreamPushLimit(state, length);
      while (GPBCodedInputStreamBytesUntilLimit(state) > 0) {
        int32_t val = GPBCodedInputStreamReadEnum(state);
        if (hasPreservingUnknownEnumSemantics || [field isValidEnumValue:val]) {
          [array addRawValue:val];
        } else {
          GPBUnknownFieldSet *unknownFields =
              GetOrMakeUnknownFields(context->self);
          [unknownFields mergeVarintField:number value:val];
        }
      }
      GPBCodedInputStreamPopLimit(state, limit);
    } else {
      int32_t val = GPBCodedInputStreamReadEnum(state);
      if (hasPreservingUnknownEnumSemantics || [field isValidEnumValue:val]) {
        [array addRawValue:val];
      } else {
        GPBUnknownFieldSet *unknownFields =
            GetOrMakeUnknownFields(context->self);
        [unknownFields mergeVarintField:number value:val];
      }
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    int32_t val = GPBCodedInputStreamReadEnum(state);
    if (hasPreservingUnknownEnumSemantics || [field isValidEnumValue:val]) {
      GPBSetInt32IvarWithFieldInternal(context->result, field, val,
                                       context->syntax);
    } else {
      GPBUnknownFieldSet *unknownFields = GetOrMakeUnknownFields(context->self);
      [unknownFields mergeVarintField:number value:val];
    }
  } else {  // fieldType == GPBFieldTypeMap
    // The exact type doesn't matter, just need to know it is a
    // GPB*EnumDictionary.
    GPBInt32EnumDictionary *map =
        GetOrCreateMapIvarWithField(context->result, field, context->syntax);
    [context->stream readMapEntry:map
                extensionRegistry:context->registry
                            field:field
                    parentMessage:context->result];
  }
  return NO;
}

//%PDDM-EXPAND MERGE_FROM_CODED_INPUT_STREAM_POD_FUNC(Bool, BOOL, Bool)
// This block of code is generated, do not edit it directly.

static BOOL DynamicMergeFromCodedInputStreamBool(GPBFieldDescriptor *field,
                                                 void *voidContext) {
  MergeFromCodedInputStreamContext *context =
      (MergeFromCodedInputStreamContext *)voidContext;
  GPBCodedInputStreamState *state = &context->stream->state_;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBBoolArray *array =
        GetOrCreateArrayIvarWithField(context->result, field, context->syntax);
    if (field.isPackable) {
      int32_t length = GPBCodedInputStreamReadInt32(state);
      size_t limit = GPBCodedInputStreamPushLimit(state, length);
      while (GPBCodedInputStreamBytesUntilLimit(state) > 0) {
        BOOL val = GPBCodedInputStreamReadBool(state);
        [array addValue:val];
      }
      GPBCodedInputStreamPopLimit(state, limit);
    } else {
      BOOL val = GPBCodedInputStreamReadBool(state);
      [array addValue:val];
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL val = GPBCodedInputStreamReadBool(state);
    GPBSetBoolIvarWithFieldInternal(context->result, field, val,
                                    context->syntax);
  } else {  // fieldType == GPBFieldTypeMap
    // The exact type doesn't matter, just need to know it is a GPB*Dictionary.
    GPBInt32Int32Dictionary *map =
        GetOrCreateMapIvarWithField(context->result, field, context->syntax);
    [context->stream readMapEntry:map
                extensionRegistry:nil
                            field:field
                    parentMessage:context->result];
  }
  return NO;
}

//%PDDM-EXPAND MERGE_FROM_CODED_INPUT_STREAM_POD_FUNC(Int32, int32_t, Int32)
// This block of code is generated, do not edit it directly.

static BOOL DynamicMergeFromCodedInputStreamInt32(GPBFieldDescriptor *field,
                                                  void *voidContext) {
  MergeFromCodedInputStreamContext *context =
      (MergeFromCodedInputStreamContext *)voidContext;
  GPBCodedInputStreamState *state = &context->stream->state_;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBInt32Array *array =
        GetOrCreateArrayIvarWithField(context->result, field, context->syntax);
    if (field.isPackable) {
      int32_t length = GPBCodedInputStreamReadInt32(state);
      size_t limit = GPBCodedInputStreamPushLimit(state, length);
      while (GPBCodedInputStreamBytesUntilLimit(state) > 0) {
        int32_t val = GPBCodedInputStreamReadInt32(state);
        [array addValue:val];
      }
      GPBCodedInputStreamPopLimit(state, limit);
    } else {
      int32_t val = GPBCodedInputStreamReadInt32(state);
      [array addValue:val];
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    int32_t val = GPBCodedInputStreamReadInt32(state);
    GPBSetInt32IvarWithFieldInternal(context->result, field, val,
                                     context->syntax);
  } else {  // fieldType == GPBFieldTypeMap
    // The exact type doesn't matter, just need to know it is a GPB*Dictionary.
    GPBInt32Int32Dictionary *map =
        GetOrCreateMapIvarWithField(context->result, field, context->syntax);
    [context->stream readMapEntry:map
                extensionRegistry:nil
                            field:field
                    parentMessage:context->result];
  }
  return NO;
}

//%PDDM-EXPAND MERGE_FROM_CODED_INPUT_STREAM_POD_FUNC(SInt32, int32_t, Int32)
// This block of code is generated, do not edit it directly.

static BOOL DynamicMergeFromCodedInputStreamSInt32(GPBFieldDescriptor *field,
                                                   void *voidContext) {
  MergeFromCodedInputStreamContext *context =
      (MergeFromCodedInputStreamContext *)voidContext;
  GPBCodedInputStreamState *state = &context->stream->state_;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBInt32Array *array =
        GetOrCreateArrayIvarWithField(context->result, field, context->syntax);
    if (field.isPackable) {
      int32_t length = GPBCodedInputStreamReadInt32(state);
      size_t limit = GPBCodedInputStreamPushLimit(state, length);
      while (GPBCodedInputStreamBytesUntilLimit(state) > 0) {
        int32_t val = GPBCodedInputStreamReadSInt32(state);
        [array addValue:val];
      }
      GPBCodedInputStreamPopLimit(state, limit);
    } else {
      int32_t val = GPBCodedInputStreamReadSInt32(state);
      [array addValue:val];
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    int32_t val = GPBCodedInputStreamReadSInt32(state);
    GPBSetInt32IvarWithFieldInternal(context->result, field, val,
                                     context->syntax);
  } else {  // fieldType == GPBFieldTypeMap
    // The exact type doesn't matter, just need to know it is a GPB*Dictionary.
    GPBInt32Int32Dictionary *map =
        GetOrCreateMapIvarWithField(context->result, field, context->syntax);
    [context->stream readMapEntry:map
                extensionRegistry:nil
                            field:field
                    parentMessage:context->result];
  }
  return NO;
}

//%PDDM-EXPAND MERGE_FROM_CODED_INPUT_STREAM_POD_FUNC(SFixed32, int32_t, Int32)
// This block of code is generated, do not edit it directly.

static BOOL DynamicMergeFromCodedInputStreamSFixed32(GPBFieldDescriptor *field,
                                                     void *voidContext) {
  MergeFromCodedInputStreamContext *context =
      (MergeFromCodedInputStreamContext *)voidContext;
  GPBCodedInputStreamState *state = &context->stream->state_;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBInt32Array *array =
        GetOrCreateArrayIvarWithField(context->result, field, context->syntax);
    if (field.isPackable) {
      int32_t length = GPBCodedInputStreamReadInt32(state);
      size_t limit = GPBCodedInputStreamPushLimit(state, length);
      while (GPBCodedInputStreamBytesUntilLimit(state) > 0) {
        int32_t val = GPBCodedInputStreamReadSFixed32(state);
        [array addValue:val];
      }
      GPBCodedInputStreamPopLimit(state, limit);
    } else {
      int32_t val = GPBCodedInputStreamReadSFixed32(state);
      [array addValue:val];
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    int32_t val = GPBCodedInputStreamReadSFixed32(state);
    GPBSetInt32IvarWithFieldInternal(context->result, field, val,
                                     context->syntax);
  } else {  // fieldType == GPBFieldTypeMap
    // The exact type doesn't matter, just need to know it is a GPB*Dictionary.
    GPBInt32Int32Dictionary *map =
        GetOrCreateMapIvarWithField(context->result, field, context->syntax);
    [context->stream readMapEntry:map
                extensionRegistry:nil
                            field:field
                    parentMessage:context->result];
  }
  return NO;
}

//%PDDM-EXPAND MERGE_FROM_CODED_INPUT_STREAM_POD_FUNC(UInt32, uint32_t, UInt32)
// This block of code is generated, do not edit it directly.

static BOOL DynamicMergeFromCodedInputStreamUInt32(GPBFieldDescriptor *field,
                                                   void *voidContext) {
  MergeFromCodedInputStreamContext *context =
      (MergeFromCodedInputStreamContext *)voidContext;
  GPBCodedInputStreamState *state = &context->stream->state_;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBUInt32Array *array =
        GetOrCreateArrayIvarWithField(context->result, field, context->syntax);
    if (field.isPackable) {
      int32_t length = GPBCodedInputStreamReadInt32(state);
      size_t limit = GPBCodedInputStreamPushLimit(state, length);
      while (GPBCodedInputStreamBytesUntilLimit(state) > 0) {
        uint32_t val = GPBCodedInputStreamReadUInt32(state);
        [array addValue:val];
      }
      GPBCodedInputStreamPopLimit(state, limit);
    } else {
      uint32_t val = GPBCodedInputStreamReadUInt32(state);
      [array addValue:val];
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    uint32_t val = GPBCodedInputStreamReadUInt32(state);
    GPBSetUInt32IvarWithFieldInternal(context->result, field, val,
                                      context->syntax);
  } else {  // fieldType == GPBFieldTypeMap
    // The exact type doesn't matter, just need to know it is a GPB*Dictionary.
    GPBInt32Int32Dictionary *map =
        GetOrCreateMapIvarWithField(context->result, field, context->syntax);
    [context->stream readMapEntry:map
                extensionRegistry:nil
                            field:field
                    parentMessage:context->result];
  }
  return NO;
}

//%PDDM-EXPAND MERGE_FROM_CODED_INPUT_STREAM_POD_FUNC(Fixed32, uint32_t, UInt32)
// This block of code is generated, do not edit it directly.

static BOOL DynamicMergeFromCodedInputStreamFixed32(GPBFieldDescriptor *field,
                                                    void *voidContext) {
  MergeFromCodedInputStreamContext *context =
      (MergeFromCodedInputStreamContext *)voidContext;
  GPBCodedInputStreamState *state = &context->stream->state_;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBUInt32Array *array =
        GetOrCreateArrayIvarWithField(context->result, field, context->syntax);
    if (field.isPackable) {
      int32_t length = GPBCodedInputStreamReadInt32(state);
      size_t limit = GPBCodedInputStreamPushLimit(state, length);
      while (GPBCodedInputStreamBytesUntilLimit(state) > 0) {
        uint32_t val = GPBCodedInputStreamReadFixed32(state);
        [array addValue:val];
      }
      GPBCodedInputStreamPopLimit(state, limit);
    } else {
      uint32_t val = GPBCodedInputStreamReadFixed32(state);
      [array addValue:val];
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    uint32_t val = GPBCodedInputStreamReadFixed32(state);
    GPBSetUInt32IvarWithFieldInternal(context->result, field, val,
                                      context->syntax);
  } else {  // fieldType == GPBFieldTypeMap
    // The exact type doesn't matter, just need to know it is a GPB*Dictionary.
    GPBInt32Int32Dictionary *map =
        GetOrCreateMapIvarWithField(context->result, field, context->syntax);
    [context->stream readMapEntry:map
                extensionRegistry:nil
                            field:field
                    parentMessage:context->result];
  }
  return NO;
}

//%PDDM-EXPAND MERGE_FROM_CODED_INPUT_STREAM_POD_FUNC(Int64, int64_t, Int64)
// This block of code is generated, do not edit it directly.

static BOOL DynamicMergeFromCodedInputStreamInt64(GPBFieldDescriptor *field,
                                                  void *voidContext) {
  MergeFromCodedInputStreamContext *context =
      (MergeFromCodedInputStreamContext *)voidContext;
  GPBCodedInputStreamState *state = &context->stream->state_;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBInt64Array *array =
        GetOrCreateArrayIvarWithField(context->result, field, context->syntax);
    if (field.isPackable) {
      int32_t length = GPBCodedInputStreamReadInt32(state);
      size_t limit = GPBCodedInputStreamPushLimit(state, length);
      while (GPBCodedInputStreamBytesUntilLimit(state) > 0) {
        int64_t val = GPBCodedInputStreamReadInt64(state);
        [array addValue:val];
      }
      GPBCodedInputStreamPopLimit(state, limit);
    } else {
      int64_t val = GPBCodedInputStreamReadInt64(state);
      [array addValue:val];
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    int64_t val = GPBCodedInputStreamReadInt64(state);
    GPBSetInt64IvarWithFieldInternal(context->result, field, val,
                                     context->syntax);
  } else {  // fieldType == GPBFieldTypeMap
    // The exact type doesn't matter, just need to know it is a GPB*Dictionary.
    GPBInt32Int32Dictionary *map =
        GetOrCreateMapIvarWithField(context->result, field, context->syntax);
    [context->stream readMapEntry:map
                extensionRegistry:nil
                            field:field
                    parentMessage:context->result];
  }
  return NO;
}

//%PDDM-EXPAND MERGE_FROM_CODED_INPUT_STREAM_POD_FUNC(SFixed64, int64_t, Int64)
// This block of code is generated, do not edit it directly.

static BOOL DynamicMergeFromCodedInputStreamSFixed64(GPBFieldDescriptor *field,
                                                     void *voidContext) {
  MergeFromCodedInputStreamContext *context =
      (MergeFromCodedInputStreamContext *)voidContext;
  GPBCodedInputStreamState *state = &context->stream->state_;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBInt64Array *array =
        GetOrCreateArrayIvarWithField(context->result, field, context->syntax);
    if (field.isPackable) {
      int32_t length = GPBCodedInputStreamReadInt32(state);
      size_t limit = GPBCodedInputStreamPushLimit(state, length);
      while (GPBCodedInputStreamBytesUntilLimit(state) > 0) {
        int64_t val = GPBCodedInputStreamReadSFixed64(state);
        [array addValue:val];
      }
      GPBCodedInputStreamPopLimit(state, limit);
    } else {
      int64_t val = GPBCodedInputStreamReadSFixed64(state);
      [array addValue:val];
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    int64_t val = GPBCodedInputStreamReadSFixed64(state);
    GPBSetInt64IvarWithFieldInternal(context->result, field, val,
                                     context->syntax);
  } else {  // fieldType == GPBFieldTypeMap
    // The exact type doesn't matter, just need to know it is a GPB*Dictionary.
    GPBInt32Int32Dictionary *map =
        GetOrCreateMapIvarWithField(context->result, field, context->syntax);
    [context->stream readMapEntry:map
                extensionRegistry:nil
                            field:field
                    parentMessage:context->result];
  }
  return NO;
}

//%PDDM-EXPAND MERGE_FROM_CODED_INPUT_STREAM_POD_FUNC(SInt64, int64_t, Int64)
// This block of code is generated, do not edit it directly.

static BOOL DynamicMergeFromCodedInputStreamSInt64(GPBFieldDescriptor *field,
                                                   void *voidContext) {
  MergeFromCodedInputStreamContext *context =
      (MergeFromCodedInputStreamContext *)voidContext;
  GPBCodedInputStreamState *state = &context->stream->state_;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBInt64Array *array =
        GetOrCreateArrayIvarWithField(context->result, field, context->syntax);
    if (field.isPackable) {
      int32_t length = GPBCodedInputStreamReadInt32(state);
      size_t limit = GPBCodedInputStreamPushLimit(state, length);
      while (GPBCodedInputStreamBytesUntilLimit(state) > 0) {
        int64_t val = GPBCodedInputStreamReadSInt64(state);
        [array addValue:val];
      }
      GPBCodedInputStreamPopLimit(state, limit);
    } else {
      int64_t val = GPBCodedInputStreamReadSInt64(state);
      [array addValue:val];
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    int64_t val = GPBCodedInputStreamReadSInt64(state);
    GPBSetInt64IvarWithFieldInternal(context->result, field, val,
                                     context->syntax);
  } else {  // fieldType == GPBFieldTypeMap
    // The exact type doesn't matter, just need to know it is a GPB*Dictionary.
    GPBInt32Int32Dictionary *map =
        GetOrCreateMapIvarWithField(context->result, field, context->syntax);
    [context->stream readMapEntry:map
                extensionRegistry:nil
                            field:field
                    parentMessage:context->result];
  }
  return NO;
}

//%PDDM-EXPAND MERGE_FROM_CODED_INPUT_STREAM_POD_FUNC(UInt64, uint64_t, UInt64)
// This block of code is generated, do not edit it directly.

static BOOL DynamicMergeFromCodedInputStreamUInt64(GPBFieldDescriptor *field,
                                                   void *voidContext) {
  MergeFromCodedInputStreamContext *context =
      (MergeFromCodedInputStreamContext *)voidContext;
  GPBCodedInputStreamState *state = &context->stream->state_;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBUInt64Array *array =
        GetOrCreateArrayIvarWithField(context->result, field, context->syntax);
    if (field.isPackable) {
      int32_t length = GPBCodedInputStreamReadInt32(state);
      size_t limit = GPBCodedInputStreamPushLimit(state, length);
      while (GPBCodedInputStreamBytesUntilLimit(state) > 0) {
        uint64_t val = GPBCodedInputStreamReadUInt64(state);
        [array addValue:val];
      }
      GPBCodedInputStreamPopLimit(state, limit);
    } else {
      uint64_t val = GPBCodedInputStreamReadUInt64(state);
      [array addValue:val];
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    uint64_t val = GPBCodedInputStreamReadUInt64(state);
    GPBSetUInt64IvarWithFieldInternal(context->result, field, val,
                                      context->syntax);
  } else {  // fieldType == GPBFieldTypeMap
    // The exact type doesn't matter, just need to know it is a GPB*Dictionary.
    GPBInt32Int32Dictionary *map =
        GetOrCreateMapIvarWithField(context->result, field, context->syntax);
    [context->stream readMapEntry:map
                extensionRegistry:nil
                            field:field
                    parentMessage:context->result];
  }
  return NO;
}

//%PDDM-EXPAND MERGE_FROM_CODED_INPUT_STREAM_POD_FUNC(Fixed64, uint64_t, UInt64)
// This block of code is generated, do not edit it directly.

static BOOL DynamicMergeFromCodedInputStreamFixed64(GPBFieldDescriptor *field,
                                                    void *voidContext) {
  MergeFromCodedInputStreamContext *context =
      (MergeFromCodedInputStreamContext *)voidContext;
  GPBCodedInputStreamState *state = &context->stream->state_;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBUInt64Array *array =
        GetOrCreateArrayIvarWithField(context->result, field, context->syntax);
    if (field.isPackable) {
      int32_t length = GPBCodedInputStreamReadInt32(state);
      size_t limit = GPBCodedInputStreamPushLimit(state, length);
      while (GPBCodedInputStreamBytesUntilLimit(state) > 0) {
        uint64_t val = GPBCodedInputStreamReadFixed64(state);
        [array addValue:val];
      }
      GPBCodedInputStreamPopLimit(state, limit);
    } else {
      uint64_t val = GPBCodedInputStreamReadFixed64(state);
      [array addValue:val];
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    uint64_t val = GPBCodedInputStreamReadFixed64(state);
    GPBSetUInt64IvarWithFieldInternal(context->result, field, val,
                                      context->syntax);
  } else {  // fieldType == GPBFieldTypeMap
    // The exact type doesn't matter, just need to know it is a GPB*Dictionary.
    GPBInt32Int32Dictionary *map =
        GetOrCreateMapIvarWithField(context->result, field, context->syntax);
    [context->stream readMapEntry:map
                extensionRegistry:nil
                            field:field
                    parentMessage:context->result];
  }
  return NO;
}

//%PDDM-EXPAND MERGE_FROM_CODED_INPUT_STREAM_POD_FUNC(Float, float, Float)
// This block of code is generated, do not edit it directly.

static BOOL DynamicMergeFromCodedInputStreamFloat(GPBFieldDescriptor *field,
                                                  void *voidContext) {
  MergeFromCodedInputStreamContext *context =
      (MergeFromCodedInputStreamContext *)voidContext;
  GPBCodedInputStreamState *state = &context->stream->state_;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBFloatArray *array =
        GetOrCreateArrayIvarWithField(context->result, field, context->syntax);
    if (field.isPackable) {
      int32_t length = GPBCodedInputStreamReadInt32(state);
      size_t limit = GPBCodedInputStreamPushLimit(state, length);
      while (GPBCodedInputStreamBytesUntilLimit(state) > 0) {
        float val = GPBCodedInputStreamReadFloat(state);
        [array addValue:val];
      }
      GPBCodedInputStreamPopLimit(state, limit);
    } else {
      float val = GPBCodedInputStreamReadFloat(state);
      [array addValue:val];
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    float val = GPBCodedInputStreamReadFloat(state);
    GPBSetFloatIvarWithFieldInternal(context->result, field, val,
                                     context->syntax);
  } else {  // fieldType == GPBFieldTypeMap
    // The exact type doesn't matter, just need to know it is a GPB*Dictionary.
    GPBInt32Int32Dictionary *map =
        GetOrCreateMapIvarWithField(context->result, field, context->syntax);
    [context->stream readMapEntry:map
                extensionRegistry:nil
                            field:field
                    parentMessage:context->result];
  }
  return NO;
}

//%PDDM-EXPAND MERGE_FROM_CODED_INPUT_STREAM_POD_FUNC(Double, double, Double)
// This block of code is generated, do not edit it directly.

static BOOL DynamicMergeFromCodedInputStreamDouble(GPBFieldDescriptor *field,
                                                   void *voidContext) {
  MergeFromCodedInputStreamContext *context =
      (MergeFromCodedInputStreamContext *)voidContext;
  GPBCodedInputStreamState *state = &context->stream->state_;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBDoubleArray *array =
        GetOrCreateArrayIvarWithField(context->result, field, context->syntax);
    if (field.isPackable) {
      int32_t length = GPBCodedInputStreamReadInt32(state);
      size_t limit = GPBCodedInputStreamPushLimit(state, length);
      while (GPBCodedInputStreamBytesUntilLimit(state) > 0) {
        double val = GPBCodedInputStreamReadDouble(state);
        [array addValue:val];
      }
      GPBCodedInputStreamPopLimit(state, limit);
    } else {
      double val = GPBCodedInputStreamReadDouble(state);
      [array addValue:val];
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    double val = GPBCodedInputStreamReadDouble(state);
    GPBSetDoubleIvarWithFieldInternal(context->result, field, val,
                                      context->syntax);
  } else {  // fieldType == GPBFieldTypeMap
    // The exact type doesn't matter, just need to know it is a GPB*Dictionary.
    GPBInt32Int32Dictionary *map =
        GetOrCreateMapIvarWithField(context->result, field, context->syntax);
    [context->stream readMapEntry:map
                extensionRegistry:nil
                            field:field
                    parentMessage:context->result];
  }
  return NO;
}

//%PDDM-EXPAND MERGE_FROM_CODED_INPUT_STREAM_OBJ_FUNC(String)
// This block of code is generated, do not edit it directly.

static BOOL DynamicMergeFromCodedInputStreamString(GPBFieldDescriptor *field,
                                                   void *voidContext) {
  MergeFromCodedInputStreamContext *context = voidContext;
  GPBCodedInputStreamState *state = &context->stream->state_;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeMap) {
    // GPB*Dictionary or NSDictionary, exact type doesn't matter at this point.
    id map =
        GetOrCreateMapIvarWithField(context->result, field, context->syntax);
    [context->stream readMapEntry:map
                extensionRegistry:nil
                            field:field
                    parentMessage:context->result];
  } else {
    id val = GPBCodedInputStreamReadRetainedString(state);
    if (fieldType == GPBFieldTypeRepeated) {
      NSMutableArray *array =
          GetOrCreateArrayIvarWithField(context->result, field, context->syntax);
      [array addObject:val];
      [val release];
    } else {  // fieldType == GPBFieldTypeSingle
      GPBSetRetainedObjectIvarWithFieldInternal(context->result, field, val,
                                                context->syntax);
    }
  }
  return NO;
}

//%PDDM-EXPAND MERGE_FROM_CODED_INPUT_STREAM_OBJ_FUNC(Data)
// This block of code is generated, do not edit it directly.

static BOOL DynamicMergeFromCodedInputStreamData(GPBFieldDescriptor *field,
                                                 void *voidContext) {
  MergeFromCodedInputStreamContext *context = voidContext;
  GPBCodedInputStreamState *state = &context->stream->state_;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeMap) {
    // GPB*Dictionary or NSDictionary, exact type doesn't matter at this point.
    id map =
        GetOrCreateMapIvarWithField(context->result, field, context->syntax);
    [context->stream readMapEntry:map
                extensionRegistry:nil
                            field:field
                    parentMessage:context->result];
  } else {
    id val = GPBCodedInputStreamReadRetainedData(state);
    if (fieldType == GPBFieldTypeRepeated) {
      NSMutableArray *array =
          GetOrCreateArrayIvarWithField(context->result, field, context->syntax);
      [array addObject:val];
      [val release];
    } else {  // fieldType == GPBFieldTypeSingle
      GPBSetRetainedObjectIvarWithFieldInternal(context->result, field, val,
                                                context->syntax);
    }
  }
  return NO;
}

//%PDDM-EXPAND-END (15 expansions)

- (void)mergeFromCodedInputStream:(GPBCodedInputStream *)input
                extensionRegistry:(GPBExtensionRegistry *)extensionRegistry {
  GPBDescriptor *descriptor = [self descriptor];
  GPBFileSyntax syntax = descriptor.file.syntax;
  MergeFromCodedInputStreamContext context = {
    self, input, self, extensionRegistry, 0, syntax
  };
  GPBApplyStrictFunctions funcs =
      GPBAPPLY_STRICT_FUNCTIONS_INIT(DynamicMergeFromCodedInputStream);
  NSUInteger startingIndex = 0;
  NSArray *fields = descriptor->fields_;
  NSUInteger count = fields.count;
  while (YES) {
    BOOL merged = NO;
    context.tag = GPBCodedInputStreamReadTag(&input->state_);
    for (NSUInteger i = 0; i < count; ++i) {
      if (startingIndex >= count) startingIndex = 0;
      GPBFieldDescriptor *fieldDescriptor = fields[startingIndex];
      if (GPBFieldTag(fieldDescriptor) == context.tag) {
        GPBApplyFunction function = funcs[GPBGetFieldType(fieldDescriptor)];
        function(fieldDescriptor, &context);
        merged = YES;
        break;
      } else {
        startingIndex += 1;
      }
    }
    if (!merged) {
      if (context.tag == 0) {
        // zero signals EOF / limit reached
        return;
      } else {
        if (GPBPreserveUnknownFields(syntax)) {
          if (![self parseUnknownField:input
                     extensionRegistry:extensionRegistry
                                   tag:context.tag]) {
            // it's an endgroup tag
            return;
          }
        } else {
          if (![input skipField:context.tag]) {
            return;
          }
        }
      }
    }
  }
}

#pragma mark - MergeFrom Support

typedef struct MergeFromContext {
  GPBMessage *other;
  GPBMessage *result;
  GPBFileSyntax syntax;
} MergeFromContext;

//%PDDM-DEFINE GPB_MERGE_FROM_FUNC(NAME)
//%static BOOL MergeFrom##NAME(GPBFieldDescriptor *field, void *voidContext) {
//%  MergeFromContext *context = (MergeFromContext *)voidContext;
//%  BOOL otherHas = GPBGetHasIvarField(context->other, field);
//%  if (otherHas) {
//%    GPBSet##NAME##IvarWithFieldInternal(
//%        context->result, field, GPBGet##NAME##IvarWithField(context->other, field),
//%        context->syntax);
//%  }
//%  return YES;
//%}
//%
//%PDDM-EXPAND GPB_MERGE_FROM_FUNC(Bool)
// This block of code is generated, do not edit it directly.

static BOOL MergeFromBool(GPBFieldDescriptor *field, void *voidContext) {
  MergeFromContext *context = (MergeFromContext *)voidContext;
  BOOL otherHas = GPBGetHasIvarField(context->other, field);
  if (otherHas) {
    GPBSetBoolIvarWithFieldInternal(
        context->result, field, GPBGetBoolIvarWithField(context->other, field),
        context->syntax);
  }
  return YES;
}

//%PDDM-EXPAND GPB_MERGE_FROM_FUNC(Int32)
// This block of code is generated, do not edit it directly.

static BOOL MergeFromInt32(GPBFieldDescriptor *field, void *voidContext) {
  MergeFromContext *context = (MergeFromContext *)voidContext;
  BOOL otherHas = GPBGetHasIvarField(context->other, field);
  if (otherHas) {
    GPBSetInt32IvarWithFieldInternal(
        context->result, field, GPBGetInt32IvarWithField(context->other, field),
        context->syntax);
  }
  return YES;
}

//%PDDM-EXPAND GPB_MERGE_FROM_FUNC(UInt32)
// This block of code is generated, do not edit it directly.

static BOOL MergeFromUInt32(GPBFieldDescriptor *field, void *voidContext) {
  MergeFromContext *context = (MergeFromContext *)voidContext;
  BOOL otherHas = GPBGetHasIvarField(context->other, field);
  if (otherHas) {
    GPBSetUInt32IvarWithFieldInternal(
        context->result, field, GPBGetUInt32IvarWithField(context->other, field),
        context->syntax);
  }
  return YES;
}

//%PDDM-EXPAND GPB_MERGE_FROM_FUNC(Int64)
// This block of code is generated, do not edit it directly.

static BOOL MergeFromInt64(GPBFieldDescriptor *field, void *voidContext) {
  MergeFromContext *context = (MergeFromContext *)voidContext;
  BOOL otherHas = GPBGetHasIvarField(context->other, field);
  if (otherHas) {
    GPBSetInt64IvarWithFieldInternal(
        context->result, field, GPBGetInt64IvarWithField(context->other, field),
        context->syntax);
  }
  return YES;
}

//%PDDM-EXPAND GPB_MERGE_FROM_FUNC(UInt64)
// This block of code is generated, do not edit it directly.

static BOOL MergeFromUInt64(GPBFieldDescriptor *field, void *voidContext) {
  MergeFromContext *context = (MergeFromContext *)voidContext;
  BOOL otherHas = GPBGetHasIvarField(context->other, field);
  if (otherHas) {
    GPBSetUInt64IvarWithFieldInternal(
        context->result, field, GPBGetUInt64IvarWithField(context->other, field),
        context->syntax);
  }
  return YES;
}

//%PDDM-EXPAND GPB_MERGE_FROM_FUNC(Float)
// This block of code is generated, do not edit it directly.

static BOOL MergeFromFloat(GPBFieldDescriptor *field, void *voidContext) {
  MergeFromContext *context = (MergeFromContext *)voidContext;
  BOOL otherHas = GPBGetHasIvarField(context->other, field);
  if (otherHas) {
    GPBSetFloatIvarWithFieldInternal(
        context->result, field, GPBGetFloatIvarWithField(context->other, field),
        context->syntax);
  }
  return YES;
}

//%PDDM-EXPAND GPB_MERGE_FROM_FUNC(Double)
// This block of code is generated, do not edit it directly.

static BOOL MergeFromDouble(GPBFieldDescriptor *field, void *voidContext) {
  MergeFromContext *context = (MergeFromContext *)voidContext;
  BOOL otherHas = GPBGetHasIvarField(context->other, field);
  if (otherHas) {
    GPBSetDoubleIvarWithFieldInternal(
        context->result, field, GPBGetDoubleIvarWithField(context->other, field),
        context->syntax);
  }
  return YES;
}

//%PDDM-EXPAND-END (7 expansions)

static BOOL MergeFromObject(GPBFieldDescriptor *field, void *voidContext) {
  MergeFromContext *context = (MergeFromContext *)voidContext;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    // In the case of a list, they need to be appended, and there is no
    // _hasIvar to worry about setting.
    id otherArray =
        GPBGetObjectIvarWithFieldNoAutocreate(context->other, field);
    if (otherArray) {
      GPBType fieldDataType = field->description_->type;
      if (GPBTypeIsObject(fieldDataType)) {
        NSMutableArray *resultArray = GetOrCreateArrayIvarWithField(
            context->result, field, context->syntax);
        [resultArray addObjectsFromArray:otherArray];
      } else if (fieldDataType == GPBTypeEnum) {
        GPBEnumArray *resultArray = GetOrCreateArrayIvarWithField(
            context->result, field, context->syntax);
        [resultArray addRawValuesFromArray:otherArray];
      } else {
        // The array type doesn't matter, that all implment
        // -addValuesFromArray:.
        GPBInt32Array *resultArray = GetOrCreateArrayIvarWithField(
            context->result, field, context->syntax);
        [resultArray addValuesFromArray:otherArray];
      }
    }
    return YES;
  }
  if (fieldType == GPBFieldTypeMap) {
    // In the case of a map, they need to be merged, and there is no
    // _hasIvar to worry about setting.
    id otherDict = GPBGetObjectIvarWithFieldNoAutocreate(context->other, field);
    if (otherDict) {
      GPBType keyType = field.mapKeyType;
      GPBType valueType = field->description_->type;
      if (GPBTypeIsObject(keyType) && GPBTypeIsObject(valueType)) {
        NSMutableDictionary *resultDict = GetOrCreateMapIvarWithField(
            context->result, field, context->syntax);
        [resultDict addEntriesFromDictionary:otherDict];
      } else if (valueType == GPBTypeEnum) {
        // The exact type doesn't matter, just need to know it is a
        // GPB*EnumDictionary.
        GPBInt32EnumDictionary *resultDict = GetOrCreateMapIvarWithField(
            context->result, field, context->syntax);
        [resultDict addRawEntriesFromDictionary:otherDict];
      } else {
        // The exact type doesn't matter, they all implement
        // -addEntriesFromDictionary:.
        GPBInt32Int32Dictionary *resultDict = GetOrCreateMapIvarWithField(
            context->result, field, context->syntax);
        [resultDict addEntriesFromDictionary:otherDict];
      }
    }
    return YES;
  }

  int32_t hasIndex = GPBFieldHasIndex(field);
  uint32_t fieldNumber = GPBFieldNumber(field);
  BOOL otherHas = GPBGetHasIvar(context->other, hasIndex, fieldNumber);
  if (!otherHas) {
    return YES;
  }
  // GPBGetObjectIvarWithFieldNoAutocreate skips the has check, faster.
  id otherVal = GPBGetObjectIvarWithFieldNoAutocreate(context->other, field);
  if (GPBFieldTypeIsMessage(field)) {
    if (GPBGetHasIvar(context->result, hasIndex, fieldNumber)) {
      GPBMessage *message =
          GPBGetObjectIvarWithFieldNoAutocreate(context->result, field);
      [message mergeFrom:otherVal];
    } else {
      GPBMessage *message = [otherVal copy];
      GPBSetRetainedObjectIvarWithFieldInternal(context->result, field, message,
                                                context->syntax);
    }
  } else {
    GPBSetObjectIvarWithFieldInternal(context->result, field, otherVal,
                                      context->syntax);
  }
  return YES;
}

- (void)mergeFrom:(GPBMessage *)other {
  Class selfClass = [self class];
  Class otherClass = [other class];
  if (!([selfClass isSubclassOfClass:otherClass] ||
        [otherClass isSubclassOfClass:selfClass])) {
    [NSException raise:NSInvalidArgumentException
                format:@"Classes must match %@ != %@", selfClass, otherClass];
  }

  GPBApplyFunctions funcs = GPBAPPLY_FUNCTIONS_INIT(MergeFrom);
  GPBFileSyntax syntax = [self descriptor].file.syntax;
  MergeFromContext context = {other, self, syntax};
  GPBApplyFunctionsToMessageFields(&funcs, self, &context);

  // We assume someting got done, and become visible.
  GPBBecomeVisibleToAutocreator(self);

  // Unknown fields.
  if (!unknownFields_) {
    [self setUnknownFields:context.other.unknownFields];
  } else {
    [unknownFields_ mergeUnknownFields:context.other.unknownFields];
  }

  if (other->extensionMap_.count == 0) {
    return;
  }

  if (extensionMap_ == nil) {
    extensionMap_ =
        CloneExtensionMap(other->extensionMap_, NSZoneFromPointer(self));
  } else {
    for (GPBExtensionField *thisField in other->extensionMap_) {
      id otherValue = [other->extensionMap_ objectForKey:thisField];
      id value = [extensionMap_ objectForKey:thisField];
      GPBExtensionDescriptor *thisFieldDescriptor = thisField.descriptor;
      BOOL isMessageExtension = GPBExtensionIsMessage(thisFieldDescriptor);

      if ([thisField isRepeated]) {
        NSMutableArray *list = value;
        if (list == nil) {
          list = [[NSMutableArray alloc] init];
          [extensionMap_ setObject:list forKey:thisField];
          [list release];
        }
        if (isMessageExtension) {
          for (GPBMessage *otherListValue in otherValue) {
            GPBMessage *copiedValue = [otherListValue copy];
            [list addObject:copiedValue];
            [copiedValue release];
          }
        } else {
          [list addObjectsFromArray:otherValue];
        }
      } else {
        if (isMessageExtension) {
          if (value) {
            [(GPBMessage *)value mergeFrom:(GPBMessage *)otherValue];
          } else {
            GPBMessage *copiedValue = [otherValue copy];
            [extensionMap_ setObject:copiedValue forKey:thisField];
            [copiedValue release];
          }
        } else {
          [extensionMap_ setObject:otherValue forKey:thisField];
        }
      }

      if (isMessageExtension && !thisFieldDescriptor.isRepeated) {
        GPBMessage *autocreatedValue =
            [[autocreatedExtensionMap_ objectForKey:thisField] retain];
        // Must remove from the map before calling GPBClearMessageAutocreator()
        // so that GPBClearMessageAutocreator() knows its safe to clear.
        [autocreatedExtensionMap_ removeObjectForKey:thisField];
        GPBClearMessageAutocreator(autocreatedValue);
        [autocreatedValue release];
      }
    }
  }
}

#pragma mark - IsEqual Support

typedef struct IsEqualContext {
  GPBMessage *other;
  GPBMessage *self;
  BOOL outIsEqual;
} IsEqualContext;

// If both self and other "has" a value then compare.
//%PDDM-DEFINE IS_EQUAL_FUNC(NAME, TYPE)
//%static BOOL IsEqual##NAME(GPBFieldDescriptor *field, void *voidContext) {
//%  IsEqualContext *context = (IsEqualContext *)voidContext;
//%  int32_t hasIndex = GPBFieldHasIndex(field);
//%  uint32_t fieldNum = GPBFieldNumber(field);
//%  BOOL selfHas = GPBGetHasIvar(context->self, hasIndex, fieldNum);
//%  BOOL otherHas = GPBGetHasIvar(context->other, hasIndex, fieldNum);
//%  if (selfHas != otherHas) {
//%    context->outIsEqual = NO;
//%    return NO;
//%  }
//%  if (!selfHas) {
//%    return YES;
//%  }
//%  TYPE selfVal = GPBGet##NAME##IvarWithField(context->self, field);
//%  TYPE otherVal = GPBGet##NAME##IvarWithField(context->other, field);
//%  if (selfVal != otherVal) {
//%    context->outIsEqual = NO;
//%    return NO;
//%  }
//%  return YES;
//%}
//%
//%PDDM-EXPAND IS_EQUAL_FUNC(Bool, BOOL)
// This block of code is generated, do not edit it directly.

static BOOL IsEqualBool(GPBFieldDescriptor *field, void *voidContext) {
  IsEqualContext *context = (IsEqualContext *)voidContext;
  int32_t hasIndex = GPBFieldHasIndex(field);
  uint32_t fieldNum = GPBFieldNumber(field);
  BOOL selfHas = GPBGetHasIvar(context->self, hasIndex, fieldNum);
  BOOL otherHas = GPBGetHasIvar(context->other, hasIndex, fieldNum);
  if (selfHas != otherHas) {
    context->outIsEqual = NO;
    return NO;
  }
  if (!selfHas) {
    return YES;
  }
  BOOL selfVal = GPBGetBoolIvarWithField(context->self, field);
  BOOL otherVal = GPBGetBoolIvarWithField(context->other, field);
  if (selfVal != otherVal) {
    context->outIsEqual = NO;
    return NO;
  }
  return YES;
}

//%PDDM-EXPAND IS_EQUAL_FUNC(Int32, int32_t)
// This block of code is generated, do not edit it directly.

static BOOL IsEqualInt32(GPBFieldDescriptor *field, void *voidContext) {
  IsEqualContext *context = (IsEqualContext *)voidContext;
  int32_t hasIndex = GPBFieldHasIndex(field);
  uint32_t fieldNum = GPBFieldNumber(field);
  BOOL selfHas = GPBGetHasIvar(context->self, hasIndex, fieldNum);
  BOOL otherHas = GPBGetHasIvar(context->other, hasIndex, fieldNum);
  if (selfHas != otherHas) {
    context->outIsEqual = NO;
    return NO;
  }
  if (!selfHas) {
    return YES;
  }
  int32_t selfVal = GPBGetInt32IvarWithField(context->self, field);
  int32_t otherVal = GPBGetInt32IvarWithField(context->other, field);
  if (selfVal != otherVal) {
    context->outIsEqual = NO;
    return NO;
  }
  return YES;
}

//%PDDM-EXPAND IS_EQUAL_FUNC(UInt32, uint32_t)
// This block of code is generated, do not edit it directly.

static BOOL IsEqualUInt32(GPBFieldDescriptor *field, void *voidContext) {
  IsEqualContext *context = (IsEqualContext *)voidContext;
  int32_t hasIndex = GPBFieldHasIndex(field);
  uint32_t fieldNum = GPBFieldNumber(field);
  BOOL selfHas = GPBGetHasIvar(context->self, hasIndex, fieldNum);
  BOOL otherHas = GPBGetHasIvar(context->other, hasIndex, fieldNum);
  if (selfHas != otherHas) {
    context->outIsEqual = NO;
    return NO;
  }
  if (!selfHas) {
    return YES;
  }
  uint32_t selfVal = GPBGetUInt32IvarWithField(context->self, field);
  uint32_t otherVal = GPBGetUInt32IvarWithField(context->other, field);
  if (selfVal != otherVal) {
    context->outIsEqual = NO;
    return NO;
  }
  return YES;
}

//%PDDM-EXPAND IS_EQUAL_FUNC(Int64, int64_t)
// This block of code is generated, do not edit it directly.

static BOOL IsEqualInt64(GPBFieldDescriptor *field, void *voidContext) {
  IsEqualContext *context = (IsEqualContext *)voidContext;
  int32_t hasIndex = GPBFieldHasIndex(field);
  uint32_t fieldNum = GPBFieldNumber(field);
  BOOL selfHas = GPBGetHasIvar(context->self, hasIndex, fieldNum);
  BOOL otherHas = GPBGetHasIvar(context->other, hasIndex, fieldNum);
  if (selfHas != otherHas) {
    context->outIsEqual = NO;
    return NO;
  }
  if (!selfHas) {
    return YES;
  }
  int64_t selfVal = GPBGetInt64IvarWithField(context->self, field);
  int64_t otherVal = GPBGetInt64IvarWithField(context->other, field);
  if (selfVal != otherVal) {
    context->outIsEqual = NO;
    return NO;
  }
  return YES;
}

//%PDDM-EXPAND IS_EQUAL_FUNC(UInt64, uint64_t)
// This block of code is generated, do not edit it directly.

static BOOL IsEqualUInt64(GPBFieldDescriptor *field, void *voidContext) {
  IsEqualContext *context = (IsEqualContext *)voidContext;
  int32_t hasIndex = GPBFieldHasIndex(field);
  uint32_t fieldNum = GPBFieldNumber(field);
  BOOL selfHas = GPBGetHasIvar(context->self, hasIndex, fieldNum);
  BOOL otherHas = GPBGetHasIvar(context->other, hasIndex, fieldNum);
  if (selfHas != otherHas) {
    context->outIsEqual = NO;
    return NO;
  }
  if (!selfHas) {
    return YES;
  }
  uint64_t selfVal = GPBGetUInt64IvarWithField(context->self, field);
  uint64_t otherVal = GPBGetUInt64IvarWithField(context->other, field);
  if (selfVal != otherVal) {
    context->outIsEqual = NO;
    return NO;
  }
  return YES;
}

//%PDDM-EXPAND IS_EQUAL_FUNC(Float, float)
// This block of code is generated, do not edit it directly.

static BOOL IsEqualFloat(GPBFieldDescriptor *field, void *voidContext) {
  IsEqualContext *context = (IsEqualContext *)voidContext;
  int32_t hasIndex = GPBFieldHasIndex(field);
  uint32_t fieldNum = GPBFieldNumber(field);
  BOOL selfHas = GPBGetHasIvar(context->self, hasIndex, fieldNum);
  BOOL otherHas = GPBGetHasIvar(context->other, hasIndex, fieldNum);
  if (selfHas != otherHas) {
    context->outIsEqual = NO;
    return NO;
  }
  if (!selfHas) {
    return YES;
  }
  float selfVal = GPBGetFloatIvarWithField(context->self, field);
  float otherVal = GPBGetFloatIvarWithField(context->other, field);
  if (selfVal != otherVal) {
    context->outIsEqual = NO;
    return NO;
  }
  return YES;
}

//%PDDM-EXPAND IS_EQUAL_FUNC(Double, double)
// This block of code is generated, do not edit it directly.

static BOOL IsEqualDouble(GPBFieldDescriptor *field, void *voidContext) {
  IsEqualContext *context = (IsEqualContext *)voidContext;
  int32_t hasIndex = GPBFieldHasIndex(field);
  uint32_t fieldNum = GPBFieldNumber(field);
  BOOL selfHas = GPBGetHasIvar(context->self, hasIndex, fieldNum);
  BOOL otherHas = GPBGetHasIvar(context->other, hasIndex, fieldNum);
  if (selfHas != otherHas) {
    context->outIsEqual = NO;
    return NO;
  }
  if (!selfHas) {
    return YES;
  }
  double selfVal = GPBGetDoubleIvarWithField(context->self, field);
  double otherVal = GPBGetDoubleIvarWithField(context->other, field);
  if (selfVal != otherVal) {
    context->outIsEqual = NO;
    return NO;
  }
  return YES;
}

//%PDDM-EXPAND-END (7 expansions)

static BOOL IsEqualObject(GPBFieldDescriptor *field, void *voidContext) {
  IsEqualContext *context = (IsEqualContext *)voidContext;
  if (GPBFieldIsMapOrArray(field)) {
    // In the case of a list/map, there is no _hasIvar to worry about checking.
    // NOTE: These are NSArray/GPB*Array/NSDictionary/GPB*Dictionary, but the
    // type doesn't really matter as the object just has to support
    // -count/-isEqual:.
    NSArray *resultMapOrArray =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    NSArray *otherMapOrArray =
        GPBGetObjectIvarWithFieldNoAutocreate(context->other, field);
    context->outIsEqual =
        (resultMapOrArray.count == 0 && otherMapOrArray.count == 0) ||
        [resultMapOrArray isEqual:otherMapOrArray];
    return context->outIsEqual;
  }
  int32_t hasIndex = GPBFieldHasIndex(field);
  uint32_t fieldNum = GPBFieldNumber(field);
  BOOL selfHas = GPBGetHasIvar(context->self, hasIndex, fieldNum);
  BOOL otherHas = GPBGetHasIvar(context->other, hasIndex, fieldNum);
  if (selfHas != otherHas) {
    context->outIsEqual = NO;
    return NO;
  }
  if (!selfHas) {
    return YES;
  }
  // GPBGetObjectIvarWithFieldNoAutocreate skips the has check, faster.
  NSObject *selfVal =
      GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
  NSObject *otherVal =
      GPBGetObjectIvarWithFieldNoAutocreate(context->other, field);

  // This covers case where selfVal is set to nil, as well as shortcuts the call
  // to isEqual: in common cases.
  if (selfVal == otherVal) {
    return YES;
  }
  if (![selfVal isEqual:otherVal]) {
    context->outIsEqual = NO;
    return NO;
  }
  return YES;
}

- (BOOL)isEqual:(GPBMessage *)other {
  if (other == self) {
    return YES;
  }
  if (![other isKindOfClass:[self class]] &&
      ![self isKindOfClass:[other class]]) {
    return NO;
  }
  GPBApplyFunctions funcs = GPBAPPLY_FUNCTIONS_INIT(IsEqual);
  IsEqualContext context = {other, self, YES};
  GPBApplyFunctionsToMessageFields(&funcs, self, &context);
  if (!context.outIsEqual) {
    return NO;
  }

  // nil and empty are equal
  if (extensionMap_.count != 0 || other->extensionMap_.count != 0) {
    if (![extensionMap_ isEqual:other->extensionMap_]) {
      return NO;
    }
  }

  // nil and empty are equal
  GPBUnknownFieldSet *otherUnknowns = other.unknownFields;
  if ([unknownFields_ countOfFields] != 0 ||
      [otherUnknowns countOfFields] != 0) {
    if (![unknownFields_ isEqual:otherUnknowns]) {
      return NO;
    }
  }
  return YES;
}

// It is very difficult to implement a generic hash for ProtoBuf messages that
// will perform well. If you need hashing on your ProtoBufs (eg you are using
// them as dictionary keys) you will probably want to implement a ProtoBuf
// message specific hash as a category on your protobuf class. Do not make it a
// category on GPBMessage as you will conflict with this hash, and will possibly
// override hash for all generated protobufs. A good implementation of hash will
// be really fast, so we would recommend only hashing protobufs that have an
// identifier field of some kind that you can easily hash. If you implement
// hash, we would strongly recommend overriding isEqual: in your category as
// well, as the default implementation of isEqual: is extremely slow, and may
// drastically affect performance in large sets.
- (NSUInteger)hash {
  GPBDescriptor *descriptor = [[self class] descriptor];
  const NSUInteger prime = 19;

  // Start with the descriptor and then mix it with the field numbers that
  // are set.  Hopefully that will give a spread based on classes and what
  // fields are set.
  NSUInteger result = (NSUInteger)descriptor;
  for (GPBFieldDescriptor *field in descriptor->fields_) {
    if (GPBFieldIsMapOrArray(field)) {
      // Exact type doesn't matter, just check if there are any elements.
      NSArray *mapOrArray = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
      if (mapOrArray.count) {
        result = prime * result + GPBFieldNumber(field);
      }
    } else if (GPBGetHasIvarField(self, field)) {
      result = prime * result + GPBFieldNumber(field);
    }
  }
  return result;
}

#pragma mark - Description Support

- (NSString *)description {
  NSString *textFormat = GPBTextFormatForMessage(self, @"    ");
  NSString *description = [NSString
      stringWithFormat:@"<%@ %p>: {\n%@}", [self class], self, textFormat];
  return description;
}

#if DEBUG

// Xcode 5.1 added support for custom quick look info.
// https://developer.apple.com/library/ios/documentation/IDEs/Conceptual/CustomClassDisplay_in_QuickLook/CH01-quick_look_for_custom_objects/CH01-quick_look_for_custom_objects.html#//apple_ref/doc/uid/TP40014001-CH2-SW1
- (id)debugQuickLookObject {
  return GPBTextFormatForMessage(self, nil);
}

#endif  // DEBUG

#pragma mark - SerializedSize Support

// Serialized size is only calculated once, and then is stored into
// memoizedSerializedSize.

typedef struct SerializedSizeContext {
  GPBMessage *self;
  size_t outSize;
} SerializedSizeContext;

//%PDDM-DEFINE SERIALIZED_SIZE_POD_FUNC(NAME, TYPE, REAL_TYPE)
//%SERIALIZED_SIZE_POD_FUNC_FULL(NAME, TYPE, REAL_TYPE, REAL_TYPE, )
//%PDDM-DEFINE SERIALIZED_SIZE_POD_FUNC_FULL(NAME, TYPE, REAL_TYPE, ARRAY_TYPE, ARRAY_ACCESSOR_NAME)
//%static BOOL DynamicSerializedSize##NAME(GPBFieldDescriptor *field,
//%                                NAME$S  void *voidContext) {
//%  SerializedSizeContext *context = (SerializedSizeContext *)voidContext;
//%  GPBFieldType fieldType = field.fieldType;
//%  if (fieldType == GPBFieldTypeRepeated) {
//%    GPB##ARRAY_TYPE##Array *array =
//%        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
//%    NSUInteger count = array.count;
//%    if (count == 0) return YES;
//%    __block size_t dataSize = 0;
//%    [array enumerate##ARRAY_ACCESSOR_NAME##ValuesWithBlock:^(TYPE value, NSUInteger idx, BOOL *stop) {
//%      #pragma unused(idx, stop)
//%      dataSize += GPBCompute##NAME##SizeNoTag(value);
//%    }];
//%    context->outSize += dataSize;
//%    size_t tagSize = GPBComputeTagSize(GPBFieldNumber(field));
//%    if (field.isPackable) {
//%      context->outSize += tagSize;
//%      context->outSize += GPBComputeSizeTSizeAsInt32NoTag(dataSize);
//%    } else {
//%      context->outSize += count * tagSize;
//%    }
//%  } else if (fieldType == GPBFieldTypeSingle) {
//%    BOOL selfHas = GPBGetHasIvarField(context->self, field);
//%    if (selfHas) {
//%      TYPE selfVal = GPBGet##REAL_TYPE##IvarWithField(context->self, field);
//%      context->outSize += GPBCompute##NAME##Size(GPBFieldNumber(field), selfVal);
//%    }
//%  } else {  // fieldType == GPBFieldTypeMap
//%    // Type will be GPB*##REAL_TYPE##Dictionary, exact type doesn't matter.
//%    GPBInt32##REAL_TYPE##Dictionary *map =
//%        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
//%    context->outSize += [map computeSerializedSizeAsField:field];
//%  }
//%  return YES;
//%}
//%
//%PDDM-DEFINE SERIALIZED_SIZE_OBJECT_FUNC(NAME)
//%static BOOL DynamicSerializedSize##NAME(GPBFieldDescriptor *field,
//%                                NAME$S  void *voidContext) {
//%  SerializedSizeContext *context = (SerializedSizeContext *)voidContext;
//%  GPBFieldType fieldType = field.fieldType;
//%  if (fieldType == GPBFieldTypeRepeated) {
//%    NSArray *array =
//%        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
//%    for (id value in array) {
//%      context->outSize += GPBCompute##NAME##SizeNoTag(value);
//%    }
//%    size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field),
//%                                                 GPBGetFieldType(field));
//%    context->outSize += array.count * tagSize;
//%  } else if (fieldType == GPBFieldTypeSingle) {
//%    BOOL selfHas = GPBGetHasIvarField(context->self, field);
//%    if (selfHas) {
//%      // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
//%      // again.
//%      id selfVal = GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
//%      context->outSize += GPBCompute##NAME##Size(GPBFieldNumber(field), selfVal);
//%    }
//%  } else {  // fieldType == GPBFieldTypeMap
//%    GPBType mapKeyType = field.mapKeyType;
//%    if (mapKeyType == GPBTypeString) {
//%      // If key type was string, then the map is an NSDictionary.
//%      NSDictionary *map =
//%          GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
//%      context->outSize += GPBDictionaryComputeSizeInternalHelper(map, field);
//%    } else {
//%      // Type will be GPB*##NAME##Dictionary, exact type doesn't matter.
//%      GPBInt32ObjectDictionary *map =
//%          GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
//%      context->outSize += [map computeSerializedSizeAsField:field];
//%    }
//%  }
//%  return YES;
//%}
//%
//%PDDM-EXPAND SERIALIZED_SIZE_POD_FUNC(Bool, BOOL, Bool)
// This block of code is generated, do not edit it directly.

static BOOL DynamicSerializedSizeBool(GPBFieldDescriptor *field,
                                      void *voidContext) {
  SerializedSizeContext *context = (SerializedSizeContext *)voidContext;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBBoolArray *array =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    NSUInteger count = array.count;
    if (count == 0) return YES;
    __block size_t dataSize = 0;
    [array enumerateValuesWithBlock:^(BOOL value, NSUInteger idx, BOOL *stop) {
      #pragma unused(idx, stop)
      dataSize += GPBComputeBoolSizeNoTag(value);
    }];
    context->outSize += dataSize;
    size_t tagSize = GPBComputeTagSize(GPBFieldNumber(field));
    if (field.isPackable) {
      context->outSize += tagSize;
      context->outSize += GPBComputeSizeTSizeAsInt32NoTag(dataSize);
    } else {
      context->outSize += count * tagSize;
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL selfHas = GPBGetHasIvarField(context->self, field);
    if (selfHas) {
      BOOL selfVal = GPBGetBoolIvarWithField(context->self, field);
      context->outSize += GPBComputeBoolSize(GPBFieldNumber(field), selfVal);
    }
  } else {  // fieldType == GPBFieldTypeMap
    // Type will be GPB*BoolDictionary, exact type doesn't matter.
    GPBInt32BoolDictionary *map =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    context->outSize += [map computeSerializedSizeAsField:field];
  }
  return YES;
}

//%PDDM-EXPAND SERIALIZED_SIZE_POD_FUNC(Int32, int32_t, Int32)
// This block of code is generated, do not edit it directly.

static BOOL DynamicSerializedSizeInt32(GPBFieldDescriptor *field,
                                       void *voidContext) {
  SerializedSizeContext *context = (SerializedSizeContext *)voidContext;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBInt32Array *array =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    NSUInteger count = array.count;
    if (count == 0) return YES;
    __block size_t dataSize = 0;
    [array enumerateValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
      #pragma unused(idx, stop)
      dataSize += GPBComputeInt32SizeNoTag(value);
    }];
    context->outSize += dataSize;
    size_t tagSize = GPBComputeTagSize(GPBFieldNumber(field));
    if (field.isPackable) {
      context->outSize += tagSize;
      context->outSize += GPBComputeSizeTSizeAsInt32NoTag(dataSize);
    } else {
      context->outSize += count * tagSize;
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL selfHas = GPBGetHasIvarField(context->self, field);
    if (selfHas) {
      int32_t selfVal = GPBGetInt32IvarWithField(context->self, field);
      context->outSize += GPBComputeInt32Size(GPBFieldNumber(field), selfVal);
    }
  } else {  // fieldType == GPBFieldTypeMap
    // Type will be GPB*Int32Dictionary, exact type doesn't matter.
    GPBInt32Int32Dictionary *map =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    context->outSize += [map computeSerializedSizeAsField:field];
  }
  return YES;
}

//%PDDM-EXPAND SERIALIZED_SIZE_POD_FUNC(SInt32, int32_t, Int32)
// This block of code is generated, do not edit it directly.

static BOOL DynamicSerializedSizeSInt32(GPBFieldDescriptor *field,
                                        void *voidContext) {
  SerializedSizeContext *context = (SerializedSizeContext *)voidContext;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBInt32Array *array =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    NSUInteger count = array.count;
    if (count == 0) return YES;
    __block size_t dataSize = 0;
    [array enumerateValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
      #pragma unused(idx, stop)
      dataSize += GPBComputeSInt32SizeNoTag(value);
    }];
    context->outSize += dataSize;
    size_t tagSize = GPBComputeTagSize(GPBFieldNumber(field));
    if (field.isPackable) {
      context->outSize += tagSize;
      context->outSize += GPBComputeSizeTSizeAsInt32NoTag(dataSize);
    } else {
      context->outSize += count * tagSize;
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL selfHas = GPBGetHasIvarField(context->self, field);
    if (selfHas) {
      int32_t selfVal = GPBGetInt32IvarWithField(context->self, field);
      context->outSize += GPBComputeSInt32Size(GPBFieldNumber(field), selfVal);
    }
  } else {  // fieldType == GPBFieldTypeMap
    // Type will be GPB*Int32Dictionary, exact type doesn't matter.
    GPBInt32Int32Dictionary *map =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    context->outSize += [map computeSerializedSizeAsField:field];
  }
  return YES;
}

//%PDDM-EXPAND SERIALIZED_SIZE_POD_FUNC(SFixed32, int32_t, Int32)
// This block of code is generated, do not edit it directly.

static BOOL DynamicSerializedSizeSFixed32(GPBFieldDescriptor *field,
                                          void *voidContext) {
  SerializedSizeContext *context = (SerializedSizeContext *)voidContext;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBInt32Array *array =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    NSUInteger count = array.count;
    if (count == 0) return YES;
    __block size_t dataSize = 0;
    [array enumerateValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
      #pragma unused(idx, stop)
      dataSize += GPBComputeSFixed32SizeNoTag(value);
    }];
    context->outSize += dataSize;
    size_t tagSize = GPBComputeTagSize(GPBFieldNumber(field));
    if (field.isPackable) {
      context->outSize += tagSize;
      context->outSize += GPBComputeSizeTSizeAsInt32NoTag(dataSize);
    } else {
      context->outSize += count * tagSize;
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL selfHas = GPBGetHasIvarField(context->self, field);
    if (selfHas) {
      int32_t selfVal = GPBGetInt32IvarWithField(context->self, field);
      context->outSize += GPBComputeSFixed32Size(GPBFieldNumber(field), selfVal);
    }
  } else {  // fieldType == GPBFieldTypeMap
    // Type will be GPB*Int32Dictionary, exact type doesn't matter.
    GPBInt32Int32Dictionary *map =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    context->outSize += [map computeSerializedSizeAsField:field];
  }
  return YES;
}

//%PDDM-EXPAND SERIALIZED_SIZE_POD_FUNC_FULL(Enum, int32_t, Int32, Enum, Raw)
// This block of code is generated, do not edit it directly.

static BOOL DynamicSerializedSizeEnum(GPBFieldDescriptor *field,
                                      void *voidContext) {
  SerializedSizeContext *context = (SerializedSizeContext *)voidContext;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBEnumArray *array =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    NSUInteger count = array.count;
    if (count == 0) return YES;
    __block size_t dataSize = 0;
    [array enumerateRawValuesWithBlock:^(int32_t value, NSUInteger idx, BOOL *stop) {
      #pragma unused(idx, stop)
      dataSize += GPBComputeEnumSizeNoTag(value);
    }];
    context->outSize += dataSize;
    size_t tagSize = GPBComputeTagSize(GPBFieldNumber(field));
    if (field.isPackable) {
      context->outSize += tagSize;
      context->outSize += GPBComputeSizeTSizeAsInt32NoTag(dataSize);
    } else {
      context->outSize += count * tagSize;
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL selfHas = GPBGetHasIvarField(context->self, field);
    if (selfHas) {
      int32_t selfVal = GPBGetInt32IvarWithField(context->self, field);
      context->outSize += GPBComputeEnumSize(GPBFieldNumber(field), selfVal);
    }
  } else {  // fieldType == GPBFieldTypeMap
    // Type will be GPB*Int32Dictionary, exact type doesn't matter.
    GPBInt32Int32Dictionary *map =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    context->outSize += [map computeSerializedSizeAsField:field];
  }
  return YES;
}

//%PDDM-EXPAND SERIALIZED_SIZE_POD_FUNC(UInt32, uint32_t, UInt32)
// This block of code is generated, do not edit it directly.

static BOOL DynamicSerializedSizeUInt32(GPBFieldDescriptor *field,
                                        void *voidContext) {
  SerializedSizeContext *context = (SerializedSizeContext *)voidContext;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBUInt32Array *array =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    NSUInteger count = array.count;
    if (count == 0) return YES;
    __block size_t dataSize = 0;
    [array enumerateValuesWithBlock:^(uint32_t value, NSUInteger idx, BOOL *stop) {
      #pragma unused(idx, stop)
      dataSize += GPBComputeUInt32SizeNoTag(value);
    }];
    context->outSize += dataSize;
    size_t tagSize = GPBComputeTagSize(GPBFieldNumber(field));
    if (field.isPackable) {
      context->outSize += tagSize;
      context->outSize += GPBComputeSizeTSizeAsInt32NoTag(dataSize);
    } else {
      context->outSize += count * tagSize;
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL selfHas = GPBGetHasIvarField(context->self, field);
    if (selfHas) {
      uint32_t selfVal = GPBGetUInt32IvarWithField(context->self, field);
      context->outSize += GPBComputeUInt32Size(GPBFieldNumber(field), selfVal);
    }
  } else {  // fieldType == GPBFieldTypeMap
    // Type will be GPB*UInt32Dictionary, exact type doesn't matter.
    GPBInt32UInt32Dictionary *map =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    context->outSize += [map computeSerializedSizeAsField:field];
  }
  return YES;
}

//%PDDM-EXPAND SERIALIZED_SIZE_POD_FUNC(Fixed32, uint32_t, UInt32)
// This block of code is generated, do not edit it directly.

static BOOL DynamicSerializedSizeFixed32(GPBFieldDescriptor *field,
                                         void *voidContext) {
  SerializedSizeContext *context = (SerializedSizeContext *)voidContext;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBUInt32Array *array =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    NSUInteger count = array.count;
    if (count == 0) return YES;
    __block size_t dataSize = 0;
    [array enumerateValuesWithBlock:^(uint32_t value, NSUInteger idx, BOOL *stop) {
      #pragma unused(idx, stop)
      dataSize += GPBComputeFixed32SizeNoTag(value);
    }];
    context->outSize += dataSize;
    size_t tagSize = GPBComputeTagSize(GPBFieldNumber(field));
    if (field.isPackable) {
      context->outSize += tagSize;
      context->outSize += GPBComputeSizeTSizeAsInt32NoTag(dataSize);
    } else {
      context->outSize += count * tagSize;
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL selfHas = GPBGetHasIvarField(context->self, field);
    if (selfHas) {
      uint32_t selfVal = GPBGetUInt32IvarWithField(context->self, field);
      context->outSize += GPBComputeFixed32Size(GPBFieldNumber(field), selfVal);
    }
  } else {  // fieldType == GPBFieldTypeMap
    // Type will be GPB*UInt32Dictionary, exact type doesn't matter.
    GPBInt32UInt32Dictionary *map =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    context->outSize += [map computeSerializedSizeAsField:field];
  }
  return YES;
}

//%PDDM-EXPAND SERIALIZED_SIZE_POD_FUNC(Int64, int64_t, Int64)
// This block of code is generated, do not edit it directly.

static BOOL DynamicSerializedSizeInt64(GPBFieldDescriptor *field,
                                       void *voidContext) {
  SerializedSizeContext *context = (SerializedSizeContext *)voidContext;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBInt64Array *array =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    NSUInteger count = array.count;
    if (count == 0) return YES;
    __block size_t dataSize = 0;
    [array enumerateValuesWithBlock:^(int64_t value, NSUInteger idx, BOOL *stop) {
      #pragma unused(idx, stop)
      dataSize += GPBComputeInt64SizeNoTag(value);
    }];
    context->outSize += dataSize;
    size_t tagSize = GPBComputeTagSize(GPBFieldNumber(field));
    if (field.isPackable) {
      context->outSize += tagSize;
      context->outSize += GPBComputeSizeTSizeAsInt32NoTag(dataSize);
    } else {
      context->outSize += count * tagSize;
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL selfHas = GPBGetHasIvarField(context->self, field);
    if (selfHas) {
      int64_t selfVal = GPBGetInt64IvarWithField(context->self, field);
      context->outSize += GPBComputeInt64Size(GPBFieldNumber(field), selfVal);
    }
  } else {  // fieldType == GPBFieldTypeMap
    // Type will be GPB*Int64Dictionary, exact type doesn't matter.
    GPBInt32Int64Dictionary *map =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    context->outSize += [map computeSerializedSizeAsField:field];
  }
  return YES;
}

//%PDDM-EXPAND SERIALIZED_SIZE_POD_FUNC(SFixed64, int64_t, Int64)
// This block of code is generated, do not edit it directly.

static BOOL DynamicSerializedSizeSFixed64(GPBFieldDescriptor *field,
                                          void *voidContext) {
  SerializedSizeContext *context = (SerializedSizeContext *)voidContext;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBInt64Array *array =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    NSUInteger count = array.count;
    if (count == 0) return YES;
    __block size_t dataSize = 0;
    [array enumerateValuesWithBlock:^(int64_t value, NSUInteger idx, BOOL *stop) {
      #pragma unused(idx, stop)
      dataSize += GPBComputeSFixed64SizeNoTag(value);
    }];
    context->outSize += dataSize;
    size_t tagSize = GPBComputeTagSize(GPBFieldNumber(field));
    if (field.isPackable) {
      context->outSize += tagSize;
      context->outSize += GPBComputeSizeTSizeAsInt32NoTag(dataSize);
    } else {
      context->outSize += count * tagSize;
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL selfHas = GPBGetHasIvarField(context->self, field);
    if (selfHas) {
      int64_t selfVal = GPBGetInt64IvarWithField(context->self, field);
      context->outSize += GPBComputeSFixed64Size(GPBFieldNumber(field), selfVal);
    }
  } else {  // fieldType == GPBFieldTypeMap
    // Type will be GPB*Int64Dictionary, exact type doesn't matter.
    GPBInt32Int64Dictionary *map =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    context->outSize += [map computeSerializedSizeAsField:field];
  }
  return YES;
}

//%PDDM-EXPAND SERIALIZED_SIZE_POD_FUNC(SInt64, int64_t, Int64)
// This block of code is generated, do not edit it directly.

static BOOL DynamicSerializedSizeSInt64(GPBFieldDescriptor *field,
                                        void *voidContext) {
  SerializedSizeContext *context = (SerializedSizeContext *)voidContext;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBInt64Array *array =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    NSUInteger count = array.count;
    if (count == 0) return YES;
    __block size_t dataSize = 0;
    [array enumerateValuesWithBlock:^(int64_t value, NSUInteger idx, BOOL *stop) {
      #pragma unused(idx, stop)
      dataSize += GPBComputeSInt64SizeNoTag(value);
    }];
    context->outSize += dataSize;
    size_t tagSize = GPBComputeTagSize(GPBFieldNumber(field));
    if (field.isPackable) {
      context->outSize += tagSize;
      context->outSize += GPBComputeSizeTSizeAsInt32NoTag(dataSize);
    } else {
      context->outSize += count * tagSize;
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL selfHas = GPBGetHasIvarField(context->self, field);
    if (selfHas) {
      int64_t selfVal = GPBGetInt64IvarWithField(context->self, field);
      context->outSize += GPBComputeSInt64Size(GPBFieldNumber(field), selfVal);
    }
  } else {  // fieldType == GPBFieldTypeMap
    // Type will be GPB*Int64Dictionary, exact type doesn't matter.
    GPBInt32Int64Dictionary *map =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    context->outSize += [map computeSerializedSizeAsField:field];
  }
  return YES;
}

//%PDDM-EXPAND SERIALIZED_SIZE_POD_FUNC(UInt64, uint64_t, UInt64)
// This block of code is generated, do not edit it directly.

static BOOL DynamicSerializedSizeUInt64(GPBFieldDescriptor *field,
                                        void *voidContext) {
  SerializedSizeContext *context = (SerializedSizeContext *)voidContext;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBUInt64Array *array =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    NSUInteger count = array.count;
    if (count == 0) return YES;
    __block size_t dataSize = 0;
    [array enumerateValuesWithBlock:^(uint64_t value, NSUInteger idx, BOOL *stop) {
      #pragma unused(idx, stop)
      dataSize += GPBComputeUInt64SizeNoTag(value);
    }];
    context->outSize += dataSize;
    size_t tagSize = GPBComputeTagSize(GPBFieldNumber(field));
    if (field.isPackable) {
      context->outSize += tagSize;
      context->outSize += GPBComputeSizeTSizeAsInt32NoTag(dataSize);
    } else {
      context->outSize += count * tagSize;
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL selfHas = GPBGetHasIvarField(context->self, field);
    if (selfHas) {
      uint64_t selfVal = GPBGetUInt64IvarWithField(context->self, field);
      context->outSize += GPBComputeUInt64Size(GPBFieldNumber(field), selfVal);
    }
  } else {  // fieldType == GPBFieldTypeMap
    // Type will be GPB*UInt64Dictionary, exact type doesn't matter.
    GPBInt32UInt64Dictionary *map =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    context->outSize += [map computeSerializedSizeAsField:field];
  }
  return YES;
}

//%PDDM-EXPAND SERIALIZED_SIZE_POD_FUNC(Fixed64, uint64_t, UInt64)
// This block of code is generated, do not edit it directly.

static BOOL DynamicSerializedSizeFixed64(GPBFieldDescriptor *field,
                                         void *voidContext) {
  SerializedSizeContext *context = (SerializedSizeContext *)voidContext;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBUInt64Array *array =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    NSUInteger count = array.count;
    if (count == 0) return YES;
    __block size_t dataSize = 0;
    [array enumerateValuesWithBlock:^(uint64_t value, NSUInteger idx, BOOL *stop) {
      #pragma unused(idx, stop)
      dataSize += GPBComputeFixed64SizeNoTag(value);
    }];
    context->outSize += dataSize;
    size_t tagSize = GPBComputeTagSize(GPBFieldNumber(field));
    if (field.isPackable) {
      context->outSize += tagSize;
      context->outSize += GPBComputeSizeTSizeAsInt32NoTag(dataSize);
    } else {
      context->outSize += count * tagSize;
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL selfHas = GPBGetHasIvarField(context->self, field);
    if (selfHas) {
      uint64_t selfVal = GPBGetUInt64IvarWithField(context->self, field);
      context->outSize += GPBComputeFixed64Size(GPBFieldNumber(field), selfVal);
    }
  } else {  // fieldType == GPBFieldTypeMap
    // Type will be GPB*UInt64Dictionary, exact type doesn't matter.
    GPBInt32UInt64Dictionary *map =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    context->outSize += [map computeSerializedSizeAsField:field];
  }
  return YES;
}

//%PDDM-EXPAND SERIALIZED_SIZE_POD_FUNC(Float, float, Float)
// This block of code is generated, do not edit it directly.

static BOOL DynamicSerializedSizeFloat(GPBFieldDescriptor *field,
                                       void *voidContext) {
  SerializedSizeContext *context = (SerializedSizeContext *)voidContext;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBFloatArray *array =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    NSUInteger count = array.count;
    if (count == 0) return YES;
    __block size_t dataSize = 0;
    [array enumerateValuesWithBlock:^(float value, NSUInteger idx, BOOL *stop) {
      #pragma unused(idx, stop)
      dataSize += GPBComputeFloatSizeNoTag(value);
    }];
    context->outSize += dataSize;
    size_t tagSize = GPBComputeTagSize(GPBFieldNumber(field));
    if (field.isPackable) {
      context->outSize += tagSize;
      context->outSize += GPBComputeSizeTSizeAsInt32NoTag(dataSize);
    } else {
      context->outSize += count * tagSize;
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL selfHas = GPBGetHasIvarField(context->self, field);
    if (selfHas) {
      float selfVal = GPBGetFloatIvarWithField(context->self, field);
      context->outSize += GPBComputeFloatSize(GPBFieldNumber(field), selfVal);
    }
  } else {  // fieldType == GPBFieldTypeMap
    // Type will be GPB*FloatDictionary, exact type doesn't matter.
    GPBInt32FloatDictionary *map =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    context->outSize += [map computeSerializedSizeAsField:field];
  }
  return YES;
}

//%PDDM-EXPAND SERIALIZED_SIZE_POD_FUNC(Double, double, Double)
// This block of code is generated, do not edit it directly.

static BOOL DynamicSerializedSizeDouble(GPBFieldDescriptor *field,
                                        void *voidContext) {
  SerializedSizeContext *context = (SerializedSizeContext *)voidContext;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    GPBDoubleArray *array =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    NSUInteger count = array.count;
    if (count == 0) return YES;
    __block size_t dataSize = 0;
    [array enumerateValuesWithBlock:^(double value, NSUInteger idx, BOOL *stop) {
      #pragma unused(idx, stop)
      dataSize += GPBComputeDoubleSizeNoTag(value);
    }];
    context->outSize += dataSize;
    size_t tagSize = GPBComputeTagSize(GPBFieldNumber(field));
    if (field.isPackable) {
      context->outSize += tagSize;
      context->outSize += GPBComputeSizeTSizeAsInt32NoTag(dataSize);
    } else {
      context->outSize += count * tagSize;
    }
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL selfHas = GPBGetHasIvarField(context->self, field);
    if (selfHas) {
      double selfVal = GPBGetDoubleIvarWithField(context->self, field);
      context->outSize += GPBComputeDoubleSize(GPBFieldNumber(field), selfVal);
    }
  } else {  // fieldType == GPBFieldTypeMap
    // Type will be GPB*DoubleDictionary, exact type doesn't matter.
    GPBInt32DoubleDictionary *map =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    context->outSize += [map computeSerializedSizeAsField:field];
  }
  return YES;
}

//%PDDM-EXPAND SERIALIZED_SIZE_OBJECT_FUNC(String)
// This block of code is generated, do not edit it directly.

static BOOL DynamicSerializedSizeString(GPBFieldDescriptor *field,
                                        void *voidContext) {
  SerializedSizeContext *context = (SerializedSizeContext *)voidContext;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    NSArray *array =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    for (id value in array) {
      context->outSize += GPBComputeStringSizeNoTag(value);
    }
    size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field),
                                                 GPBGetFieldType(field));
    context->outSize += array.count * tagSize;
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL selfHas = GPBGetHasIvarField(context->self, field);
    if (selfHas) {
      // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
      // again.
      id selfVal = GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
      context->outSize += GPBComputeStringSize(GPBFieldNumber(field), selfVal);
    }
  } else {  // fieldType == GPBFieldTypeMap
    GPBType mapKeyType = field.mapKeyType;
    if (mapKeyType == GPBTypeString) {
      // If key type was string, then the map is an NSDictionary.
      NSDictionary *map =
          GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
      context->outSize += GPBDictionaryComputeSizeInternalHelper(map, field);
    } else {
      // Type will be GPB*StringDictionary, exact type doesn't matter.
      GPBInt32ObjectDictionary *map =
          GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
      context->outSize += [map computeSerializedSizeAsField:field];
    }
  }
  return YES;
}

//%PDDM-EXPAND SERIALIZED_SIZE_OBJECT_FUNC(Data)
// This block of code is generated, do not edit it directly.

static BOOL DynamicSerializedSizeData(GPBFieldDescriptor *field,
                                      void *voidContext) {
  SerializedSizeContext *context = (SerializedSizeContext *)voidContext;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    NSArray *array =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    for (id value in array) {
      context->outSize += GPBComputeDataSizeNoTag(value);
    }
    size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field),
                                                 GPBGetFieldType(field));
    context->outSize += array.count * tagSize;
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL selfHas = GPBGetHasIvarField(context->self, field);
    if (selfHas) {
      // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
      // again.
      id selfVal = GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
      context->outSize += GPBComputeDataSize(GPBFieldNumber(field), selfVal);
    }
  } else {  // fieldType == GPBFieldTypeMap
    GPBType mapKeyType = field.mapKeyType;
    if (mapKeyType == GPBTypeString) {
      // If key type was string, then the map is an NSDictionary.
      NSDictionary *map =
          GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
      context->outSize += GPBDictionaryComputeSizeInternalHelper(map, field);
    } else {
      // Type will be GPB*DataDictionary, exact type doesn't matter.
      GPBInt32ObjectDictionary *map =
          GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
      context->outSize += [map computeSerializedSizeAsField:field];
    }
  }
  return YES;
}

//%PDDM-EXPAND SERIALIZED_SIZE_OBJECT_FUNC(Message)
// This block of code is generated, do not edit it directly.

static BOOL DynamicSerializedSizeMessage(GPBFieldDescriptor *field,
                                         void *voidContext) {
  SerializedSizeContext *context = (SerializedSizeContext *)voidContext;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    NSArray *array =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    for (id value in array) {
      context->outSize += GPBComputeMessageSizeNoTag(value);
    }
    size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field),
                                                 GPBGetFieldType(field));
    context->outSize += array.count * tagSize;
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL selfHas = GPBGetHasIvarField(context->self, field);
    if (selfHas) {
      // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
      // again.
      id selfVal = GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
      context->outSize += GPBComputeMessageSize(GPBFieldNumber(field), selfVal);
    }
  } else {  // fieldType == GPBFieldTypeMap
    GPBType mapKeyType = field.mapKeyType;
    if (mapKeyType == GPBTypeString) {
      // If key type was string, then the map is an NSDictionary.
      NSDictionary *map =
          GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
      context->outSize += GPBDictionaryComputeSizeInternalHelper(map, field);
    } else {
      // Type will be GPB*MessageDictionary, exact type doesn't matter.
      GPBInt32ObjectDictionary *map =
          GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
      context->outSize += [map computeSerializedSizeAsField:field];
    }
  }
  return YES;
}

//%PDDM-EXPAND SERIALIZED_SIZE_OBJECT_FUNC(Group)
// This block of code is generated, do not edit it directly.

static BOOL DynamicSerializedSizeGroup(GPBFieldDescriptor *field,
                                       void *voidContext) {
  SerializedSizeContext *context = (SerializedSizeContext *)voidContext;
  GPBFieldType fieldType = field.fieldType;
  if (fieldType == GPBFieldTypeRepeated) {
    NSArray *array =
        GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
    for (id value in array) {
      context->outSize += GPBComputeGroupSizeNoTag(value);
    }
    size_t tagSize = GPBComputeWireFormatTagSize(GPBFieldNumber(field),
                                                 GPBGetFieldType(field));
    context->outSize += array.count * tagSize;
  } else if (fieldType == GPBFieldTypeSingle) {
    BOOL selfHas = GPBGetHasIvarField(context->self, field);
    if (selfHas) {
      // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
      // again.
      id selfVal = GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
      context->outSize += GPBComputeGroupSize(GPBFieldNumber(field), selfVal);
    }
  } else {  // fieldType == GPBFieldTypeMap
    GPBType mapKeyType = field.mapKeyType;
    if (mapKeyType == GPBTypeString) {
      // If key type was string, then the map is an NSDictionary.
      NSDictionary *map =
          GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
      context->outSize += GPBDictionaryComputeSizeInternalHelper(map, field);
    } else {
      // Type will be GPB*GroupDictionary, exact type doesn't matter.
      GPBInt32ObjectDictionary *map =
          GPBGetObjectIvarWithFieldNoAutocreate(context->self, field);
      context->outSize += [map computeSerializedSizeAsField:field];
    }
  }
  return YES;
}

//%PDDM-EXPAND-END (18 expansions)

- (size_t)serializedSize {
  // Fields.
  SerializedSizeContext context = {self, 0};
  GPBApplyStrictFunctions funcs =
      GPBAPPLY_STRICT_FUNCTIONS_INIT(DynamicSerializedSize);
  GPBApplyStrictFunctionsToMessageFields(&funcs, self, &context);
  // Add any unknown fields.
  const GPBDescriptor *descriptor = [self descriptor];
  if (descriptor.wireFormat) {
    context.outSize += [unknownFields_ serializedSizeAsMessageSet];
  } else {
    context.outSize += [unknownFields_ serializedSize];
  }
  // Add any extensions.
  for (GPBExtensionField *extension in extensionMap_) {
    id value = [extensionMap_ objectForKey:extension];
    context.outSize += [extension computeSerializedSizeIncludingTag:value];
  }

  return context.outSize;
}

#pragma mark - Resolve Methods Support

typedef struct IvarAccessorMethodContext {
  GPBFileSyntax syntax;
  IMP impToAdd;
  SEL encodingSelector;
} IvarAccessorMethodContext;

//%PDDM-DEFINE IVAR_ACCESSOR_FUNC_COMMON(NAME, TYPE, TRUE_NAME)
//%static BOOL IvarGet##NAME(GPBFieldDescriptor *field, void *voidContext) {
//%  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
//%  context->impToAdd = imp_implementationWithBlock(^(id obj) {
//%    return GPBGet##TRUE_NAME##IvarWithField(obj, field);
//%  });
//%  context->encodingSelector = @selector(get##NAME);
//%  return NO;
//%}
//%
//%PDDM-DEFINE IVAR_ACCESSOR_FUNC_OBJECT(NAME, TYPE)
//%IVAR_ACCESSOR_FUNC_COMMON(NAME, TYPE, Object)
//%static BOOL IvarSet##NAME(GPBFieldDescriptor *field, void *voidContext) {
//%  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
//%  // Local for syntax so the block doesn't capture context and use random
//%  // memory in the future.
//%  const GPBFileSyntax syntax = context->syntax;
//%  context->impToAdd = imp_implementationWithBlock(^(id obj, TYPE value) {
//%    return GPBSetObjectIvarWithFieldInternal(obj, field, value, syntax);
//%  });
//%  context->encodingSelector = @selector(set##NAME:);
//%  return NO;
//%}
//%
//%PDDM-DEFINE IVAR_ACCESSOR_FUNC_PER_VERSION(NAME, TYPE)
//%IVAR_ACCESSOR_FUNC_PER_VERSION_ALIASED(NAME, TYPE, NAME)
//%PDDM-DEFINE IVAR_ACCESSOR_FUNC_PER_VERSION_ALIASED(NAME, TYPE, TRUE_NAME)
//%IVAR_ACCESSOR_FUNC_COMMON(NAME, TYPE, TRUE_NAME)
//%static BOOL IvarSet##NAME(GPBFieldDescriptor *field, void *voidContext) {
//%  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
//%  // Local for syntax so the block doesn't capture context and use random
//%  // memory in the future.
//%  const GPBFileSyntax syntax = context->syntax;
//%  context->impToAdd = imp_implementationWithBlock(^(id obj, TYPE value) {
//%    return GPBSet##TRUE_NAME##IvarWithFieldInternal(obj, field, value, syntax);
//%  });
//%  context->encodingSelector = @selector(set##NAME:);
//%  return NO;
//%}
//%
//%PDDM-EXPAND IVAR_ACCESSOR_FUNC_PER_VERSION(Bool, BOOL)
// This block of code is generated, do not edit it directly.

static BOOL IvarGetBool(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  context->impToAdd = imp_implementationWithBlock(^(id obj) {
    return GPBGetBoolIvarWithField(obj, field);
  });
  context->encodingSelector = @selector(getBool);
  return NO;
}

static BOOL IvarSetBool(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  // Local for syntax so the block doesn't capture context and use random
  // memory in the future.
  const GPBFileSyntax syntax = context->syntax;
  context->impToAdd = imp_implementationWithBlock(^(id obj, BOOL value) {
    return GPBSetBoolIvarWithFieldInternal(obj, field, value, syntax);
  });
  context->encodingSelector = @selector(setBool:);
  return NO;
}

//%PDDM-EXPAND IVAR_ACCESSOR_FUNC_PER_VERSION(Int32, int32_t)
// This block of code is generated, do not edit it directly.

static BOOL IvarGetInt32(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  context->impToAdd = imp_implementationWithBlock(^(id obj) {
    return GPBGetInt32IvarWithField(obj, field);
  });
  context->encodingSelector = @selector(getInt32);
  return NO;
}

static BOOL IvarSetInt32(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  // Local for syntax so the block doesn't capture context and use random
  // memory in the future.
  const GPBFileSyntax syntax = context->syntax;
  context->impToAdd = imp_implementationWithBlock(^(id obj, int32_t value) {
    return GPBSetInt32IvarWithFieldInternal(obj, field, value, syntax);
  });
  context->encodingSelector = @selector(setInt32:);
  return NO;
}

//%PDDM-EXPAND IVAR_ACCESSOR_FUNC_PER_VERSION_ALIASED(SInt32, int32_t, Int32)
// This block of code is generated, do not edit it directly.

static BOOL IvarGetSInt32(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  context->impToAdd = imp_implementationWithBlock(^(id obj) {
    return GPBGetInt32IvarWithField(obj, field);
  });
  context->encodingSelector = @selector(getSInt32);
  return NO;
}

static BOOL IvarSetSInt32(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  // Local for syntax so the block doesn't capture context and use random
  // memory in the future.
  const GPBFileSyntax syntax = context->syntax;
  context->impToAdd = imp_implementationWithBlock(^(id obj, int32_t value) {
    return GPBSetInt32IvarWithFieldInternal(obj, field, value, syntax);
  });
  context->encodingSelector = @selector(setSInt32:);
  return NO;
}

//%PDDM-EXPAND IVAR_ACCESSOR_FUNC_PER_VERSION_ALIASED(SFixed32, int32_t, Int32)
// This block of code is generated, do not edit it directly.

static BOOL IvarGetSFixed32(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  context->impToAdd = imp_implementationWithBlock(^(id obj) {
    return GPBGetInt32IvarWithField(obj, field);
  });
  context->encodingSelector = @selector(getSFixed32);
  return NO;
}

static BOOL IvarSetSFixed32(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  // Local for syntax so the block doesn't capture context and use random
  // memory in the future.
  const GPBFileSyntax syntax = context->syntax;
  context->impToAdd = imp_implementationWithBlock(^(id obj, int32_t value) {
    return GPBSetInt32IvarWithFieldInternal(obj, field, value, syntax);
  });
  context->encodingSelector = @selector(setSFixed32:);
  return NO;
}

//%PDDM-EXPAND IVAR_ACCESSOR_FUNC_PER_VERSION(UInt32, uint32_t)
// This block of code is generated, do not edit it directly.

static BOOL IvarGetUInt32(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  context->impToAdd = imp_implementationWithBlock(^(id obj) {
    return GPBGetUInt32IvarWithField(obj, field);
  });
  context->encodingSelector = @selector(getUInt32);
  return NO;
}

static BOOL IvarSetUInt32(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  // Local for syntax so the block doesn't capture context and use random
  // memory in the future.
  const GPBFileSyntax syntax = context->syntax;
  context->impToAdd = imp_implementationWithBlock(^(id obj, uint32_t value) {
    return GPBSetUInt32IvarWithFieldInternal(obj, field, value, syntax);
  });
  context->encodingSelector = @selector(setUInt32:);
  return NO;
}

//%PDDM-EXPAND IVAR_ACCESSOR_FUNC_PER_VERSION_ALIASED(Fixed32, uint32_t, UInt32)
// This block of code is generated, do not edit it directly.

static BOOL IvarGetFixed32(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  context->impToAdd = imp_implementationWithBlock(^(id obj) {
    return GPBGetUInt32IvarWithField(obj, field);
  });
  context->encodingSelector = @selector(getFixed32);
  return NO;
}

static BOOL IvarSetFixed32(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  // Local for syntax so the block doesn't capture context and use random
  // memory in the future.
  const GPBFileSyntax syntax = context->syntax;
  context->impToAdd = imp_implementationWithBlock(^(id obj, uint32_t value) {
    return GPBSetUInt32IvarWithFieldInternal(obj, field, value, syntax);
  });
  context->encodingSelector = @selector(setFixed32:);
  return NO;
}

//%PDDM-EXPAND IVAR_ACCESSOR_FUNC_PER_VERSION(Int64, int64_t)
// This block of code is generated, do not edit it directly.

static BOOL IvarGetInt64(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  context->impToAdd = imp_implementationWithBlock(^(id obj) {
    return GPBGetInt64IvarWithField(obj, field);
  });
  context->encodingSelector = @selector(getInt64);
  return NO;
}

static BOOL IvarSetInt64(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  // Local for syntax so the block doesn't capture context and use random
  // memory in the future.
  const GPBFileSyntax syntax = context->syntax;
  context->impToAdd = imp_implementationWithBlock(^(id obj, int64_t value) {
    return GPBSetInt64IvarWithFieldInternal(obj, field, value, syntax);
  });
  context->encodingSelector = @selector(setInt64:);
  return NO;
}

//%PDDM-EXPAND IVAR_ACCESSOR_FUNC_PER_VERSION_ALIASED(SFixed64, int64_t, Int64)
// This block of code is generated, do not edit it directly.

static BOOL IvarGetSFixed64(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  context->impToAdd = imp_implementationWithBlock(^(id obj) {
    return GPBGetInt64IvarWithField(obj, field);
  });
  context->encodingSelector = @selector(getSFixed64);
  return NO;
}

static BOOL IvarSetSFixed64(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  // Local for syntax so the block doesn't capture context and use random
  // memory in the future.
  const GPBFileSyntax syntax = context->syntax;
  context->impToAdd = imp_implementationWithBlock(^(id obj, int64_t value) {
    return GPBSetInt64IvarWithFieldInternal(obj, field, value, syntax);
  });
  context->encodingSelector = @selector(setSFixed64:);
  return NO;
}

//%PDDM-EXPAND IVAR_ACCESSOR_FUNC_PER_VERSION_ALIASED(SInt64, int64_t, Int64)
// This block of code is generated, do not edit it directly.

static BOOL IvarGetSInt64(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  context->impToAdd = imp_implementationWithBlock(^(id obj) {
    return GPBGetInt64IvarWithField(obj, field);
  });
  context->encodingSelector = @selector(getSInt64);
  return NO;
}

static BOOL IvarSetSInt64(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  // Local for syntax so the block doesn't capture context and use random
  // memory in the future.
  const GPBFileSyntax syntax = context->syntax;
  context->impToAdd = imp_implementationWithBlock(^(id obj, int64_t value) {
    return GPBSetInt64IvarWithFieldInternal(obj, field, value, syntax);
  });
  context->encodingSelector = @selector(setSInt64:);
  return NO;
}

//%PDDM-EXPAND IVAR_ACCESSOR_FUNC_PER_VERSION(UInt64, uint64_t)
// This block of code is generated, do not edit it directly.

static BOOL IvarGetUInt64(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  context->impToAdd = imp_implementationWithBlock(^(id obj) {
    return GPBGetUInt64IvarWithField(obj, field);
  });
  context->encodingSelector = @selector(getUInt64);
  return NO;
}

static BOOL IvarSetUInt64(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  // Local for syntax so the block doesn't capture context and use random
  // memory in the future.
  const GPBFileSyntax syntax = context->syntax;
  context->impToAdd = imp_implementationWithBlock(^(id obj, uint64_t value) {
    return GPBSetUInt64IvarWithFieldInternal(obj, field, value, syntax);
  });
  context->encodingSelector = @selector(setUInt64:);
  return NO;
}

//%PDDM-EXPAND IVAR_ACCESSOR_FUNC_PER_VERSION_ALIASED(Fixed64, uint64_t, UInt64)
// This block of code is generated, do not edit it directly.

static BOOL IvarGetFixed64(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  context->impToAdd = imp_implementationWithBlock(^(id obj) {
    return GPBGetUInt64IvarWithField(obj, field);
  });
  context->encodingSelector = @selector(getFixed64);
  return NO;
}

static BOOL IvarSetFixed64(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  // Local for syntax so the block doesn't capture context and use random
  // memory in the future.
  const GPBFileSyntax syntax = context->syntax;
  context->impToAdd = imp_implementationWithBlock(^(id obj, uint64_t value) {
    return GPBSetUInt64IvarWithFieldInternal(obj, field, value, syntax);
  });
  context->encodingSelector = @selector(setFixed64:);
  return NO;
}

//%PDDM-EXPAND IVAR_ACCESSOR_FUNC_PER_VERSION(Float, float)
// This block of code is generated, do not edit it directly.

static BOOL IvarGetFloat(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  context->impToAdd = imp_implementationWithBlock(^(id obj) {
    return GPBGetFloatIvarWithField(obj, field);
  });
  context->encodingSelector = @selector(getFloat);
  return NO;
}

static BOOL IvarSetFloat(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  // Local for syntax so the block doesn't capture context and use random
  // memory in the future.
  const GPBFileSyntax syntax = context->syntax;
  context->impToAdd = imp_implementationWithBlock(^(id obj, float value) {
    return GPBSetFloatIvarWithFieldInternal(obj, field, value, syntax);
  });
  context->encodingSelector = @selector(setFloat:);
  return NO;
}

//%PDDM-EXPAND IVAR_ACCESSOR_FUNC_PER_VERSION(Double, double)
// This block of code is generated, do not edit it directly.

static BOOL IvarGetDouble(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  context->impToAdd = imp_implementationWithBlock(^(id obj) {
    return GPBGetDoubleIvarWithField(obj, field);
  });
  context->encodingSelector = @selector(getDouble);
  return NO;
}

static BOOL IvarSetDouble(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  // Local for syntax so the block doesn't capture context and use random
  // memory in the future.
  const GPBFileSyntax syntax = context->syntax;
  context->impToAdd = imp_implementationWithBlock(^(id obj, double value) {
    return GPBSetDoubleIvarWithFieldInternal(obj, field, value, syntax);
  });
  context->encodingSelector = @selector(setDouble:);
  return NO;
}

//%PDDM-EXPAND IVAR_ACCESSOR_FUNC_OBJECT(String, id)
// This block of code is generated, do not edit it directly.

static BOOL IvarGetString(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  context->impToAdd = imp_implementationWithBlock(^(id obj) {
    return GPBGetObjectIvarWithField(obj, field);
  });
  context->encodingSelector = @selector(getString);
  return NO;
}

static BOOL IvarSetString(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  // Local for syntax so the block doesn't capture context and use random
  // memory in the future.
  const GPBFileSyntax syntax = context->syntax;
  context->impToAdd = imp_implementationWithBlock(^(id obj, id value) {
    return GPBSetObjectIvarWithFieldInternal(obj, field, value, syntax);
  });
  context->encodingSelector = @selector(setString:);
  return NO;
}

//%PDDM-EXPAND IVAR_ACCESSOR_FUNC_OBJECT(Data, id)
// This block of code is generated, do not edit it directly.

static BOOL IvarGetData(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  context->impToAdd = imp_implementationWithBlock(^(id obj) {
    return GPBGetObjectIvarWithField(obj, field);
  });
  context->encodingSelector = @selector(getData);
  return NO;
}

static BOOL IvarSetData(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  // Local for syntax so the block doesn't capture context and use random
  // memory in the future.
  const GPBFileSyntax syntax = context->syntax;
  context->impToAdd = imp_implementationWithBlock(^(id obj, id value) {
    return GPBSetObjectIvarWithFieldInternal(obj, field, value, syntax);
  });
  context->encodingSelector = @selector(setData:);
  return NO;
}

//%PDDM-EXPAND IVAR_ACCESSOR_FUNC_OBJECT(Message, id)
// This block of code is generated, do not edit it directly.

static BOOL IvarGetMessage(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  context->impToAdd = imp_implementationWithBlock(^(id obj) {
    return GPBGetObjectIvarWithField(obj, field);
  });
  context->encodingSelector = @selector(getMessage);
  return NO;
}

static BOOL IvarSetMessage(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  // Local for syntax so the block doesn't capture context and use random
  // memory in the future.
  const GPBFileSyntax syntax = context->syntax;
  context->impToAdd = imp_implementationWithBlock(^(id obj, id value) {
    return GPBSetObjectIvarWithFieldInternal(obj, field, value, syntax);
  });
  context->encodingSelector = @selector(setMessage:);
  return NO;
}

//%PDDM-EXPAND IVAR_ACCESSOR_FUNC_OBJECT(Group, id)
// This block of code is generated, do not edit it directly.

static BOOL IvarGetGroup(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  context->impToAdd = imp_implementationWithBlock(^(id obj) {
    return GPBGetObjectIvarWithField(obj, field);
  });
  context->encodingSelector = @selector(getGroup);
  return NO;
}

static BOOL IvarSetGroup(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  // Local for syntax so the block doesn't capture context and use random
  // memory in the future.
  const GPBFileSyntax syntax = context->syntax;
  context->impToAdd = imp_implementationWithBlock(^(id obj, id value) {
    return GPBSetObjectIvarWithFieldInternal(obj, field, value, syntax);
  });
  context->encodingSelector = @selector(setGroup:);
  return NO;
}

//%PDDM-EXPAND-END (17 expansions)

// Enum gets custom hooks because get needs the syntax to Get.

static BOOL IvarGetEnum(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  // Local for syntax so the block doesn't capture context and use random
  // memory in the future.
  const GPBFileSyntax syntax = context->syntax;
  context->impToAdd = imp_implementationWithBlock(^(id obj) {
    return GPBGetEnumIvarWithFieldInternal(obj, field, syntax);
  });
  context->encodingSelector = @selector(getEnum);
  return NO;
}

static BOOL IvarSetEnum(GPBFieldDescriptor *field, void *voidContext) {
  IvarAccessorMethodContext *context = (IvarAccessorMethodContext *)voidContext;
  // Local for syntax so the block doesn't capture context and use random
  // memory in the future.
  const GPBFileSyntax syntax = context->syntax;
  context->impToAdd = imp_implementationWithBlock(^(id obj, int32_t value) {
    return GPBSetEnumIvarWithFieldInternal(obj, field, value, syntax);
  });
  context->encodingSelector = @selector(setEnum:);
  return NO;
}

+ (BOOL)resolveInstanceMethod:(SEL)sel {
  const GPBDescriptor *descriptor = [self descriptor];
  if (!descriptor) {
    return NO;
  }

  // NOTE: hasSel_/setHasSel_ will be NULL if the field for the given message
  // should not have has support (done in GPBDescriptor.m), so there is no need
  // for checks here to see if has*/setHas* are allowed.

  IvarAccessorMethodContext context = {descriptor.file.syntax, NULL, NULL};
  for (GPBFieldDescriptor *field in descriptor->fields_) {
    BOOL isMapOrArray = GPBFieldIsMapOrArray(field);
    if (!isMapOrArray) {
      if (sel == field->getSel_) {
        static const GPBApplyStrictFunctions funcs =
            GPBAPPLY_STRICT_FUNCTIONS_INIT(IvarGet);
        funcs[GPBGetFieldType(field)](field, &context);
        break;
      } else if (sel == field->hasSel_) {
        int32_t index = GPBFieldHasIndex(field);
        uint32_t fieldNum = GPBFieldNumber(field);
        context.impToAdd = imp_implementationWithBlock(^(id obj) {
          return GPBGetHasIvar(obj, index, fieldNum);
        });
        context.encodingSelector = @selector(getBool);
        break;
      } else if (sel == field->setHasSel_) {
        context.impToAdd = imp_implementationWithBlock(^(id obj, BOOL value) {
          if (value) {
            [NSException raise:NSInvalidArgumentException
                        format:@"%@: %@ can only be set to NO (to clear field).",
                               [obj class],
                               NSStringFromSelector(field->setHasSel_)];
          }
          GPBClearMessageField(obj, field);
        });
        context.encodingSelector = @selector(setBool:);
        break;
      } else if (sel == field->setSel_) {
        GPBApplyStrictFunctions funcs = GPBAPPLY_STRICT_FUNCTIONS_INIT(IvarSet);
        funcs[GPBGetFieldType(field)](field, &context);
        break;
      } else {
        GPBOneofDescriptor *oneof = field->containingOneof_;
        if (oneof && (sel == oneof->caseSel_)) {
          int32_t index = oneof->oneofDescription_->index;
          context.impToAdd = imp_implementationWithBlock(^(id obj) {
            return GPBGetHasOneof(obj, index);
          });
          context.encodingSelector = @selector(getEnum);
          break;
        }
      }
    } else {
      if (sel == field->getSel_) {
        if (field.fieldType == GPBFieldTypeRepeated) {
          context.impToAdd = imp_implementationWithBlock(^(id obj) {
            return GetArrayIvarWithField(obj, field);
          });
        } else {
          context.impToAdd = imp_implementationWithBlock(^(id obj) {
            return GetMapIvarWithField(obj, field);
          });
        }
        context.encodingSelector = @selector(getArray);
        break;
      } else if (sel == field->setSel_) {
        // Local for syntax so the block doesn't capture context and use random
        // memory in the future.
        const GPBFileSyntax syntax = context.syntax;
        context.impToAdd = imp_implementationWithBlock(^(id obj, id value) {
          return GPBSetObjectIvarWithFieldInternal(obj, field, value, syntax);
        });
        context.encodingSelector = @selector(setArray:);
        break;
      }
    }
  }
  if (context.impToAdd) {
    const char *encoding =
        GPBMessageEncodingForSelector(context.encodingSelector, YES);
    BOOL methodAdded = class_addMethod(descriptor.messageClass, sel,
                                       context.impToAdd, encoding);
    return methodAdded;
  }
  return [super resolveInstanceMethod:sel];
}

+ (BOOL)resolveClassMethod:(SEL)sel {
  // Extensions scoped to a Message and looked up via class methods.
  if (GPBResolveExtensionClassMethod(self, sel)) {
    return YES;
  }
  return [super resolveClassMethod:sel];
}

#pragma mark - NSCoding Support

+ (BOOL)supportsSecureCoding {
  return YES;
}

- (instancetype)initWithCoder:(NSCoder *)aDecoder {
  self = [self init];
  if (self) {
    NSData *data =
        [aDecoder decodeObjectOfClass:[NSData class] forKey:kGPBDataCoderKey];
    if (data.length) {
      [self mergeFromData:data extensionRegistry:nil];
    }
  }
  return self;
}

- (void)encodeWithCoder:(NSCoder *)aCoder {
  NSData *data = [self data];
  if (data.length) {
    [aCoder encodeObject:data forKey:kGPBDataCoderKey];
  }
}

#pragma mark - KVC Support

+ (BOOL)accessInstanceVariablesDirectly {
  // Make sure KVC doesn't use instance variables.
  return NO;
}

@end
