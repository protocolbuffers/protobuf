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
using Google.ProtocolBuffers;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class ExtendableMessageTest {

    [Test, ExpectedException(typeof(ArgumentException))]
    public void ExtensionWriterInvalidExtension() {
      TestPackedExtensions.CreateBuilder()[UnitTestProtoFile.OptionalForeignMessageExtension.Descriptor] =
        ForeignMessage.DefaultInstance;
    }

    [Test]
    public void ExtensionWriterTest() {
      TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder()
        .SetExtension(UnitTestProtoFile.DefaultBoolExtension, true)
        .SetExtension(UnitTestProtoFile.DefaultBytesExtension, ByteString.CopyFromUtf8("123"))
        .SetExtension(UnitTestProtoFile.DefaultCordExtension, "123")
        .SetExtension(UnitTestProtoFile.DefaultDoubleExtension, 123)
        .SetExtension(UnitTestProtoFile.DefaultFixed32Extension, 123u)
        .SetExtension(UnitTestProtoFile.DefaultFixed64Extension, 123u)
        .SetExtension(UnitTestProtoFile.DefaultFloatExtension, 123)
        .SetExtension(UnitTestProtoFile.DefaultForeignEnumExtension, ForeignEnum.FOREIGN_BAZ)
        .SetExtension(UnitTestProtoFile.DefaultImportEnumExtension, ImportEnum.IMPORT_BAZ)
        .SetExtension(UnitTestProtoFile.DefaultInt32Extension, 123)
        .SetExtension(UnitTestProtoFile.DefaultInt64Extension, 123)
        .SetExtension(UnitTestProtoFile.DefaultNestedEnumExtension, TestAllTypes.Types.NestedEnum.FOO)
        .SetExtension(UnitTestProtoFile.DefaultSfixed32Extension, 123)
        .SetExtension(UnitTestProtoFile.DefaultSfixed64Extension, 123)
        .SetExtension(UnitTestProtoFile.DefaultSint32Extension, 123)
        .SetExtension(UnitTestProtoFile.DefaultSint64Extension, 123)
        .SetExtension(UnitTestProtoFile.DefaultStringExtension, "123")
        .SetExtension(UnitTestProtoFile.DefaultStringPieceExtension, "123")
        .SetExtension(UnitTestProtoFile.DefaultUint32Extension, 123u)
        .SetExtension(UnitTestProtoFile.DefaultUint64Extension, 123u)
        //Optional
        .SetExtension(UnitTestProtoFile.OptionalBoolExtension, true)
        .SetExtension(UnitTestProtoFile.OptionalBytesExtension, ByteString.CopyFromUtf8("123"))
        .SetExtension(UnitTestProtoFile.OptionalCordExtension, "123")
        .SetExtension(UnitTestProtoFile.OptionalDoubleExtension, 123)
        .SetExtension(UnitTestProtoFile.OptionalFixed32Extension, 123u)
        .SetExtension(UnitTestProtoFile.OptionalFixed64Extension, 123u)
        .SetExtension(UnitTestProtoFile.OptionalFloatExtension, 123)
        .SetExtension(UnitTestProtoFile.OptionalForeignEnumExtension, ForeignEnum.FOREIGN_BAZ)
        .SetExtension(UnitTestProtoFile.OptionalImportEnumExtension, ImportEnum.IMPORT_BAZ)
        .SetExtension(UnitTestProtoFile.OptionalInt32Extension, 123)
        .SetExtension(UnitTestProtoFile.OptionalInt64Extension, 123)
        .SetExtension(UnitTestProtoFile.OptionalNestedEnumExtension, TestAllTypes.Types.NestedEnum.FOO)
        .SetExtension(UnitTestProtoFile.OptionalSfixed32Extension, 123)
        .SetExtension(UnitTestProtoFile.OptionalSfixed64Extension, 123)
        .SetExtension(UnitTestProtoFile.OptionalSint32Extension, 123)
        .SetExtension(UnitTestProtoFile.OptionalSint64Extension, 123)
        .SetExtension(UnitTestProtoFile.OptionalStringExtension, "123")
        .SetExtension(UnitTestProtoFile.OptionalStringPieceExtension, "123")
        .SetExtension(UnitTestProtoFile.OptionalUint32Extension, 123u)
        .SetExtension(UnitTestProtoFile.OptionalUint64Extension, 123u)
        //Repeated
        .AddExtension(UnitTestProtoFile.RepeatedBoolExtension, true)
        .AddExtension(UnitTestProtoFile.RepeatedBytesExtension, ByteString.CopyFromUtf8("123"))
        .AddExtension(UnitTestProtoFile.RepeatedCordExtension, "123")
        .AddExtension(UnitTestProtoFile.RepeatedDoubleExtension, 123)
        .AddExtension(UnitTestProtoFile.RepeatedFixed32Extension, 123u)
        .AddExtension(UnitTestProtoFile.RepeatedFixed64Extension, 123u)
        .AddExtension(UnitTestProtoFile.RepeatedFloatExtension, 123)
        .AddExtension(UnitTestProtoFile.RepeatedForeignEnumExtension, ForeignEnum.FOREIGN_BAZ)
        .AddExtension(UnitTestProtoFile.RepeatedImportEnumExtension, ImportEnum.IMPORT_BAZ)
        .AddExtension(UnitTestProtoFile.RepeatedInt32Extension, 123)
        .AddExtension(UnitTestProtoFile.RepeatedInt64Extension, 123)
        .AddExtension(UnitTestProtoFile.RepeatedNestedEnumExtension, TestAllTypes.Types.NestedEnum.FOO)
        .AddExtension(UnitTestProtoFile.RepeatedSfixed32Extension, 123)
        .AddExtension(UnitTestProtoFile.RepeatedSfixed64Extension, 123)
        .AddExtension(UnitTestProtoFile.RepeatedSint32Extension, 123)
        .AddExtension(UnitTestProtoFile.RepeatedSint64Extension, 123)
        .AddExtension(UnitTestProtoFile.RepeatedStringExtension, "123")
        .AddExtension(UnitTestProtoFile.RepeatedStringPieceExtension, "123")
        .AddExtension(UnitTestProtoFile.RepeatedUint32Extension, 123u)
        .AddExtension(UnitTestProtoFile.RepeatedUint64Extension, 123u)
        ;
      TestAllExtensions msg = builder.Build();

      ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
      UnitTestProtoFile.RegisterAllExtensions(registry);

      TestAllExtensions.Builder copyBuilder = TestAllExtensions.CreateBuilder().MergeFrom(msg.ToByteArray(), registry);
      TestAllExtensions copy = copyBuilder.Build();

      Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());

      Assert.AreEqual(true, copy.GetExtension(UnitTestProtoFile.DefaultBoolExtension));
      Assert.AreEqual(ByteString.CopyFromUtf8("123"), copy.GetExtension(UnitTestProtoFile.DefaultBytesExtension));
      Assert.AreEqual("123", copy.GetExtension(UnitTestProtoFile.DefaultCordExtension));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.DefaultDoubleExtension));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestProtoFile.DefaultFixed32Extension));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestProtoFile.DefaultFixed64Extension));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.DefaultFloatExtension));
      Assert.AreEqual(ForeignEnum.FOREIGN_BAZ, copy.GetExtension(UnitTestProtoFile.DefaultForeignEnumExtension));
      Assert.AreEqual(ImportEnum.IMPORT_BAZ, copy.GetExtension(UnitTestProtoFile.DefaultImportEnumExtension));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.DefaultInt32Extension));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.DefaultInt64Extension));
      Assert.AreEqual(TestAllTypes.Types.NestedEnum.FOO, copy.GetExtension(UnitTestProtoFile.DefaultNestedEnumExtension));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.DefaultSfixed32Extension));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.DefaultSfixed64Extension));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.DefaultSint32Extension));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.DefaultSint64Extension));
      Assert.AreEqual("123", copy.GetExtension(UnitTestProtoFile.DefaultStringExtension));
      Assert.AreEqual("123", copy.GetExtension(UnitTestProtoFile.DefaultStringPieceExtension));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestProtoFile.DefaultUint32Extension));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestProtoFile.DefaultUint64Extension));

      Assert.AreEqual(true, copy.GetExtension(UnitTestProtoFile.OptionalBoolExtension));
      Assert.AreEqual(ByteString.CopyFromUtf8("123"), copy.GetExtension(UnitTestProtoFile.OptionalBytesExtension));
      Assert.AreEqual("123", copy.GetExtension(UnitTestProtoFile.OptionalCordExtension));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.OptionalDoubleExtension));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestProtoFile.OptionalFixed32Extension));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestProtoFile.OptionalFixed64Extension));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.OptionalFloatExtension));
      Assert.AreEqual(ForeignEnum.FOREIGN_BAZ, copy.GetExtension(UnitTestProtoFile.OptionalForeignEnumExtension));
      Assert.AreEqual(ImportEnum.IMPORT_BAZ, copy.GetExtension(UnitTestProtoFile.OptionalImportEnumExtension));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.OptionalInt32Extension));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.OptionalInt64Extension));
      Assert.AreEqual(TestAllTypes.Types.NestedEnum.FOO, copy.GetExtension(UnitTestProtoFile.OptionalNestedEnumExtension));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.OptionalSfixed32Extension));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.OptionalSfixed64Extension));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.OptionalSint32Extension));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.OptionalSint64Extension));
      Assert.AreEqual("123", copy.GetExtension(UnitTestProtoFile.OptionalStringExtension));
      Assert.AreEqual("123", copy.GetExtension(UnitTestProtoFile.OptionalStringPieceExtension));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestProtoFile.OptionalUint32Extension));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestProtoFile.OptionalUint64Extension));

      Assert.AreEqual(true, copy.GetExtension(UnitTestProtoFile.RepeatedBoolExtension, 0));
      Assert.AreEqual(ByteString.CopyFromUtf8("123"), copy.GetExtension(UnitTestProtoFile.RepeatedBytesExtension, 0));
      Assert.AreEqual("123", copy.GetExtension(UnitTestProtoFile.RepeatedCordExtension, 0));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.RepeatedDoubleExtension, 0));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestProtoFile.RepeatedFixed32Extension, 0));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestProtoFile.RepeatedFixed64Extension, 0));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.RepeatedFloatExtension, 0));
      Assert.AreEqual(ForeignEnum.FOREIGN_BAZ, copy.GetExtension(UnitTestProtoFile.RepeatedForeignEnumExtension, 0));
      Assert.AreEqual(ImportEnum.IMPORT_BAZ, copy.GetExtension(UnitTestProtoFile.RepeatedImportEnumExtension, 0));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.RepeatedInt32Extension, 0));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.RepeatedInt64Extension, 0));
      Assert.AreEqual(TestAllTypes.Types.NestedEnum.FOO, copy.GetExtension(UnitTestProtoFile.RepeatedNestedEnumExtension, 0));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.RepeatedSfixed32Extension, 0));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.RepeatedSfixed64Extension, 0));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.RepeatedSint32Extension, 0));
      Assert.AreEqual(123, copy.GetExtension(UnitTestProtoFile.RepeatedSint64Extension, 0));
      Assert.AreEqual("123", copy.GetExtension(UnitTestProtoFile.RepeatedStringExtension, 0));
      Assert.AreEqual("123", copy.GetExtension(UnitTestProtoFile.RepeatedStringPieceExtension, 0));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestProtoFile.RepeatedUint32Extension, 0));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestProtoFile.RepeatedUint64Extension, 0));
    }

  }
}
