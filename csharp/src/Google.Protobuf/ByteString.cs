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
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
#if !NET35
using System.Threading;
using System.Threading.Tasks;
#endif
#if NET35
using Google.Protobuf.Compatibility;
#endif

namespace Google.Protobuf
{
    /// <summary>
    /// Immutable array of bytes.
    /// </summary>
    public sealed class ByteString : IEnumerable<byte>, IEquatable<ByteString>
    {
        private static readonly ByteString empty = new ByteString(new byte[0]);

        private readonly byte[] bytes;

        /// <summary>
        /// Unsafe operations that can cause IO Failure and/or other catastrophic side-effects.
        /// </summary>
        internal static class Unsafe
        {
            /// <summary>
            /// Constructs a new ByteString from the given byte array. The array is
            /// *not* copied, and must not be modified after this constructor is called.
            /// </summary>
            internal static ByteString FromBytes(byte[] bytes)
            {
                return new ByteString(bytes);
            }
        }

        /// <summary>
        /// Internal use only.  Ensure that the provided array is not mutated and belongs to this instance.
        /// </summary>
        internal static ByteString AttachBytes(byte[] bytes)
        {
            return new ByteString(bytes);
        }

        /// <summary>
        /// Constructs a new ByteString from the given byte array. The array is
        /// *not* copied, and must not be modified after this constructor is called.
        /// </summary>
        private ByteString(byte[] bytes)
        {
            this.bytes = bytes;
        }

        /// <summary>
        /// Returns an empty ByteString.
        /// </summary>
        public static ByteString Empty
        {
            get { return empty; }
        }

        /// <summary>
        /// Returns the length of this ByteString in bytes.
        /// </summary>
        public int Length
        {
            get { return bytes.Length; }
        }

        /// <summary>
        /// Returns <c>true</c> if this byte string is empty, <c>false</c> otherwise.
        /// </summary>
        public bool IsEmpty
        {
            get { return Length == 0; }
        }

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
        /// <summary>
        /// Provides read-only access to the data of this <see cref="ByteString"/>.
        /// No data is copied so this is the most efficient way of accessing.
        /// </summary>
        public ReadOnlySpan<byte> Span => new ReadOnlySpan<byte>(bytes);
#endif

        /// <summary>
        /// Converts this <see cref="ByteString"/> into a byte array.
        /// </summary>
        /// <remarks>The data is copied - changes to the returned array will not be reflected in this <c>ByteString</c>.</remarks>
        /// <returns>A byte array with the same data as this <c>ByteString</c>.</returns>
        public byte[] ToByteArray()
        {
            return (byte[]) bytes.Clone();
        }

        /// <summary>
        /// Converts this <see cref="ByteString"/> into a standard base64 representation.
        /// </summary>
        /// <returns>A base64 representation of this <c>ByteString</c>.</returns>
        public string ToBase64()
        {
            return Convert.ToBase64String(bytes);
        }

        /// <summary>
        /// Constructs a <see cref="ByteString" /> from the Base64 Encoded String.
        /// </summary>
        public static ByteString FromBase64(string bytes)
        {
            // By handling the empty string explicitly, we not only optimize but we fix a
            // problem on CF 2.0. See issue 61 for details.
            return bytes == "" ? Empty : new ByteString(Convert.FromBase64String(bytes));
        }

