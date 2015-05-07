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

#import "GPBUtilities.h"

#import "GPBDescriptor_PackagePrivate.h"

// Macros for stringifying library symbols. These are used in the generated
// PB descriptor classes wherever a library symbol name is represented as a
// string. See README.google for more information.
#define GPBStringify(S) #S
#define GPBStringifySymbol(S) GPBStringify(S)

#define GPBNSStringify(S) @#S
#define GPBNSStringifySymbol(S) GPBNSStringify(S)

// Constant to internally mark when there is no has bit.
#define GPBNoHasBit INT32_MAX

CF_EXTERN_C_BEGIN

// Conversion functions for de/serializing floating point types.

GPB_INLINE int64_t GPBConvertDoubleToInt64(double v) {
  union { double f; int64_t i; } u;
  u.f = v;
  return u.i;
}

GPB_INLINE int32_t GPBConvertFloatToInt32(float v) {
  union { float f; int32_t i; } u;
  u.f = v;
  return u.i;
}

GPB_INLINE double GPBConvertInt64ToDouble(int64_t v) {
  union { double f; int64_t i; } u;
  u.i = v;
  return u.f;
}

GPB_INLINE float GPBConvertInt32ToFloat(int32_t v) {
  union { float f; int32_t i; } u;
  u.i = v;
  return u.f;
}

GPB_INLINE int32_t GPBLogicalRightShift32(int32_t value, int32_t spaces) {
  return (int32_t)((uint32_t)(value) >> spaces);
}

GPB_INLINE int64_t GPBLogicalRightShift64(int64_t value, int32_t spaces) {
  return (int64_t)((uint64_t)(value) >> spaces);
}

// Decode a ZigZag-encoded 32-bit value.  ZigZag encodes signed integers
// into values that can be efficiently encoded with varint.  (Otherwise,
// negative values must be sign-extended to 64 bits to be varint encoded,
// thus always taking 10 bytes on the wire.)
GPB_INLINE int32_t GPBDecodeZigZag32(uint32_t n) {
  return GPBLogicalRightShift32(n, 1) ^ -(n & 1);
}

// Decode a ZigZag-encoded 64-bit value.  ZigZag encodes signed integers
// into values that can be efficiently encoded with varint.  (Otherwise,
// negative values must be sign-extended to 64 bits to be varint encoded,
// thus always taking 10 bytes on the wire.)
GPB_INLINE int64_t GPBDecodeZigZag64(uint64_t n) {
  return GPBLogicalRightShift64(n, 1) ^ -(n & 1);
}

// Encode a ZigZag-encoded 32-bit value.  ZigZag encodes signed integers
// into values that can be efficiently encoded with varint.  (Otherwise,
// negative values must be sign-extended to 64 bits to be varint encoded,
// thus always taking 10 bytes on the wire.)
GPB_INLINE uint32_t GPBEncodeZigZag32(int32_t n) {
  // Note:  the right-shift must be arithmetic
  return (n << 1) ^ (n >> 31);
}

// Encode a ZigZag-encoded 64-bit value.  ZigZag encodes signed integers
// into values that can be efficiently encoded with varint.  (Otherwise,
// negative values must be sign-extended to 64 bits to be varint encoded,
// thus always taking 10 bytes on the wire.)
GPB_INLINE uint64_t GPBEncodeZigZag64(int64_t n) {
  // Note:  the right-shift must be arithmetic
  return (n << 1) ^ (n >> 63);
}

GPB_INLINE BOOL GPBTypeIsObject(GPBType type) {
  switch (type) {
    case GPBTypeData:
    case GPBTypeString:
    case GPBTypeMessage:
    case GPBTypeGroup:
      return YES;
    default:
      return NO;
  }
}

GPB_INLINE BOOL GPBTypeIsMessage(GPBType type) {
  switch (type) {
    case GPBTypeMessage:
    case GPBTypeGroup:
      return YES;
    default:
      return NO;
  }
}

GPB_INLINE BOOL GPBTypeIsEnum(GPBType type) { return type == GPBTypeEnum; }

GPB_INLINE BOOL GPBFieldTypeIsMessage(GPBFieldDescriptor *field) {
  return GPBTypeIsMessage(field->description_->type);
}

GPB_INLINE BOOL GPBFieldTypeIsObject(GPBFieldDescriptor *field) {
  return GPBTypeIsObject(field->description_->type);
}

