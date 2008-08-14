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
using System;
using System.Collections.Generic;
using System.IO;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers {

  /// <summary>
  /// Non-generic interface implemented by all Protocol Buffers messages.
  /// Some members are repeated in the generic interface but with a
  /// type-specific signature. Type-safe implementations
  /// are encouraged to implement these non-generic members explicitly,
  /// and the generic members implicitly.
  /// </summary>
  public interface IMessage {
    /// <summary>
    /// Returns the message's type's descriptor. This differs from the
    /// Descriptor property of each generated message class in that this
    /// method is an abstract method of IMessage whereas Descriptor is
    /// a static property of a specific class. They return the same thing.
    /// </summary>
    MessageDescriptor DescriptorForType { get; }
    /// <summary>
    /// Returns a collection of all the fields in this message which are set
    /// and their corresponding values.  A singular ("required" or "optional")
    /// field is set iff HasField() returns true for that field.  A "repeated"
    /// field is set iff GetRepeatedFieldSize() is greater than zero.  The
    /// values are exactly what would be returned by calling
    /// GetField(Descriptors.FieldDescriptor) for each field.  The map
    /// is guaranteed to be a sorted map, so iterating over it will return fields
    /// in order by field number. 
    /// </summary>
    IDictionary<Descriptors.FieldDescriptor, object> AllFields { get; }

    /// <summary>
    /// Returns true if the given field is set. This is exactly equivalent
    /// to calling the generated "Has" property corresponding to the field.
    /// </summary>
    /// <exception cref="ArgumentException">the field is a repeated field,
    /// or it's not a field of this type</exception>
    bool HasField(Descriptors.FieldDescriptor field);

    /// <summary>
    /// Obtains the value of the given field, or the default value if
    /// it isn't set. For value type fields including enums, the boxed
    /// value is returned. For embedded message fields, the sub-message
    /// is returned. For repeated fields, an IList&lt;T&gt; is returned.
    /// </summary>
    object this[Descriptors.FieldDescriptor field] { get; }

    /// <summary>
    /// Returns the number of elements of a repeated field. This is
    /// exactly equivalent to calling the generated "Count" property
    /// corresponding to the field.
    /// </summary>
    /// <exception cref="ArgumentException">the field is not a repeated field,
    /// or it's not a field of this type</exception>
    int GetRepeatedFieldCount(Descriptors.FieldDescriptor field);

    /// <summary>
    /// Gets an element of a repeated field. For value type fields 
    /// including enums, the boxed value is returned. For embedded
    /// message fields, the sub-message is returned.
    /// </summary>
    /// <exception cref="ArgumentException">the field is not a repeated field,
    /// or it's not a field of this type</exception>
    /// <exception cref="ArgumentOutOfRangeException">the index is out of
    /// range for the repeated field's value</exception>
    object this[Descriptors.FieldDescriptor field, int index] { get; }

    /// <summary>
    /// Returns the unknown fields for this message.
    /// </summary>
    UnknownFieldSet UnknownFields { get; }

    /// <summary>
    /// Returns true iff all required fields in the message and all embedded
    /// messages are set.
    /// </summary>
    bool IsInitialized { get; }

    /// <summary>
    /// Serializes the message and writes it to the given output stream.
    /// This does not flush or close the stream.
    /// </summary>
    /// <param name="output"></param>
    void WriteTo(CodedOutputStream output);

    /// <summary>
    /// Returns the number of bytes required to encode this message.
    /// The result is only computed on the first call and memoized after that.
    /// </summary>
    int SerializedSize { get; }

    #region Comparison and hashing
    /// <summary>
    /// Compares the specified object with this message for equality.
    /// Returns true iff the given object is a message of the same type
    /// (as defined by DescriptorForType) and has identical values
    /// for all its fields.
    /// </summary>
    bool Equals(object other);

    /// <summary>
    /// Returns the hash code value for this message.
    /// TODO(jonskeet): Specify the hash algorithm, but better than the Java one!
    /// </summary>
    int GetHashCode();
    #endregion

    #region Convenience methods
    /// <summary>
    /// Converts the message to a string in protocol buffer text format.
    /// This is just a trivial wrapper around TextFormat.PrintToString.
    /// </summary>
    string ToString();

    /// <summary>
    /// Serializes the message to a ByteString. This is a trivial wrapper
    /// around WriteTo(CodedOutputStream).
    /// </summary>
    ByteString ToByteString();

    /// <summary>
    /// Serializes the message to a byte array. This is a trivial wrapper
    /// around WriteTo(CodedOutputStream).
    /// </summary>
    byte[] ToByteArray();

    /// <summary>
    /// Serializes the message and writes it to the given stream.
    /// This is just a wrapper around WriteTo(CodedOutputStream). This
    /// does not flush or close the stream.
    /// </summary>
    /// <param name="output"></param>
    void WriteTo(Stream output);
    #endregion

    #region Weakly typed members
    /// <summary>
    /// Returns an instance of this message type with all fields set to
    /// their default values. This may or may not be a singleton. This differs
    /// from the DefaultInstance property of each generated message class in that this
    /// method is an abstract method of IMessage whereas DefaultInstance is
    /// a static property of a specific class. They return the same thing.
    /// </summary>
    IMessage DefaultInstanceForType { get; }

    /// <summary>
    /// Constructs a new builder for a message of the same type as this message.
    /// </summary>
    IBuilder CreateBuilderForType();
    #endregion
  }

  /// <summary>
  /// Type-safe interface for all generated messages to implement.
  /// </summary>
  public interface IMessage<T> : IMessage where T : IMessage<T> {
    /// <summary>
    /// Returns an instance of this message type with all fields set to
    /// their default values. This may or may not be a singleton. This differs
    /// from the DefaultInstance property of each generated message class in that this
    /// method is an abstract method of IMessage whereas DefaultInstance is
    /// a static property of a specific class. They return the same thing.
    /// </summary>
    new IMessage<T> DefaultInstanceForType { get; }


    #region Builders
    /// <summary>
    /// Constructs a new builder for a message of the same type as this message.
    /// </summary>
    new IBuilder<T> CreateBuilderForType();
    #endregion
  }
}
