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
using System.Text;
using Xunit;

namespace Google.ProtocolBuffers
{
    public class ByteStringTest
    {
        [Fact]
        public void EmptyByteStringHasZeroSize()
        {
            Assert.Equal(0, ByteString.Empty.Length);
        }

        [Fact]
        public void CopyFromStringWithExplicitEncoding()
        {
            ByteString bs = ByteString.CopyFrom("AB", Encoding.Unicode);
            Assert.Equal(4, bs.Length);
            Assert.Equal(65, bs[0]);
            Assert.Equal(0, bs[1]);
            Assert.Equal(66, bs[2]);
            Assert.Equal(0, bs[3]);
        }

        [Fact]
        public void IsEmptyWhenEmpty()
        {
            Assert.True(ByteString.CopyFromUtf8("").IsEmpty);
        }

        [Fact]
        public void IsEmptyWhenNotEmpty()
        {
            Assert.False(ByteString.CopyFromUtf8("X").IsEmpty);
        }

        [Fact]
        public void CopyFromByteArrayCopiesContents()
        {
            byte[] data = new byte[1];
            data[0] = 10;
            ByteString bs = ByteString.CopyFrom(data);
            Assert.Equal(10, bs[0]);
            data[0] = 5;
            Assert.Equal(10, bs[0]);
        }

        [Fact]
        public void ToByteArrayCopiesContents()
        {
            ByteString bs = ByteString.CopyFromUtf8("Hello");
            byte[] data = bs.ToByteArray();
            Assert.Equal((byte)'H', data[0]);
            Assert.Equal((byte)'H', bs[0]);
            data[0] = 0;
            Assert.Equal(0, data[0]);
            Assert.Equal((byte)'H', bs[0]);
        }

        [Fact]
        public void CopyFromUtf8UsesUtf8()
        {
            ByteString bs = ByteString.CopyFromUtf8("\u20ac");
            Assert.Equal(3, bs.Length);
            Assert.Equal(0xe2, bs[0]);
            Assert.Equal(0x82, bs[1]);
            Assert.Equal(0xac, bs[2]);
        }

        [Fact]
        public void CopyFromPortion()
        {
            byte[] data = new byte[] {0, 1, 2, 3, 4, 5, 6};
            ByteString bs = ByteString.CopyFrom(data, 2, 3);
            Assert.Equal(3, bs.Length);
            Assert.Equal(2, bs[0]);
            Assert.Equal(3, bs[1]);
        }

        [Fact]
        public void ToStringUtf8()
        {
            ByteString bs = ByteString.CopyFromUtf8("\u20ac");
            Assert.Equal("\u20ac", bs.ToStringUtf8());
        }

        [Fact]
        public void ToStringWithExplicitEncoding()
        {
            ByteString bs = ByteString.CopyFrom("\u20ac", Encoding.Unicode);
            Assert.Equal("\u20ac", bs.ToString(Encoding.Unicode));
        }

        [Fact]
        public void FromBase64_WithText()
        {
            byte[] data = new byte[] {0, 1, 2, 3, 4, 5, 6};
            string base64 = Convert.ToBase64String(data);
            ByteString bs = ByteString.FromBase64(base64);
            Assert.Equal(data, bs.ToByteArray());
        }

        [Fact]
        public void FromBase64_Empty()
        {
            // Optimization which also fixes issue 61.
            Assert.Same(ByteString.Empty, ByteString.FromBase64(""));
        }
    }
}