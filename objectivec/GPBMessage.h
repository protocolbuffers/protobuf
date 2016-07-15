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

#import "GPBBootstrap.h"

@class GPBDescriptor;
@class GPBCodedInputStream;
@class GPBCodedOutputStream;
@class GPBExtensionDescriptor;
@class GPBExtensionRegistry;
@class GPBFieldDescriptor;
@class GPBUnknownFieldSet;

NS_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

/// NSError domain used for errors.
extern NSString *const GPBMessageErrorDomain;

/// Error code for NSError with GPBMessageErrorDomain.
typedef NS_ENUM(NSInteger, GPBMessageErrorCode) {
  /// Uncategorized error.
  GPBMessageErrorCodeOther = -100,
  /// A message can't be serialized because it is missing required fields.
  GPBMessageErrorCodeMissingRequiredField = -101,
};

/// Key under which the error's reason is stored inside the userInfo dictionary.
extern NSString *const GPBErrorReasonKey;

CF_EXTERN_C_END

/// Base class for all of the generated message classes.
@interface GPBMessage : NSObject<NSSecureCoding, NSCopying>

// NOTE: If you add a instance method/property to this class that may conflict
// with methods declared in protos, you need to update objective_helpers.cc.
// The main cases are methods that take no arguments, or setFoo:/hasFoo: type
// methods.

/// The unknown fields for this message.
///
/// Only messages from proto files declared with "proto2" syntax support unknown
/// fields. For "proto3" syntax, any unknown fields found while parsing are
/// dropped.
@property(nonatomic, copy, nullable) GPBUnknownFieldSet *unknownFields;

/// Are all required fields set in the message and all embedded messages.
@property(nonatomic, readonly, getter=isInitialized) BOOL initialized;

/// Returns an autoreleased instance.
+ (instancetype)message;

/// Creates a new instance by parsing the data. This method should be sent to
/// the generated message class that the data should be interpreted as. If
/// there is an error the method returns nil and the error is returned in
/// errorPtr (when provided).
///
/// @note In DEBUG builds, the parsed message is checked to be sure all required
///       fields were provided, and the parse will fail if some are missing.
///
/// @note The errors returned are likely coming from the domain and codes listed
///       at the top of this file and GPBCodedInputStream.h.
///
/// @param data     The data to parse.
/// @param errorPtr An optional error pointer to fill in with a failure reason if
///                 the data can not be parsed.
///
/// @return A new instance of the class messaged.
+ (nullable instancetype)parseFromData:(NSData *)data error:(NSError **)errorPtr;

/// Creates a new instance by parsing the data. This method should be sent to
/// the generated message class that the data should be interpreted as. If
/// there is an error the method returns nil and the error is returned in
/// errorPtr (when provided).
///
/// @note In DEBUG builds, the parsed message is checked to be sure all required
///       fields were provided, and the parse will fail if some are missing.
///
/// @note The errors returned are likely coming from the domain and codes listed
///       at the top of this file and GPBCodedInputStream.h.
///
/// @param data              The data to parse.
/// @param extensionRegistry The extension registry to use to look up extensions.
/// @param errorPtr          An optional error pointer to fill in with a failure
///                          reason if the data can not be parsed.
///
/// @return A new instance of the class messaged.
+ (nullable instancetype)parseFromData:(NSData *)data
                     extensionRegistry:(nullable GPBExtensionRegistry *)extensionRegistry
                                 error:(NSError **)errorPtr;

/// Creates a new instance by parsing the data from the given input stream. This
/// method should be sent to the generated message class that the data should
/// be interpreted as. If there is an error the method returns nil and the error
/// is returned in errorPtr (when provided).
///
/// @note In DEBUG builds, the parsed message is checked to be sure all required
///       fields were provided, and the parse will fail if some are missing.
///
/// @note The errors returned are likely coming from the domain and codes listed
///       at the top of this file and GPBCodedInputStream.h.
///
/// @param input             The stream to read data from.
/// @param extensionRegistry The extension registry to use to look up extensions.
/// @param errorPtr          An optional error pointer to fill in with a failure
///                          reason if the data can not be parsed.
///
/// @return A new instance of the class messaged.
+ (nullable instancetype)parseFromCodedInputStream:(GPBCodedInputStream *)input
                                 extensionRegistry:
                                     (nullable GPBExtensionRegistry *)extensionRegistry
                                             error:(NSError **)errorPtr;