        /// <summary>
        /// Constructs a <see cref="ByteString"/> from data in the given stream, synchronously.
        /// </summary>
        /// <remarks>If successful, <paramref name="stream"/> will be read completely, from the position
        /// at the start of the call.</remarks>
        /// <param name="stream">The stream to copy into a ByteString.</param>
        /// <returns>A ByteString with content read from the given stream.</returns>
        public static ByteString FromStream(Stream stream)
        {
            ProtoPreconditions.CheckNotNull(stream, nameof(stream));
            int capacity = stream.CanSeek ? checked((int) (stream.Length - stream.Position)) : 0;
            var memoryStream = new MemoryStream(capacity);
            stream.CopyTo(memoryStream);
#if NETSTANDARD1_0 || NETSTANDARD2_0
            byte[] bytes = memoryStream.ToArray();
#else
            // Avoid an extra copy if we can.
            byte[] bytes = memoryStream.Length == memoryStream.Capacity ? memoryStream.GetBuffer() : memoryStream.ToArray();
#endif
            return AttachBytes(bytes);
        }

#if !NET35
        /// <summary>
        /// Constructs a <see cref="ByteString"/> from data in the given stream, asynchronously.
        /// </summary>
        /// <remarks>If successful, <paramref name="stream"/> will be read completely, from the position
        /// at the start of the call.</remarks>
        /// <param name="stream">The stream to copy into a ByteString.</param>
        /// <param name="cancellationToken">The cancellation token to use when reading from the stream, if any.</param>
        /// <returns>A ByteString with content read from the given stream.</returns>
        public async static Task<ByteString> FromStreamAsync(Stream stream, CancellationToken cancellationToken = default(CancellationToken))
        {
            ProtoPreconditions.CheckNotNull(stream, nameof(stream));
            int capacity = stream.CanSeek ? checked((int) (stream.Length - stream.Position)) : 0;
            var memoryStream = new MemoryStream(capacity);
            // We have to specify the buffer size here, as there's no overload accepting the cancellation token
            // alone. But it's documented to use 81920 by default if not specified.
            await stream.CopyToAsync(memoryStream, 81920, cancellationToken);
#if NETSTANDARD1_0 || NETSTANDARD2_0
            byte[] bytes = memoryStream.ToArray();
#else
            // Avoid an extra copy if we can.
            byte[] bytes = memoryStream.Length == memoryStream.Capacity ? memoryStream.GetBuffer() : memoryStream.ToArray();
#endif
            return AttachBytes(bytes);
        }
#endif

        /// <summary>
        /// Constructs a <see cref="ByteString" /> from the given array. The contents
        /// are copied, so further modifications to the array will not
        /// be reflected in the returned ByteString.
        /// This method can also be invoked in <c>ByteString.CopyFrom(0xaa, 0xbb, ...)</c> form
        /// which is primarily useful for testing.
        /// </summary>
        public static ByteString CopyFrom(params byte[] bytes)
        {
            return new ByteString((byte[]) bytes.Clone());
        }

        /// <summary>
        /// Constructs a <see cref="ByteString" /> from a portion of a byte array.
        /// </summary>
        public static ByteString CopyFrom(byte[] bytes, int offset, int count)
        {
            byte[] portion = new byte[count];
            ByteArray.Copy(bytes, offset, portion, 0, count);
            return new ByteString(portion);
        }

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
        /// <summary>
        /// Constructs a <see cref="ByteString" /> from a read only span. The contents
        /// are copied, so further modifications to the span will not
        /// be reflected in the returned <see cref="ByteString" />.
        /// </summary>
        public static ByteString CopyFrom(ReadOnlySpan<byte> bytes)
        {
            return new ByteString(bytes.ToArray());
        }
#endif

        /// <summary>
        /// Creates a new <see cref="ByteString" /> by encoding the specified text with
        /// the given encoding.
        /// </summary>
        public static ByteString CopyFrom(string text, Encoding encoding)
        {
            return new ByteString(encoding.GetBytes(text));
        }

        /// <summary>
        /// Creates a new <see cref="ByteString" /> by encoding the specified text in UTF-8.
        /// </summary>
        public static ByteString CopyFromUtf8(string text)
        {
            return CopyFrom(text, Encoding.UTF8);
        }

        /// <summary>
        /// Retuns the byte at the given index.
        /// </summary>
        public byte this[int index]
        {
            get { return bytes[index]; }
        }

        /// <summary>
        /// Converts this <see cref="ByteString"/> into a string by applying the given encoding.
        /// </summary>
        /// <remarks>
        /// This method should only be used to convert binary data which was the result of encoding
        /// text with the given encoding.
        /// </remarks>
        /// <param name="encoding">The encoding to use to decode the binary data into text.</param>
        /// <returns>The result of decoding the binary data with the given decoding.</returns>
        public string ToString(Encoding encoding)
        {
            return encoding.GetString(bytes, 0, bytes.Length);
        }

