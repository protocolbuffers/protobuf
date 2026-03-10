#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using Google.Protobuf.Reflection;

namespace Google.Protobuf
{
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

        /// <summary>
        /// Descriptor for this message. All instances are expected to return the same descriptor,
        /// and for generated types this will be an explicitly-implemented member, returning the
        /// same value as the static property declared on the type.
        /// </summary>
        MessageDescriptor Descriptor { get; }
    }

    /// <summary>
    /// Generic interface for a Protocol Buffers message,
    /// where the type parameter is expected to be the same type as
    /// the implementation class.
    /// </summary>
    /// <typeparam name="T">The message type.</typeparam>
    public interface IMessage<T> : IMessage, IEquatable<T>, IDeepCloneable<T> where T : IMessage<T>
    {
        /// <summary>
        /// Merges the given message into this one.
        /// </summary>
        /// <remarks>See the user guide for precise merge semantics.</remarks>
        /// <param name="message">The message to merge with this one. Must not be null.</param>
        void MergeFrom(T message);
    }
}