/// Creates a new instance by parsing the data from the given input stream. This
/// method should be sent to the generated message class that the data should
/// be interpreted as. If there is an error the method returns nil and the error
/// is returned in errorPtr (when provided).
///
/// @note Unlike the parseFrom... methods, this never checks to see if all of
///       the required fields are set. So this method can be used to reload
///       messages that may not be complete.
///
/// @note The errors returned are likely coming from the domain and codes listed
///       at the top of this file and GPBCodedInputStream.h.
///
/// @param input             The stream to read data from.
/// @param extensionRegistry The extension registry to use to look up extensions.
/// @param errorPtr          An optional error pointer to fill in with a failure
///                          reason if the data can not be parsed.
///
/// @return A new instance of the class messaged.
+ (nullable instancetype)parseDelimitedFromCodedInputStream:(GPBCodedInputStream *)input
                                          extensionRegistry:
                                              (nullable GPBExtensionRegistry *)extensionRegistry
                                                      error:(NSError **)errorPtr;

/// Initializes an instance by parsing the data. This method should be sent to
/// the generated message class that the data should be interpreted as. If
/// there is an error the method returns nil and the error is returned in
/// errorPtr (when provided).
///
/// @note In DEBUG builds, the parsed message is checked to be sure all required
///       fields were provided, and the parse will fail if some are missing.
///
/// @note The errors returned are likely coming from the domain and codes listed
///       at the top of this file and GPBCodedInputStream.h.
///
/// @param data     The data to parse.
/// @param errorPtr An optional error pointer to fill in with a failure reason if
///                 the data can not be parsed.
- (nullable instancetype)initWithData:(NSData *)data error:(NSError **)errorPtr;

/// Initializes an instance by parsing the data. This method should be sent to
/// the generated message class that the data should be interpreted as. If
/// there is an error the method returns nil and the error is returned in
/// errorPtr (when provided).
///
/// @note In DEBUG builds, the parsed message is checked to be sure all required
///       fields were provided, and the parse will fail if some are missing.
///
/// @note The errors returned are likely coming from the domain and codes listed
///       at the top of this file and GPBCodedInputStream.h.
///
/// @param data              The data to parse.
/// @param extensionRegistry The extension registry to use to look up extensions.
/// @param errorPtr          An optional error pointer to fill in with a failure
///                          reason if the data can not be parsed.
- (nullable instancetype)initWithData:(NSData *)data
                    extensionRegistry:(nullable GPBExtensionRegistry *)extensionRegistry
                                error:(NSError **)errorPtr;

/// Initializes an instance by parsing the data from the given input stream. This
/// method should be sent to the generated message class that the data should
/// be interpreted as. If there is an error the method returns nil and the error
/// is returned in errorPtr (when provided).
///
/// @note Unlike the parseFrom... methods, this never checks to see if all of
///       the required fields are set. So this method can be used to reload
///       messages that may not be complete.
///
/// @note The errors returned are likely coming from the domain and codes listed
///       at the top of this file and GPBCodedInputStream.h.
///
/// @param input             The stream to read data from.
/// @param extensionRegistry The extension registry to use to look up extensions.
/// @param errorPtr          An optional error pointer to fill in with a failure
///                          reason if the data can not be parsed.
- (nullable instancetype)initWithCodedInputStream:(GPBCodedInputStream *)input
                                extensionRegistry:
                                    (nullable GPBExtensionRegistry *)extensionRegistry
                                            error:(NSError **)errorPtr;

