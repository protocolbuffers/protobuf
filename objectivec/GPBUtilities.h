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

/// Generates a string that should be a valid "Text Format" for the C++ version
/// of Protocol Buffers.
///
///  @param message    The message to generate from.
///  @param lineIndent A string to use as the prefix for all lines generated. Can
///                    be nil if no extra indent is needed.
///
/// @return A @c NSString with the Text Format of the message.
NSString *GPBTextFormatForMessage(GPBMessage *message,
                                  NSString * __nullable lineIndent);

/// Generates a string that should be a valid "Text Format" for the C++ version
/// of Protocol Buffers.
///
///  @param unknownSet The unknown field set to generate from.
///  @param lineIndent A string to use as the prefix for all lines generated. Can
///                    be nil if no extra indent is needed.
///
/// @return A @c NSString with the Text Format of the unknown field set.
NSString *GPBTextFormatForUnknownFieldSet(GPBUnknownFieldSet * __nullable unknownSet,
                                          NSString * __nullable lineIndent);

/// Test if the given field is set on a message.
BOOL GPBMessageHasFieldNumberSet(GPBMessage *self, uint32_t fieldNumber);
/// Test if the given field is set on a message.
BOOL GPBMessageHasFieldSet(GPBMessage *self, GPBFieldDescriptor *field);

/// Clear the given field of a message.
void GPBClearMessageField(GPBMessage *self, GPBFieldDescriptor *field);

//%PDDM-EXPAND GPB_ACCESSORS()
// This block of code is generated, do not edit it directly.


//
// Get/Set the given field of a message.
//

// Single Fields

/// Gets the value of a bytes field.
NSData *GPBGetMessageBytesField(GPBMessage *self, GPBFieldDescriptor *field);
/// Sets the value of a bytes field.
void GPBSetMessageBytesField(GPBMessage *self, GPBFieldDescriptor *field, NSData *value);

/// Gets the value of a string field.
NSString *GPBGetMessageStringField(GPBMessage *self, GPBFieldDescriptor *field);
/// Sets the value of a string field.
void GPBSetMessageStringField(GPBMessage *self, GPBFieldDescriptor *field, NSString *value);

/// Gets the value of a message field.
GPBMessage *GPBGetMessageMessageField(GPBMessage *self, GPBFieldDescriptor *field);
/// Sets the value of a message field.
void GPBSetMessageMessageField(GPBMessage *self, GPBFieldDescriptor *field, GPBMessage *value);

/// Gets the value of a group field.
GPBMessage *GPBGetMessageGroupField(GPBMessage *self, GPBFieldDescriptor *field);
/// Sets the value of a group field.
void GPBSetMessageGroupField(GPBMessage *self, GPBFieldDescriptor *field, GPBMessage *value);

/// Gets the value of a bool field.
BOOL GPBGetMessageBoolField(GPBMessage *self, GPBFieldDescriptor *field);
/// Sets the value of a bool field.
void GPBSetMessageBoolField(GPBMessage *self, GPBFieldDescriptor *field, BOOL value);

/// Gets the value of an int32 field.
int32_t GPBGetMessageInt32Field(GPBMessage *self, GPBFieldDescriptor *field);
/// Sets the value of an int32 field.
void GPBSetMessageInt32Field(GPBMessage *self, GPBFieldDescriptor *field, int32_t value);

/// Gets the value of an uint32 field.
uint32_t GPBGetMessageUInt32Field(GPBMessage *self, GPBFieldDescriptor *field);
/// Sets the value of an uint32 field.
void GPBSetMessageUInt32Field(GPBMessage *self, GPBFieldDescriptor *field, uint32_t value);

/// Gets the value of an int64 field.
int64_t GPBGetMessageInt64Field(GPBMessage *self, GPBFieldDescriptor *field);
/// Sets the value of an int64 field.
void GPBSetMessageInt64Field(GPBMessage *self, GPBFieldDescriptor *field, int64_t value);

/// Gets the value of an uint64 field.
uint64_t GPBGetMessageUInt64Field(GPBMessage *self, GPBFieldDescriptor *field);
/// Sets the value of an uint64 field.
void GPBSetMessageUInt64Field(GPBMessage *self, GPBFieldDescriptor *field, uint64_t value);

/// Gets the value of a float field.
float GPBGetMessageFloatField(GPBMessage *self, GPBFieldDescriptor *field);
/// Sets the value of a float field.
void GPBSetMessageFloatField(GPBMessage *self, GPBFieldDescriptor *field, float value);

/// Gets the value of a double field.
double GPBGetMessageDoubleField(GPBMessage *self, GPBFieldDescriptor *field);
/// Sets the value of a double field.
void GPBSetMessageDoubleField(GPBMessage *self, GPBFieldDescriptor *field, double value);

