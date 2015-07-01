#region Copyright notice and license
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
#endregion

using System;
using Google.Protobuf.FieldAccess;

namespace Google.Protobuf
{

    // TODO(jonskeet): Do we want a "weak" (non-generic) version of IReflectedMessage?
    // TODO(jonskeet): Split these interfaces into separate files when we're happy with them.

    /// <summary>
    /// Reflection support for a specific message type.
    /// </summary>
    public interface IReflectedMessage
    {
        FieldAccessorTable Fields { get; }
        // TODO(jonskeet): Descriptor? Or a single property which has "all you need for reflection"?
    }

    /// <summary>
    /// Interface for a Protocol Buffers message, supporting
    /// basic operations required for serialization.
    /// </summary>
    public interface IMessage
    {
        /// <summary>
        /// Merges the data from the specified coded input stream with the current message.
        /// </summary>
        /// <remarks>See the user guide for precise merge semantics.</remarks>
        /// <param name="input"></param>
        void MergeFrom(CodedInputStream input);

        /// <summary>
        /// Writes the data to the given coded output stream.
        /// </summary>
        /// <param name="output">Coded output stream to write the data to. Must not be null.</param>
        void WriteTo(CodedOutputStream output);

        /// <summary>
        /// Calculates the size of this message in Protocol Buffer wire format, in bytes.
        /// </summary>
        /// <returns>The number of bytes required to write this message
        /// to a coded output stream.</returns>
        int CalculateSize();
    }

    /// <summary>
    /// Generic interface for a Protocol Buffers message,
    /// where the type parameter is expected to be the same type as
    /// the implementation class.
    /// </summary>
    /// <typeparam name="T">The message type.</typeparam>
    public interface IMessage<T> : IMessage, IEquatable<T>, IDeepCloneable<T>, IFreezable where T : IMessage<T>
    {
        /// <summary>
        /// Merges the given message into this one.
        /// </summary>
        /// <remarks>See the user guide for precise merge semantics.</remarks>
        /// <param name="message">The message to merge with this one. Must not be null.</param>
        void MergeFrom(T message);
    }

    /// <summary>
    /// Generic interface for a deeply cloneable type.
    /// <summary>
    /// <remarks>
    /// All generated messages implement this interface, but so do some non-message types.
    /// Additionally, due to the type constraint on <c>T</c> in <see cref="IMessage{T}"/>,
    /// it is simpler to keep this as a separate interface.
    /// </para>
    /// <para>
    /// Freezable types which implement this interface should always return a mutable clone,
    /// even if the original object is frozen.
    /// </para>
    /// </remarks>
    /// <typeparam name="T">The type itself, returned by the <see cref="Clone"/> method.</typeparam>
    public interface IDeepCloneable<T>
    {
        /// <summary>
        /// Creates a deep clone of this object.
        /// </summary>
        /// <returns>A deep clone of this object.</returns>
        T Clone();
    }

    /// <summary>
    /// Provides a mechanism for freezing a message (or repeated field collection)
    /// to make it immutable.
    /// </summary>
    /// <remarks>
    /// Implementations are under no obligation to make this thread-safe: if a freezable
    /// type instance is shared between threads before being frozen, and one thread then
    /// freezes it, it is possible for other threads to make changes during the freezing
    /// operation and also to observe stale values for mutated fields. Objects should be
    /// frozen before being made available to other threads.
    /// </remarks>
    public interface IFreezable
    {
        /// <summary>
        /// Freezes this object.
        /// </summary>
        /// <remarks>
        /// If the object is already frozen, this method has no effect.
        /// </remarks>
        void Freeze();

        /// <summary>
        /// Returns whether or not this object is frozen (and therefore immutable).
        /// </summary>
        /// <value><c>true</c> if this object is frozen; <c>false</c> otherwise.</value>
        bool IsFrozen { get; }
    }
}
