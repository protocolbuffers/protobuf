// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// This file contains the abstract interface for all protocol messages.
// Although it's possible to implement this interface manually, most users
// will use the protocol compiler to generate implementations.
//
// Example usage:
//
// Say you have a message defined as:
//
//   message Foo {
//     optional string text = 1;
//     repeated int32 numbers = 2;
//   }
//
// Then, if you used the protocol compiler to generate a class from the above
// definition, you could use it like so:
//
//   string data;  // Will store a serialized version of the message.
//
//   {
//     // Create a message and serialize it.
//     Foo foo;
//     foo.set_text("Hello World!");
//     foo.add_numbers(1);
//     foo.add_numbers(5);
//     foo.add_numbers(42);
//
//     foo.SerializeToString(&data);
//   }
//
//   {
//     // Parse the serialized message and check that it contains the
//     // correct data.
//     Foo foo;
//     foo.ParseFromString(data);
//
//     assert(foo.text() == "Hello World!");
//     assert(foo.numbers_size() == 3);
//     assert(foo.numbers(0) == 1);
//     assert(foo.numbers(1) == 5);
//     assert(foo.numbers(2) == 42);
//   }
//
//   {
//     // Same as the last block, but do it dynamically via the Message
//     // reflection interface.
//     Message* foo = new Foo;
//     Descriptor* descriptor = foo->GetDescriptor();
//
//     // Get the descriptors for the fields we're interested in and verify
//     // their types.
//     FieldDescriptor* text_field = descriptor->FindFieldByName("text");
//     assert(text_field != NULL);
//     assert(text_field->type() == FieldDescriptor::TYPE_STRING);
//     assert(text_field->label() == FieldDescriptor::TYPE_OPTIONAL);
//     FieldDescriptor* numbers_field = descriptor->FindFieldByName("numbers");
//     assert(numbers_field != NULL);
//     assert(numbers_field->type() == FieldDescriptor::TYPE_INT32);
//     assert(numbers_field->label() == FieldDescriptor::TYPE_REPEATED);
//
//     // Parse the message.
//     foo->ParseFromString(data);
//
//     // Use the reflection interface to examine the contents.
//     Message::Reflection* reflection = foo->GetReflection();
//     assert(reflection->GetString(text_field) == "Hello World!");
//     assert(reflection->CountField(numbers_field) == 3);
//     assert(reflection->GetInt32(numbers_field, 0) == 1);
//     assert(reflection->GetInt32(numbers_field, 1) == 5);
//     assert(reflection->GetInt32(numbers_field, 2) == 42);
//
//     delete foo;
//   }

#ifndef GOOGLE_PROTOBUF_MESSAGE_H__
#define GOOGLE_PROTOBUF_MESSAGE_H__

#include <vector>
#include <string>
#include <iosfwd>
#include <google/protobuf/stubs/common.h>

namespace google {

namespace protobuf {

// Defined in this file.
class Message;

// Defined in other files.
class Descriptor;            // descriptor.h
class FieldDescriptor;       // descriptor.h
class EnumValueDescriptor;   // descriptor.h
namespace io {
  class ZeroCopyInputStream;   // zero_copy_stream.h
  class ZeroCopyOutputStream;  // zero_copy_stream.h
  class CodedInputStream;      // coded_stream.h
  class CodedOutputStream;     // coded_stream.h
}
class UnknownFieldSet;       // unknown_field_set.h

// Abstract interface for protocol messages.
//
// The methods of this class that are virtual but not pure-virtual have
// default implementations based on reflection.  Message classes which are
// optimized for speed will want to override these with faster implementations,
// but classes optimized for code size may be happy with keeping them.  See
// the optimize_for option in descriptor.proto.
class LIBPROTOBUF_EXPORT Message {
 public:
  inline Message() {}
  virtual ~Message();

  // Basic Operations ------------------------------------------------

  // Construct a new instance of the same type.  Ownership is passed to the
  // caller.
  virtual Message* New() const = 0;

  // Make this message into a copy of the given message.  The given message
  // must have the same descriptor, but need not necessarily be the same class.
  // By default this is just implemented as "Clear(); MergeFrom(from);".
  virtual void CopyFrom(const Message& from);

