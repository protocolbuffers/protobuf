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

#import "GPBMessage.h"
#import "GPBTypes.h"

CF_EXTERN_C_BEGIN

BOOL GPBMessageHasFieldNumberSet(GPBMessage *self, uint32_t fieldNumber);
BOOL GPBMessageHasFieldSet(GPBMessage *self, GPBFieldDescriptor *field);

void GPBClearMessageField(GPBMessage *self, GPBFieldDescriptor *field);

// Returns an empty NSData to assign to byte fields when you wish
// to assign them to empty. Prevents allocating a lot of little [NSData data]
// objects.
NSData *GPBEmptyNSData(void) __attribute__((pure));

//%PDDM-EXPAND GPB_IVAR_ACCESSORS()
// This block of code is generated, do not edit it directly.

// Getters and Setters for ivars named |name| from instance self.

NSData* GPBGetDataIvarWithField(GPBMessage *self,
                                GPBFieldDescriptor *field);
void GPBSetDataIvarWithField(GPBMessage *self,
                             GPBFieldDescriptor *field,
                             NSData* value);
NSString* GPBGetStringIvarWithField(GPBMessage *self,
                                    GPBFieldDescriptor *field);
void GPBSetStringIvarWithField(GPBMessage *self,
                               GPBFieldDescriptor *field,
                               NSString* value);
GPBMessage* GPBGetMessageIvarWithField(GPBMessage *self,
                                       GPBFieldDescriptor *field);
void GPBSetMessageIvarWithField(GPBMessage *self,
                                GPBFieldDescriptor *field,
                                GPBMessage* value);
GPBMessage* GPBGetGroupIvarWithField(GPBMessage *self,
                                     GPBFieldDescriptor *field);
void GPBSetGroupIvarWithField(GPBMessage *self,
                              GPBFieldDescriptor *field,
                              GPBMessage* value);
BOOL GPBGetBoolIvarWithField(GPBMessage *self,
                             GPBFieldDescriptor *field);
void GPBSetBoolIvarWithField(GPBMessage *self,
                             GPBFieldDescriptor *field,
                             BOOL value);
int32_t GPBGetInt32IvarWithField(GPBMessage *self,
                                 GPBFieldDescriptor *field);
void GPBSetInt32IvarWithField(GPBMessage *self,
                              GPBFieldDescriptor *field,
                              int32_t value);
int32_t GPBGetSFixed32IvarWithField(GPBMessage *self,
                                    GPBFieldDescriptor *field);
void GPBSetSFixed32IvarWithField(GPBMessage *self,
                                 GPBFieldDescriptor *field,
                                 int32_t value);
int32_t GPBGetSInt32IvarWithField(GPBMessage *self,
                                  GPBFieldDescriptor *field);
void GPBSetSInt32IvarWithField(GPBMessage *self,
                               GPBFieldDescriptor *field,
                               int32_t value);
int32_t GPBGetEnumIvarWithField(GPBMessage *self,
                                GPBFieldDescriptor *field);
void GPBSetEnumIvarWithField(GPBMessage *self,
                             GPBFieldDescriptor *field,
                             int32_t value);
uint32_t GPBGetUInt32IvarWithField(GPBMessage *self,
                                   GPBFieldDescriptor *field);
void GPBSetUInt32IvarWithField(GPBMessage *self,
                               GPBFieldDescriptor *field,
                               uint32_t value);
uint32_t GPBGetFixed32IvarWithField(GPBMessage *self,
                                    GPBFieldDescriptor *field);
void GPBSetFixed32IvarWithField(GPBMessage *self,
                                GPBFieldDescriptor *field,
                                uint32_t value);
int64_t GPBGetInt64IvarWithField(GPBMessage *self,
                                 GPBFieldDescriptor *field);
void GPBSetInt64IvarWithField(GPBMessage *self,
                              GPBFieldDescriptor *field,
                              int64_t value);
int64_t GPBGetSInt64IvarWithField(GPBMessage *self,
                                  GPBFieldDescriptor *field);