        /// <summary>
        /// Converts this <see cref="ByteString"/> into a string by applying the UTF-8 encoding.
        /// </summary>
        /// <remarks>
        /// This method should only be used to convert binary data which was the result of encoding
        /// text with UTF-8.
        /// </remarks>
        /// <returns>The result of decoding the binary data with the given decoding.</returns>
        public string ToStringUtf8()
        {
            return ToString(Encoding.UTF8);
        }

        /// <summary>
        /// Returns an iterator over the bytes in this <see cref="ByteString"/>.
        /// </summary>
        /// <returns>An iterator over the bytes in this object.</returns>
        public IEnumerator<byte> GetEnumerator()
        {
            return ((IEnumerable<byte>) bytes).GetEnumerator();
        }

        /// <summary>
        /// Returns an iterator over the bytes in this <see cref="ByteString"/>.
        /// </summary>
        /// <returns>An iterator over the bytes in this object.</returns>
        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        /// <summary>
        /// Creates a CodedInputStream from this ByteString's data.
        /// </summary>
        public CodedInputStream CreateCodedInput()
        {
            // We trust CodedInputStream not to reveal the provided byte array or modify it
            return new CodedInputStream(bytes);
        }

        /// <summary>
        /// Compares two byte strings for equality.
        /// </summary>
        /// <param name="lhs">The first byte string to compare.</param>
        /// <param name="rhs">The second byte string to compare.</param>
        /// <returns><c>true</c> if the byte strings are equal; false otherwise.</returns>
        public static bool operator ==(ByteString lhs, ByteString rhs)
        {
            if (ReferenceEquals(lhs, rhs))
            {
                return true;
            }
            if (ReferenceEquals(lhs, null) || ReferenceEquals(rhs, null))
            {
                return false;
            }
            if (lhs.bytes.Length != rhs.bytes.Length)
            {
                return false;
            }
            for (int i = 0; i < lhs.Length; i++)
            {
                if (rhs.bytes[i] != lhs.bytes[i])
                {
                    return false;
                }
            }
            return true;
        }

        /// <summary>
        /// Compares two byte strings for inequality.
        /// </summary>
        /// <param name="lhs">The first byte string to compare.</param>
        /// <param name="rhs">The second byte string to compare.</param>
        /// <returns><c>false</c> if the byte strings are equal; true otherwise.</returns>
        public static bool operator !=(ByteString lhs, ByteString rhs)
        {
            return !(lhs == rhs);
        }

        /// <summary>
        /// Compares this byte string with another object.
        /// </summary>
        /// <param name="obj">The object to compare this with.</param>
        /// <returns><c>true</c> if <paramref name="obj"/> refers to an equal <see cref="ByteString"/>; <c>false</c> otherwise.</returns>
        public override bool Equals(object obj)
        {
            return this == (obj as ByteString);
        }

        /// <summary>
        /// Returns a hash code for this object. Two equal byte strings
        /// will return the same hash code.
        /// </summary>
        /// <returns>A hash code for this object.</returns>
        public override int GetHashCode()
        {
            int ret = 23;
            foreach (byte b in bytes)
            {
                ret = (ret * 31) + b;
            }
            return ret;
        }

        /// <summary>
        /// Compares this byte string with another.
        /// </summary>
        /// <param name="other">The <see cref="ByteString"/> to compare this with.</param>
        /// <returns><c>true</c> if <paramref name="other"/> refers to an equal byte string; <c>false</c> otherwise.</returns>
        public bool Equals(ByteString other)
        {
            return this == other;
        }

        /// <summary>
        /// Used internally by CodedOutputStream to avoid creating a copy for the write
        /// </summary>
        internal void WriteRawBytesTo(CodedOutputStream outputStream)
        {
            outputStream.WriteRawBytes(bytes, 0, bytes.Length);
        }

        /// <summary>
        /// Copies the entire byte array to the destination array provided at the offset specified.
        /// </summary>
        public void CopyTo(byte[] array, int position)
        {
            ByteArray.Copy(bytes, 0, array, position, bytes.Length);
        }

        /// <summary>
        /// Writes the entire byte array to the provided stream
        /// </summary>
        public void WriteTo(Stream outputStream)
        {
            outputStream.Write(bytes, 0, bytes.Length);
        }
    }
}