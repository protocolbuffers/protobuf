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

#import <Foundation/Foundation.h>

#import "GPBArray.h"
#import "GPBMessage.h"
#import "GPBRuntimeTypes.h"

CF_EXTERN_C_BEGIN

NS_ASSUME_NONNULL_BEGIN

// Generates a string that should be a valid "Text Format" for the C++ version
// of Protocol Buffers. lineIndent can be nil if no additional line indent is
// needed. The comments provide the names according to the ObjC library, they
// most likely won't exactly match the original .proto file.
NSString *GPBTextFormatForMessage(GPBMessage *message,
                                  NSString * __nullable lineIndent);
NSString *GPBTextFormatForUnknownFieldSet(GPBUnknownFieldSet * __nullable unknownSet,
                                          NSString * __nullable lineIndent);

//
// Test if the given field is set on a message.
//
BOOL GPBMessageHasFieldNumberSet(GPBMessage *self, uint32_t fieldNumber);
BOOL GPBMessageHasFieldSet(GPBMessage *self, GPBFieldDescriptor *field);

//
// Clear the given field of a message.
//
void GPBClearMessageField(GPBMessage *self, GPBFieldDescriptor *field);

//%PDDM-EXPAND GPB_ACCESSORS()
// This block of code is generated, do not edit it directly.


//
// Get/Set the given field of a message.
//

// Single Fields

NSData *GPBGetMessageBytesField(GPBMessage *self, GPBFieldDescriptor *field);
void GPBSetMessageBytesField(GPBMessage *self, GPBFieldDescriptor *field, NSData *value);

NSString *GPBGetMessageStringField(GPBMessage *self, GPBFieldDescriptor *field);
void GPBSetMessageStringField(GPBMessage *self, GPBFieldDescriptor *field, NSString *value);

GPBMessage *GPBGetMessageMessageField(GPBMessage *self, GPBFieldDescriptor *field);
void GPBSetMessageMessageField(GPBMessage *self, GPBFieldDescriptor *field, GPBMessage *value);

GPBMessage *GPBGetMessageGroupField(GPBMessage *self, GPBFieldDescriptor *field);
void GPBSetMessageGroupField(GPBMessage *self, GPBFieldDescriptor *field, GPBMessage *value);

BOOL GPBGetMessageBoolField(GPBMessage *self, GPBFieldDescriptor *field);
void GPBSetMessageBoolField(GPBMessage *self, GPBFieldDescriptor *field, BOOL value);

int32_t GPBGetMessageInt32Field(GPBMessage *self, GPBFieldDescriptor *field);
void GPBSetMessageInt32Field(GPBMessage *self, GPBFieldDescriptor *field, int32_t value);

uint32_t GPBGetMessageUInt32Field(GPBMessage *self, GPBFieldDescriptor *field);
void GPBSetMessageUInt32Field(GPBMessage *self, GPBFieldDescriptor *field, uint32_t value);

int64_t GPBGetMessageInt64Field(GPBMessage *self, GPBFieldDescriptor *field);
void GPBSetMessageInt64Field(GPBMessage *self, GPBFieldDescriptor *field, int64_t value);

uint64_t GPBGetMessageUInt64Field(GPBMessage *self, GPBFieldDescriptor *field);
void GPBSetMessageUInt64Field(GPBMessage *self, GPBFieldDescriptor *field, uint64_t value);

float GPBGetMessageFloatField(GPBMessage *self, GPBFieldDescriptor *field);
void GPBSetMessageFloatField(GPBMessage *self, GPBFieldDescriptor *field, float value);

double GPBGetMessageDoubleField(GPBMessage *self, GPBFieldDescriptor *field);
void GPBSetMessageDoubleField(GPBMessage *self, GPBFieldDescriptor *field, double value);

// Get/Set the given enum field of a message.  You can only Set values that are
// members of the enum.  For proto3, when doing a Get, if the value isn't a
// memeber of the enum, kGPBUnrecognizedEnumeratorValue will be returned. The
// the functions with "Raw" in the will bypass all checks.
int32_t GPBGetMessageEnumField(GPBMessage *self, GPBFieldDescriptor *field);
void GPBSetMessageEnumField(GPBMessage *self, GPBFieldDescriptor *field, int32_t value);
int32_t GPBGetMessageRawEnumField(GPBMessage *self, GPBFieldDescriptor *field);
void GPBSetMessageRawEnumField(GPBMessage *self, GPBFieldDescriptor *field, int32_t value);

