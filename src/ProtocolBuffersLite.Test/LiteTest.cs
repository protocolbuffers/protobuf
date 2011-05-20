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
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Miscellaneous tests for message operations that apply to both
  /// generated and dynamic messages.
  /// </summary>
  [TestFixture]
  public class LiteTest {
    [Test]
    public void TestLite() {
      // Since lite messages are a subset of regular messages, we can mostly
      // assume that the functionality of lite messages is already thoroughly
      // tested by the regular tests.  All this test really verifies is that
      // a proto with optimize_for = LITE_RUNTIME compiles correctly when
      // linked only against the lite library.  That is all tested at compile
      // time, leaving not much to do in this method.  Let's just do some random
      // stuff to make sure the lite message is actually here and usable.

      TestAllTypesLite message =
        TestAllTypesLite.CreateBuilder()
                        .SetOptionalInt32(123)
                        .AddRepeatedString("hello")
                        .SetOptionalNestedMessage(
                            TestAllTypesLite.Types.NestedMessage.CreateBuilder().SetBb(7))
                        .Build();

      ByteString data = message.ToByteString();

      TestAllTypesLite message2 = TestAllTypesLite.ParseFrom(data);

      Assert.AreEqual(123, message2.OptionalInt32);
      Assert.AreEqual(1, message2.RepeatedStringCount);
      Assert.AreEqual("hello", message2.RepeatedStringList[0]);
      Assert.AreEqual(7, message2.OptionalNestedMessage.Bb);
    }

    [Test]
    public void TestLiteExtensions() {
      // TODO(kenton):  Unlike other features of the lite library, extensions are
      //   implemented completely differently from the regular library.  We
      //   should probably test them more thoroughly.

      TestAllExtensionsLite message =
        TestAllExtensionsLite.CreateBuilder()
          .SetExtension(UnitTestLiteProtoFile.OptionalInt32ExtensionLite, 123)
          .AddExtension(UnitTestLiteProtoFile.RepeatedStringExtensionLite, "hello")
          .SetExtension(UnitTestLiteProtoFile.OptionalNestedEnumExtensionLite,
              TestAllTypesLite.Types.NestedEnum.BAZ)
          .SetExtension(UnitTestLiteProtoFile.OptionalNestedMessageExtensionLite,
              TestAllTypesLite.Types.NestedMessage.CreateBuilder().SetBb(7).Build())
          .Build();

      // Test copying a message, since coping extensions actually does use a
      // different code path between lite and regular libraries, and as of this
      // writing, parsing hasn't been implemented yet.
      TestAllExtensionsLite message2 = message.ToBuilder().Build();

      Assert.AreEqual(123, (int)message2.GetExtension(
          UnitTestLiteProtoFile.OptionalInt32ExtensionLite));
      Assert.AreEqual(1, message2.GetExtensionCount(
          UnitTestLiteProtoFile.RepeatedStringExtensionLite));
      Assert.AreEqual(1, message2.GetExtension(
          UnitTestLiteProtoFile.RepeatedStringExtensionLite).Count);
      Assert.AreEqual("hello", message2.GetExtension(
          UnitTestLiteProtoFile.RepeatedStringExtensionLite, 0));
      Assert.AreEqual(TestAllTypesLite.Types.NestedEnum.BAZ, message2.GetExtension(
          UnitTestLiteProtoFile.OptionalNestedEnumExtensionLite));
      Assert.AreEqual(7, message2.GetExtension(
          UnitTestLiteProtoFile.OptionalNestedMessageExtensionLite).Bb);
    }
  }
}