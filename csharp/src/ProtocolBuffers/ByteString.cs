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
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Immutable array of bytes.
    /// TODO(jonskeet): Implement the common collection interfaces?
    /// </summary>
    public sealed class ByteString : IEnumerable<byte>, IEquatable<ByteString>
    {
        private static readonly ByteString empty = new ByteString(new byte[0]);

        private readonly byte[] bytes;

        /// <summary>
        /// Unsafe operations that can cause IO Failure and/or other catestrophic side-effects.
        /// </summary>
        public static class Unsafe
        {
            /// <summary>
            /// Constructs a new ByteString from the given byte array. The array is
            /// *not* copied, and must not be modified after this constructor is called.
            /// </summary>
            public static ByteString FromBytes(byte[] bytes)
            {
                return new ByteString(bytes);
            }

            /// <summary>
            /// Provides direct, unrestricted access to the bytes contained in this instance.
            /// You must not modify or resize the byte array returned by this method.
            /// </summary>
            public static byte[] GetBuffer(ByteString bytes)
            {
                return bytes.bytes;
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

        public bool IsEmpty
        {
            get { return Length == 0; }
        }

        public byte[] ToByteArray()
        {
            return (byte[]) bytes.Clone();
        }

        public string ToBase64()
        {
            return Convert.ToBase64String(bytes);
        }

        /// <summary>
        /// Constructs a ByteString from the Base64 Encoded String.
        /// </summary>
        public static ByteString FromBase64(string bytes)
        {
            // By handling the empty string explicitly, we not only optimize but we fix a
            // problem on CF 2.0. See issue 61 for details.
            return bytes == "" ? Empty : new ByteString(Convert.FromBase64String(bytes));
        }

        /// <summary>
        /// Constructs a ByteString from the given array. The contents
        /// are copied, so further modifications to the array will not
        /// be reflected in the returned ByteString.
        /// </summary>
        public static ByteString CopyFrom(byte[] bytes)
        {
            return new ByteString((byte[]) bytes.Clone());
        }

        /// <summary>
        /// Constructs a ByteString from a portion of a byte array.
        /// </summary>
        public static ByteString CopyFrom(byte[] bytes, int offset, int count)
        {
            byte[] portion = new byte[count];
            ByteArray.Copy(bytes, offset, portion, 0, count);
            return new ByteString(portion);
        }

        /// <summary>
        /// Creates a new ByteString by encoding the specified text with
        /// the given encoding.
        /// </summary>
        public static ByteString CopyFrom(string text, Encoding encoding)
        {
            return new ByteString(encoding.GetBytes(text));
        }

        /// <summary>
        /// Creates a new ByteString by encoding the specified text in UTF-8.
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

        public string ToString(Encoding encoding)
        {
            return encoding.GetString(bytes, 0, bytes.Length);
        }

        public string ToStringUtf8()
        {
            return ToString(Encoding.UTF8);
        }

        public IEnumerator<byte> GetEnumerator()
        {
            return ((IEnumerable<byte>) bytes).GetEnumerator();
        }

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
            return CodedInputStream.CreateInstance(bytes);
        }

        // TODO(jonskeet): CopyTo if it turns out to be required

        public override bool Equals(object obj)
        {
            ByteString other = obj as ByteString;
            if (obj == null)
            {
                return false;
            }
            return Equals(other);
        }

        public override int GetHashCode()
        {
            int ret = 23;
            foreach (byte b in bytes)
            {
                ret = (ret << 8) | b;
            }
            return ret;
        }

        public bool Equals(ByteString other)
        {
            if (other.bytes.Length != bytes.Length)
            {
                return false;
            }
            for (int i = 0; i < bytes.Length; i++)
            {
                if (other.bytes[i] != bytes[i])
                {
                    return false;
                }
            }
            return true;
        }

        /// <summary>
        /// Builder for ByteStrings which allows them to be created without extra
        /// copying being involved. This has to be a nested type in order to have access
        /// to the private ByteString constructor.
        /// </summary>
        internal sealed class CodedBuilder
        {
            private readonly CodedOutputStream output;
            private readonly byte[] buffer;

            internal CodedBuilder(int size)
            {
                buffer = new byte[size];
                output = CodedOutputStream.CreateInstance(buffer);
            }

            internal ByteString Build()
            {
                output.CheckNoSpaceLeft();

                // We can be confident that the CodedOutputStream will not modify the
                // underlying bytes anymore because it already wrote all of them.  So,
                // no need to make a copy.
                return new ByteString(buffer);
            }

            internal CodedOutputStream CodedOutput
            {
                get { return output; }
            }
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