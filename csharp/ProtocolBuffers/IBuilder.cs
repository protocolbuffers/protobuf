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
  /// Non-generic interface for all members whose signatures don't require knowledge of
  /// the type being built. The generic interface extends this one. Some methods return
  /// either an IBuilder or an IMessage; in these cases the generic interface redeclares
  /// the same method with a type-specific signature. Implementations are encouraged to
  /// use explicit interface implemenation for the non-generic form. This mirrors
  /// how IEnumerable and IEnumerable&lt;T&gt; work.
  /// </summary>
  public interface IBuilder {
    /// <summary>
    /// Returns true iff all required fields in the message and all
    /// embedded messages are set.
    /// </summary>
    bool Initialized { get; }

    /// <summary>
    /// Behaves like the equivalent property in IMessage&lt;T&gt;.
    /// The returned map may or may not reflect future changes to the builder.
    /// Either way, the returned map is unmodifiable.
    /// </summary>
    IDictionary<FieldDescriptor, object> AllFields { get; }

    /// <summary>
    /// Allows getting and setting of a field.
    /// <see cref="IMessage{T}.Item(FieldDescriptor)"/>
    /// </summary>
    /// <param name="field"></param>
    /// <returns></returns>
    object this[FieldDescriptor field] { get; set; }

    /// <summary>
    /// Get the message's type's descriptor.
    /// <see cref="IMessage{T}.DescriptorForType"/>
    /// </summary>
    MessageDescriptor DescriptorForType { get; }

    /// <summary>
    /// <see cref="IMessage{T}.GetRepeatedFieldCount"/>
    /// </summary>
    /// <param name="field"></param>
    /// <returns></returns>
    int GetRepeatedFieldCount(FieldDescriptor field);

    /// <summary>
    /// Allows getting and setting of a repeated field value.
    /// <see cref="IMessage{T}.Item(FieldDescriptor, int)"/>
    /// </summary>
    object this[FieldDescriptor field, int index] { get; set; }

    /// <summary>
    /// <see cref="IMessage{T}.HasField"/>
    /// </summary>
    bool HasField(FieldDescriptor field);

    /// <summary>
    /// <see cref="IMessage{T}.UnknownFields"/>
    /// </summary>
    UnknownFieldSet UnknownFields { get; set; }

    #region Non-generic versions of generic methods in IBuilder<T>
    IBuilder Clear();
    IBuilder MergeFrom(IMessage other);
    IMessage Build();
    IMessage BuildPartial();
    IBuilder Clone();
    IBuilder MergeFrom(CodedInputStream input);
    IBuilder MergeFrom(CodedInputStream codedInputStream, ExtensionRegistry extensionRegistry);
    IMessage DefaultInstanceForType { get; }
    IBuilder NewBuilderForField<TField>(FieldDescriptor field);
    IBuilder ClearField(FieldDescriptor field);
    IBuilder AddRepeatedField(FieldDescriptor field, object value);
    IBuilder MergeUnknownFields(UnknownFieldSet unknownFields);
    IBuilder MergeFrom(ByteString data);
    IBuilder MergeFrom(ByteString data, ExtensionRegistry extensionRegistry);
    IBuilder MergeFrom(byte[] data);
    IBuilder MergeFrom(byte[] data, ExtensionRegistry extensionRegistry);
    IBuilder MergeFrom(Stream input);
    IBuilder MergeFrom(Stream input, ExtensionRegistry extensionRegistry);
    #endregion
  }

  /// <summary>
  /// Interface implemented by Protocol Message builders.
  /// TODO(jonskeet): Consider "SetXXX" methods returning the builder, as well as the properties.
  /// </summary>
  /// <typeparam name="T">Type of message</typeparam>
  public interface IBuilder<T> : IBuilder where T : IMessage<T> {
    /// <summary>
    /// Resets all fields to their default values.
    /// </summary>
    new IBuilder<T> Clear();

    /// <summary>
    /// Merge the specified other message into the message being
    /// built. Merging occurs as follows. For each field:
    /// For singular primitive fields, if the field is set in <paramref name="other"/>,
    /// then <paramref name="other"/>'s value overwrites the value in this message.
    /// For singular message fields, if the field is set in <paramref name="other"/>,
    /// it is merged into the corresponding sub-message of this message using the same
    /// merging rules.
    /// For repeated fields, the elements in <paramref name="other"/> are concatenated
    /// with the elements in this message.
    /// </summary>
    /// <param name="other"></param>
    /// <returns></returns>
    IBuilder<T> MergeFrom(IMessage<T> other);

    /// <summary>
    /// Constructs the final message. Once this is called, this Builder instance
    /// is no longer valid, and calling any other method may throw a
    /// NullReferenceException. If you need to continue working with the builder
    /// after calling Build, call Clone first.
    /// </summary>
    /// <exception cref="UninitializedMessageException">the message
    /// is missing one or more required fields; use BuildPartial to bypass
    /// this check</exception>
    new IMessage<T> Build();

    /// <summary>
    /// Like Build(), but does not throw an exception if the message is missing
    /// required fields. Instead, a partial message is returned.
    /// </summary>
    /// <returns></returns>
    new IMessage<T> BuildPartial();

    /// <summary>
    /// Clones this builder.
    /// TODO(jonskeet): Explain depth of clone.
    /// </summary>
    new IBuilder<T> Clone();

    /// <summary>
    /// Parses a message of this type from the input and merges it with this
    /// message, as if using MergeFrom(IMessage&lt;T&gt;).
    /// </summary>
    /// <remarks>
    /// Warning: This does not verify that all required fields are present
    /// in the input message. If you call Build() without setting all
    /// required fields, it will throw an UninitializedMessageException.
    /// There are a few good ways to deal with this:
    /// <list>
    /// <item>Call Initialized to verify to verify that all required fields are
    /// set before building.</item>
    /// <item>Parse  the message separately using one of the static ParseFrom
    /// methods, then use MergeFrom(IMessage&lt;T&gt;) to merge it with
    /// this one. ParseFrom will throw an InvalidProtocolBufferException
    /// (an IOException) if some required fields are missing.
    /// Use BuildPartial to build, which ignores missing required fields.
    /// </list>
    /// </remarks>
    new IBuilder<T> MergeFrom(CodedInputStream input);

    /// <summary>
    /// Like MergeFrom(CodedInputStream), but also parses extensions.
    /// The extensions that you want to be able to parse must be registered
    /// in <paramref name="extensionRegistry"/>. Extensions not in the registry
    /// will be treated as unknown fields.
    /// </summary>
    new IBuilder<T> MergeFrom(CodedInputStream input, ExtensionRegistry extensionRegistry);

    /// <summary>
    /// Get's the message's type's default instance.
    /// <see cref="IMessage{T}.DefaultInstanceForType" />
    /// </summary>
    new IMessage<T> DefaultInstanceForType { get; }

    /// <summary>
    /// Create a builder for messages of the appropriate type for the given field.
    /// Messages built with this can then be passed to the various mutation properties
    /// and methods.
    /// </summary>
    new IBuilder<TField> NewBuilderForField<TField>(FieldDescriptor field) where TField : IMessage<TField>;

    /// <summary>
    /// Clears the field. This is exactly equivalent to calling the generated
    /// Clear method corresponding to the field.
    /// </summary>
    /// <param name="field"></param>
    /// <returns></returns>
    new IBuilder<T> ClearField(FieldDescriptor field);

    /// <summary>
    /// Appends the given value as a new element for the specified repeated field.
    /// </summary>
    /// <exception cref="ArgumentException">the field is not a repeated field,
    /// the field does not belong to this builder's type, or the value is
    /// of the incorrect type
    /// </exception>
    new IBuilder<T> AddRepeatedField(FieldDescriptor field, object value);

    /// <summary>
    /// Merge some unknown fields into the set for this message.
    /// </summary>
    new IBuilder<T> MergeUnknownFields(UnknownFieldSet unknownFields);

    #region Convenience methods
    // TODO(jonskeet): Implement these as extension methods?
    /// <summary>
    /// Parse <paramref name="data"/> as a message of this type and merge
    /// it with the message being built. This is just a small wrapper around
    /// MergeFrom(CodedInputStream).
    /// </summary>
    new IBuilder<T> MergeFrom(ByteString data);

    /// <summary>
    /// Parse <paramref name="data"/> as a message of this type and merge
    /// it with the message being built. This is just a small wrapper around
    /// MergeFrom(CodedInputStream, ExtensionRegistry).
    /// </summary>
    new IBuilder<T> MergeFrom(ByteString data, ExtensionRegistry extensionRegistry);

    /// <summary>
    /// Parse <paramref name="data"/> as a message of this type and merge
    /// it with the message being built. This is just a small wrapper around
    /// MergeFrom(CodedInputStream).
    /// </summary>
    new IBuilder<T> MergeFrom(byte[] data);

    /// <summary>
    /// Parse <paramref name="data"/> as a message of this type and merge
    /// it with the message being built. This is just a small wrapper around
    /// MergeFrom(CodedInputStream, ExtensionRegistry).
    /// </summary>
    new IBuilder<T> MergeFrom(byte[] data, ExtensionRegistry extensionRegistry);

    /// <summary>
    /// Parse <paramref name="input"/> as a message of this type and merge
    /// it with the message being built. This is just a small wrapper around
    /// MergeFrom(CodedInputStream). Note that this method always reads
    /// the entire input (unless it throws an exception). If you want it to
    /// stop earlier, you will need to wrap the input in a wrapper
    /// stream which limits reading. Despite usually reading the entire
    /// stream, this method never closes the stream.
    /// </summary>
    new IBuilder<T> MergeFrom(Stream input);

    /// <summary>
    /// Parse <paramref name="input"/> as a message of this type and merge
    /// it with the message being built. This is just a small wrapper around
    /// MergeFrom(CodedInputStream, ExtensionRegistry).
    /// </summary>
    new IBuilder<T> MergeFrom(Stream input, ExtensionRegistry extensionRegistry);
    #endregion
  }
}