/// Get the given enum field of a message. For proto3, if the value isn't a
/// member of the enum, @c kGPBUnrecognizedEnumeratorValue will be returned.
/// GPBGetMessageRawEnumField will bypass the check and return whatever value
/// was set.
int32_t GPBGetMessageEnumField(GPBMessage *self, GPBFieldDescriptor *field);
/// Set the given enum field of a message. You can only set values that are
/// members of the enum.
void GPBSetMessageEnumField(GPBMessage *self, GPBFieldDescriptor *field, int32_t value);
/// Get the given enum field of a message. No check is done to ensure the value
/// was defined in the enum.
int32_t GPBGetMessageRawEnumField(GPBMessage *self, GPBFieldDescriptor *field);
/// Set the given enum field of a message. You can set the value to anything,
/// even a value that is not a member of the enum.
void GPBSetMessageRawEnumField(GPBMessage *self, GPBFieldDescriptor *field, int32_t value);

// Repeated Fields

/// Gets the value of a repeated field.
///
/// The result will be @c GPB*Array or @c NSMutableArray based on the
/// field's type.
id GPBGetMessageRepeatedField(GPBMessage *self, GPBFieldDescriptor *field);
/// Sets the value of a repeated field.
///
/// The value should be @c GPB*Array or @c NSMutableArray based on the
/// field's type.
void GPBSetMessageRepeatedField(GPBMessage *self, GPBFieldDescriptor *field, id array);

// Map Fields

/// Gets the value of a map<> field.
///
/// The result will be @c GPB*Dictionary or @c NSMutableDictionary based on
/// the field's type.
id GPBGetMessageMapField(GPBMessage *self, GPBFieldDescriptor *field);
/// Sets the value of a map<> field.
///
/// The object should be @c GPB*Dictionary or @c NSMutableDictionary based
/// on the field's type.
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
//%GPB_ACCESSOR_SINGLE_FULL(Bytes, NSData, , *)
//%GPB_ACCESSOR_SINGLE_FULL(String, NSString, , *)
//%GPB_ACCESSOR_SINGLE_FULL(Message, GPBMessage, , *)
//%GPB_ACCESSOR_SINGLE_FULL(Group, GPBMessage, , *)
//%GPB_ACCESSOR_SINGLE(Bool, BOOL, )
//%GPB_ACCESSOR_SINGLE(Int32, int32_t, n)
//%GPB_ACCESSOR_SINGLE(UInt32, uint32_t, n)
//%GPB_ACCESSOR_SINGLE(Int64, int64_t, n)
//%GPB_ACCESSOR_SINGLE(UInt64, uint64_t, n)
//%GPB_ACCESSOR_SINGLE(Float, float, )
//%GPB_ACCESSOR_SINGLE(Double, double, )
//%/// Get the given enum field of a message. For proto3, if the value isn't a
//%/// member of the enum, @c kGPBUnrecognizedEnumeratorValue will be returned.
//%/// GPBGetMessageRawEnumField will bypass the check and return whatever value
//%/// was set.
//%int32_t GPBGetMessageEnumField(GPBMessage *self, GPBFieldDescriptor *field);
//%/// Set the given enum field of a message. You can only set values that are
//%/// members of the enum.
//%void GPBSetMessageEnumField(GPBMessage *self, GPBFieldDescriptor *field, int32_t value);
//%/// Get the given enum field of a message. No check is done to ensure the value
//%/// was defined in the enum.
//%int32_t GPBGetMessageRawEnumField(GPBMessage *self, GPBFieldDescriptor *field);
//%/// Set the given enum field of a message. You can set the value to anything,
//%/// even a value that is not a member of the enum.
//%void GPBSetMessageRawEnumField(GPBMessage *self, GPBFieldDescriptor *field, int32_t value);
//%
//%// Repeated Fields
//%
//%/// Gets the value of a repeated field.
//%///
//%/// The result will be @c GPB*Array or @c NSMutableArray based on the
//%/// field's type.
//%id GPBGetMessageRepeatedField(GPBMessage *self, GPBFieldDescriptor *field);
//%/// Sets the value of a repeated field.
//%///
//%/// The value should be @c GPB*Array or @c NSMutableArray based on the
//%/// field's type.
//%void GPBSetMessageRepeatedField(GPBMessage *self, GPBFieldDescriptor *field, id array);
//%
//%// Map Fields
//%
//%/// Gets the value of a map<> field.
//%///
//%/// The result will be @c GPB*Dictionary or @c NSMutableDictionary based on
//%/// the field's type.
//%id GPBGetMessageMapField(GPBMessage *self, GPBFieldDescriptor *field);
//%/// Sets the value of a map<> field.
//%///
//%/// The object should be @c GPB*Dictionary or @c NSMutableDictionary based
//%/// on the field's type.
//%void GPBSetMessageMapField(GPBMessage *self, GPBFieldDescriptor *field, id dictionary);
//%

//%PDDM-DEFINE GPB_ACCESSOR_SINGLE(NAME, TYPE, AN)
//%GPB_ACCESSOR_SINGLE_FULL(NAME, TYPE, AN, )
//%PDDM-DEFINE GPB_ACCESSOR_SINGLE_FULL(NAME, TYPE, AN, TisP)
//%/// Gets the value of a##AN NAME$L field.
//%TYPE TisP##GPBGetMessage##NAME##Field(GPBMessage *self, GPBFieldDescriptor *field);
//%/// Sets the value of a##AN NAME$L field.
//%void GPBSetMessage##NAME##Field(GPBMessage *self, GPBFieldDescriptor *field, TYPE TisP##value);
//%
