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
using System.Runtime.InteropServices;
using System.Security;
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
    [SecuritySafeCritical]
    public sealed class ByteString : IEnumerable<byte>, IEquatable<ByteString>
    {
        private static readonly ByteString empty = new ByteString(new byte[0]);

        private readonly ReadOnlyMemory<byte> bytes;

        /// <summary>
        /// Internal use only. Ensure that the provided memory is not mutated and belongs to this instance.
        /// </summary>
        internal static ByteString AttachBytes(ReadOnlyMemory<byte> bytes)
        {
            return new ByteString(bytes);
        }

        /// <summary>
        /// Internal use only. Ensure that the provided memory is not mutated and belongs to this instance.
        /// This method encapsulates converting array to memory. Reduces need for SecuritySafeCritical
        /// in .NET Framework.
        /// </summary>
        internal static ByteString AttachBytes(byte[] bytes)
        {
            return AttachBytes(bytes.AsMemory());
        }

        /// <summary>
        /// Constructs a new ByteString from the given memory. The memory is
        /// *not* copied, and must not be modified after this constructor is called.
        /// </summary>
        private ByteString(ReadOnlyMemory<byte> bytes)
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

        /// <summary>
        /// Provides read-only access to the data of this <see cref="ByteString"/>.
        /// No data is copied so this is the most efficient way of accessing.
        /// </summary>
        public ReadOnlySpan<byte> Span
        {
            get { return bytes.Span; }
        }

        /// <summary>
        /// Provides read-only access to the data of this <see cref="ByteString"/>.
        /// No data is copied so this is the most efficient way of accessing.
        /// </summary>
        public ReadOnlyMemory<byte> Memory
        {
            get { return bytes; }
        }

        /// <summary>
        /// Converts this <see cref="ByteString"/> into a byte array.
        /// </summary>
        /// <remarks>The data is copied - changes to the returned array will not be reflected in this <c>ByteString</c>.</remarks>
        /// <returns>A byte array with the same data as this <c>ByteString</c>.</returns>
        public byte[] ToByteArray()
        {
            return bytes.ToArray();
        }

        /// <summary>
        /// Converts this <see cref="ByteString"/> into a standard base64 representation.
        /// </summary>
        /// <returns>A base64 representation of this <c>ByteString</c>.</returns>
        public string ToBase64()
        {
            if (MemoryMarshal.TryGetArray(bytes, out ArraySegment<byte> segment))
            {
                // Fast path. ByteString was created with an array, so pass the underlying array.
                return Convert.ToBase64String(segment.Array, segment.Offset, segment.Count);
            }
            else
            {
                // Slow path. BytesString is not an array. Convert memory and pass result to ToBase64String.
                return Convert.ToBase64String(bytes.ToArray());
            }
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
#if NETSTANDARD1_1 || NETSTANDARD2_0
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
        public static Task<ByteString> FromStreamAsync(Stream stream, CancellationToken cancellationToken = default(CancellationToken))
        {
            ProtoPreconditions.CheckNotNull(stream, nameof(stream));
            return ByteStringAsync.FromStreamAsyncCore(stream, cancellationToken);
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

        /// <summary>
        /// Constructs a <see cref="ByteString" /> from a read only span. The contents
        /// are copied, so further modifications to the span will not
        /// be reflected in the returned <see cref="ByteString" />.
        /// </summary>
        public static ByteString CopyFrom(ReadOnlySpan<byte> bytes)
        {
            return new ByteString(bytes.ToArray());
        }

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
        /// Returns the byte at the given index.
        /// </summary>
        public byte this[int index]
        {
            get { return bytes.Span[index]; }
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
            if (MemoryMarshal.TryGetArray(bytes, out ArraySegment<byte> segment))
            {
                // Fast path. ByteString was created with an array.
                return encoding.GetString(segment.Array, segment.Offset, segment.Count);
            }
            else
            {
                // Slow path. BytesString is not an array. Convert memory and pass result to GetString.
                // TODO: Consider using GetString overload that takes a pointer.
                byte[] array = bytes.ToArray();
                return encoding.GetString(array, 0, array.Length);
            }
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
        [SecuritySafeCritical]
        public IEnumerator<byte> GetEnumerator()
        {
            return MemoryMarshal.ToEnumerable(bytes).GetEnumerator();
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
            if (MemoryMarshal.TryGetArray(bytes, out ArraySegment<byte> segment) && segment.Count == bytes.Length)
            {
                // Fast path. ByteString was created with a complete array.
                return new CodedInputStream(segment.Array);
            }
            else
            {
                // Slow path. BytesString is not an array, or is a slice of an array.
                // Convert memory and pass result to WriteRawBytes.
                return new CodedInputStream(bytes.ToArray());
            }
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

            return lhs.bytes.Span.SequenceEqual(rhs.bytes.Span);
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
        [SecuritySafeCritical]
        public override bool Equals(object obj)
        {
            return this == (obj as ByteString);
        }

        /// <summary>
        /// Returns a hash code for this object. Two equal byte strings
        /// will return the same hash code.
        /// </summary>
        /// <returns>A hash code for this object.</returns>
        [SecuritySafeCritical]
        public override int GetHashCode()
        {
            ReadOnlySpan<byte> b = bytes.Span;

            int ret = 23;
            for (int i = 0; i < b.Length; i++)
            {
                ret = (ret * 31) + b[i];
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
        /// Copies the entire byte array to the destination array provided at the offset specified.
        /// </summary>
        public void CopyTo(byte[] array, int position)
        {
            bytes.CopyTo(array.AsMemory(position));
        }

        /// <summary>
        /// Writes the entire byte array to the provided stream
        /// </summary>
        public void WriteTo(Stream outputStream)
        {
            if (MemoryMarshal.TryGetArray(bytes, out ArraySegment<byte> segment))
            {
                // Fast path. ByteString was created with an array, so pass the underlying array.
                outputStream.Write(segment.Array, segment.Offset, segment.Count);
            }
            else
            {
                // Slow path. BytesString is not an array. Convert memory and pass result to WriteRawBytes.
                var array = bytes.ToArray();
                outputStream.Write(array, 0, array.Length);
            }
        }
    }
}