  // Merge the fields from the given message into this message.  Singular
  // fields will be overwritten, except for embedded messages which will
  // be merged.  Repeated fields will be concatenated.  The given message
  // must be of the same type as this message (i.e. the exact same class).
  virtual void MergeFrom(const Message& from);

  // Clear all fields of the message and set them to their default values.
  // Clear() avoids freeing memory, assuming that any memory allocated
  // to hold parts of the message will be needed again to hold the next
  // message.  If you actually want to free the memory used by a Message,
  // you must delete it.
  virtual void Clear();

  // Quickly check if all required fields have values set.
  virtual bool IsInitialized() const;

  // Verifies that IsInitialized() returns true.  GOOGLE_CHECK-fails otherwise, with
  // a nice error message.
  void CheckInitialized() const;

  // Slowly build a list of all required fields that are not set.
  // This is much, much slower than IsInitialized() as it is implemented
  // purely via reflection.  Generally, you should not call this unless you
  // have already determined that an error exists by calling IsInitialized().
  void FindInitializationErrors(vector<string>* errors) const;

  // Like FindInitializationErrors, but joins all the strings, delimited by
  // commas, and returns them.
  string InitializationErrorString() const;

  // Clears all unknown fields from this message and all embedded messages.
  // Normally, if unknown tag numbers are encountered when parsing a message,
  // the tag and value are stored in the message's UnknownFieldSet and
  // then written back out when the message is serialized.  This allows servers
  // which simply route messages to other servers to pass through messages
  // that have new field definitions which they don't yet know about.  However,
  // this behavior can have security implications.  To avoid it, call this
  // method after parsing.
  //
  // See Reflection::GetUnknownFields() for more on unknown fields.
  virtual void DiscardUnknownFields();

  // Debugging -------------------------------------------------------

  // Generates a human readable form of this message, useful for debugging
  // and other purposes.
  string DebugString() const;
  // Like DebugString(), but with less whitespace.
  string ShortDebugString() const;
  // Convenience function useful in GDB.  Prints DebugString() to stdout.
  void PrintDebugString() const;

  // Parsing ---------------------------------------------------------
  // Methods for parsing in protocol buffer format.  Most of these are
  // just simple wrappers around MergeFromCodedStream().

  // Fill the message with a protocol buffer parsed from the given input
  // stream.  Returns false on a read error or if the input is in the
  // wrong format.
  bool ParseFromCodedStream(io::CodedInputStream* input);
  // Like ParseFromCodedStream(), but accepts messages that are missing
  // required fields.
  bool ParsePartialFromCodedStream(io::CodedInputStream* input);
  // Read a protocol buffer from the given zero-copy input stream.  If
  // successful, the entire input will be consumed.
  bool ParseFromZeroCopyStream(io::ZeroCopyInputStream* input);
  // Like ParseFromZeroCopyStream(), but accepts messages that are missing
  // required fields.
  bool ParsePartialFromZeroCopyStream(io::ZeroCopyInputStream* input);
  // Parse a protocol buffer contained in a string.
  bool ParseFromString(const string& data);
  // Like ParseFromString(), but accepts messages that are missing
  // required fields.
  bool ParsePartialFromString(const string& data);
  // Parse a protocol buffer contained in an array of bytes.
  bool ParseFromArray(const void* data, int size);
  // Like ParseFromArray(), but accepts messages that are missing
  // required fields.
  bool ParsePartialFromArray(const void* data, int size);

  // Parse a protocol buffer from a file descriptor.  If successful, the entire
  // input will be consumed.
  bool ParseFromFileDescriptor(int file_descriptor);
  // Like ParseFromFileDescriptor(), but accepts messages that are missing
  // required fields.
  bool ParsePartialFromFileDescriptor(int file_descriptor);
  // Parse a protocol buffer from a C++ istream.  If successful, the entire
  // input will be consumed.
  bool ParseFromIstream(istream* input);
  // Like ParseFromIstream(), but accepts messages that are missing
  // required fields.
  bool ParsePartialFromIstream(istream* input);


