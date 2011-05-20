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

using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class TestLiteByApi {

    [Test]
    public void TestAllTypesEquality() {
      TestAllTypesLite msg = TestAllTypesLite.DefaultInstance;
      TestAllTypesLite copy = msg.ToBuilder().Build();
      Assert.AreEqual(msg.GetHashCode(), copy.GetHashCode());
      Assert.IsTrue(msg.Equals(copy));
      msg = msg.ToBuilder().SetOptionalString("Hi").Build();
      Assert.AreNotEqual(msg.GetHashCode(), copy.GetHashCode());
      Assert.IsFalse(msg.Equals(copy));
      copy = copy.ToBuilder().SetOptionalString("Hi").Build();
      Assert.AreEqual(msg.GetHashCode(), copy.GetHashCode());
      Assert.IsTrue(msg.Equals(copy));
    }

    [Test]
    public void TestEqualityOnExtensions() {
      TestAllExtensionsLite msg = TestAllExtensionsLite.DefaultInstance;
      TestAllExtensionsLite copy = msg.ToBuilder().Build();
      Assert.AreEqual(msg.GetHashCode(), copy.GetHashCode());
      Assert.IsTrue(msg.Equals(copy));
      msg = msg.ToBuilder().SetExtension(UnitTestLiteProtoFile.OptionalStringExtensionLite, "Hi").Build();
      Assert.AreNotEqual(msg.GetHashCode(), copy.GetHashCode());
      Assert.IsFalse(msg.Equals(copy));
      copy = copy.ToBuilder().SetExtension(UnitTestLiteProtoFile.OptionalStringExtensionLite, "Hi").Build();
      Assert.AreEqual(msg.GetHashCode(), copy.GetHashCode());
      Assert.IsTrue(msg.Equals(copy));
    }

    [Test]
    public void TestAllTypesToString() {
      TestAllTypesLite msg = TestAllTypesLite.DefaultInstance;
      TestAllTypesLite copy = msg.ToBuilder().Build();
      Assert.AreEqual(msg.ToString(), copy.ToString());
      Assert.IsEmpty(msg.ToString());
      msg = msg.ToBuilder().SetOptionalInt32(-1).Build();
      Assert.AreEqual("optional_int32: -1", msg.ToString().TrimEnd());
      msg = msg.ToBuilder().SetOptionalString("abc123").Build();
      Assert.AreEqual("optional_int32: -1\noptional_string: \"abc123\"", msg.ToString().Replace("\r", "").TrimEnd());
    }

    [Test]
    public void TestAllTypesDefaultedRoundTrip() {
      TestAllTypesLite msg = TestAllTypesLite.DefaultInstance;
      Assert.IsTrue(msg.IsInitialized);
      TestAllTypesLite copy = TestAllTypesLite.CreateBuilder().MergeFrom(msg.ToByteArray()).Build();
      Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
    }

    [Test]
    public void TestAllTypesModifiedRoundTrip() {
      TestAllTypesLite msg = TestAllTypesLite.DefaultInstance;
      msg.ToBuilder()
        .SetOptionalBool(true)
        .SetOptionalCord("Hi")
        .SetOptionalDouble(1.123)
        .SetOptionalForeignEnum(ForeignEnumLite.FOREIGN_LITE_FOO)
        .SetOptionalForeignMessage(ForeignMessageLite.CreateBuilder().SetC('c').Build())
        .SetOptionalGroup(TestAllTypesLite.Types.OptionalGroup.CreateBuilder().SetA('a').Build())
        .SetOptionalImportEnum(ImportEnumLite.IMPORT_LITE_BAR)
        .SetOptionalInt32(32)
        .SetOptionalInt64(64)
        .SetOptionalNestedEnum(TestAllTypesLite.Types.NestedEnum.FOO)
        .SetOptionalString("SetOptionalString")
        .AddRepeatedGroup(TestAllTypesLite.Types.RepeatedGroup.CreateBuilder().SetA('a').Build())
        .AddRepeatedGroup(TestAllTypesLite.Types.RepeatedGroup.CreateBuilder().SetA('A').Build())
        ;
      TestAllTypesLite copy = TestAllTypesLite.CreateBuilder().MergeFrom(msg.ToByteArray()).Build();
      Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
    }

  }
}