void GPBSetSInt64IvarWithField(GPBMessage *self,
                               GPBFieldDescriptor *field,
                               int64_t value);
int64_t GPBGetSFixed64IvarWithField(GPBMessage *self,
                                    GPBFieldDescriptor *field);
void GPBSetSFixed64IvarWithField(GPBMessage *self,
                                 GPBFieldDescriptor *field,
                                 int64_t value);
uint64_t GPBGetUInt64IvarWithField(GPBMessage *self,
                                   GPBFieldDescriptor *field);
void GPBSetUInt64IvarWithField(GPBMessage *self,
                               GPBFieldDescriptor *field,
                               uint64_t value);
uint64_t GPBGetFixed64IvarWithField(GPBMessage *self,
                                    GPBFieldDescriptor *field);
void GPBSetFixed64IvarWithField(GPBMessage *self,
                                GPBFieldDescriptor *field,
                                uint64_t value);
float GPBGetFloatIvarWithField(GPBMessage *self,
                               GPBFieldDescriptor *field);
void GPBSetFloatIvarWithField(GPBMessage *self,
                              GPBFieldDescriptor *field,
                              float value);
double GPBGetDoubleIvarWithField(GPBMessage *self,
                                 GPBFieldDescriptor *field);
void GPBSetDoubleIvarWithField(GPBMessage *self,
                               GPBFieldDescriptor *field,
                               double value);
//%PDDM-EXPAND-END GPB_IVAR_ACCESSORS()

// Generates a sting that should be a valid "Text Format" for the C++ version
// of Protocol Buffers. lineIndent can be nil if no additional line indent is
// needed. The comments provide the names according to the ObjC library, they
// most likely won't exactly match the original .proto file.
NSString *GPBTextFormatForMessage(GPBMessage *message, NSString *lineIndent);
NSString *GPBTextFormatForUnknownFieldSet(GPBUnknownFieldSet *unknownSet,
                                          NSString *lineIndent);

CF_EXTERN_C_END

//%PDDM-DEFINE GPB_IVAR_ACCESSORS()
//%// Getters and Setters for ivars named |name| from instance self.
//%
//%GPB_IVAR_ACCESSORS_DECL(Data, NSData*)
//%GPB_IVAR_ACCESSORS_DECL(String, NSString*)
//%GPB_IVAR_ACCESSORS_DECL(Message, GPBMessage*)
//%GPB_IVAR_ACCESSORS_DECL(Group, GPBMessage*)
//%GPB_IVAR_ACCESSORS_DECL(Bool, BOOL)
//%GPB_IVAR_ACCESSORS_DECL(Int32, int32_t)
//%GPB_IVAR_ACCESSORS_DECL(SFixed32, int32_t)
//%GPB_IVAR_ACCESSORS_DECL(SInt32, int32_t)
//%GPB_IVAR_ACCESSORS_DECL(Enum, int32_t)
//%GPB_IVAR_ACCESSORS_DECL(UInt32, uint32_t)
//%GPB_IVAR_ACCESSORS_DECL(Fixed32, uint32_t)
//%GPB_IVAR_ACCESSORS_DECL(Int64, int64_t)
//%GPB_IVAR_ACCESSORS_DECL(SInt64, int64_t)
//%GPB_IVAR_ACCESSORS_DECL(SFixed64, int64_t)
//%GPB_IVAR_ACCESSORS_DECL(UInt64, uint64_t)
//%GPB_IVAR_ACCESSORS_DECL(Fixed64, uint64_t)
//%GPB_IVAR_ACCESSORS_DECL(Float, float)
//%GPB_IVAR_ACCESSORS_DECL(Double, double)
//%PDDM-DEFINE GPB_IVAR_ACCESSORS_DECL(NAME, TYPE)
//%TYPE GPBGet##NAME##IvarWithField(GPBMessage *self,
//% TYPE$S     NAME$S               GPBFieldDescriptor *field);
//%void GPBSet##NAME##IvarWithField(GPBMessage *self,
//%            NAME$S             GPBFieldDescriptor *field,
//%            NAME$S             TYPE value);