  // Reads a protocol buffer from the stream and merges it into this
  // Message.  Singular fields read from the input overwrite what is
  // already in the Message and repeated fields are appended to those
  // already present.
  //
  // It is the responsibility of the caller to call input->LastTagWas()
  // (for groups) or input->ConsumedEntireMessage() (for non-groups) after
  // this returns to verify that the message's end was delimited correctly.
  //
  // ParsefromCodedStream() is implemented as Clear() followed by
  // MergeFromCodedStream().
  bool MergeFromCodedStream(io::CodedInputStream* input);

  // Like MergeFromCodedStream(), but succeeds even if required fields are
  // missing in the input.
  //
  // MergeFromCodedStream() is just implemented as MergePartialFromCodedStream()
  // followed by IsInitialized().
  virtual bool MergePartialFromCodedStream(io::CodedInputStream* input);

  // Serialization ---------------------------------------------------
  // Methods for serializing in protocol buffer format.  Most of these
  // are just simple wrappers around ByteSize() and SerializeWithCachedSizes().

  // Write a protocol buffer of this message to the given output.  Returns
  // false on a write error.  If the message is missing required fields,
  // this may GOOGLE_CHECK-fail.
  bool SerializeToCodedStream(io::CodedOutputStream* output) const;
  // Like SerializeToCodedStream(), but allows missing required fields.
  bool SerializePartialToCodedStream(io::CodedOutputStream* output) const;
  // Write the message to the given zero-copy output stream.  All required
  // fields must be set.
  bool SerializeToZeroCopyStream(io::ZeroCopyOutputStream* output) const;
  // Like SerializeToZeroCopyStream(), but allows missing required fields.
  bool SerializePartialToZeroCopyStream(io::ZeroCopyOutputStream* output) const;
  // Serialize the message and store it in the given string.  All required
  // fields must be set.
  bool SerializeToString(string* output) const;
  // Like SerializeToString(), but allows missing required fields.
  bool SerializePartialToString(string* output) const;
  // Serialize the message and store it in the given byte array.  All required
  // fields must be set.
  bool SerializeToArray(void* data, int size) const;
  // Like SerializeToArray(), but allows missing required fields.
  bool SerializePartialToArray(void* data, int size) const;

  // Serialize the message and write it to the given file descriptor.  All
  // required fields must be set.
  bool SerializeToFileDescriptor(int file_descriptor) const;
  // Like SerializeToFileDescriptor(), but allows missing required fields.
  bool SerializePartialToFileDescriptor(int file_descriptor) const;
  // Serialize the message and write it to the given C++ ostream.  All
  // required fields must be set.
  bool SerializeToOstream(ostream* output) const;
  // Like SerializeToOstream(), but allows missing required fields.
  bool SerializePartialToOstream(ostream* output) const;


  // Like SerializeToString(), but appends to the data to the string's existing
  // contents.  All required fields must be set.
  bool AppendToString(string* output) const;
  // Like AppendToString(), but allows missing required fields.
  bool AppendPartialToString(string* output) const;

  // Computes the serialized size of the message.  This recursively calls
  // ByteSize() on all embedded messages.  If a subclass does not override
  // this, it MUST override SetCachedSize().
  virtual int ByteSize() const;

  // Serializes the message without recomputing the size.  The message must
  // not have changed since the last call to ByteSize(); if it has, the results
  // are undefined.
  virtual bool SerializeWithCachedSizes(io::CodedOutputStream* output) const;

  // Returns the result of the last call to ByteSize().  An embedded message's
  // size is needed both to serialize it (because embedded messages are
  // length-delimited) and to compute the outer message's size.  Caching
  // the size avoids computing it multiple times.
  //
  // ByteSize() does not automatically use the cached size when available
  // because this would require invalidating it every time the message was
  // modified, which would be too hard and expensive.  (E.g. if a deeply-nested
  // sub-message is changed, all of its parents' cached sizes would need to be
  // invalidated, which is too much work for an otherwise inlined setter
  // method.)
  virtual int GetCachedSize() const = 0;

 private:
  // This is called only by the default implementation of ByteSize(), to
  // update the cached size.  If you override ByteSize(), you do not need
  // to override this.  If you do not override ByteSize(), you MUST override
  // this; the default implementation will crash.
  //
  // The method is private because subclasses should never call it; only
  // override it.  Yes, C++ lets you do that.  Crazy, huh?
  virtual void SetCachedSize(int size) const;

