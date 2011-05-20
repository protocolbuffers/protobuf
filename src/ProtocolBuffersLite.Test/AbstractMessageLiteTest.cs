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
using Google.ProtocolBuffers;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class AbstractMessageLiteTest {

    [Test]
    public void TestMessageLiteToByteString() {
      TestRequiredLite msg = TestRequiredLite.CreateBuilder()
        .SetD(42)
        .SetEn(ExtraEnum.EXLITE_BAZ)
        .Build();
      
      ByteString b = msg.ToByteString();
      Assert.AreEqual(4, b.Length);
      Assert.AreEqual(TestRequiredLite.DFieldNumber << 3, b[0]);
      Assert.AreEqual(42, b[1]);
      Assert.AreEqual(TestRequiredLite.EnFieldNumber << 3, b[2]);
      Assert.AreEqual((int)ExtraEnum.EXLITE_BAZ, b[3]);
    }

    [Test]
    public void TestMessageLiteToByteArray() {
      TestRequiredLite msg = TestRequiredLite.CreateBuilder()
        .SetD(42)
        .SetEn(ExtraEnum.EXLITE_BAZ)
        .Build();

      ByteString b = msg.ToByteString();
      ByteString copy = ByteString.CopyFrom(msg.ToByteArray());
      Assert.AreEqual(b, copy);
    }

    [Test]
    public void TestMessageLiteWriteTo() {
      TestRequiredLite msg = TestRequiredLite.CreateBuilder()
        .SetD(42)
        .SetEn(ExtraEnum.EXLITE_BAZ)
        .Build();

      MemoryStream ms = new MemoryStream();
      msg.WriteTo(ms);
      Assert.AreEqual(msg.ToByteArray(), ms.ToArray());
    }

    [Test]
    public void TestMessageLiteWriteDelimitedTo() {
      TestRequiredLite msg = TestRequiredLite.CreateBuilder()
        .SetD(42)
        .SetEn(ExtraEnum.EXLITE_BAZ)
        .Build();

      MemoryStream ms = new MemoryStream();
      msg.WriteDelimitedTo(ms);
      byte[] buffer = ms.ToArray();

      Assert.AreEqual(5, buffer.Length);
      Assert.AreEqual(4, buffer[0]);
      byte[] msgBytes = new byte[4];
      Array.Copy(buffer, 1, msgBytes, 0, 4);
      Assert.AreEqual(msg.ToByteArray(), msgBytes);
    }

    [Test]
    public void TestIMessageLiteWeakCreateBuilderForType() {
      IMessageLite msg = TestRequiredLite.DefaultInstance;
      Assert.AreEqual(typeof(TestRequiredLite.Builder), msg.WeakCreateBuilderForType().GetType());
    }

    [Test]
    public void TestMessageLiteWeakToBuilder() {
      IMessageLite msg = TestRequiredLite.CreateBuilder()
        .SetD(42)
        .SetEn(ExtraEnum.EXLITE_BAZ)
        .Build();

      IMessageLite copy = msg.WeakToBuilder().WeakBuild();
      Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
    }

    [Test]
    public void TestMessageLiteWeakDefaultInstanceForType() {
      IMessageLite msg = TestRequiredLite.DefaultInstance;
      Assert.IsTrue(Object.ReferenceEquals(TestRequiredLite.DefaultInstance, msg.WeakDefaultInstanceForType));
    }
  }
}
