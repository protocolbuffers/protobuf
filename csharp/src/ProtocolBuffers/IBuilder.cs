#region Copyright notice and license

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
// http://code.google.com/p/protobuf/
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

#endregion

using System;
using System.Collections.Generic;
using System.IO;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Non-generic interface for all members whose signatures don't require knowledge of
    /// the type being built. The generic interface extends this one. Some methods return
    /// either an IBuilder or an IMessage; in these cases the generic interface redeclares
    /// the same method with a type-specific signature. Implementations are encouraged to
    /// use explicit interface implemenation for the non-generic form. This mirrors
    /// how IEnumerable and IEnumerable&lt;T&gt; work.
    /// </summary>
    public interface IBuilder : IBuilderLite
    {
        /// <summary>
        /// Returns true iff all required fields in the message and all
        /// embedded messages are set.
        /// </summary>
        new bool IsInitialized { get; }

        /// <summary>
        /// Only present in the nongeneric interface - useful for tests, but
        /// not as much in real life.
        /// </summary>
        IBuilder SetField(FieldDescriptor field, object value);

        /// <summary>
        /// Only present in the nongeneric interface - useful for tests, but
        /// not as much in real life.
        /// </summary>
        IBuilder SetRepeatedField(FieldDescriptor field, int index, object value);

        /// <summary>
        /// Behaves like the equivalent property in IMessage&lt;T&gt;.
        /// The returned map may or may not reflect future changes to the builder.
        /// Either way, the returned map is unmodifiable.
        /// </summary>
        IDictionary<FieldDescriptor, object> AllFields { get; }

        /// <summary>
        /// Allows getting and setting of a field.
        /// <see cref="IMessage{TMessage, TBuilder}.Item(FieldDescriptor)"/>
        /// </summary>
        /// <param name="field"></param>
        /// <returns></returns>
        object this[FieldDescriptor field] { get; set; }

        /// <summary>
        /// Get the message's type descriptor.
        /// <see cref="IMessage{TMessage, TBuilder}.DescriptorForType"/>
        /// </summary>
        MessageDescriptor DescriptorForType { get; }

        /// <summary>
        /// <see cref="IMessage{TMessage, TBuilder}.GetRepeatedFieldCount"/>
        /// </summary>
        /// <param name="field"></param>
        /// <returns></returns>
        int GetRepeatedFieldCount(FieldDescriptor field);

        /// <summary>
        /// Allows getting and setting of a repeated field value.
        /// <see cref="IMessage{TMessage, TBuilder}.Item(FieldDescriptor, int)"/>
        /// </summary>
        object this[FieldDescriptor field, int index] { get; set; }

        /// <summary>
        /// <see cref="IMessage{TMessage, TBuilder}.HasField"/>
        /// </summary>
        bool HasField(FieldDescriptor field);

        /// <summary>
        /// <see cref="IMessage{TMessage, TBuilder}.UnknownFields"/>
        /// </summary>
        UnknownFieldSet UnknownFields { get; set; }

        /// <summary>
        /// Create a builder for messages of the appropriate type for the given field.
        /// Messages built with this can then be passed to the various mutation properties
        /// and methods.
        /// </summary>
        IBuilder CreateBuilderForField(FieldDescriptor field);

        #region Methods which are like those of the generic form, but without any knowledge of the type parameters

        IBuilder WeakAddRepeatedField(FieldDescriptor field, object value);
        new IBuilder WeakClear();
        IBuilder WeakClearField(FieldDescriptor field);
        IBuilder WeakMergeFrom(IMessage message);
        new IBuilder WeakMergeFrom(ByteString data);
        new IBuilder WeakMergeFrom(ByteString data, ExtensionRegistry registry);
        new IBuilder WeakMergeFrom(ICodedInputStream input);
        new IBuilder WeakMergeFrom(ICodedInputStream input, ExtensionRegistry registry);
        new IMessage WeakBuild();
        new IMessage WeakBuildPartial();
        new IBuilder WeakClone();
        new IMessage WeakDefaultInstanceForType { get; }

        #endregion
    }

    /// <summary>
    /// Interface implemented by Protocol Message builders.
    /// TODO(jonskeet): Consider "SetXXX" methods returning the builder, as well as the properties.
    /// </summary>
    /// <typeparam name="TMessage">Type of message</typeparam>
    /// <typeparam name="TBuilder">Type of builder</typeparam>
    public interface IBuilder<TMessage, TBuilder> : IBuilder, IBuilderLite<TMessage, TBuilder>
        where TMessage : IMessage<TMessage, TBuilder>
        where TBuilder : IBuilder<TMessage, TBuilder>
    {
        TBuilder SetUnknownFields(UnknownFieldSet unknownFields);

        /// <summary>
        /// Resets all fields to their default values.
        /// </summary>
        new TBuilder Clear();

        /// <summary>
        /// Merge the specified other message which may be a different implementation of
        /// the same message descriptor.
        /// </summary>
        TBuilder MergeFrom(IMessage other);

        /// <summary>
        /// Constructs the final message. Once this is called, this Builder instance
        /// is no longer valid, and calling any other method may throw a
        /// NullReferenceException. If you need to continue working with the builder
        /// after calling Build, call Clone first.
        /// </summary>
        /// <exception cref="UninitializedMessageException">the message
        /// is missing one or more required fields; use BuildPartial to bypass
        /// this check</exception>
        new TMessage Build();

        /// <summary>
        /// Like Build(), but does not throw an exception if the message is missing
        /// required fields. Instead, a partial message is returned.
        /// </summary>
        new TMessage BuildPartial();

        /// <summary>
        /// Clones this builder.
        /// TODO(jonskeet): Explain depth of clone.
        /// </summary>
        new TBuilder Clone();

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
        /// <item>Call IsInitialized to verify to verify that all required fields are
        /// set before building.</item>
        /// <item>Parse  the message separately using one of the static ParseFrom
        /// methods, then use MergeFrom(IMessage&lt;T&gt;) to merge it with
        /// this one. ParseFrom will throw an InvalidProtocolBufferException
        /// (an IOException) if some required fields are missing.
        /// Use BuildPartial to build, which ignores missing required fields.
        /// </list>
        /// </remarks>
        new TBuilder MergeFrom(ICodedInputStream input);

        /// <summary>
        /// Like MergeFrom(ICodedInputStream), but also parses extensions.
        /// The extensions that you want to be able to parse must be registered
        /// in <paramref name="extensionRegistry"/>. Extensions not in the registry
        /// will be treated as unknown fields.
        /// </summary>
        new TBuilder MergeFrom(ICodedInputStream input, ExtensionRegistry extensionRegistry);

        /// <summary>
        /// Get's the message's type's default instance.
        /// <see cref="IMessage{TMessage}.DefaultInstanceForType" />
        /// </summary>
        new TMessage DefaultInstanceForType { get; }

        /// <summary>
        /// Clears the field. This is exactly equivalent to calling the generated
        /// Clear method corresponding to the field.
        /// </summary>
        /// <param name="field"></param>
        /// <returns></returns>
        TBuilder ClearField(FieldDescriptor field);

        /// <summary>
        /// Appends the given value as a new element for the specified repeated field.
        /// </summary>
        /// <exception cref="ArgumentException">the field is not a repeated field,
        /// the field does not belong to this builder's type, or the value is
        /// of the incorrect type
        /// </exception>
        TBuilder AddRepeatedField(FieldDescriptor field, object value);

        /// <summary>
        /// Merge some unknown fields into the set for this message.
        /// </summary>
        TBuilder MergeUnknownFields(UnknownFieldSet unknownFields);

        /// <summary>
        /// Like MergeFrom(Stream), but does not read until the end of the file.
        /// Instead, the size of the message (encoded as a varint) is read first,
        /// then the message data. Use Message.WriteDelimitedTo(Stream) to
        /// write messages in this format.
        /// </summary>
        /// <param name="input"></param>
        new TBuilder MergeDelimitedFrom(Stream input);

        /// <summary>
        /// Like MergeDelimitedFrom(Stream) but supporting extensions.
        /// </summary>
        new TBuilder MergeDelimitedFrom(Stream input, ExtensionRegistry extensionRegistry);

        #region Convenience methods

        /// <summary>
        /// Parse <paramref name="data"/> as a message of this type and merge
        /// it with the message being built. This is just a small wrapper around
        /// MergeFrom(ICodedInputStream).
        /// </summary>
        new TBuilder MergeFrom(ByteString data);

        /// <summary>
        /// Parse <paramref name="data"/> as a message of this type and merge
        /// it with the message being built. This is just a small wrapper around
        /// MergeFrom(ICodedInputStream, extensionRegistry).
        /// </summary>
        new TBuilder MergeFrom(ByteString data, ExtensionRegistry extensionRegistry);

        /// <summary>
        /// Parse <paramref name="data"/> as a message of this type and merge
        /// it with the message being built. This is just a small wrapper around
        /// MergeFrom(ICodedInputStream).
        /// </summary>
        new TBuilder MergeFrom(byte[] data);

        /// <summary>
        /// Parse <paramref name="data"/> as a message of this type and merge
        /// it with the message being built. This is just a small wrapper around
        /// MergeFrom(ICodedInputStream, extensionRegistry).
        /// </summary>
        new TBuilder MergeFrom(byte[] data, ExtensionRegistry extensionRegistry);

        /// <summary>
        /// Parse <paramref name="input"/> as a message of this type and merge
        /// it with the message being built. This is just a small wrapper around
        /// MergeFrom(ICodedInputStream). Note that this method always reads
        /// the entire input (unless it throws an exception). If you want it to
        /// stop earlier, you will need to wrap the input in a wrapper
        /// stream which limits reading. Or, use IMessage.WriteDelimitedTo(Stream)
        /// to write your message and MmergeDelimitedFrom(Stream) to read it.
        /// Despite usually reading the entire stream, this method never closes the stream. 
        /// </summary>
        new TBuilder MergeFrom(Stream input);

        /// <summary>
        /// Parse <paramref name="input"/> as a message of this type and merge
        /// it with the message being built. This is just a small wrapper around
        /// MergeFrom(ICodedInputStream, extensionRegistry).
        /// </summary>
        new TBuilder MergeFrom(Stream input, ExtensionRegistry extensionRegistry);

        #endregion
    }
}