 public:

  // Introspection ---------------------------------------------------

  class Reflection;  // Defined below.

  // Get a Descriptor for this message's type.  This describes what
  // fields the message contains, the types of those fields, etc.
  virtual const Descriptor* GetDescriptor() const = 0;

  // Get the Reflection interface for this Message, which can be used to
  // read and modify the fields of the Message dynamically (in other words,
  // without knowing the message type at compile time).  This object remains
  // property of the Message.
  virtual const Reflection* GetReflection() const = 0;

  // Get the Reflection interface for this Message, which can be used to
  // read and modify the fields of the Message dynamically (in other words,
  // without knowing the message type at compile time).  This object remains
  // property of the Message.
  virtual Reflection* GetReflection() = 0;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Message);
};

// This interface contains methods that can be used to dynamically access
// and modify the fields of a protocol message.  Their semantics are
// similar to the accessors the protocol compiler generates.
//
// To get the Reflection for a given Message, call Message::GetReflection().
//
// This interface is separate from Message only for efficiency reasons;
// the vast majority of implementations of Message will share the same
// implementation of Reflection (GeneratedMessageReflection,
// defined in generated_message.h).
//
// There are several ways that these methods can be used incorrectly.  For
// example, any of the following conditions will lead to undefined
// results (probably assertion failures):
// - The FieldDescriptor is not a field of this message type.
// - The method called is not appropriate for the field's type.  For
//   each field type in FieldDescriptor::TYPE_*, there is only one
//   Get*() method, one Set*() method, and one Add*() method that is
//   valid for that type.  It should be obvious which (except maybe
//   for TYPE_BYTES, which are represented using strings in C++).
// - A Get*() or Set*() method for singular fields is called on a repeated
//   field.
// - GetRepeated*(), SetRepeated*(), or Add*() is called on a non-repeated
//   field.
//
// You might wonder why there is not any abstract representation for a field
// of arbitrary type.  E.g., why isn't there just a "GetField()" method that
// returns "const Field&", where "Field" is some class with accessors like
// "GetInt32Value()".  The problem is that someone would have to deal with
// allocating these Field objects.  For generated message classes, having to
// allocate space for an additional object to wrap every field would at least
// double the message's memory footprint, probably worse.  Allocating the
// objects on-demand, on the other hand, would be expensive and prone to
// memory leaks.  So, instead we ended up with this flat interface.
//
// TODO(kenton):  Create a utility class which callers can use to read and
//   write fields from a Reflection without paying attention to the type.
class LIBPROTOBUF_EXPORT Message::Reflection {
 public:
  inline Reflection() {}
  virtual ~Reflection();

  // Get the UnknownFieldSet for the message.  This contains fields which
  // were seen when the Message was parsed but were not recognized according
  // to the Message's definition.
  virtual const UnknownFieldSet& GetUnknownFields() const = 0;
  // Get a mutable pointer to the UnknownFieldSet for the message.  This
  // contains fields which were seen when the Message was parsed but were not
  // recognized according to the Message's definition.
  virtual UnknownFieldSet* MutableUnknownFields() = 0;

  // Check if the given non-repeated field is set.
  virtual bool HasField(const FieldDescriptor* field) const = 0;

  // Get the number of elements of a repeated field.
  virtual int FieldSize(const FieldDescriptor* field) const = 0;

  // Clear the value of a field, so that HasField() returns false or
  // FieldSize() returns zero.
  virtual void ClearField(const FieldDescriptor* field) = 0;

  // List all fields of the message which are currently set.  This includes
  // extensions.  Singular fields will only be listed if HasField(field) would
  // return true and repeated fields will only be listed if FieldSize(field)
  // would return non-zero.  Fields (both normal fields and extension fields)
  // will be listed ordered by field number.
  virtual void ListFields(vector<const FieldDescriptor*>* output) const = 0;

  // Singular field getters ------------------------------------------
  // These get the value of a non-repeated field.  They return the default
  // value for fields that aren't set.

