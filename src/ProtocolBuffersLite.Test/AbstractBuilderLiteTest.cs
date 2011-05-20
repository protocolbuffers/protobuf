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
  public class AbstractBuilderLiteTest {

    [Test]
    public void TestMergeFromCodedInputStream() {
      TestAllTypesLite copy, msg = TestAllTypesLite.CreateBuilder()
        .SetOptionalUint32(uint.MaxValue).Build();

      copy = TestAllTypesLite.DefaultInstance;
      Assert.AreNotEqual(msg.ToByteArray(), copy.ToByteArray());

      using (MemoryStream ms = new MemoryStream(msg.ToByteArray())) {
        CodedInputStream ci = CodedInputStream.CreateInstance(ms);
        copy = copy.ToBuilder().MergeFrom(ci).Build();
      }

      Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
    }

    [Test]
    public void TestIBuilderLiteWeakClear() {
      TestAllTypesLite copy, msg = TestAllTypesLite.DefaultInstance;

      copy = msg.ToBuilder().SetOptionalString("Should be removed.").Build();
      Assert.AreNotEqual(msg.ToByteArray(), copy.ToByteArray());

      copy = (TestAllTypesLite)((IBuilderLite)copy.ToBuilder()).WeakClear().WeakBuild();
      Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
    }

    [Test]
    public void TestBuilderLiteMergeFromCodedInputStream() {
      TestAllTypesLite copy, msg = TestAllTypesLite.CreateBuilder()
        .SetOptionalString("Should be merged.").Build();

      copy = TestAllTypesLite.DefaultInstance;
      Assert.AreNotEqual(msg.ToByteArray(), copy.ToByteArray());

      copy = copy.ToBuilder().MergeFrom(CodedInputStream.CreateInstance(new MemoryStream(msg.ToByteArray()))).Build();
      Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
    }

    [Test]
    public void TestBuilderLiteMergeDelimitedFrom() {
      TestAllTypesLite copy, msg = TestAllTypesLite.CreateBuilder()
        .SetOptionalString("Should be merged.").Build();

      copy = TestAllTypesLite.DefaultInstance;
      Assert.AreNotEqual(msg.ToByteArray(), copy.ToByteArray());
      Stream s = new MemoryStream();
      msg.WriteDelimitedTo(s);
      s.Position = 0;
      copy = copy.ToBuilder().MergeDelimitedFrom(s).Build();
      Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
    }

    [Test]
    public void TestBuilderLiteMergeDelimitedFromExtensions() {
      TestAllExtensionsLite copy, msg = TestAllExtensionsLite.CreateBuilder()
        .SetExtension(UnitTestLiteProtoFile.OptionalStringExtensionLite, "Should be merged.").Build();

      copy = TestAllExtensionsLite.DefaultInstance;
      Assert.AreNotEqual(msg.ToByteArray(), copy.ToByteArray());

      Stream s = new MemoryStream();
      msg.WriteDelimitedTo(s);
      s.Position = 0;

      ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
      UnitTestLiteProtoFile.RegisterAllExtensions(registry);

      copy = copy.ToBuilder().MergeDelimitedFrom(s, registry).Build();
      Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
      Assert.AreEqual("Should be merged.", copy.GetExtension(UnitTestLiteProtoFile.OptionalStringExtensionLite));
    }

    [Test]
    public void TestBuilderLiteMergeFromStream() {
      TestAllTypesLite copy, msg = TestAllTypesLite.CreateBuilder()
        .SetOptionalString("Should be merged.").Build();

      copy = TestAllTypesLite.DefaultInstance;
      Assert.AreNotEqual(msg.ToByteArray(), copy.ToByteArray());
      Stream s = new MemoryStream();
      msg.WriteTo(s);
      s.Position = 0;
      copy = copy.ToBuilder().MergeFrom(s).Build();
      Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
    }

    [Test]
    public void TestBuilderLiteMergeFromStreamExtensions() {
      TestAllExtensionsLite copy, msg = TestAllExtensionsLite.CreateBuilder()
        .SetExtension(UnitTestLiteProtoFile.OptionalStringExtensionLite, "Should be merged.").Build();

      copy = TestAllExtensionsLite.DefaultInstance;
      Assert.AreNotEqual(msg.ToByteArray(), copy.ToByteArray());

      Stream s = new MemoryStream();
      msg.WriteTo(s);
      s.Position = 0;

      ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
      UnitTestLiteProtoFile.RegisterAllExtensions(registry);

      copy = copy.ToBuilder().MergeFrom(s, registry).Build();
      Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
      Assert.AreEqual("Should be merged.", copy.GetExtension(UnitTestLiteProtoFile.OptionalStringExtensionLite));
    }

    [Test]
    public void TestIBuilderLiteWeakMergeFromIMessageLite() {
      TestAllTypesLite copy, msg = TestAllTypesLite.CreateBuilder()
        .SetOptionalString("Should be merged.").Build();

      copy = TestAllTypesLite.DefaultInstance;
      Assert.AreNotEqual(msg.ToByteArray(), copy.ToByteArray());

      copy = (TestAllTypesLite)((IBuilderLite)copy.ToBuilder()).WeakMergeFrom((IMessageLite)msg).WeakBuild();
      Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
    }

    [Test]
    public void TestIBuilderLiteWeakMergeFromByteString() {
      TestAllTypesLite copy, msg = TestAllTypesLite.CreateBuilder()
        .SetOptionalString("Should be merged.").Build();

      copy = TestAllTypesLite.DefaultInstance;
      Assert.AreNotEqual(msg.ToByteArray(), copy.ToByteArray());

      copy = (TestAllTypesLite)((IBuilderLite)copy.ToBuilder()).WeakMergeFrom(msg.ToByteString()).WeakBuild();
      Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
    }

    [Test]
    public void TestIBuilderLiteWeakMergeFromByteStringExtensions() {
      TestAllExtensionsLite copy, msg = TestAllExtensionsLite.CreateBuilder()
        .SetExtension(UnitTestLiteProtoFile.OptionalStringExtensionLite, "Should be merged.").Build();

      copy = TestAllExtensionsLite.DefaultInstance;
      Assert.AreNotEqual(msg.ToByteArray(), copy.ToByteArray());

      copy = (TestAllExtensionsLite)((IBuilderLite)copy.ToBuilder()).WeakMergeFrom(msg.ToByteString(), ExtensionRegistry.Empty).WeakBuild();
      Assert.AreNotEqual(msg.ToByteArray(), copy.ToByteArray());

      ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
      UnitTestLiteProtoFile.RegisterAllExtensions(registry);

      copy = (TestAllExtensionsLite)((IBuilderLite)copy.ToBuilder()).WeakMergeFrom(msg.ToByteString(), registry).WeakBuild();
      Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
      Assert.AreEqual("Should be merged.", copy.GetExtension(UnitTestLiteProtoFile.OptionalStringExtensionLite));
    }
    
    [Test]
    public void TestIBuilderLiteWeakMergeFromCodedInputStream() {
      TestAllTypesLite copy, msg = TestAllTypesLite.CreateBuilder()
        .SetOptionalUint32(uint.MaxValue).Build();

      copy = TestAllTypesLite.DefaultInstance;
      Assert.AreNotEqual(msg.ToByteArray(), copy.ToByteArray());

      using (MemoryStream ms = new MemoryStream(msg.ToByteArray())) {
        CodedInputStream ci = CodedInputStream.CreateInstance(ms);
        copy = (TestAllTypesLite)((IBuilderLite)copy.ToBuilder()).WeakMergeFrom(ci).WeakBuild();
      }

      Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
    }

    [Test]
    public void TestIBuilderLiteWeakBuildPartial() {
      IBuilderLite builder = TestRequiredLite.CreateBuilder();
      Assert.IsFalse(builder.IsInitialized);

      IMessageLite msg = builder.WeakBuildPartial();
      Assert.IsFalse(msg.IsInitialized);

      Assert.AreEqual(msg.ToByteArray(), TestRequiredLite.DefaultInstance.ToByteArray());
    }

    [Test, ExpectedException(typeof(UninitializedMessageException ))]
    public void TestIBuilderLiteWeakBuildUninitialized() {
      IBuilderLite builder = TestRequiredLite.CreateBuilder();
      Assert.IsFalse(builder.IsInitialized);
      builder.WeakBuild();
    }

    [Test]
    public void TestIBuilderLiteWeakBuild() {
      IBuilderLite builder = TestRequiredLite.CreateBuilder()
        .SetD(0)
        .SetEn(ExtraEnum.EXLITE_BAZ);
      Assert.IsTrue(builder.IsInitialized);
      builder.WeakBuild();
    }

    [Test]
    public void TestIBuilderLiteWeakClone() {
      TestRequiredLite msg = TestRequiredLite.CreateBuilder()
        .SetD(1).SetEn(ExtraEnum.EXLITE_BAR).Build();
      Assert.IsTrue(msg.IsInitialized);

      IMessageLite copy = ((IBuilderLite)msg.ToBuilder()).WeakClone().WeakBuild();
      Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
    }

    [Test]
    public void TestIBuilderLiteWeakDefaultInstance() {
      Assert.IsTrue(ReferenceEquals(TestRequiredLite.DefaultInstance,
        ((IBuilderLite)TestRequiredLite.CreateBuilder()).WeakDefaultInstanceForType));
    }

    [Test]
    public void TestGeneratedBuilderLiteAddRange() {
      TestAllTypesLite copy, msg = TestAllTypesLite.CreateBuilder()
        .SetOptionalUint32(123)
        .AddRepeatedInt32(1)
        .AddRepeatedInt32(2)
        .AddRepeatedInt32(3)
        .Build();

      copy = msg.DefaultInstanceForType.ToBuilder().MergeFrom(msg).Build();
      Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
    }
  }
}