GPB_INLINE BOOL GPBExtensionIsMessage(GPBExtensionDescriptor *ext) {
  return GPBTypeIsMessage(ext->description_->type);
}

// The field is an array/map or it has an object value.
GPB_INLINE BOOL GPBFieldStoresObject(GPBFieldDescriptor *field) {
  GPBMessageFieldDescription *desc = field->description_;
  if ((desc->flags & (GPBFieldRepeated | GPBFieldMapKeyMask)) != 0) {
    return YES;
  }
  return GPBTypeIsObject(desc->type);
}

BOOL GPBGetHasIvar(GPBMessage *self, int32_t index, uint32_t fieldNumber);
void GPBSetHasIvar(GPBMessage *self, int32_t idx, uint32_t fieldNumber,
                   BOOL value);
uint32_t GPBGetHasOneof(GPBMessage *self, int32_t index);

GPB_INLINE BOOL
GPBGetHasIvarField(GPBMessage *self, GPBFieldDescriptor *field) {
  GPBMessageFieldDescription *fieldDesc = field->description_;
  return GPBGetHasIvar(self, fieldDesc->hasIndex, fieldDesc->number);
}
GPB_INLINE void GPBSetHasIvarField(GPBMessage *self, GPBFieldDescriptor *field,
                                   BOOL value) {
  GPBMessageFieldDescription *fieldDesc = field->description_;
  GPBSetHasIvar(self, fieldDesc->hasIndex, fieldDesc->number, value);
}

void GPBMaybeClearOneof(GPBMessage *self, GPBOneofDescriptor *oneof,
                        uint32_t fieldNumberNotToClear);

//%PDDM-DEFINE GPB_IVAR_SET_DECL(NAME, TYPE)
//%void GPBSet##NAME##IvarWithFieldInternal(GPBMessage *self,
//%            NAME$S                     GPBFieldDescriptor *field,
//%            NAME$S                     TYPE value,
//%            NAME$S                     GPBFileSyntax syntax);
//%PDDM-EXPAND GPB_IVAR_SET_DECL(Bool, BOOL)
// This block of code is generated, do not edit it directly.

void GPBSetBoolIvarWithFieldInternal(GPBMessage *self,
                                     GPBFieldDescriptor *field,
                                     BOOL value,
                                     GPBFileSyntax syntax);
//%PDDM-EXPAND GPB_IVAR_SET_DECL(Int32, int32_t)
// This block of code is generated, do not edit it directly.

void GPBSetInt32IvarWithFieldInternal(GPBMessage *self,
                                      GPBFieldDescriptor *field,
                                      int32_t value,
                                      GPBFileSyntax syntax);
//%PDDM-EXPAND GPB_IVAR_SET_DECL(UInt32, uint32_t)
// This block of code is generated, do not edit it directly.

void GPBSetUInt32IvarWithFieldInternal(GPBMessage *self,
                                       GPBFieldDescriptor *field,
                                       uint32_t value,
                                       GPBFileSyntax syntax);
//%PDDM-EXPAND GPB_IVAR_SET_DECL(Int64, int64_t)
// This block of code is generated, do not edit it directly.

void GPBSetInt64IvarWithFieldInternal(GPBMessage *self,
                                      GPBFieldDescriptor *field,
                                      int64_t value,
                                      GPBFileSyntax syntax);
//%PDDM-EXPAND GPB_IVAR_SET_DECL(UInt64, uint64_t)
// This block of code is generated, do not edit it directly.

void GPBSetUInt64IvarWithFieldInternal(GPBMessage *self,
                                       GPBFieldDescriptor *field,
                                       uint64_t value,
                                       GPBFileSyntax syntax);
//%PDDM-EXPAND GPB_IVAR_SET_DECL(Float, float)
// This block of code is generated, do not edit it directly.

void GPBSetFloatIvarWithFieldInternal(GPBMessage *self,
                                      GPBFieldDescriptor *field,
                                      float value,
                                      GPBFileSyntax syntax);
//%PDDM-EXPAND GPB_IVAR_SET_DECL(Double, double)
// This block of code is generated, do not edit it directly.

void GPBSetDoubleIvarWithFieldInternal(GPBMessage *self,
                                       GPBFieldDescriptor *field,
                                       double value,
                                       GPBFileSyntax syntax);
//%PDDM-EXPAND GPB_IVAR_SET_DECL(Enum, int32_t)
// This block of code is generated, do not edit it directly.