  virtual int32  GetInt32 (const FieldDescriptor* field) const = 0;
  virtual int64  GetInt64 (const FieldDescriptor* field) const = 0;
  virtual uint32 GetUInt32(const FieldDescriptor* field) const = 0;
  virtual uint64 GetUInt64(const FieldDescriptor* field) const = 0;
  virtual float  GetFloat (const FieldDescriptor* field) const = 0;
  virtual double GetDouble(const FieldDescriptor* field) const = 0;
  virtual bool   GetBool  (const FieldDescriptor* field) const = 0;
  virtual string GetString(const FieldDescriptor* field) const = 0;
  virtual const EnumValueDescriptor* GetEnum(
      const FieldDescriptor* field) const = 0;
  virtual const Message& GetMessage(const FieldDescriptor* field) const = 0;

  // Get a string value without copying, if possible.
  //
  // GetString() necessarily returns a copy of the string.  This can be
  // inefficient when the string is already stored in a string object in the
  // underlying message.  GetStringReference() will return a reference to the
  // underlying string in this case.  Otherwise, it will copy the string into
  // *scratch and return that.
  //
  // Note:  It is perfectly reasonable and useful to write code like:
  //     str = reflection->GetStringReference(field, &str);
  //   This line would ensure that only one copy of the string is made
  //   regardless of the field's underlying representation.  When initializing
  //   a newly-constructed string, though, it's just as fast and more readable
  //   to use code like:
  //     string str = reflection->GetString(field);
  virtual const string& GetStringReference(const FieldDescriptor* field,
                                           string* scratch) const = 0;


  // Singular field mutators -----------------------------------------
  // These mutate the value of a non-repeated field.

  virtual void SetInt32 (const FieldDescriptor* field, int32  value) = 0;
  virtual void SetInt64 (const FieldDescriptor* field, int64  value) = 0;
  virtual void SetUInt32(const FieldDescriptor* field, uint32 value) = 0;
  virtual void SetUInt64(const FieldDescriptor* field, uint64 value) = 0;
  virtual void SetFloat (const FieldDescriptor* field, float  value) = 0;
  virtual void SetDouble(const FieldDescriptor* field, double value) = 0;
  virtual void SetBool  (const FieldDescriptor* field, bool   value) = 0;
  virtual void SetString(const FieldDescriptor* field, const string& value) = 0;
  virtual void SetEnum  (const FieldDescriptor* field,
                         const EnumValueDescriptor* value) = 0;
  // Get a mutable pointer to a field with a message type.
  virtual Message* MutableMessage(const FieldDescriptor* field) = 0;


  // Repeated field getters ------------------------------------------
  // These get the value of one element of a repeated field.

  virtual int32  GetRepeatedInt32 (const FieldDescriptor* field,
                                   int index) const = 0;
  virtual int64  GetRepeatedInt64 (const FieldDescriptor* field,
                                   int index) const = 0;
  virtual uint32 GetRepeatedUInt32(const FieldDescriptor* field,
                                   int index) const = 0;
  virtual uint64 GetRepeatedUInt64(const FieldDescriptor* field,
                                   int index) const = 0;
  virtual float  GetRepeatedFloat (const FieldDescriptor* field,
                                   int index) const = 0;
  virtual double GetRepeatedDouble(const FieldDescriptor* field,
                                   int index) const = 0;
  virtual bool   GetRepeatedBool  (const FieldDescriptor* field,
                                   int index) const = 0;
  virtual string GetRepeatedString(const FieldDescriptor* field,
                                   int index) const = 0;
  virtual const EnumValueDescriptor* GetRepeatedEnum(
      const FieldDescriptor* field, int index) const = 0;
  virtual const Message& GetRepeatedMessage(
      const FieldDescriptor* field, int index) const = 0;

  // See GetStringReference(), above.
  virtual const string& GetRepeatedStringReference(
      const FieldDescriptor* field, int index,
      string* scratch) const = 0;


  // Repeated field mutators -----------------------------------------
  // These mutate the value of one element of a repeated field.