/// Writes out the message to the given output stream.
- (void)writeToCodedOutputStream:(GPBCodedOutputStream *)output;
/// Writes out the message to the given output stream.
- (void)writeToOutputStream:(NSOutputStream *)output;

/// Writes out a varint for the message size followed by the the message to
/// the given output stream.
- (void)writeDelimitedToCodedOutputStream:(GPBCodedOutputStream *)output;
/// Writes out a varint for the message size followed by the the message to
/// the given output stream.
- (void)writeDelimitedToOutputStream:(NSOutputStream *)output;

/// Serializes the message to a @c NSData.
///
/// If there is an error while generating the data, nil is returned.
///
/// @note This value is not cached, so if you are using it repeatedly, cache
///       it yourself.
///
/// @note In DEBUG ONLY, the message is also checked for all required field,
///       if one is missing, nil will be returned.
- (nullable NSData *)data;

/// Serializes a varint with the message size followed by the message data,
/// returning that as a @c NSData.
///
/// @note This value is not cached, so if you are using it repeatedly, cache
///       it yourself.
- (NSData *)delimitedData;

/// Calculates the size of the object if it were serialized.
///
/// This is not a cached value. If you are following a pattern like this:
/// @code
///   size_t size = [aMsg serializedSize];
///   NSMutableData *foo = [NSMutableData dataWithCapacity:size + sizeof(size)];
///   [foo writeSize:size];
///   [foo appendData:[aMsg data]];
/// @endcode
/// you would be better doing:
/// @code
///   NSData *data = [aMsg data];
///   NSUInteger size = [aMsg length];
///   NSMutableData *foo = [NSMutableData dataWithCapacity:size + sizeof(size)];
///   [foo writeSize:size];
///   [foo appendData:data];
/// @endcode
- (size_t)serializedSize;

/// Return the descriptor for the message class.
+ (GPBDescriptor *)descriptor;
/// Return the descriptor for the message.
- (GPBDescriptor *)descriptor;

/// Returns an array with the currently set GPBExtensionDescriptors.
- (NSArray *)extensionsCurrentlySet;

/// Test to see if the given extension is set on the message.
- (BOOL)hasExtension:(GPBExtensionDescriptor *)extension;

/// Fetches the given extension's value for this message.
///
/// Extensions use boxed values (NSNumbers) for PODs and NSMutableArrays for
/// repeated fields. If the extension is a Message one will be auto created for you
/// and returned similar to fields.
- (nullable id)getExtension:(GPBExtensionDescriptor *)extension;

/// Sets the given extension's value for this message. This is only for single
/// field extensions (i.e. - not repeated fields).
///
/// Extensions use boxed values (@c NSNumbers).
- (void)setExtension:(GPBExtensionDescriptor *)extension value:(nullable id)value;

/// Adds the given value to the extension for this message. This is only for
/// repeated field extensions. If the field is a repeated POD type the @c value
/// is a @c NSNumber.
- (void)addExtension:(GPBExtensionDescriptor *)extension value:(id)value;

/// Replaces the given value at an index for the extension on this message. This
/// is only for repeated field extensions. If the field is a repeated POD type
/// the @c value is a @c NSNumber.
- (void)setExtension:(GPBExtensionDescriptor *)extension
               index:(NSUInteger)index
               value:(id)value;

/// Clears the given extension for this message.
- (void)clearExtension:(GPBExtensionDescriptor *)extension;

/// Resets all of the fields of this message to their default values.
- (void)clear;

/// Parses a message of this type from the input and merges it with this
/// message.
///
/// @note This will throw if there is an error parsing the data.
- (void)mergeFromData:(NSData *)data
    extensionRegistry:(nullable GPBExtensionRegistry *)extensionRegistry;

/// Merges the fields from another message (of the same type) into this
/// message.
- (void)mergeFrom:(GPBMessage *)other;

@end

NS_ASSUME_NONNULL_END
