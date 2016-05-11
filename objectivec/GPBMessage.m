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
#import "GPBCodedOutputStream_PackagePrivate.h"
#import "GPBDescriptor_PackagePrivate.h"
#import "GPBDictionary_PackagePrivate.h"
#import "GPBExtensionInternals.h"
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
  GPBExtensionDescriptor *autocreatorExtension_;
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


static void CheckExtension(GPBMessage *self,
                           GPBExtensionDescriptor *extension) {
  if ([self class] != extension.containingMessageClass) {
    [NSException
         raise:NSInvalidArgumentException
        format:@"Extension %@ used on wrong class (%@ instead of %@)",
               extension.singletonName,
               [self class], extension.containingMessageClass];
  }
}

static NSMutableDictionary *CloneExtensionMap(NSDictionary *extensionMap,
                                              NSZone *zone) {
  if (extensionMap.count == 0) {
    return nil;
  }
  NSMutableDictionary *result = [[NSMutableDictionary allocWithZone:zone]
      initWithCapacity:extensionMap.count];

  for (GPBExtensionDescriptor *extension in extensionMap) {
    id value = [extensionMap objectForKey:extension];
    BOOL isMessageExtension = GPBExtensionIsMessage(extension);

    if (extension.repeated) {
      if (isMessageExtension) {
        NSMutableArray *list =
            [[NSMutableArray alloc] initWithCapacity:[value count]];
        for (GPBMessage *listValue in value) {
          GPBMessage *copiedValue = [listValue copyWithZone:zone];
          [list addObject:copiedValue];
          [copiedValue release];
        }
        [result setObject:list forKey:extension];
        [list release];
      } else {
        NSMutableArray *copiedValue = [value mutableCopyWithZone:zone];
        [result setObject:copiedValue forKey:extension];
        [copiedValue release];
      }
    } else {
      if (isMessageExtension) {
        GPBMessage *copiedValue = [value copyWithZone:zone];
        [result setObject:copiedValue forKey:extension];
        [copiedValue release];
      } else {
        [result setObject:value forKey:extension];
      }
    }
  }

  return result;
}