  virtual void SetRepeatedInt32 (const FieldDescriptor* field,
                                 int index, int32  value) = 0;
  virtual void SetRepeatedInt64 (const FieldDescriptor* field,
                                 int index, int64  value) = 0;
  virtual void SetRepeatedUInt32(const FieldDescriptor* field,
                                 int index, uint32 value) = 0;
  virtual void SetRepeatedUInt64(const FieldDescriptor* field,
                                 int index, uint64 value) = 0;
  virtual void SetRepeatedFloat (const FieldDescriptor* field,
                                 int index, float  value) = 0;
  virtual void SetRepeatedDouble(const FieldDescriptor* field,
                                 int index, double value) = 0;
  virtual void SetRepeatedBool  (const FieldDescriptor* field,
                                 int index, bool   value) = 0;
  virtual void SetRepeatedString(const FieldDescriptor* field,
                                 int index, const string& value) = 0;
  virtual void SetRepeatedEnum(const FieldDescriptor* field,
                               int index, const EnumValueDescriptor* value) = 0;
  // Get a mutable pointer to an element of a repeated field with a message
  // type.
  virtual Message* MutableRepeatedMessage(
      const FieldDescriptor* field, int index) = 0;


  // Repeated field adders -------------------------------------------
  // These add an element to a repeated field.

  virtual void AddInt32 (const FieldDescriptor* field, int32  value) = 0;
  virtual void AddInt64 (const FieldDescriptor* field, int64  value) = 0;
  virtual void AddUInt32(const FieldDescriptor* field, uint32 value) = 0;
  virtual void AddUInt64(const FieldDescriptor* field, uint64 value) = 0;
  virtual void AddFloat (const FieldDescriptor* field, float  value) = 0;
  virtual void AddDouble(const FieldDescriptor* field, double value) = 0;
  virtual void AddBool  (const FieldDescriptor* field, bool   value) = 0;
  virtual void AddString(const FieldDescriptor* field, const string& value) = 0;
  virtual void AddEnum  (const FieldDescriptor* field,
                         const EnumValueDescriptor* value) = 0;
  virtual Message* AddMessage(const FieldDescriptor* field) = 0;


  // Extensions ------------------------------------------------------

  // Try to find an extension of this message type by fully-qualified field
  // name.  Returns NULL if no extension is known for this name or number.
  virtual const FieldDescriptor* FindKnownExtensionByName(
      const string& name) const = 0;

  // Try to find an extension of this message type by field number.
  // Returns NULL if no extension is known for this name or number.
  virtual const FieldDescriptor* FindKnownExtensionByNumber(
      int number) const = 0;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Reflection);
};

// Abstract interface for a factory for message objects.
class LIBPROTOBUF_EXPORT MessageFactory {
 public:
  inline MessageFactory() {}
  virtual ~MessageFactory();

  // Given a Descriptor, gets or constructs the default (prototype) Message
  // of that type.  You can then call that message's New() method to construct
  // a mutable message of that type.
  //
  // Calling this method twice with the same Descriptor returns the same
  // object.  The returned object remains property of the factory.  Also, any
  // objects created by calling the prototype's New() method share some data
  // with the prototype, so these must be destoyed before the MessageFactory
  // is destroyed.
  //
  // The given descriptor must outlive the returned message, and hence must
  // outlive the MessageFactory.
  //
  // Some implementations do not support all types.  GetPrototype() will
  // return NULL if the descriptor passed in is not supported.
  //
  // This method may or may not be thread-safe depending on the implementation.
  // Each implementation should document its own degree thread-safety.
  virtual const Message* GetPrototype(const Descriptor* type) = 0;

  // Gets a MessageFactory which supports all generated, compiled-in messages.
  // In other words, for any compiled-in type FooMessage, the following is true:
  //   MessageFactory::generated_factory()->GetPrototype(
  //     FooMessage::descriptor()) == FooMessage::default_instance()
  // This factory supports all types which are found in
  // DescriptorPool::generated_pool().  If given a descriptor from any other
  // pool, GetPrototype() will return NULL.  (You can also check if a
  // descriptor is for a generated message by checking if
  // descriptor->file()->pool() == DescriptorPool::generated_pool().)
  //
  // This factory is 100% thread-safe; calling GetPrototype() does not modify
  // any shared data.
  //
  // This factory is a singleton.  The caller must not delete the object.
  static MessageFactory* generated_factory();

  // For internal use only:  Registers a message type at static initialization
  // time, to be placed in generated_factory().
  static void InternalRegisterGeneratedMessage(const Descriptor* descriptor,
                                               const Message* prototype);

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(MessageFactory);
};

}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_MESSAGE_H__
