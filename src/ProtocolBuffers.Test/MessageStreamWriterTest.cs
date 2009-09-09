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

using System.IO;
using NUnit.Framework;
using NestedMessage = Google.ProtocolBuffers.TestProtos.TestAllTypes.Types.NestedMessage;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class MessageStreamWriterTest {

    internal static readonly byte[] ThreeMessageData = new byte[] {
        (1 << 3) | 2, 2, // Field 1, 2 bytes long (first message)
        (1 << 3) | 0, 5, // Field 1, value 5
        (1 << 3) | 2, 3, // Field 1, 3 bytes long (second message)
        (1 << 3) | 0, (1500 & 0x7f) | 0x80, 1500 >> 7, // Field 1, value 1500
        (1 << 3) | 2, 0, // Field 1, no data (third message)
    };

    [Test]
    public void ThreeMessages() {
      NestedMessage message1 = new NestedMessage.Builder { Bb = 5 }.Build();
      NestedMessage message2 = new NestedMessage.Builder { Bb = 1500 }.Build();
      NestedMessage message3 = new NestedMessage.Builder().Build();

      byte[] data;
      using (MemoryStream stream = new MemoryStream()) {
        MessageStreamWriter<NestedMessage> writer = new MessageStreamWriter<NestedMessage>(stream);
        writer.Write(message1);
        writer.Write(message2);
        writer.Write(message3);
        writer.Flush();
        data = stream.ToArray();
      }

      TestUtil.AssertEqualBytes(ThreeMessageData, data);
    }
  }
}
