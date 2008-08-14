using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace Google.ProtocolBuffers {

  public interface IBuilder {
    void MergeFrom(CodedInputStream codedInputStream, ExtensionRegistry extensionRegistry);
  }

  /// <summary>
  /// Interface implemented by Protocol Message builders.
  /// TODO(jonskeet): Consider "SetXXX" methods returning the builder, as well as the properties.
  /// </summary>
  /// <typeparam name="T">Type of message</typeparam>
  public interface IBuilder<T> where T : IMessage<T> {
    /// <summary>
    /// Resets all fields to their default values.
    /// </summary>
    IBuilder<T> Clear();

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
    IMessage<T> Build();

    /// <summary>
    /// Like Build(), but does not throw an exception if the message is missing
    /// required fields. Instead, a partial message is returned.
    /// </summary>
    /// <returns></returns>
    IMessage<T> BuildPartial();

    /// <summary>
    /// Clones this builder.
    /// TODO(jonskeet): Explain depth of clone.
    /// </summary>
    IBuilder<T> Clone();

    /// <summary>
    /// Returns true iff all required fields in the message and all
    /// embedded messages are set.
    /// </summary>
    bool Initialized { get; }

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
    IBuilder<T> MergeFrom(CodedInputStream input);

    /// <summary>
    /// Like MergeFrom(CodedInputStream), but also parses extensions.
    /// The extensions that you want to be able to parse must be registered
    /// in <paramref name="extensionRegistry"/>. Extensions not in the registry
    /// will be treated as unknown fields.
    /// </summary>
    IBuilder<T> MergeFrom(CodedInputStream input, ExtensionRegistry extensionRegistry);

    /// <summary>
    /// Get the message's type's descriptor.
    /// <see cref="IMessage{T}.DescriptorForType"/>
    /// </summary>
    Descriptors.Descriptor DescriptorForType { get; }

    /// <summary>
    /// Get's the message's type's default instance.
    /// <see cref="IMessage{T}.DefaultInstanceForType" />
    /// </summary>
    IMessage<T> DefaultInstanceForType { get; }

    /// <summary>
    /// Behaves like the equivalent property in IMessage&lt;T&gt;.
    /// The returned map may or may not reflect future changes to the builder.
    /// Either way, the returned map is unmodifiable.
    /// </summary>
    IDictionary<ProtocolBuffers.Descriptors.FieldDescriptor, object> AllFields { get; }

    /// <summary>
    /// Create a builder for messages of the appropriate type for the given field.
    /// Messages built with this can then be passed to the various mutation properties
    /// and methods.
    /// </summary>
    /// <typeparam name="TField"></typeparam>
    /// <param name="field"></param>
    /// <returns></returns>
    IBuilder<TField> NewBuilderForField<TField>(Descriptors.FieldDescriptor field)
        where TField : IMessage<TField>;

    /// <summary>
    /// <see cref="IMessage{T}.HasField"/>
    /// </summary>
    bool HasField(Descriptors.FieldDescriptor field);

    /// <summary>
    /// Allows getting and setting of a field.
    /// <see cref="IMessage{T}.Item(Descriptors.FieldDescriptor)"/>
    /// </summary>
    /// <param name="field"></param>
    /// <returns></returns>
    object this[Descriptors.FieldDescriptor field] { get; set; }

    /// <summary>
    /// Clears the field. This is exactly equivalent to calling the generated
    /// Clear method corresponding to the field.
    /// </summary>
    /// <param name="field"></param>
    /// <returns></returns>
    IBuilder<T> ClearField(Descriptors.FieldDescriptor field);

    /// <summary>
    /// <see cref="IMessage{T}.GetRepeatedFieldCount"/>
    /// </summary>
    /// <param name="field"></param>
    /// <returns></returns>
    int GetRepeatedFieldCount(Descriptors.FieldDescriptor field);


    /// <summary>
    /// Allows getting and setting of a repeated field value.
    /// <see cref="IMessage{T}.Item(Descriptors.FieldDescriptor, int)"/>
    /// </summary>
    object this[Descriptors.FieldDescriptor field, int index] { get; set; }

    /// <summary>
    /// Appends the given value as a new element for the specified repeated field.
    /// </summary>
    /// <exception cref="ArgumentException">the field is not a repeated field,
    /// the field does not belong to this builder's type, or the value is
    /// of the incorrect type
    /// </exception>
    IBuilder<T> AddRepeatedField(Descriptors.FieldDescriptor field, object value);

    /// <summary>
    /// <see cref="IMessage{T}.UnknownFields"/>
    /// </summary>
    UnknownFieldSet UnknownFields { get; set; }

    /// <summary>
    /// Merge some unknown fields into the set for this message.
    /// </summary>
    IBuilder<T> MergeUnknownFields(UnknownFieldSet unknownFields);

    #region Convenience methods
    // TODO(jonskeet): Implement these as extension methods?

    /// <summary>
    /// Parse <paramref name="data"/> as a message of this type and merge
    /// it with the message being built. This is just a small wrapper around
    /// MergeFrom(CodedInputStream).
    /// </summary>
    IBuilder<T> MergeFrom(ByteString data);

    /// <summary>
    /// Parse <paramref name="data"/> as a message of this type and merge
    /// it with the message being built. This is just a small wrapper around
    /// MergeFrom(CodedInputStream, ExtensionRegistry).
    /// </summary>
    IBuilder<T> MergeFrom(ByteString data, ExtensionRegistry extensionRegistry);

    /// <summary>
    /// Parse <paramref name="data"/> as a message of this type and merge
    /// it with the message being built. This is just a small wrapper around
    /// MergeFrom(CodedInputStream).
    /// </summary>
    IBuilder<T> MergeFrom(byte[] data);

    /// <summary>
    /// Parse <paramref name="data"/> as a message of this type and merge
    /// it with the message being built. This is just a small wrapper around
    /// MergeFrom(CodedInputStream, ExtensionRegistry).
    /// </summary>
    IBuilder<T> MergeFrom(byte[] data, ExtensionRegistry extensionRegistry);

    /// <summary>
    /// Parse <paramref name="data"/> as a message of this type and merge
    /// it with the message being built. This is just a small wrapper around
    /// MergeFrom(CodedInputStream). Note that this method always reads
    /// the entire input (unless it throws an exception). If you want it to
    /// stop earlier, you will need to wrap the input in a wrapper
    /// stream which limits reading. Despite usually reading the entire
    /// stream, this method never closes the stream.
    /// </summary>
    IBuilder<T> MergeFrom(Stream input);

    /// <summary>
    /// Parse <paramref name="data"/> as a message of this type and merge
    /// it with the message being built. This is just a small wrapper around
    /// MergeFrom(CodedInputStream, ExtensionRegistry).
    /// </summary>
    IBuilder<T> MergeFrom(Stream input, ExtensionRegistry extensionRegistry);
    #endregion
  }
}