static id CreateArrayForField(GPBFieldDescriptor *field,
                              GPBMessage *autocreator) {
  id result;
  GPBDataType fieldDataType = GPBGetFieldDataType(field);
  switch (fieldDataType) {
    case GPBDataTypeBool:
      result = [[GPBBoolArray alloc] init];
      break;
    case GPBDataTypeFixed32:
    case GPBDataTypeUInt32:
      result = [[GPBUInt32Array alloc] init];
      break;
    case GPBDataTypeInt32:
    case GPBDataTypeSFixed32:
    case GPBDataTypeSInt32:
      result = [[GPBInt32Array alloc] init];
      break;
    case GPBDataTypeFixed64:
    case GPBDataTypeUInt64:
      result = [[GPBUInt64Array alloc] init];
      break;
    case GPBDataTypeInt64:
    case GPBDataTypeSFixed64:
    case GPBDataTypeSInt64:
      result = [[GPBInt64Array alloc] init];
      break;
    case GPBDataTypeFloat:
      result = [[GPBFloatArray alloc] init];
      break;
    case GPBDataTypeDouble:
      result = [[GPBDoubleArray alloc] init];
      break;

    case GPBDataTypeEnum:
      result = [[GPBEnumArray alloc]
                  initWithValidationFunction:field.enumDescriptor.enumVerifier];
      break;

    case GPBDataTypeBytes:
    case GPBDataTypeGroup:
    case GPBDataTypeMessage:
    case GPBDataTypeString:
      if (autocreator) {
        result = [[GPBAutocreatedArray alloc] init];
      } else {
        result = [[NSMutableArray alloc] init];
      }
      break;
  }

  if (autocreator) {
    if (GPBDataTypeIsObject(fieldDataType)) {
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
  GPBDataType keyDataType = field.mapKeyDataType;
  GPBDataType valueDataType = GPBGetFieldDataType(field);
  switch (keyDataType) {
    case GPBDataTypeBool:
      switch (valueDataType) {
        case GPBDataTypeBool:
          result = [[GPBBoolBoolDictionary alloc] init];
          break;
        case GPBDataTypeFixed32:
        case GPBDataTypeUInt32:
          result = [[GPBBoolUInt32Dictionary alloc] init];
          break;
        case GPBDataTypeInt32:
        case GPBDataTypeSFixed32:
        case GPBDataTypeSInt32:
          result = [[GPBBoolInt32Dictionary alloc] init];
          break;
        case GPBDataTypeFixed64:
        case GPBDataTypeUInt64:
          result = [[GPBBoolUInt64Dictionary alloc] init];
          break;
        case GPBDataTypeInt64:
        case GPBDataTypeSFixed64:
        case GPBDataTypeSInt64:
          result = [[GPBBoolInt64Dictionary alloc] init];
          break;
        case GPBDataTypeFloat:
          result = [[GPBBoolFloatDictionary alloc] init];
          break;
        case GPBDataTypeDouble:
          result = [[GPBBoolDoubleDictionary alloc] init];
          break;
        case GPBDataTypeEnum:
          result = [[GPBBoolEnumDictionary alloc]
              initWithValidationFunction:field.enumDescriptor.enumVerifier];
          break;
        case GPBDataTypeBytes:
        case GPBDataTypeMessage:
        case GPBDataTypeString:
          result = [[GPBBoolObjectDictionary alloc] init];
          break;
        case GPBDataTypeGroup:
          NSCAssert(NO, @"shouldn't happen");
          return nil;
      }
      break;
    case GPBDataTypeFixed32:
    case GPBDataTypeUInt32:
      switch (valueDataType) {
        case GPBDataTypeBool:
          result = [[GPBUInt32BoolDictionary alloc] init];
          break;
        case GPBDataTypeFixed32:
        case GPBDataTypeUInt32:
          result = [[GPBUInt32UInt32Dictionary alloc] init];
          break;
        case GPBDataTypeInt32:
        case GPBDataTypeSFixed32:
        case GPBDataTypeSInt32:
          result = [[GPBUInt32Int32Dictionary alloc] init];
          break;
        case GPBDataTypeFixed64:
        case GPBDataTypeUInt64:
          result = [[GPBUInt32UInt64Dictionary alloc] init];
          break;
        case GPBDataTypeInt64:
        case GPBDataTypeSFixed64:
        case GPBDataTypeSInt64:
          result = [[GPBUInt32Int64Dictionary alloc] init];
          break;
        case GPBDataTypeFloat:
          result = [[GPBUInt32FloatDictionary alloc] init];
          break;
        case GPBDataTypeDouble:
          result = [[GPBUInt32DoubleDictionary alloc] init];
          break;
        case GPBDataTypeEnum:
          result = [[GPBUInt32EnumDictionary alloc]
              initWithValidationFunction:field.enumDescriptor.enumVerifier];
          break;
        case GPBDataTypeBytes:
        case GPBDataTypeMessage:
        case GPBDataTypeString:
          result = [[GPBUInt32ObjectDictionary alloc] init];
          break;
        case GPBDataTypeGroup:
          NSCAssert(NO, @"shouldn't happen");
          return nil;
      }
      break;
    case GPBDataTypeInt32:
    case GPBDataTypeSFixed32:
    case GPBDataTypeSInt32:
      switch (valueDataType) {
        case GPBDataTypeBool:
          result = [[GPBInt32BoolDictionary alloc] init];
          break;
        case GPBDataTypeFixed32:
        case GPBDataTypeUInt32:
          result = [[GPBInt32UInt32Dictionary alloc] init];
          break;
        case GPBDataTypeInt32:
        case GPBDataTypeSFixed32:
        case GPBDataTypeSInt32:
          result = [[GPBInt32Int32Dictionary alloc] init];
          break;
        case GPBDataTypeFixed64:
        case GPBDataTypeUInt64:
          result = [[GPBInt32UInt64Dictionary alloc] init];
          break;
        case GPBDataTypeInt64:
        case GPBDataTypeSFixed64:
        case GPBDataTypeSInt64:
          result = [[GPBInt32Int64Dictionary alloc] init];
          break;
        case GPBDataTypeFloat:
          result = [[GPBInt32FloatDictionary alloc] init];
          break;
        case GPBDataTypeDouble:
          result = [[GPBInt32DoubleDictionary alloc] init];
          break;
        case GPBDataTypeEnum:
          result = [[GPBInt32EnumDictionary alloc]
              initWithValidationFunction:field.enumDescriptor.enumVerifier];
          break;
        case GPBDataTypeBytes:
        case GPBDataTypeMessage:
        case GPBDataTypeString:
          result = [[GPBInt32ObjectDictionary alloc] init];
          break;
        case GPBDataTypeGroup:
          NSCAssert(NO, @"shouldn't happen");
          return nil;
      }
      break;
    case GPBDataTypeFixed64:
    case GPBDataTypeUInt64:
      switch (valueDataType) {
        case GPBDataTypeBool:
          result = [[GPBUInt64BoolDictionary alloc] init];
          break;
        case GPBDataTypeFixed32:
        case GPBDataTypeUInt32:
          result = [[GPBUInt64UInt32Dictionary alloc] init];
          break;
        case GPBDataTypeInt32:
        case GPBDataTypeSFixed32:
        case GPBDataTypeSInt32:
          result = [[GPBUInt64Int32Dictionary alloc] init];
          break;
        case GPBDataTypeFixed64:
        case GPBDataTypeUInt64:
          result = [[GPBUInt64UInt64Dictionary alloc] init];
          break;
        case GPBDataTypeInt64:
        case GPBDataTypeSFixed64:
        case GPBDataTypeSInt64:
          result = [[GPBUInt64Int64Dictionary alloc] init];
          break;
        case GPBDataTypeFloat:
          result = [[GPBUInt64FloatDictionary alloc] init];
          break;
        case GPBDataTypeDouble:
          result = [[GPBUInt64DoubleDictionary alloc] init];
          break;
        case GPBDataTypeEnum:
          result = [[GPBUInt64EnumDictionary alloc]
              initWithValidationFunction:field.enumDescriptor.enumVerifier];
          break;
        case GPBDataTypeBytes:
        case GPBDataTypeMessage:
        case GPBDataTypeString:
          result = [[GPBUInt64ObjectDictionary alloc] init];
          break;
        case GPBDataTypeGroup:
          NSCAssert(NO, @"shouldn't happen");
          return nil;
      }
      break;
    case GPBDataTypeInt64:
    case GPBDataTypeSFixed64:
    case GPBDataTypeSInt64:
      switch (valueDataType) {
        case GPBDataTypeBool:
          result = [[GPBInt64BoolDictionary alloc] init];
          break;
        case GPBDataTypeFixed32:
        case GPBDataTypeUInt32:
          result = [[GPBInt64UInt32Dictionary alloc] init];
          break;
        case GPBDataTypeInt32:
        case GPBDataTypeSFixed32:
        case GPBDataTypeSInt32:
          result = [[GPBInt64Int32Dictionary alloc] init];
          break;
        case GPBDataTypeFixed64:
        case GPBDataTypeUInt64:
          result = [[GPBInt64UInt64Dictionary alloc] init];
          break;
        case GPBDataTypeInt64:
        case GPBDataTypeSFixed64:
        case GPBDataTypeSInt64:
          result = [[GPBInt64Int64Dictionary alloc] init];
          break;
        case GPBDataTypeFloat:
          result = [[GPBInt64FloatDictionary alloc] init];
          break;
        case GPBDataTypeDouble:
          result = [[GPBInt64DoubleDictionary alloc] init];
          break;
        case GPBDataTypeEnum:
          result = [[GPBInt64EnumDictionary alloc]
              initWithValidationFunction:field.enumDescriptor.enumVerifier];
          break;
        case GPBDataTypeBytes:
        case GPBDataTypeMessage:
        case GPBDataTypeString:
          result = [[GPBInt64ObjectDictionary alloc] init];
          break;
        case GPBDataTypeGroup:
          NSCAssert(NO, @"shouldn't happen");
          return nil;
      }
      break;
    case GPBDataTypeString:
      switch (valueDataType) {
        case GPBDataTypeBool:
          result = [[GPBStringBoolDictionary alloc] init];
          break;
        case GPBDataTypeFixed32:
        case GPBDataTypeUInt32:
          result = [[GPBStringUInt32Dictionary alloc] init];
          break;
        case GPBDataTypeInt32:
        case GPBDataTypeSFixed32:
        case GPBDataTypeSInt32:
          result = [[GPBStringInt32Dictionary alloc] init];
          break;
        case GPBDataTypeFixed64:
        case GPBDataTypeUInt64:
          result = [[GPBStringUInt64Dictionary alloc] init];
          break;
        case GPBDataTypeInt64:
        case GPBDataTypeSFixed64:
        case GPBDataTypeSInt64:
          result = [[GPBStringInt64Dictionary alloc] init];
          break;
        case GPBDataTypeFloat:
          result = [[GPBStringFloatDictionary alloc] init];
          break;
        case GPBDataTypeDouble:
          result = [[GPBStringDoubleDictionary alloc] init];
          break;
        case GPBDataTypeEnum:
          result = [[GPBStringEnumDictionary alloc]
              initWithValidationFunction:field.enumDescriptor.enumVerifier];
          break;
        case GPBDataTypeBytes:
        case GPBDataTypeMessage:
        case GPBDataTypeString:
          if (autocreator) {
            result = [[GPBAutocreatedDictionary alloc] init];
          } else {
            result = [[NSMutableDictionary alloc] init];
          }
          break;
        case GPBDataTypeGroup:
          NSCAssert(NO, @"shouldn't happen");
          return nil;
      }
      break;

    case GPBDataTypeFloat:
    case GPBDataTypeDouble:
    case GPBDataTypeEnum:
    case GPBDataTypeBytes:
    case GPBDataTypeGroup:
    case GPBDataTypeMessage:
      NSCAssert(NO, @"shouldn't happen");
      return nil;
  }

  if (autocreator) {
    if ((keyDataType == GPBDataTypeString) &&
        GPBDataTypeIsObject(valueDataType)) {
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
    GPBPrepareReadOnlySemaphore(self);
    dispatch_semaphore_wait(self->readOnlySemaphore_, DISPATCH_TIME_FOREVER);
    array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
    if (!array) {
      array = CreateArrayForField(field, self);
      GPBSetAutocreatedRetainedObjectIvarWithField(self, field, array);
    }
    dispatch_semaphore_signal(self->readOnlySemaphore_);
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
    GPBPrepareReadOnlySemaphore(self);
    dispatch_semaphore_wait(self->readOnlySemaphore_, DISPATCH_TIME_FOREVER);
    dict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
    if (!dict) {
      dict = CreateMapForField(field, self);
      GPBSetAutocreatedRetainedObjectIvarWithField(self, field, dict);
    }
    dispatch_semaphore_signal(self->readOnlySemaphore_);
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
    Class msgClass, GPBMessage *autocreator, GPBExtensionDescriptor *extension)
    __attribute__((ns_returns_retained));

static GPBMessage *CreateMessageWithAutocreatorForExtension(
    Class msgClass, GPBMessage *autocreator,
    GPBExtensionDescriptor *extension) {
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
        if (GPBFieldDataTypeIsObject(field)) {
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
        if ((field.mapKeyDataType == GPBDataTypeString) &&
            GPBFieldDataTypeIsObject(field)) {
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
                                            storageSize:0
                                                  flags:0];
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
      if (errorPtr) {
        *errorPtr = nil;
      }
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
      if (errorPtr) {
        *errorPtr = nil;
      }
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
  if (readOnlySemaphore_) {
    dispatch_release(readOnlySemaphore_);
  }
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
        if (GPBFieldDataTypeIsMessage(field)) {
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
            if (field.mapKeyDataType == GPBDataTypeString) {
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
            if (GPBFieldDataTypeIsObject(field)) {
              // NSArray
              newValue = [value mutableCopyWithZone:zone];
            } else {
              // GPB*Array
              newValue = [value copyWithZone:zone];
            }
          } else {
            if (field.mapKeyDataType == GPBDataTypeString) {
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
    } else if (GPBFieldDataTypeIsMessage(field)) {
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
    } else if (GPBFieldDataTypeIsObject(field) &&
               GPBGetHasIvarField(self, field)) {
      // A set string/data value (message picked off above), copy it.
      id value = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
      id newValue = [value copyWithZone:zone];
      // We retain here because the memcpy picked up the pointer value and
      // the next call to SetRetainedObject... will release the current value.
      [value retain];
      GPBSetRetainedObjectIvarWithFieldInternal(message, field, newValue,
                                                syntax);
    } else {
      // memcpy took care of the rest of the primitive fields if they were set.
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
          if (GPBFieldDataTypeIsObject(field)) {
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
          if ((field.mapKeyDataType == GPBDataTypeString) &&
              GPBFieldDataTypeIsObject(field)) {
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
    } else if (GPBFieldDataTypeIsMessage(field)) {
      GPBClearAutocreatedMessageIvarWithField(self, field);
      GPBMessage *value = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
      [value release];
    } else if (GPBFieldDataTypeIsObject(field) &&
               GPBGetHasIvarField(self, field)) {
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
    if (GPBFieldDataTypeIsMessage(field)) {
      GPBFieldType fieldType = field.fieldType;
      if (fieldType == GPBFieldTypeSingle) {
        if (field.isRequired) {
          GPBMessage *message = GPBGetMessageMessageField(self, field);
          if (!message.initialized) {
            return NO;
          }
        } else {
          NSAssert(field.isOptional,
                   @"%@: Single message field %@ not required or optional?",
                   [self class], field.name);
          if (GPBGetHasIvarField(self, field)) {
            GPBMessage *message = GPBGetMessageMessageField(self, field);
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
        if (field.mapKeyDataType == GPBDataTypeString) {
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
      enumerateKeysAndObjectsUsingBlock:^(GPBExtensionDescriptor *extension,
                                          id obj,
                                          BOOL *stop) {
        if (GPBExtensionIsMessage(extension)) {
          if (extension.isRepeated) {
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
    // If it happens, truncate.
    data.length = 0;
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
//%    case GPBDataType##TYPE:
//%      if (fieldType == GPBFieldTypeRepeated) {
//%        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
//%        GPB##ARRAY_TYPE##Array *array =
//%            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
//%        [output write##TYPE##Array:fieldNumber values:array tag:tag];
//%      } else if (fieldType == GPBFieldTypeSingle) {
//%        [output write##TYPE:fieldNumber
//%              TYPE$S  value:GPBGetMessage##REAL_TYPE##Field(self, field)];
//%      } else {  // fieldType == GPBFieldTypeMap
//%        // Exact type here doesn't matter.
//%        GPBInt32##ARRAY_TYPE##Dictionary *dict =
//%            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
//%        [dict writeToCodedOutputStream:output asField:field];
//%      }
//%      break;
//%
//%PDDM-DEFINE FIELD_CASE2(TYPE)
//%    case GPBDataType##TYPE:
//%      if (fieldType == GPBFieldTypeRepeated) {
//%        NSArray *array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
//%        [output write##TYPE##Array:fieldNumber values:array];
//%      } else if (fieldType == GPBFieldTypeSingle) {
//%        // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
//%        // again.
//%        [output write##TYPE:fieldNumber
//%              TYPE$S  value:GPBGetObjectIvarWithFieldNoAutocreate(self, field)];
//%      } else {  // fieldType == GPBFieldTypeMap
//%        // Exact type here doesn't matter.
//%        id dict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
//%        GPBDataType mapKeyDataType = field.mapKeyDataType;
//%        if (mapKeyDataType == GPBDataTypeString) {
//%          GPBDictionaryWriteToStreamInternalHelper(output, dict, field);
//%        } else {
//%          [dict writeToCodedOutputStream:output asField:field];
//%        }
//%      }
//%      break;
//%

  switch (GPBGetFieldDataType(field)) {

//%PDDM-EXPAND FIELD_CASE(Bool, Bool)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeBool:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBBoolArray *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeBoolArray:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeBool:fieldNumber
                    value:GPBGetMessageBoolField(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32BoolDictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(Fixed32, UInt32)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeFixed32:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBUInt32Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeFixed32Array:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeFixed32:fieldNumber
                       value:GPBGetMessageUInt32Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32UInt32Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(SFixed32, Int32)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeSFixed32:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBInt32Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeSFixed32Array:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeSFixed32:fieldNumber
                        value:GPBGetMessageInt32Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32Int32Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(Float, Float)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeFloat:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBFloatArray *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeFloatArray:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeFloat:fieldNumber
                     value:GPBGetMessageFloatField(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32FloatDictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(Fixed64, UInt64)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeFixed64:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBUInt64Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeFixed64Array:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeFixed64:fieldNumber
                       value:GPBGetMessageUInt64Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32UInt64Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(SFixed64, Int64)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeSFixed64:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBInt64Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeSFixed64Array:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeSFixed64:fieldNumber
                        value:GPBGetMessageInt64Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32Int64Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(Double, Double)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeDouble:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBDoubleArray *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeDoubleArray:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeDouble:fieldNumber
                      value:GPBGetMessageDoubleField(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32DoubleDictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(Int32, Int32)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeInt32:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBInt32Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeInt32Array:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeInt32:fieldNumber
                     value:GPBGetMessageInt32Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32Int32Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(Int64, Int64)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeInt64:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBInt64Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeInt64Array:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeInt64:fieldNumber
                     value:GPBGetMessageInt64Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32Int64Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(SInt32, Int32)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeSInt32:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBInt32Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeSInt32Array:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeSInt32:fieldNumber
                      value:GPBGetMessageInt32Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32Int32Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(SInt64, Int64)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeSInt64:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBInt64Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeSInt64Array:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeSInt64:fieldNumber
                      value:GPBGetMessageInt64Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32Int64Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(UInt32, UInt32)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeUInt32:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBUInt32Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeUInt32Array:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeUInt32:fieldNumber
                      value:GPBGetMessageUInt32Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32UInt32Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE(UInt64, UInt64)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeUInt64:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBUInt64Array *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeUInt64Array:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeUInt64:fieldNumber
                      value:GPBGetMessageUInt64Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32UInt64Dictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE_FULL(Enum, Int32, Enum)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeEnum:
      if (fieldType == GPBFieldTypeRepeated) {
        uint32_t tag = field.isPackable ? GPBFieldTag(field) : 0;
        GPBEnumArray *array =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeEnumArray:fieldNumber values:array tag:tag];
      } else if (fieldType == GPBFieldTypeSingle) {
        [output writeEnum:fieldNumber
                    value:GPBGetMessageInt32Field(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        GPBInt32EnumDictionary *dict =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [dict writeToCodedOutputStream:output asField:field];
      }
      break;

//%PDDM-EXPAND FIELD_CASE2(Bytes)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeBytes:
      if (fieldType == GPBFieldTypeRepeated) {
        NSArray *array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeBytesArray:fieldNumber values:array];
      } else if (fieldType == GPBFieldTypeSingle) {
        // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
        // again.
        [output writeBytes:fieldNumber
                     value:GPBGetObjectIvarWithFieldNoAutocreate(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        id dict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        GPBDataType mapKeyDataType = field.mapKeyDataType;
        if (mapKeyDataType == GPBDataTypeString) {
          GPBDictionaryWriteToStreamInternalHelper(output, dict, field);
        } else {
          [dict writeToCodedOutputStream:output asField:field];
        }
      }
      break;

//%PDDM-EXPAND FIELD_CASE2(String)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeString:
      if (fieldType == GPBFieldTypeRepeated) {
        NSArray *array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeStringArray:fieldNumber values:array];
      } else if (fieldType == GPBFieldTypeSingle) {
        // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
        // again.
        [output writeString:fieldNumber
                      value:GPBGetObjectIvarWithFieldNoAutocreate(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        id dict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        GPBDataType mapKeyDataType = field.mapKeyDataType;
        if (mapKeyDataType == GPBDataTypeString) {
          GPBDictionaryWriteToStreamInternalHelper(output, dict, field);
        } else {
          [dict writeToCodedOutputStream:output asField:field];
        }
      }
      break;

//%PDDM-EXPAND FIELD_CASE2(Message)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeMessage:
      if (fieldType == GPBFieldTypeRepeated) {
        NSArray *array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeMessageArray:fieldNumber values:array];
      } else if (fieldType == GPBFieldTypeSingle) {
        // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
        // again.
        [output writeMessage:fieldNumber
                       value:GPBGetObjectIvarWithFieldNoAutocreate(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        id dict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        GPBDataType mapKeyDataType = field.mapKeyDataType;
        if (mapKeyDataType == GPBDataTypeString) {
          GPBDictionaryWriteToStreamInternalHelper(output, dict, field);
        } else {
          [dict writeToCodedOutputStream:output asField:field];
        }
      }
      break;

//%PDDM-EXPAND FIELD_CASE2(Group)
// This block of code is generated, do not edit it directly.

    case GPBDataTypeGroup:
      if (fieldType == GPBFieldTypeRepeated) {
        NSArray *array = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [output writeGroupArray:fieldNumber values:array];
      } else if (fieldType == GPBFieldTypeSingle) {
        // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has check
        // again.
        [output writeGroup:fieldNumber
                     value:GPBGetObjectIvarWithFieldNoAutocreate(self, field)];
      } else {  // fieldType == GPBFieldTypeMap
        // Exact type here doesn't matter.
        id dict = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        GPBDataType mapKeyDataType = field.mapKeyDataType;
        if (mapKeyDataType == GPBDataTypeString) {
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

- (id)getExtension:(GPBExtensionDescriptor *)extension {
  CheckExtension(self, extension);
  id value = [extensionMap_ objectForKey:extension];
  if (value != nil) {
    return value;
  }

  // No default for repeated.
  if (extension.isRepeated) {
    return nil;
  }
  // Non messages get their default.
  if (!GPBExtensionIsMessage(extension)) {
    return extension.defaultValue;
  }

  // Check for an autocreated value.
  GPBPrepareReadOnlySemaphore(self);
  dispatch_semaphore_wait(readOnlySemaphore_, DISPATCH_TIME_FOREVER);
  value = [autocreatedExtensionMap_ objectForKey:extension];
  if (!value) {
    // Auto create the message extensions to match normal fields.
    value = CreateMessageWithAutocreatorForExtension(extension.msgClass, self,
                                                     extension);

    if (autocreatedExtensionMap_ == nil) {
      autocreatedExtensionMap_ = [[NSMutableDictionary alloc] init];
    }

    // We can't simply call setExtension here because that would clear the new
    // value's autocreator.
    [autocreatedExtensionMap_ setObject:value forKey:extension];
    [value release];
  }

  dispatch_semaphore_signal(readOnlySemaphore_);
  return value;
}

- (id)getExistingExtension:(GPBExtensionDescriptor *)extension {
  // This is an internal method so we don't need to call CheckExtension().
  return [extensionMap_ objectForKey:extension];
}

- (BOOL)hasExtension:(GPBExtensionDescriptor *)extension {
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
  for (GPBExtensionDescriptor *extension in sortedExtensions) {
    uint32_t fieldNumber = extension.fieldNumber;
    if (fieldNumber >= start && fieldNumber < end) {
      id value = [extensionMap_ objectForKey:extension];
      GPBWriteExtensionValueToOutputStream(extension, value, output);
    }
  }
}

- (NSArray *)sortedExtensionsInUse {
  return [[extensionMap_ allKeys]
      sortedArrayUsingSelector:@selector(compareByFieldNumber:)];
}

- (void)setExtension:(GPBExtensionDescriptor *)extension value:(id)value {
  if (!value) {
    [self clearExtension:extension];
    return;
  }

  CheckExtension(self, extension);

  if (extension.repeated) {
    [NSException raise:NSInvalidArgumentException
                format:@"Must call addExtension() for repeated types."];
  }

  if (extensionMap_ == nil) {
    extensionMap_ = [[NSMutableDictionary alloc] init];
  }

  // This pointless cast is for CLANG_WARN_NULLABLE_TO_NONNULL_CONVERSION.
  // Without it, the compiler complains we're passing an id nullable when
  // setObject:forKey: requires a id nonnull for the value. The check for
  // !value at the start of the method ensures it isn't nil, but the check
  // isn't smart enough to realize that.
  [extensionMap_ setObject:(id)value forKey:extension];

  GPBExtensionDescriptor *descriptor = extension;

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

- (void)addExtension:(GPBExtensionDescriptor *)extension value:(id)value {
  CheckExtension(self, extension);

  if (!extension.repeated) {
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

- (void)setExtension:(GPBExtensionDescriptor *)extension
               index:(NSUInteger)idx
               value:(id)value {
  CheckExtension(self, extension);

  if (!extension.repeated) {
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

- (void)clearExtension:(GPBExtensionDescriptor *)extension {
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
  NSData *data = GPBCodedInputStreamReadRetainedBytesNoCopy(state);
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
    if (errorPtr) {
      *errorPtr = nil;
    }
  }
  @catch (NSException *exception) {
    message = nil;
    if (errorPtr) {
      *errorPtr = MessageErrorWithReason(GPBMessageErrorCodeMalformedData,
                                         exception.reason);
    }
  }
#ifdef DEBUG
  if (message && !message.initialized) {
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
  GPBExtensionDescriptor *extension = nil;
  GPBCodedInputStreamState *state = &input->state_;
  while (true) {
    uint32_t tag = GPBCodedInputStreamReadTag(state);
    if (tag == 0) {
      break;
    }

    if (tag == GPBWireFormatMessageSetTypeIdTag) {
      typeId = GPBCodedInputStreamReadUInt32(state);
      if (typeId != 0) {
        extension = [extensionRegistry extensionForDescriptor:[self descriptor]
                                                  fieldNumber:typeId];
      }
    } else if (tag == GPBWireFormatMessageSetMessageTag) {
      rawBytes =
          [GPBCodedInputStreamReadRetainedBytesNoCopy(state) autorelease];
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
      GPBExtensionMergeFromInputStream(extension,
                                       extension.packable,
                                       newInput,
                                       extensionRegistry,
                                       self);
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
  GPBExtensionDescriptor *extension =
      [extensionRegistry extensionForDescriptor:descriptor
                                    fieldNumber:fieldNumber];
  if (extension == nil) {
    if (descriptor.wireFormat && GPBWireFormatMessageSetItemTag == tag) {
      [self parseMessageSet:input extensionRegistry:extensionRegistry];
      return YES;
    }
  } else {
    if (extension.wireType == wireType) {
      GPBExtensionMergeFromInputStream(extension,
                                       extension.packable,
                                       input,
                                       extensionRegistry,
                                       self);
      return YES;
    }
    // Primitive, repeated types can be packed on unpacked on the wire, and are
    // parsed either way.
    if ([extension isRepeated] &&
        !GPBDataTypeIsObject(extension->description_->dataType) &&
        (extension.alternateWireType == wireType)) {
      GPBExtensionMergeFromInputStream(extension,
                                       !extension.packable,
                                       input,
                                       extensionRegistry,
                                       self);
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

static void MergeSingleFieldFromCodedInputStream(
    GPBMessage *self, GPBFieldDescriptor *field, GPBFileSyntax syntax,
    GPBCodedInputStream *input, GPBExtensionRegistry *extensionRegistry) {
  GPBDataType fieldDataType = GPBGetFieldDataType(field);
  switch (fieldDataType) {
#define CASE_SINGLE_POD(NAME, TYPE, FUNC_TYPE)                             \
    case GPBDataType##NAME: {                                              \
      TYPE val = GPBCodedInputStreamRead##NAME(&input->state_);            \
      GPBSet##FUNC_TYPE##IvarWithFieldInternal(self, field, val, syntax);  \
      break;                                                               \
            }
#define CASE_SINGLE_OBJECT(NAME)                                           \
    case GPBDataType##NAME: {                                              \
      id val = GPBCodedInputStreamReadRetained##NAME(&input->state_);      \
      GPBSetRetainedObjectIvarWithFieldInternal(self, field, val, syntax); \
      break;                                                               \
    }
      CASE_SINGLE_POD(Bool, BOOL, Bool)
      CASE_SINGLE_POD(Fixed32, uint32_t, UInt32)
      CASE_SINGLE_POD(SFixed32, int32_t, Int32)
      CASE_SINGLE_POD(Float, float, Float)
      CASE_SINGLE_POD(Fixed64, uint64_t, UInt64)
      CASE_SINGLE_POD(SFixed64, int64_t, Int64)
      CASE_SINGLE_POD(Double, double, Double)
      CASE_SINGLE_POD(Int32, int32_t, Int32)
      CASE_SINGLE_POD(Int64, int64_t, Int64)
      CASE_SINGLE_POD(SInt32, int32_t, Int32)
      CASE_SINGLE_POD(SInt64, int64_t, Int64)
      CASE_SINGLE_POD(UInt32, uint32_t, UInt32)
      CASE_SINGLE_POD(UInt64, uint64_t, UInt64)
      CASE_SINGLE_OBJECT(Bytes)
      CASE_SINGLE_OBJECT(String)
#undef CASE_SINGLE_POD
#undef CASE_SINGLE_OBJECT

    case GPBDataTypeMessage: {
      if (GPBGetHasIvarField(self, field)) {
        // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has
        // check again.
        GPBMessage *message =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [input readMessage:message extensionRegistry:extensionRegistry];
      } else {
        GPBMessage *message = [[field.msgClass alloc] init];
        [input readMessage:message extensionRegistry:extensionRegistry];
        GPBSetRetainedObjectIvarWithFieldInternal(self, field, message, syntax);
      }
      break;
    }

    case GPBDataTypeGroup: {
      if (GPBGetHasIvarField(self, field)) {
        // GPBGetObjectIvarWithFieldNoAutocreate() avoids doing the has
        // check again.
        GPBMessage *message =
            GPBGetObjectIvarWithFieldNoAutocreate(self, field);
        [input readGroup:GPBFieldNumber(field)
                      message:message
            extensionRegistry:extensionRegistry];
      } else {
        GPBMessage *message = [[field.msgClass alloc] init];
        [input readGroup:GPBFieldNumber(field)
                      message:message
            extensionRegistry:extensionRegistry];
        GPBSetRetainedObjectIvarWithFieldInternal(self, field, message, syntax);
      }
      break;
    }

    case GPBDataTypeEnum: {
      int32_t val = GPBCodedInputStreamReadEnum(&input->state_);
      if (GPBHasPreservingUnknownEnumSemantics(syntax) ||
          [field isValidEnumValue:val]) {
        GPBSetInt32IvarWithFieldInternal(self, field, val, syntax);
      } else {
        GPBUnknownFieldSet *unknownFields = GetOrMakeUnknownFields(self);
        [unknownFields mergeVarintField:GPBFieldNumber(field) value:val];
      }
    }
  }  // switch
}

static void MergeRepeatedPackedFieldFromCodedInputStream(
    GPBMessage *self, GPBFieldDescriptor *field, GPBFileSyntax syntax,
    GPBCodedInputStream *input) {
  GPBDataType fieldDataType = GPBGetFieldDataType(field);
  GPBCodedInputStreamState *state = &input->state_;
  id genericArray = GetOrCreateArrayIvarWithField(self, field, syntax);
  int32_t length = GPBCodedInputStreamReadInt32(state);
  size_t limit = GPBCodedInputStreamPushLimit(state, length);
  while (GPBCodedInputStreamBytesUntilLimit(state) > 0) {
    switch (fieldDataType) {
#define CASE_REPEATED_PACKED_POD(NAME, TYPE, ARRAY_TYPE)      \
     case GPBDataType##NAME: {                                \
       TYPE val = GPBCodedInputStreamRead##NAME(state);       \
       [(GPB##ARRAY_TYPE##Array *)genericArray addValue:val]; \
       break;                                                 \
     }
        CASE_REPEATED_PACKED_POD(Bool, BOOL, Bool)
        CASE_REPEATED_PACKED_POD(Fixed32, uint32_t, UInt32)
        CASE_REPEATED_PACKED_POD(SFixed32, int32_t, Int32)
        CASE_REPEATED_PACKED_POD(Float, float, Float)
        CASE_REPEATED_PACKED_POD(Fixed64, uint64_t, UInt64)
        CASE_REPEATED_PACKED_POD(SFixed64, int64_t, Int64)
        CASE_REPEATED_PACKED_POD(Double, double, Double)
        CASE_REPEATED_PACKED_POD(Int32, int32_t, Int32)
        CASE_REPEATED_PACKED_POD(Int64, int64_t, Int64)
        CASE_REPEATED_PACKED_POD(SInt32, int32_t, Int32)
        CASE_REPEATED_PACKED_POD(SInt64, int64_t, Int64)
        CASE_REPEATED_PACKED_POD(UInt32, uint32_t, UInt32)
        CASE_REPEATED_PACKED_POD(UInt64, uint64_t, UInt64)
#undef CASE_REPEATED_PACKED_POD

      case GPBDataTypeBytes:
      case GPBDataTypeString:
      case GPBDataTypeMessage:
      case GPBDataTypeGroup:
        NSCAssert(NO, @"Non primitive types can't be packed");
        break;

      case GPBDataTypeEnum: {
        int32_t val = GPBCodedInputStreamReadEnum(state);
        if (GPBHasPreservingUnknownEnumSemantics(syntax) ||
            [field isValidEnumValue:val]) {
          [(GPBEnumArray*)genericArray addRawValue:val];
        } else {
          GPBUnknownFieldSet *unknownFields = GetOrMakeUnknownFields(self);
          [unknownFields mergeVarintField:GPBFieldNumber(field) value:val];
        }
        break;
      }
    }  // switch
  }  // while(BytesUntilLimit() > 0)
  GPBCodedInputStreamPopLimit(state, limit);
}

static void MergeRepeatedNotPackedFieldFromCodedInputStream(
    GPBMessage *self, GPBFieldDescriptor *field, GPBFileSyntax syntax,
    GPBCodedInputStream *input, GPBExtensionRegistry *extensionRegistry) {
  GPBCodedInputStreamState *state = &input->state_;
  id genericArray = GetOrCreateArrayIvarWithField(self, field, syntax);
  switch (GPBGetFieldDataType(field)) {
#define CASE_REPEATED_NOT_PACKED_POD(NAME, TYPE, ARRAY_TYPE) \
   case GPBDataType##NAME: {                                 \
     TYPE val = GPBCodedInputStreamRead##NAME(state);        \
     [(GPB##ARRAY_TYPE##Array *)genericArray addValue:val];  \
     break;                                                  \
   }
#define CASE_REPEATED_NOT_PACKED_OBJECT(NAME)                \
   case GPBDataType##NAME: {                                 \
     id val = GPBCodedInputStreamReadRetained##NAME(state);  \
     [(NSMutableArray*)genericArray addObject:val];          \
     [val release];                                          \
     break;                                                  \
   }
      CASE_REPEATED_NOT_PACKED_POD(Bool, BOOL, Bool)
      CASE_REPEATED_NOT_PACKED_POD(Fixed32, uint32_t, UInt32)
      CASE_REPEATED_NOT_PACKED_POD(SFixed32, int32_t, Int32)
      CASE_REPEATED_NOT_PACKED_POD(Float, float, Float)
      CASE_REPEATED_NOT_PACKED_POD(Fixed64, uint64_t, UInt64)
      CASE_REPEATED_NOT_PACKED_POD(SFixed64, int64_t, Int64)
      CASE_REPEATED_NOT_PACKED_POD(Double, double, Double)
      CASE_REPEATED_NOT_PACKED_POD(Int32, int32_t, Int32)
      CASE_REPEATED_NOT_PACKED_POD(Int64, int64_t, Int64)
      CASE_REPEATED_NOT_PACKED_POD(SInt32, int32_t, Int32)
      CASE_REPEATED_NOT_PACKED_POD(SInt64, int64_t, Int64)
      CASE_REPEATED_NOT_PACKED_POD(UInt32, uint32_t, UInt32)
      CASE_REPEATED_NOT_PACKED_POD(UInt64, uint64_t, UInt64)
      CASE_REPEATED_NOT_PACKED_OBJECT(Bytes)
      CASE_REPEATED_NOT_PACKED_OBJECT(String)
#undef CASE_REPEATED_NOT_PACKED_POD
#undef CASE_NOT_PACKED_OBJECT
    case GPBDataTypeMessage: {
      GPBMessage *message = [[field.msgClass alloc] init];
      [input readMessage:message extensionRegistry:extensionRegistry];
      [(NSMutableArray*)genericArray addObject:message];
      [message release];
      break;
    }
    case GPBDataTypeGroup: {
      GPBMessage *message = [[field.msgClass alloc] init];
      [input readGroup:GPBFieldNumber(field)
                    message:message
          extensionRegistry:extensionRegistry];
      [(NSMutableArray*)genericArray addObject:message];
      [message release];
      break;
    }
    case GPBDataTypeEnum: {
      int32_t val = GPBCodedInputStreamReadEnum(state);
      if (GPBHasPreservingUnknownEnumSemantics(syntax) ||
          [field isValidEnumValue:val]) {
        [(GPBEnumArray*)genericArray addRawValue:val];
      } else {
        GPBUnknownFieldSet *unknownFields = GetOrMakeUnknownFields(self);
        [unknownFields mergeVarintField:GPBFieldNumber(field) value:val];
      }
      break;
    }
  }  // switch
}

- (void)mergeFromCodedInputStream:(GPBCodedInputStream *)input
                extensionRegistry:(GPBExtensionRegistry *)extensionRegistry {
  GPBDescriptor *descriptor = [self descriptor];
  GPBFileSyntax syntax = descriptor.file.syntax;
  GPBCodedInputStreamState *state = &input->state_;
  uint32_t tag = 0;
  NSUInteger startingIndex = 0;
  NSArray *fields = descriptor->fields_;
  NSUInteger numFields = fields.count;
  while (YES) {
    BOOL merged = NO;
    tag = GPBCodedInputStreamReadTag(state);
    for (NSUInteger i = 0; i < numFields; ++i) {
      if (startingIndex >= numFields) startingIndex = 0;
      GPBFieldDescriptor *fieldDescriptor = fields[startingIndex];
      if (GPBFieldTag(fieldDescriptor) == tag) {
        GPBFieldType fieldType = fieldDescriptor.fieldType;
        if (fieldType == GPBFieldTypeSingle) {
          MergeSingleFieldFromCodedInputStream(self, fieldDescriptor, syntax,
                                               input, extensionRegistry);
          // Well formed protos will only have a single field once, advance
          // the starting index to the next field.
          startingIndex += 1;
        } else if (fieldType == GPBFieldTypeRepeated) {
          if (fieldDescriptor.isPackable) {
            MergeRepeatedPackedFieldFromCodedInputStream(
                self, fieldDescriptor, syntax, input);
            // Well formed protos will only have a repeated field that is
            // packed once, advance the starting index to the next field.
            startingIndex += 1;
          } else {
            MergeRepeatedNotPackedFieldFromCodedInputStream(
                self, fieldDescriptor, syntax, input, extensionRegistry);
          }
        } else {  // fieldType == GPBFieldTypeMap
          // GPB*Dictionary or NSDictionary, exact type doesn't matter at this
          // point.
          id map = GetOrCreateMapIvarWithField(self, fieldDescriptor, syntax);
          [input readMapEntry:map
            extensionRegistry:extensionRegistry
                        field:fieldDescriptor
                parentMessage:self];
        }
        merged = YES;
        break;
      } else {
        startingIndex += 1;
      }
    }  // for(i < numFields)

    if (!merged) {
      // Primitive, repeated types can be packed on unpacked on the wire, and
      // are parsed either way.  The above loop covered tag in the preferred
      // for, so this need to check the alternate form.
      for (NSUInteger i = 0; i < numFields; ++i) {
        if (startingIndex >= numFields) startingIndex = 0;
        GPBFieldDescriptor *fieldDescriptor = fields[startingIndex];
        if ((fieldDescriptor.fieldType == GPBFieldTypeRepeated) &&
            !GPBFieldDataTypeIsObject(fieldDescriptor) &&
            (GPBFieldAlternateTag(fieldDescriptor) == tag)) {
          BOOL alternateIsPacked = !fieldDescriptor.isPackable;
          if (alternateIsPacked) {
            MergeRepeatedPackedFieldFromCodedInputStream(
                self, fieldDescriptor, syntax, input);
            // Well formed protos will only have a repeated field that is
            // packed once, advance the starting index to the next field.
            startingIndex += 1;
          } else {
            MergeRepeatedNotPackedFieldFromCodedInputStream(
                self, fieldDescriptor, syntax, input, extensionRegistry);
          }
          merged = YES;
          break;
        } else {
          startingIndex += 1;
        }
      }
    }

    if (!merged) {
      if (tag == 0) {
        // zero signals EOF / limit reached
        return;
      } else {
        if (GPBPreserveUnknownFields(syntax)) {
          if (![self parseUnknownField:input
                     extensionRegistry:extensionRegistry
                                   tag:tag]) {
            // it's an endgroup tag
            return;
          }
        } else {
          if (![input skipField:tag]) {
            return;
          }
        }
      }
    }  // if(!merged)

  }  // while(YES)
}

#pragma mark - MergeFrom Support

- (void)mergeFrom:(GPBMessage *)other {
  Class selfClass = [self class];
  Class otherClass = [other class];
  if (!([selfClass isSubclassOfClass:otherClass] ||
        [otherClass isSubclassOfClass:selfClass])) {
    [NSException raise:NSInvalidArgumentException
                format:@"Classes must match %@ != %@", selfClass, otherClass];
  }

  // We assume something will be done and become visible.
  GPBBecomeVisibleToAutocreator(self);

  GPBDescriptor *descriptor = [[self class] descriptor];
  GPBFileSyntax syntax = descriptor.file.syntax;

  for (GPBFieldDescriptor *field in descriptor->fields_) {
    GPBFieldType fieldType = field.fieldType;
    if (fieldType == GPBFieldTypeSingle) {
      int32_t hasIndex = GPBFieldHasIndex(field);
      uint32_t fieldNumber = GPBFieldNumber(field);
      if (!GPBGetHasIvar(other, hasIndex, fieldNumber)) {
        // Other doesn't have the field set, on to the next.
        continue;
      }
      GPBDataType fieldDataType = GPBGetFieldDataType(field);
      switch (fieldDataType) {
        case GPBDataTypeBool:
          GPBSetBoolIvarWithFieldInternal(
              self, field, GPBGetMessageBoolField(other, field), syntax);
          break;
        case GPBDataTypeSFixed32:
        case GPBDataTypeEnum:
        case GPBDataTypeInt32:
        case GPBDataTypeSInt32:
          GPBSetInt32IvarWithFieldInternal(
              self, field, GPBGetMessageInt32Field(other, field), syntax);
          break;
        case GPBDataTypeFixed32:
        case GPBDataTypeUInt32:
          GPBSetUInt32IvarWithFieldInternal(
              self, field, GPBGetMessageUInt32Field(other, field), syntax);
          break;
        case GPBDataTypeSFixed64:
        case GPBDataTypeInt64:
        case GPBDataTypeSInt64:
          GPBSetInt64IvarWithFieldInternal(
              self, field, GPBGetMessageInt64Field(other, field), syntax);
          break;
        case GPBDataTypeFixed64:
        case GPBDataTypeUInt64:
          GPBSetUInt64IvarWithFieldInternal(
              self, field, GPBGetMessageUInt64Field(other, field), syntax);
          break;
        case GPBDataTypeFloat:
          GPBSetFloatIvarWithFieldInternal(
              self, field, GPBGetMessageFloatField(other, field), syntax);
          break;
        case GPBDataTypeDouble:
          GPBSetDoubleIvarWithFieldInternal(
              self, field, GPBGetMessageDoubleField(other, field), syntax);
          break;
        case GPBDataTypeBytes:
        case GPBDataTypeString: {
          id otherVal = GPBGetObjectIvarWithFieldNoAutocreate(other, field);
          GPBSetObjectIvarWithFieldInternal(self, field, otherVal, syntax);
          break;
        }
        case GPBDataTypeMessage:
        case GPBDataTypeGroup: {
          id otherVal = GPBGetObjectIvarWithFieldNoAutocreate(other, field);
          if (GPBGetHasIvar(self, hasIndex, fieldNumber)) {
            GPBMessage *message =
                GPBGetObjectIvarWithFieldNoAutocreate(self, field);
            [message mergeFrom:otherVal];
          } else {
            GPBMessage *message = [otherVal copy];
            GPBSetRetainedObjectIvarWithFieldInternal(self, field, message,
                                                      syntax);
          }
          break;
        }
      } // switch()
    } else if (fieldType == GPBFieldTypeRepeated) {
      // In the case of a list, they need to be appended, and there is no
      // _hasIvar to worry about setting.
      id otherArray =
          GPBGetObjectIvarWithFieldNoAutocreate(other, field);
      if (otherArray) {
        GPBDataType fieldDataType = field->description_->dataType;
        if (GPBDataTypeIsObject(fieldDataType)) {
          NSMutableArray *resultArray =
              GetOrCreateArrayIvarWithField(self, field, syntax);
          [resultArray addObjectsFromArray:otherArray];
        } else if (fieldDataType == GPBDataTypeEnum) {
          GPBEnumArray *resultArray =
              GetOrCreateArrayIvarWithField(self, field, syntax);
          [resultArray addRawValuesFromArray:otherArray];
        } else {
          // The array type doesn't matter, that all implment
          // -addValuesFromArray:.
          GPBInt32Array *resultArray =
              GetOrCreateArrayIvarWithField(self, field, syntax);
          [resultArray addValuesFromArray:otherArray];
        }
      }
    } else {  // fieldType = GPBFieldTypeMap
      // In the case of a map, they need to be merged, and there is no
      // _hasIvar to worry about setting.
      id otherDict = GPBGetObjectIvarWithFieldNoAutocreate(other, field);
      if (otherDict) {
        GPBDataType keyDataType = field.mapKeyDataType;
        GPBDataType valueDataType = field->description_->dataType;
        if (GPBDataTypeIsObject(keyDataType) &&
            GPBDataTypeIsObject(valueDataType)) {
          NSMutableDictionary *resultDict =
              GetOrCreateMapIvarWithField(self, field, syntax);
          [resultDict addEntriesFromDictionary:otherDict];
        } else if (valueDataType == GPBDataTypeEnum) {
          // The exact type doesn't matter, just need to know it is a
          // GPB*EnumDictionary.
          GPBInt32EnumDictionary *resultDict =
              GetOrCreateMapIvarWithField(self, field, syntax);
          [resultDict addRawEntriesFromDictionary:otherDict];
        } else {
          // The exact type doesn't matter, they all implement
          // -addEntriesFromDictionary:.
          GPBInt32Int32Dictionary *resultDict =
              GetOrCreateMapIvarWithField(self, field, syntax);
          [resultDict addEntriesFromDictionary:otherDict];
        }
      }
    }  // if (fieldType)..else if...else
  }  // for(fields)

  // Unknown fields.
  if (!unknownFields_) {
    [self setUnknownFields:other.unknownFields];
  } else {
    [unknownFields_ mergeUnknownFields:other.unknownFields];
  }

  // Extensions

  if (other->extensionMap_.count == 0) {
    return;
  }

  if (extensionMap_ == nil) {
    extensionMap_ =
        CloneExtensionMap(other->extensionMap_, NSZoneFromPointer(self));
  } else {
    for (GPBExtensionDescriptor *extension in other->extensionMap_) {
      id otherValue = [other->extensionMap_ objectForKey:extension];
      id value = [extensionMap_ objectForKey:extension];
      BOOL isMessageExtension = GPBExtensionIsMessage(extension);

      if (extension.repeated) {
        NSMutableArray *list = value;
        if (list == nil) {
          list = [[NSMutableArray alloc] init];
          [extensionMap_ setObject:list forKey:extension];
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
            [extensionMap_ setObject:copiedValue forKey:extension];
            [copiedValue release];
          }
        } else {
          [extensionMap_ setObject:otherValue forKey:extension];
        }
      }

      if (isMessageExtension && !extension.isRepeated) {
        GPBMessage *autocreatedValue =
            [[autocreatedExtensionMap_ objectForKey:extension] retain];
        // Must remove from the map before calling GPBClearMessageAutocreator()
        // so that GPBClearMessageAutocreator() knows its safe to clear.
        [autocreatedExtensionMap_ removeObjectForKey:extension];
        GPBClearMessageAutocreator(autocreatedValue);
        [autocreatedValue release];
      }
    }
  }
}

#pragma mark - isEqual: & hash Support

- (BOOL)isEqual:(GPBMessage *)other {
  if (other == self) {
    return YES;
  }
  if (![other isKindOfClass:[self class]] &&
      ![self isKindOfClass:[other class]]) {
    return NO;
  }

  GPBDescriptor *descriptor = [[self class] descriptor];
  uint8_t *selfStorage = (uint8_t *)messageStorage_;
  uint8_t *otherStorage = (uint8_t *)other->messageStorage_;

  for (GPBFieldDescriptor *field in descriptor->fields_) {
    if (GPBFieldIsMapOrArray(field)) {
      // In the case of a list or map, there is no _hasIvar to worry about.
      // NOTE: These are NSArray/GPB*Array or NSDictionary/GPB*Dictionary, but
      // the type doesn't really matter as the objects all support -count and
      // -isEqual:.
      NSArray *resultMapOrArray =
          GPBGetObjectIvarWithFieldNoAutocreate(self, field);
      NSArray *otherMapOrArray =
          GPBGetObjectIvarWithFieldNoAutocreate(other, field);
      // nil and empty are equal
      if (resultMapOrArray.count != 0 || otherMapOrArray.count != 0) {
        if (![resultMapOrArray isEqual:otherMapOrArray]) {
          return NO;
        }
      }
    } else {  // Single field
      int32_t hasIndex = GPBFieldHasIndex(field);
      uint32_t fieldNum = GPBFieldNumber(field);
      BOOL selfHas = GPBGetHasIvar(self, hasIndex, fieldNum);
      BOOL otherHas = GPBGetHasIvar(other, hasIndex, fieldNum);
      if (selfHas != otherHas) {
        return NO;  // Differing has values, not equal.
      }
      if (!selfHas) {
        // Same has values, was no, nothing else to check for this field.
        continue;
      }
      // Now compare the values.
      GPBDataType fieldDataType = GPBGetFieldDataType(field);
      size_t fieldOffset = field->description_->offset;
      switch (fieldDataType) {
        case GPBDataTypeBool: {
          // Bools are stored in has_bits to avoid needing explicit space in
          // the storage structure.
          // (the field number passed to the HasIvar helper doesn't really
          // matter since the offset is never negative)
          BOOL selfValue = GPBGetHasIvar(self, (int32_t)(fieldOffset), 0);
          BOOL otherValue = GPBGetHasIvar(other, (int32_t)(fieldOffset), 0);
          if (selfValue != otherValue) {
            return NO;
          }
          break;
        }
        case GPBDataTypeSFixed32:
        case GPBDataTypeInt32:
        case GPBDataTypeSInt32:
        case GPBDataTypeEnum:
        case GPBDataTypeFixed32:
        case GPBDataTypeUInt32:
        case GPBDataTypeFloat: {
          _GPBCompileAssert(sizeof(float) == sizeof(uint32_t), float_not_32_bits);
          // These are all 32bit, signed/unsigned doesn't matter for equality.
          uint32_t *selfValPtr = (uint32_t *)&selfStorage[fieldOffset];
          uint32_t *otherValPtr = (uint32_t *)&otherStorage[fieldOffset];
          if (*selfValPtr != *otherValPtr) {
            return NO;
          }
          break;
        }
        case GPBDataTypeSFixed64:
        case GPBDataTypeInt64:
        case GPBDataTypeSInt64:
        case GPBDataTypeFixed64:
        case GPBDataTypeUInt64:
        case GPBDataTypeDouble: {
          _GPBCompileAssert(sizeof(double) == sizeof(uint64_t), double_not_64_bits);
          // These are all 64bit, signed/unsigned doesn't matter for equality.
          uint64_t *selfValPtr = (uint64_t *)&selfStorage[fieldOffset];
          uint64_t *otherValPtr = (uint64_t *)&otherStorage[fieldOffset];
          if (*selfValPtr != *otherValPtr) {
            return NO;
          }
          break;
        }
        case GPBDataTypeBytes:
        case GPBDataTypeString:
        case GPBDataTypeMessage:
        case GPBDataTypeGroup: {
          // Type doesn't matter here, they all implement -isEqual:.
          id *selfValPtr = (id *)&selfStorage[fieldOffset];
          id *otherValPtr = (id *)&otherStorage[fieldOffset];
          if (![*selfValPtr isEqual:*otherValPtr]) {
            return NO;
          }
          break;
        }
      } // switch()
    }   // if(mapOrArray)...else
  }  // for(fields)

  // nil and empty are equal
  if (extensionMap_.count != 0 || other->extensionMap_.count != 0) {
    if (![extensionMap_ isEqual:other->extensionMap_]) {
      return NO;
    }
  }

  // nil and empty are equal
  GPBUnknownFieldSet *otherUnknowns = other->unknownFields_;
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
  uint8_t *storage = (uint8_t *)messageStorage_;

  // Start with the descriptor and then mix it with some instance info.
  // Hopefully that will give a spread based on classes and what fields are set.
  NSUInteger result = (NSUInteger)descriptor;

  for (GPBFieldDescriptor *field in descriptor->fields_) {
    if (GPBFieldIsMapOrArray(field)) {
      // Exact type doesn't matter, just check if there are any elements.
      NSArray *mapOrArray = GPBGetObjectIvarWithFieldNoAutocreate(self, field);
      NSUInteger count = mapOrArray.count;
      if (count) {
        // NSArray/NSDictionary use count, use the field number and the count.
        result = prime * result + GPBFieldNumber(field);
        result = prime * result + count;
      }
    } else if (GPBGetHasIvarField(self, field)) {
      // Just using the field number seemed simple/fast, but then a small
      // message class where all the same fields are always set (to different
      // things would end up all with the same hash, so pull in some data).
      GPBDataType fieldDataType = GPBGetFieldDataType(field);
      size_t fieldOffset = field->description_->offset;
      switch (fieldDataType) {
        case GPBDataTypeBool: {
          // Bools are stored in has_bits to avoid needing explicit space in
          // the storage structure.
          // (the field number passed to the HasIvar helper doesn't really
          // matter since the offset is never negative)
          BOOL value = GPBGetHasIvar(self, (int32_t)(fieldOffset), 0);
          result = prime * result + value;
          break;
        }
        case GPBDataTypeSFixed32:
        case GPBDataTypeInt32:
        case GPBDataTypeSInt32:
        case GPBDataTypeEnum:
        case GPBDataTypeFixed32:
        case GPBDataTypeUInt32:
        case GPBDataTypeFloat: {
          _GPBCompileAssert(sizeof(float) == sizeof(uint32_t), float_not_32_bits);
          // These are all 32bit, just mix it in.
          uint32_t *valPtr = (uint32_t *)&storage[fieldOffset];
          result = prime * result + *valPtr;
          break;
        }
        case GPBDataTypeSFixed64:
        case GPBDataTypeInt64:
        case GPBDataTypeSInt64:
        case GPBDataTypeFixed64:
        case GPBDataTypeUInt64:
        case GPBDataTypeDouble: {
          _GPBCompileAssert(sizeof(double) == sizeof(uint64_t), double_not_64_bits);
          // These are all 64bit, just mix what fits into an NSUInteger in.
          uint64_t *valPtr = (uint64_t *)&storage[fieldOffset];
          result = prime * result + (NSUInteger)(*valPtr);
          break;
        }
        case GPBDataTypeBytes:
        case GPBDataTypeString: {
          // Type doesn't matter here, they both implement -hash:.
          id *valPtr = (id *)&storage[fieldOffset];
          result = prime * result + [*valPtr hash];
          break;
        }

        case GPBDataTypeMessage:
        case GPBDataTypeGroup: {
          GPBMessage **valPtr = (GPBMessage **)&storage[fieldOffset];
          // Could call -hash on the sub message, but that could recurse pretty
          // deep; follow the lead of NSArray/NSDictionary and don't really
          // recurse for hash, instead use the field number and the descriptor
          // of the sub message.  Yes, this could suck for a bunch of messages
          // where they all only differ in the sub messages, but if you are
          // using a message with sub messages for something that needs -hash,
          // odds are you are also copying them as keys, and that deep copy
          // will also suck.
          result = prime * result + GPBFieldNumber(field);
          result = prime * result + (NSUInteger)[[*valPtr class] descriptor];
          break;
        }
      } // switch()
    }
  }

  // Unknowns and extensions are not included.

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

#pragma mark - SerializedSize

- (size_t)serializedSize {
  GPBDescriptor *descriptor = [[self class] descriptor];
  size_t result = 0;

  // Has check is done explicitly, so GPBGetObjectIvarWithFieldNoAutocreate()
  // avoids doing the has check again.

  // Fields.
  for (GPBFieldDescriptor *fieldDescriptor in descriptor->fields_) {
    GPBFieldType fieldType = fieldDescriptor.fieldType;
    GPBDataType fieldDataType = GPBGetFieldDataType(fieldDescriptor);

    // Single Fields
    if (fieldType == GPBFieldTypeSingle) {
      BOOL selfHas = GPBGetHasIvarField(self, fieldDescriptor);
      if (!selfHas) {
        continue;  // Nothing to do.
      }

      uint32_t fieldNumber = GPBFieldNumber(fieldDescriptor);

      switch (fieldDataType) {
#define CASE_SINGLE_POD(NAME, TYPE, FUNC_TYPE)                                \
        case GPBDataType##NAME: {                                             \
          TYPE fieldVal = GPBGetMessage##FUNC_TYPE##Field(self, fieldDescriptor); \
          result += GPBCompute##NAME##Size(fieldNumber, fieldVal);            \
          break;                                                              \
        }
#define CASE_SINGLE_OBJECT(NAME)                                              \
        case GPBDataType##NAME: {                                             \
          id fieldVal = GPBGetObjectIvarWithFieldNoAutocreate(self, fieldDescriptor); \
          result += GPBCompute##NAME##Size(fieldNumber, fieldVal);            \
          break;                                                              \
        }
          CASE_SINGLE_POD(Bool, BOOL, Bool)
          CASE_SINGLE_POD(Fixed32, uint32_t, UInt32)
          CASE_SINGLE_POD(SFixed32, int32_t, Int32)
          CASE_SINGLE_POD(Float, float, Float)
          CASE_SINGLE_POD(Fixed64, uint64_t, UInt64)
          CASE_SINGLE_POD(SFixed64, int64_t, Int64)
          CASE_SINGLE_POD(Double, double, Double)
          CASE_SINGLE_POD(Int32, int32_t, Int32)
          CASE_SINGLE_POD(Int64, int64_t, Int64)
          CASE_SINGLE_POD(SInt32, int32_t, Int32)
          CASE_SINGLE_POD(SInt64, int64_t, Int64)
          CASE_SINGLE_POD(UInt32, uint32_t, UInt32)
          CASE_SINGLE_POD(UInt64, uint64_t, UInt64)
          CASE_SINGLE_OBJECT(Bytes)
          CASE_SINGLE_OBJECT(String)
          CASE_SINGLE_OBJECT(Message)
          CASE_SINGLE_OBJECT(Group)
          CASE_SINGLE_POD(Enum, int32_t, Int32)
#undef CASE_SINGLE_POD
#undef CASE_SINGLE_OBJECT
      }

    // Repeated Fields
    } else if (fieldType == GPBFieldTypeRepeated) {
      id genericArray =
          GPBGetObjectIvarWithFieldNoAutocreate(self, fieldDescriptor);
      NSUInteger count = [genericArray count];
      if (count == 0) {
        continue;  // Nothing to add.
      }
      __block size_t dataSize = 0;

      switch (fieldDataType) {
#define CASE_REPEATED_POD(NAME, TYPE, ARRAY_TYPE)                             \
    CASE_REPEATED_POD_EXTRA(NAME, TYPE, ARRAY_TYPE, )
#define CASE_REPEATED_POD_EXTRA(NAME, TYPE, ARRAY_TYPE, ARRAY_ACCESSOR_NAME)  \
        case GPBDataType##NAME: {                                             \
          GPB##ARRAY_TYPE##Array *array = genericArray;                       \
          [array enumerate##ARRAY_ACCESSOR_NAME##ValuesWithBlock:^(TYPE value, NSUInteger idx, BOOL *stop) { \
            _Pragma("unused(idx, stop)");                                     \
            dataSize += GPBCompute##NAME##SizeNoTag(value);                   \
          }];                                                                 \
          break;                                                              \
        }
#define CASE_REPEATED_OBJECT(NAME)                                            \
        case GPBDataType##NAME: {                                             \
          for (id value in genericArray) {                                    \
            dataSize += GPBCompute##NAME##SizeNoTag(value);                   \
          }                                                                   \
          break;                                                              \
        }
          CASE_REPEATED_POD(Bool, BOOL, Bool)
          CASE_REPEATED_POD(Fixed32, uint32_t, UInt32)
          CASE_REPEATED_POD(SFixed32, int32_t, Int32)
          CASE_REPEATED_POD(Float, float, Float)
          CASE_REPEATED_POD(Fixed64, uint64_t, UInt64)
          CASE_REPEATED_POD(SFixed64, int64_t, Int64)
          CASE_REPEATED_POD(Double, double, Double)
          CASE_REPEATED_POD(Int32, int32_t, Int32)
          CASE_REPEATED_POD(Int64, int64_t, Int64)
          CASE_REPEATED_POD(SInt32, int32_t, Int32)
          CASE_REPEATED_POD(SInt64, int64_t, Int64)
          CASE_REPEATED_POD(UInt32, uint32_t, UInt32)
          CASE_REPEATED_POD(UInt64, uint64_t, UInt64)
          CASE_REPEATED_OBJECT(Bytes)
          CASE_REPEATED_OBJECT(String)
          CASE_REPEATED_OBJECT(Message)
          CASE_REPEATED_OBJECT(Group)
          CASE_REPEATED_POD_EXTRA(Enum, int32_t, Enum, Raw)
#undef CASE_REPEATED_POD
#undef CASE_REPEATED_POD_EXTRA
#undef CASE_REPEATED_OBJECT
      }  // switch
      result += dataSize;
      size_t tagSize = GPBComputeTagSize(GPBFieldNumber(fieldDescriptor));
      if (fieldDataType == GPBDataTypeGroup) {
        // Groups have both a start and an end tag.
        tagSize *= 2;
      }
      if (fieldDescriptor.isPackable) {
        result += tagSize;
        result += GPBComputeSizeTSizeAsInt32NoTag(dataSize);
      } else {
        result += count * tagSize;
      }

    // Map<> Fields
    } else {  // fieldType == GPBFieldTypeMap
      if (GPBDataTypeIsObject(fieldDataType) &&
          (fieldDescriptor.mapKeyDataType == GPBDataTypeString)) {
        // If key type was string, then the map is an NSDictionary.
        NSDictionary *map =
            GPBGetObjectIvarWithFieldNoAutocreate(self, fieldDescriptor);
        if (map) {
          result += GPBDictionaryComputeSizeInternalHelper(map, fieldDescriptor);
        }
      } else {
        // Type will be GPB*GroupDictionary, exact type doesn't matter.
        GPBInt32Int32Dictionary *map =
            GPBGetObjectIvarWithFieldNoAutocreate(self, fieldDescriptor);
        result += [map computeSerializedSizeAsField:fieldDescriptor];
      }
    }
  }  // for(fields)

  // Add any unknown fields.
  if (descriptor.wireFormat) {
    result += [unknownFields_ serializedSizeAsMessageSet];
  } else {
    result += [unknownFields_ serializedSize];
  }

  // Add any extensions.
  for (GPBExtensionDescriptor *extension in extensionMap_) {
    id value = [extensionMap_ objectForKey:extension];
    result += GPBComputeExtensionSerializedSizeIncludingTag(extension, value);
  }

  return result;
}

#pragma mark - Resolve Methods Support

typedef struct ResolveIvarAccessorMethodResult {
  IMP impToAdd;
  SEL encodingSelector;
} ResolveIvarAccessorMethodResult;

static void ResolveIvarGet(GPBFieldDescriptor *field,
                           ResolveIvarAccessorMethodResult *result) {
  GPBDataType fieldDataType = GPBGetFieldDataType(field);
  switch (fieldDataType) {
#define CASE_GET(NAME, TYPE, TRUE_NAME)                          \
    case GPBDataType##NAME: {                                    \
      result->impToAdd = imp_implementationWithBlock(^(id obj) { \
        return GPBGetMessage##TRUE_NAME##Field(obj, field);      \
       });                                                       \
      result->encodingSelector = @selector(get##NAME);           \
      break;                                                     \
    }
#define CASE_GET_OBJECT(NAME, TYPE, TRUE_NAME)                   \
    case GPBDataType##NAME: {                                    \
      result->impToAdd = imp_implementationWithBlock(^(id obj) { \
        return GPBGetObjectIvarWithField(obj, field);            \
       });                                                       \
      result->encodingSelector = @selector(get##NAME);           \
      break;                                                     \
    }
      CASE_GET(Bool, BOOL, Bool)
      CASE_GET(Fixed32, uint32_t, UInt32)
      CASE_GET(SFixed32, int32_t, Int32)
      CASE_GET(Float, float, Float)
      CASE_GET(Fixed64, uint64_t, UInt64)
      CASE_GET(SFixed64, int64_t, Int64)
      CASE_GET(Double, double, Double)
      CASE_GET(Int32, int32_t, Int32)
      CASE_GET(Int64, int64_t, Int64)
      CASE_GET(SInt32, int32_t, Int32)
      CASE_GET(SInt64, int64_t, Int64)
      CASE_GET(UInt32, uint32_t, UInt32)
      CASE_GET(UInt64, uint64_t, UInt64)
      CASE_GET_OBJECT(Bytes, id, Object)
      CASE_GET_OBJECT(String, id, Object)
      CASE_GET_OBJECT(Message, id, Object)
      CASE_GET_OBJECT(Group, id, Object)
      CASE_GET(Enum, int32_t, Enum)
#undef CASE_GET
  }
}

static void ResolveIvarSet(GPBFieldDescriptor *field,
                           GPBFileSyntax syntax,
                           ResolveIvarAccessorMethodResult *result) {
  GPBDataType fieldDataType = GPBGetFieldDataType(field);
  switch (fieldDataType) {
#define CASE_SET(NAME, TYPE, TRUE_NAME)                                       \
    case GPBDataType##NAME: {                                                 \
      result->impToAdd = imp_implementationWithBlock(^(id obj, TYPE value) {  \
        return GPBSet##TRUE_NAME##IvarWithFieldInternal(obj, field, value, syntax); \
      });                                                                     \
      result->encodingSelector = @selector(set##NAME:);                       \
      break;                                                                  \
    }
      CASE_SET(Bool, BOOL, Bool)
      CASE_SET(Fixed32, uint32_t, UInt32)
      CASE_SET(SFixed32, int32_t, Int32)
      CASE_SET(Float, float, Float)
      CASE_SET(Fixed64, uint64_t, UInt64)
      CASE_SET(SFixed64, int64_t, Int64)
      CASE_SET(Double, double, Double)
      CASE_SET(Int32, int32_t, Int32)
      CASE_SET(Int64, int64_t, Int64)
      CASE_SET(SInt32, int32_t, Int32)
      CASE_SET(SInt64, int64_t, Int64)
      CASE_SET(UInt32, uint32_t, UInt32)
      CASE_SET(UInt64, uint64_t, UInt64)
      CASE_SET(Bytes, id, Object)
      CASE_SET(String, id, Object)
      CASE_SET(Message, id, Object)
      CASE_SET(Group, id, Object)
      CASE_SET(Enum, int32_t, Enum)
#undef CASE_SET
  }
}

+ (BOOL)resolveInstanceMethod:(SEL)sel {
  const GPBDescriptor *descriptor = [self descriptor];
  if (!descriptor) {
    return NO;
  }

  // NOTE: hasOrCountSel_/setHasSel_ will be NULL if the field for the given
  // message should not have has support (done in GPBDescriptor.m), so there is
  // no need for checks here to see if has*/setHas* are allowed.

  ResolveIvarAccessorMethodResult result = {NULL, NULL};
  for (GPBFieldDescriptor *field in descriptor->fields_) {
    BOOL isMapOrArray = GPBFieldIsMapOrArray(field);
    if (!isMapOrArray) {
      // Single fields.
      if (sel == field->getSel_) {
        ResolveIvarGet(field, &result);
        break;
      } else if (sel == field->setSel_) {
        ResolveIvarSet(field, descriptor.file.syntax, &result);
        break;
      } else if (sel == field->hasOrCountSel_) {
        int32_t index = GPBFieldHasIndex(field);
        uint32_t fieldNum = GPBFieldNumber(field);
        result.impToAdd = imp_implementationWithBlock(^(id obj) {
          return GPBGetHasIvar(obj, index, fieldNum);
        });
        result.encodingSelector = @selector(getBool);
        break;
      } else if (sel == field->setHasSel_) {
        result.impToAdd = imp_implementationWithBlock(^(id obj, BOOL value) {
          if (value) {
            [NSException raise:NSInvalidArgumentException
                        format:@"%@: %@ can only be set to NO (to clear field).",
                               [obj class],
                               NSStringFromSelector(field->setHasSel_)];
          }
          GPBClearMessageField(obj, field);
        });
        result.encodingSelector = @selector(setBool:);
        break;
      } else {
        GPBOneofDescriptor *oneof = field->containingOneof_;
        if (oneof && (sel == oneof->caseSel_)) {
          int32_t index = GPBFieldHasIndex(field);
          result.impToAdd = imp_implementationWithBlock(^(id obj) {
            return GPBGetHasOneof(obj, index);
          });
          result.encodingSelector = @selector(getEnum);
          break;
        }
      }
    } else {
      // map<>/repeated fields.
      if (sel == field->getSel_) {
        if (field.fieldType == GPBFieldTypeRepeated) {
          result.impToAdd = imp_implementationWithBlock(^(id obj) {
            return GetArrayIvarWithField(obj, field);
          });
        } else {
          result.impToAdd = imp_implementationWithBlock(^(id obj) {
            return GetMapIvarWithField(obj, field);
          });
        }
        result.encodingSelector = @selector(getArray);
        break;
      } else if (sel == field->setSel_) {
        // Local for syntax so the block can directly capture it and not the
        // full lookup.
        const GPBFileSyntax syntax = descriptor.file.syntax;
        result.impToAdd = imp_implementationWithBlock(^(id obj, id value) {
          return GPBSetObjectIvarWithFieldInternal(obj, field, value, syntax);
        });
        result.encodingSelector = @selector(setArray:);
        break;
      } else if (sel == field->hasOrCountSel_) {
        result.impToAdd = imp_implementationWithBlock(^(id obj) {
          // Type doesn't matter, all *Array and *Dictionary types support
          // -count.
          NSArray *arrayOrMap =
              GPBGetObjectIvarWithFieldNoAutocreate(obj, field);
          return [arrayOrMap count];
        });
        result.encodingSelector = @selector(getArrayCount);
        break;
      }
    }
  }
  if (result.impToAdd) {
    const char *encoding =
        GPBMessageEncodingForSelector(result.encodingSelector, YES);
    BOOL methodAdded = class_addMethod(descriptor.messageClass, sel,
                                       result.impToAdd, encoding);
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