void GPBSetEnumIvarWithFieldInternal(GPBMessage *self,
                                     GPBFieldDescriptor *field,
                                     int32_t value,
                                     GPBFileSyntax syntax);
//%PDDM-EXPAND-END (8 expansions)

int32_t GPBGetEnumIvarWithFieldInternal(GPBMessage *self,
                                        GPBFieldDescriptor *field,
                                        GPBFileSyntax syntax);

id GPBGetObjectIvarWithField(GPBMessage *self, GPBFieldDescriptor *field);

void GPBSetObjectIvarWithFieldInternal(GPBMessage *self,
                                       GPBFieldDescriptor *field, id value,
                                       GPBFileSyntax syntax);
void GPBSetRetainedObjectIvarWithFieldInternal(GPBMessage *self,
                                               GPBFieldDescriptor *field,
                                               id __attribute__((ns_consumed))
                                               value,
                                               GPBFileSyntax syntax);

// GPBGetObjectIvarWithField will automatically create the field (message) if
// it doesn't exist. GPBGetObjectIvarWithFieldNoAutocreate will return nil.
id GPBGetObjectIvarWithFieldNoAutocreate(GPBMessage *self,
                                         GPBFieldDescriptor *field);

void GPBSetAutocreatedRetainedObjectIvarWithField(
    GPBMessage *self, GPBFieldDescriptor *field,
    id __attribute__((ns_consumed)) value);

// Clears and releases the autocreated message ivar, if it's autocreated. If
// it's not set as autocreated, this method does nothing.
void GPBClearAutocreatedMessageIvarWithField(GPBMessage *self,
                                             GPBFieldDescriptor *field);

// Utilities for applying various functions based on Objective C types.

// A basic functor that is passed a field and a context. Returns YES
// if the calling function should continue processing, and NO if the calling
// function should stop processing.
typedef BOOL (*GPBApplyFunction)(GPBFieldDescriptor *field, void *context);

// Functions called for various types. See ApplyFunctionsToMessageFields.
typedef enum {
  GPBApplyFunctionObject,
  GPBApplyFunctionBool,
  GPBApplyFunctionInt32,
  GPBApplyFunctionUInt32,
  GPBApplyFunctionInt64,
  GPBApplyFunctionUInt64,
  GPBApplyFunctionFloat,
  GPBApplyFunctionDouble,
} GPBApplyFunctionOrder;

enum {
  // A count of the number of types in GPBApplyFunctionOrder. Separated out
  // from the GPBApplyFunctionOrder enum to avoid warnings regarding not
  // handling GPBApplyFunctionCount in switch statements.
  GPBApplyFunctionCount = GPBApplyFunctionDouble + 1
};

typedef GPBApplyFunction GPBApplyFunctions[GPBApplyFunctionCount];

// Functions called for various types.
// See ApplyStrictFunctionsToMessageFields.
// They are in the same order as the GPBTypes enum.
typedef GPBApplyFunction GPBApplyStrictFunctions[GPBTypeCount];

// A macro for easily initializing a GPBApplyFunctions struct
// GPBApplyFunctions foo = GPBAPPLY_FUNCTIONS_INIT(Foo);
#define GPBAPPLY_FUNCTIONS_INIT(PREFIX) \
  { \
    PREFIX##Object,  \
    PREFIX##Bool,    \
    PREFIX##Int32,   \
    PREFIX##UInt32,  \
    PREFIX##Int64,   \
    PREFIX##UInt64,  \
    PREFIX##Float,   \
    PREFIX##Double,  \
  }

// A macro for easily initializing a GPBApplyStrictFunctions struct
// GPBApplyStrictFunctions foo = GPBAPPLY_STRICT_FUNCTIONS_INIT(Foo);
// These need to stay in the same order as
// the GPBType enum.
#define GPBAPPLY_STRICT_FUNCTIONS_INIT(PREFIX) \
  { \
    PREFIX##Bool,     \
    PREFIX##Fixed32,  \
    PREFIX##SFixed32, \
    PREFIX##Float,    \
    PREFIX##Fixed64,  \
    PREFIX##SFixed64, \
    PREFIX##Double,   \
    PREFIX##Int32,    \
    PREFIX##Int64,    \
    PREFIX##SInt32,   \
    PREFIX##SInt64,   \
    PREFIX##UInt32,   \
    PREFIX##UInt64,   \
    PREFIX##Data,     \
    PREFIX##String,   \
    PREFIX##Message,  \
    PREFIX##Group,    \
    PREFIX##Enum,     \
}