// Repeated Fields

// The object will/should be GPB*Array or NSMutableArray based on the field's
// type.
id GPBGetMessageRepeatedField(GPBMessage *self, GPBFieldDescriptor *field);
void GPBSetMessageRepeatedField(GPBMessage *self, GPBFieldDescriptor *field, id array);

// Map Fields

// The object will/should be GPB*Dictionary or NSMutableDictionary based on the
// field's type.
id GPBGetMessageMapField(GPBMessage *self, GPBFieldDescriptor *field);
void GPBSetMessageMapField(GPBMessage *self, GPBFieldDescriptor *field, id dictionary);

//%PDDM-EXPAND-END GPB_ACCESSORS()

// Returns an empty NSData to assign to byte fields when you wish
// to assign them to empty. Prevents allocating a lot of little [NSData data]
// objects.
NSData *GPBEmptyNSData(void) __attribute__((pure));

NS_ASSUME_NONNULL_END

CF_EXTERN_C_END


//%PDDM-DEFINE GPB_ACCESSORS()
//%
//%//
//%// Get/Set the given field of a message.
//%//
//%
//%// Single Fields
//%
//%GPB_ACCESSOR_SINGLE_FULL(Bytes, NSData, *)
//%GPB_ACCESSOR_SINGLE_FULL(String, NSString, *)
//%GPB_ACCESSOR_SINGLE_FULL(Message, GPBMessage, *)
//%GPB_ACCESSOR_SINGLE_FULL(Group, GPBMessage, *)
//%GPB_ACCESSOR_SINGLE(Bool, BOOL)
//%GPB_ACCESSOR_SINGLE(Int32, int32_t)
//%GPB_ACCESSOR_SINGLE(UInt32, uint32_t)
//%GPB_ACCESSOR_SINGLE(Int64, int64_t)
//%GPB_ACCESSOR_SINGLE(UInt64, uint64_t)
//%GPB_ACCESSOR_SINGLE(Float, float)
//%GPB_ACCESSOR_SINGLE(Double, double)
//%// Get/Set the given enum field of a message.  You can only Set values that are
//%// members of the enum.  For proto3, when doing a Get, if the value isn't a
//%// memeber of the enum, kGPBUnrecognizedEnumeratorValue will be returned. The
//%// the functions with "Raw" in the will bypass all checks.
//%int32_t GPBGetMessageEnumField(GPBMessage *self, GPBFieldDescriptor *field);
//%void GPBSetMessageEnumField(GPBMessage *self, GPBFieldDescriptor *field, int32_t value);
//%int32_t GPBGetMessageRawEnumField(GPBMessage *self, GPBFieldDescriptor *field);
//%void GPBSetMessageRawEnumField(GPBMessage *self, GPBFieldDescriptor *field, int32_t value);
//%
//%// Repeated Fields
//%
//%// The object will/should be GPB*Array or NSMutableArray based on the field's
//%// type.
//%id GPBGetMessageRepeatedField(GPBMessage *self, GPBFieldDescriptor *field);
//%void GPBSetMessageRepeatedField(GPBMessage *self, GPBFieldDescriptor *field, id array);
//%
//%// Map Fields
//%
//%// The object will/should be GPB*Dictionary or NSMutableDictionary based on the
//%// field's type.
//%id GPBGetMessageMapField(GPBMessage *self, GPBFieldDescriptor *field);
//%void GPBSetMessageMapField(GPBMessage *self, GPBFieldDescriptor *field, id dictionary);
//%

//%PDDM-DEFINE GPB_ACCESSOR_SINGLE(NAME, TYPE)
//%GPB_ACCESSOR_SINGLE_FULL(NAME, TYPE, )
//%PDDM-DEFINE GPB_ACCESSOR_SINGLE_FULL(NAME, TYPE, TisP)
//%TYPE TisP##GPBGetMessage##NAME##Field(GPBMessage *self, GPBFieldDescriptor *field);
//%void GPBSetMessage##NAME##Field(GPBMessage *self, GPBFieldDescriptor *field, TYPE TisP##value);
//%