// Iterates over the fields of a proto |msg| and applies the functions in
// |functions| to them with |context|. If one of the functions in |functions|
// returns NO, it will return immediately and not process the rest of the
// ivars. The types in the fields are mapped so:
// Int32, Enum, SInt32 and SFixed32 will be mapped to the int32Function,
// UInt32 and Fixed32 will be mapped to the uint32Function,
// Bytes, String, Message and Group will be mapped to the objectFunction,
// etc..
// If you require more specific mappings look at
// GPBApplyStrictFunctionsToMessageFields.
void GPBApplyFunctionsToMessageFields(GPBApplyFunctions *functions,
                                      GPBMessage *msg, void *context);

// Iterates over the fields of a proto |msg| and applies the functions in
// |functions| to them with |context|. If one of the functions in |functions|
// returns NO, it will return immediately and not process the rest of the
// ivars. The types in the fields are mapped directly:
// Int32 -> functions[GPBTypeInt32],
// SFixed32 -> functions[GPBTypeSFixed32],
// etc...
// If you can use looser mappings look at GPBApplyFunctionsToMessageFields.
void GPBApplyStrictFunctionsToMessageFields(GPBApplyStrictFunctions *functions,
                                            GPBMessage *msg, void *context);

// Applies the appropriate function in |functions| based on |field|.
// Returns the value from function(name, context).
// Throws an exception if the type is unrecognized.
BOOL GPBApplyFunctionsBasedOnField(GPBFieldDescriptor *field,
                                   GPBApplyFunctions *functions, void *context);

// Returns an Objective C encoding for |selector|. |instanceSel| should be
// YES if it's an instance selector (as opposed to a class selector).
// |selector| must be a selector from MessageSignatureProtocol.
const char *GPBMessageEncodingForSelector(SEL selector, BOOL instanceSel);

// Helper for text format name encoding.
// decodeData is the data describing the sepecial decodes.
// key and inputString are the input that needs decoding.
NSString *GPBDecodeTextFormatName(const uint8_t *decodeData, int32_t key,
                                  NSString *inputString);

// A series of selectors that are used solely to get @encoding values
// for them by the dynamic protobuf runtime code. See
// GPBMessageEncodingForSelector for details.
@protocol GPBMessageSignatureProtocol
@optional

#define GPB_MESSAGE_SIGNATURE_ENTRY(TYPE, NAME) \
  -(TYPE)get##NAME;                             \
  -(void)set##NAME : (TYPE)value;               \
  -(TYPE)get##NAME##AtIndex : (NSUInteger)index;

GPB_MESSAGE_SIGNATURE_ENTRY(BOOL, Bool)
GPB_MESSAGE_SIGNATURE_ENTRY(uint32_t, Fixed32)
GPB_MESSAGE_SIGNATURE_ENTRY(int32_t, SFixed32)
GPB_MESSAGE_SIGNATURE_ENTRY(float, Float)
GPB_MESSAGE_SIGNATURE_ENTRY(uint64_t, Fixed64)
GPB_MESSAGE_SIGNATURE_ENTRY(int64_t, SFixed64)
GPB_MESSAGE_SIGNATURE_ENTRY(double, Double)
GPB_MESSAGE_SIGNATURE_ENTRY(int32_t, Int32)
GPB_MESSAGE_SIGNATURE_ENTRY(int64_t, Int64)
GPB_MESSAGE_SIGNATURE_ENTRY(int32_t, SInt32)
GPB_MESSAGE_SIGNATURE_ENTRY(int64_t, SInt64)
GPB_MESSAGE_SIGNATURE_ENTRY(uint32_t, UInt32)
GPB_MESSAGE_SIGNATURE_ENTRY(uint64_t, UInt64)
GPB_MESSAGE_SIGNATURE_ENTRY(NSData *, Data)
GPB_MESSAGE_SIGNATURE_ENTRY(NSString *, String)
GPB_MESSAGE_SIGNATURE_ENTRY(GPBMessage *, Message)
GPB_MESSAGE_SIGNATURE_ENTRY(GPBMessage *, Group)
GPB_MESSAGE_SIGNATURE_ENTRY(int32_t, Enum)

#undef GPB_MESSAGE_SIGNATURE_ENTRY

- (id)getArray;
- (void)setArray:(NSArray *)array;
+ (id)getClassValue;
@end

CF_EXTERN_C_END
