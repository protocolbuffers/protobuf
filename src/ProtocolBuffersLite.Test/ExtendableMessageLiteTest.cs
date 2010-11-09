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
using System.Text;
using Google.ProtocolBuffers;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class ExtendableMessageLiteTest {


    [Test]
    public void ExtensionWriterTest() {
      TestAllExtensionsLite.Builder builder = TestAllExtensionsLite.CreateBuilder()
        .SetExtension(UnitTestLiteProtoFile.DefaultBoolExtensionLite, true)
        .SetExtension(UnitTestLiteProtoFile.DefaultBytesExtensionLite, ByteString.CopyFromUtf8("123"))
        .SetExtension(UnitTestLiteProtoFile.DefaultCordExtensionLite, "123")
        .SetExtension(UnitTestLiteProtoFile.DefaultDoubleExtensionLite, 123)
        .SetExtension(UnitTestLiteProtoFile.DefaultFixed32ExtensionLite, 123u)
        .SetExtension(UnitTestLiteProtoFile.DefaultFixed64ExtensionLite, 123u)
        .SetExtension(UnitTestLiteProtoFile.DefaultFloatExtensionLite, 123)
        .SetExtension(UnitTestLiteProtoFile.DefaultForeignEnumExtensionLite, ForeignEnumLite.FOREIGN_LITE_BAZ)
        .SetExtension(UnitTestLiteProtoFile.DefaultImportEnumExtensionLite, ImportEnumLite.IMPORT_LITE_BAZ)
        .SetExtension(UnitTestLiteProtoFile.DefaultInt32ExtensionLite, 123)
        .SetExtension(UnitTestLiteProtoFile.DefaultInt64ExtensionLite, 123)
        .SetExtension(UnitTestLiteProtoFile.DefaultNestedEnumExtensionLite, TestAllTypesLite.Types.NestedEnum.FOO)
        .SetExtension(UnitTestLiteProtoFile.DefaultSfixed32ExtensionLite, 123)
        .SetExtension(UnitTestLiteProtoFile.DefaultSfixed64ExtensionLite, 123)
        .SetExtension(UnitTestLiteProtoFile.DefaultSint32ExtensionLite, 123)
        .SetExtension(UnitTestLiteProtoFile.DefaultSint64ExtensionLite, 123)
        .SetExtension(UnitTestLiteProtoFile.DefaultStringExtensionLite, "123")
        .SetExtension(UnitTestLiteProtoFile.DefaultStringPieceExtensionLite, "123")
        .SetExtension(UnitTestLiteProtoFile.DefaultUint32ExtensionLite, 123u)
        .SetExtension(UnitTestLiteProtoFile.DefaultUint64ExtensionLite, 123u)
        //Optional
        .SetExtension(UnitTestLiteProtoFile.OptionalBoolExtensionLite, true)
        .SetExtension(UnitTestLiteProtoFile.OptionalBytesExtensionLite, ByteString.CopyFromUtf8("123"))
        .SetExtension(UnitTestLiteProtoFile.OptionalCordExtensionLite, "123")
        .SetExtension(UnitTestLiteProtoFile.OptionalDoubleExtensionLite, 123)
        .SetExtension(UnitTestLiteProtoFile.OptionalFixed32ExtensionLite, 123u)
        .SetExtension(UnitTestLiteProtoFile.OptionalFixed64ExtensionLite, 123u)
        .SetExtension(UnitTestLiteProtoFile.OptionalFloatExtensionLite, 123)
        .SetExtension(UnitTestLiteProtoFile.OptionalForeignEnumExtensionLite, ForeignEnumLite.FOREIGN_LITE_BAZ)
        .SetExtension(UnitTestLiteProtoFile.OptionalImportEnumExtensionLite, ImportEnumLite.IMPORT_LITE_BAZ)
        .SetExtension(UnitTestLiteProtoFile.OptionalInt32ExtensionLite, 123)
        .SetExtension(UnitTestLiteProtoFile.OptionalInt64ExtensionLite, 123)
        .SetExtension(UnitTestLiteProtoFile.OptionalNestedEnumExtensionLite, TestAllTypesLite.Types.NestedEnum.FOO)
        .SetExtension(UnitTestLiteProtoFile.OptionalSfixed32ExtensionLite, 123)
        .SetExtension(UnitTestLiteProtoFile.OptionalSfixed64ExtensionLite, 123)
        .SetExtension(UnitTestLiteProtoFile.OptionalSint32ExtensionLite, 123)
        .SetExtension(UnitTestLiteProtoFile.OptionalSint64ExtensionLite, 123)
        .SetExtension(UnitTestLiteProtoFile.OptionalStringExtensionLite, "123")
        .SetExtension(UnitTestLiteProtoFile.OptionalStringPieceExtensionLite, "123")
        .SetExtension(UnitTestLiteProtoFile.OptionalUint32ExtensionLite, 123u)
        .SetExtension(UnitTestLiteProtoFile.OptionalUint64ExtensionLite, 123u)
        //Repeated
        .AddExtension(UnitTestLiteProtoFile.RepeatedBoolExtensionLite, true)
        .AddExtension(UnitTestLiteProtoFile.RepeatedBytesExtensionLite, ByteString.CopyFromUtf8("123"))
        .AddExtension(UnitTestLiteProtoFile.RepeatedCordExtensionLite, "123")
        .AddExtension(UnitTestLiteProtoFile.RepeatedDoubleExtensionLite, 123)
        .AddExtension(UnitTestLiteProtoFile.RepeatedFixed32ExtensionLite, 123u)
        .AddExtension(UnitTestLiteProtoFile.RepeatedFixed64ExtensionLite, 123u)
        .AddExtension(UnitTestLiteProtoFile.RepeatedFloatExtensionLite, 123)
        .AddExtension(UnitTestLiteProtoFile.RepeatedForeignEnumExtensionLite, ForeignEnumLite.FOREIGN_LITE_BAZ)
        .AddExtension(UnitTestLiteProtoFile.RepeatedImportEnumExtensionLite, ImportEnumLite.IMPORT_LITE_BAZ)
        .AddExtension(UnitTestLiteProtoFile.RepeatedInt32ExtensionLite, 123)
        .AddExtension(UnitTestLiteProtoFile.RepeatedInt64ExtensionLite, 123)
        .AddExtension(UnitTestLiteProtoFile.RepeatedNestedEnumExtensionLite, TestAllTypesLite.Types.NestedEnum.FOO)
        .AddExtension(UnitTestLiteProtoFile.RepeatedSfixed32ExtensionLite, 123)
        .AddExtension(UnitTestLiteProtoFile.RepeatedSfixed64ExtensionLite, 123)
        .AddExtension(UnitTestLiteProtoFile.RepeatedSint32ExtensionLite, 123)
        .AddExtension(UnitTestLiteProtoFile.RepeatedSint64ExtensionLite, 123)
        .AddExtension(UnitTestLiteProtoFile.RepeatedStringExtensionLite, "123")
        .AddExtension(UnitTestLiteProtoFile.RepeatedStringPieceExtensionLite, "123")
        .AddExtension(UnitTestLiteProtoFile.RepeatedUint32ExtensionLite, 123u)
        .AddExtension(UnitTestLiteProtoFile.RepeatedUint64ExtensionLite, 123u)
        ;
      TestAllExtensionsLite msg = builder.Build();

      ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
      UnitTestLiteProtoFile.RegisterAllExtensions(registry);

      TestAllExtensionsLite.Builder copyBuilder = TestAllExtensionsLite.CreateBuilder().MergeFrom(msg.ToByteArray(), registry);
      TestAllExtensionsLite copy = copyBuilder.Build();

      Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());

      Assert.AreEqual(true, copy.GetExtension(UnitTestLiteProtoFile.DefaultBoolExtensionLite));
      Assert.AreEqual(ByteString.CopyFromUtf8("123"), copy.GetExtension(UnitTestLiteProtoFile.DefaultBytesExtensionLite));
      Assert.AreEqual("123", copy.GetExtension(UnitTestLiteProtoFile.DefaultCordExtensionLite));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.DefaultDoubleExtensionLite));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestLiteProtoFile.DefaultFixed32ExtensionLite));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestLiteProtoFile.DefaultFixed64ExtensionLite));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.DefaultFloatExtensionLite));
      Assert.AreEqual(ForeignEnumLite.FOREIGN_LITE_BAZ, copy.GetExtension(UnitTestLiteProtoFile.DefaultForeignEnumExtensionLite));
      Assert.AreEqual(ImportEnumLite.IMPORT_LITE_BAZ, copy.GetExtension(UnitTestLiteProtoFile.DefaultImportEnumExtensionLite));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.DefaultInt32ExtensionLite));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.DefaultInt64ExtensionLite));
      Assert.AreEqual(TestAllTypesLite.Types.NestedEnum.FOO, copy.GetExtension(UnitTestLiteProtoFile.DefaultNestedEnumExtensionLite));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.DefaultSfixed32ExtensionLite));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.DefaultSfixed64ExtensionLite));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.DefaultSint32ExtensionLite));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.DefaultSint64ExtensionLite));
      Assert.AreEqual("123", copy.GetExtension(UnitTestLiteProtoFile.DefaultStringExtensionLite));
      Assert.AreEqual("123", copy.GetExtension(UnitTestLiteProtoFile.DefaultStringPieceExtensionLite));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestLiteProtoFile.DefaultUint32ExtensionLite));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestLiteProtoFile.DefaultUint64ExtensionLite));

      Assert.AreEqual(true, copy.GetExtension(UnitTestLiteProtoFile.OptionalBoolExtensionLite));
      Assert.AreEqual(ByteString.CopyFromUtf8("123"), copy.GetExtension(UnitTestLiteProtoFile.OptionalBytesExtensionLite));
      Assert.AreEqual("123", copy.GetExtension(UnitTestLiteProtoFile.OptionalCordExtensionLite));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.OptionalDoubleExtensionLite));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestLiteProtoFile.OptionalFixed32ExtensionLite));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestLiteProtoFile.OptionalFixed64ExtensionLite));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.OptionalFloatExtensionLite));
      Assert.AreEqual(ForeignEnumLite.FOREIGN_LITE_BAZ, copy.GetExtension(UnitTestLiteProtoFile.OptionalForeignEnumExtensionLite));
      Assert.AreEqual(ImportEnumLite.IMPORT_LITE_BAZ, copy.GetExtension(UnitTestLiteProtoFile.OptionalImportEnumExtensionLite));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.OptionalInt32ExtensionLite));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.OptionalInt64ExtensionLite));
      Assert.AreEqual(TestAllTypesLite.Types.NestedEnum.FOO, copy.GetExtension(UnitTestLiteProtoFile.OptionalNestedEnumExtensionLite));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.OptionalSfixed32ExtensionLite));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.OptionalSfixed64ExtensionLite));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.OptionalSint32ExtensionLite));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.OptionalSint64ExtensionLite));
      Assert.AreEqual("123", copy.GetExtension(UnitTestLiteProtoFile.OptionalStringExtensionLite));
      Assert.AreEqual("123", copy.GetExtension(UnitTestLiteProtoFile.OptionalStringPieceExtensionLite));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestLiteProtoFile.OptionalUint32ExtensionLite));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestLiteProtoFile.OptionalUint64ExtensionLite));

      Assert.AreEqual(true, copy.GetExtension(UnitTestLiteProtoFile.RepeatedBoolExtensionLite, 0));
      Assert.AreEqual(ByteString.CopyFromUtf8("123"), copy.GetExtension(UnitTestLiteProtoFile.RepeatedBytesExtensionLite, 0));
      Assert.AreEqual("123", copy.GetExtension(UnitTestLiteProtoFile.RepeatedCordExtensionLite, 0));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.RepeatedDoubleExtensionLite, 0));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestLiteProtoFile.RepeatedFixed32ExtensionLite, 0));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestLiteProtoFile.RepeatedFixed64ExtensionLite, 0));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.RepeatedFloatExtensionLite, 0));
      Assert.AreEqual(ForeignEnumLite.FOREIGN_LITE_BAZ, copy.GetExtension(UnitTestLiteProtoFile.RepeatedForeignEnumExtensionLite, 0));
      Assert.AreEqual(ImportEnumLite.IMPORT_LITE_BAZ, copy.GetExtension(UnitTestLiteProtoFile.RepeatedImportEnumExtensionLite, 0));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.RepeatedInt32ExtensionLite, 0));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.RepeatedInt64ExtensionLite, 0));
      Assert.AreEqual(TestAllTypesLite.Types.NestedEnum.FOO, copy.GetExtension(UnitTestLiteProtoFile.RepeatedNestedEnumExtensionLite, 0));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.RepeatedSfixed32ExtensionLite, 0));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.RepeatedSfixed64ExtensionLite, 0));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.RepeatedSint32ExtensionLite, 0));
      Assert.AreEqual(123, copy.GetExtension(UnitTestLiteProtoFile.RepeatedSint64ExtensionLite, 0));
      Assert.AreEqual("123", copy.GetExtension(UnitTestLiteProtoFile.RepeatedStringExtensionLite, 0));
      Assert.AreEqual("123", copy.GetExtension(UnitTestLiteProtoFile.RepeatedStringPieceExtensionLite, 0));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestLiteProtoFile.RepeatedUint32ExtensionLite, 0));
      Assert.AreEqual(123u, copy.GetExtension(UnitTestLiteProtoFile.RepeatedUint64ExtensionLite, 0));
    }

    
    
    /*
    
    [Test]
    public void ExtensionWriterTest() {
      TestAllExtensionsLite.Builder builder = TestAllExtensionsLite.CreateBuilder()
          .SetExtension(UnitTestLiteProtoFile.DefaultBoolExtensionLite, true)
          .SetExtension(UnitTestLiteProtoFile.DefaultBytesExtensionLite, ByteString.CopyFromUtf8("123"))
          .SetExtension(UnitTestLiteProtoFile.DefaultCordExtensionLite, "123")
          .SetExtension(UnitTestLiteProtoFile.DefaultDoubleExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.DefaultFixed32ExtensionLite, 123u)
          .SetExtension(UnitTestLiteProtoFile.DefaultFixed64ExtensionLite, 123u)
          .SetExtension(UnitTestLiteProtoFile.DefaultFloatExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.DefaultForeignEnumExtensionLite, ForeignEnumLite.FOREIGN_LITE_FOO)
          .SetExtension(UnitTestLiteProtoFile.DefaultImportEnumExtensionLite, ImportEnumLite.IMPORT_LITE_BAZ)
          .SetExtension(UnitTestLiteProtoFile.DefaultInt32ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.DefaultInt64ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.DefaultNestedEnumExtensionLite, TestAllTypesLite.Types.NestedEnum.FOO)
          .SetExtension(UnitTestLiteProtoFile.DefaultSfixed32ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.DefaultSfixed64ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.DefaultSint32ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.DefaultSint64ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.DefaultStringExtensionLite, "123")
          .SetExtension(UnitTestLiteProtoFile.DefaultStringPieceExtensionLite, "123")
          .SetExtension(UnitTestLiteProtoFile.DefaultUint32ExtensionLite, 123u)
          .SetExtension(UnitTestLiteProtoFile.DefaultUint64ExtensionLite, 123u)
          //Optional
          .SetExtension(UnitTestLiteProtoFile.OptionalBoolExtensionLite, true)
          .SetExtension(UnitTestLiteProtoFile.OptionalBytesExtensionLite, ByteString.CopyFromUtf8("123"))
          .SetExtension(UnitTestLiteProtoFile.OptionalCordExtensionLite, "123")
          .SetExtension(UnitTestLiteProtoFile.OptionalDoubleExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.OptionalFixed32ExtensionLite, 123u)
          .SetExtension(UnitTestLiteProtoFile.OptionalFixed64ExtensionLite, 123u)
          .SetExtension(UnitTestLiteProtoFile.OptionalFloatExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.OptionalForeignEnumExtensionLite, ForeignEnumLite.FOREIGN_LITE_FOO)
          .SetExtension(UnitTestLiteProtoFile.OptionalImportEnumExtensionLite, ImportEnumLite.IMPORT_LITE_BAZ)
          .SetExtension(UnitTestLiteProtoFile.OptionalInt32ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.OptionalInt64ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.OptionalNestedEnumExtensionLite, TestAllTypesLite.Types.NestedEnum.FOO)
          .SetExtension(UnitTestLiteProtoFile.OptionalSfixed32ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.OptionalSfixed64ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.OptionalSint32ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.OptionalSint64ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.OptionalStringExtensionLite, "123")
          .SetExtension(UnitTestLiteProtoFile.OptionalStringPieceExtensionLite, "123")
          .SetExtension(UnitTestLiteProtoFile.OptionalUint32ExtensionLite, 123u)
          .SetExtension(UnitTestLiteProtoFile.OptionalUint64ExtensionLite, 123u)
          //Repeated
          .AddExtension(UnitTestLiteProtoFile.RepeatedBoolExtensionLite, true)
          .AddExtension(UnitTestLiteProtoFile.RepeatedBytesExtensionLite, ByteString.CopyFromUtf8("123"))
          .AddExtension(UnitTestLiteProtoFile.RepeatedCordExtensionLite, "123")
          .AddExtension(UnitTestLiteProtoFile.RepeatedDoubleExtensionLite, 123)
          .AddExtension(UnitTestLiteProtoFile.RepeatedFixed32ExtensionLite, 123u)
          .AddExtension(UnitTestLiteProtoFile.RepeatedFixed64ExtensionLite, 123u)
          .AddExtension(UnitTestLiteProtoFile.RepeatedFloatExtensionLite, 123)
          .AddExtension(UnitTestLiteProtoFile.RepeatedForeignEnumExtensionLite, ForeignEnumLite.FOREIGN_LITE_FOO)
          .AddExtension(UnitTestLiteProtoFile.RepeatedImportEnumExtensionLite, ImportEnumLite.IMPORT_LITE_BAZ)
          .AddExtension(UnitTestLiteProtoFile.RepeatedInt32ExtensionLite, 123)
          .AddExtension(UnitTestLiteProtoFile.RepeatedInt64ExtensionLite, 123)
          .AddExtension(UnitTestLiteProtoFile.RepeatedNestedEnumExtensionLite, TestAllTypesLite.Types.NestedEnum.FOO)
          .AddExtension(UnitTestLiteProtoFile.RepeatedSfixed32ExtensionLite, 123)
          .AddExtension(UnitTestLiteProtoFile.RepeatedSfixed64ExtensionLite, 123)
          .AddExtension(UnitTestLiteProtoFile.RepeatedSint32ExtensionLite, 123)
          .AddExtension(UnitTestLiteProtoFile.RepeatedSint64ExtensionLite, 123)
          .AddExtension(UnitTestLiteProtoFile.RepeatedStringExtensionLite, "123")
          .AddExtension(UnitTestLiteProtoFile.RepeatedStringPieceExtensionLite, "123")
          .AddExtension(UnitTestLiteProtoFile.RepeatedUint32ExtensionLite, 123u)
          .AddExtension(UnitTestLiteProtoFile.RepeatedUint64ExtensionLite, 123u)
        ;
      builder = TestAllExtensionsLite.CreateBuilder()
          .SetExtension(UnitTestLiteProtoFile.DefaultBoolExtensionLite, true)
          .SetExtension(UnitTestLiteProtoFile.DefaultBytesExtensionLite, ByteString.CopyFromUtf8("123"))
          .SetExtension(UnitTestLiteProtoFile.DefaultCordExtensionLite, "123")
          .SetExtension(UnitTestLiteProtoFile.DefaultDoubleExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.DefaultFixed32ExtensionLite, 123u)
          .SetExtension(UnitTestLiteProtoFile.DefaultFixed64ExtensionLite, 123u)
          .SetExtension(UnitTestLiteProtoFile.DefaultFloatExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.DefaultForeignEnumExtensionLite, ForeignEnumLite.FOREIGN_LITE_FOO)
          .SetExtension(UnitTestLiteProtoFile.DefaultImportEnumExtensionLite, ImportEnumLite.IMPORT_LITE_BAZ)
          .SetExtension(UnitTestLiteProtoFile.DefaultInt32ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.DefaultInt64ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.DefaultNestedEnumExtensionLite, TestAllTypesLite.Types.NestedEnum.FOO)
          .SetExtension(UnitTestLiteProtoFile.DefaultSfixed32ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.DefaultSfixed64ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.DefaultSint32ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.DefaultSint64ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.DefaultStringExtensionLite, "123")
          .SetExtension(UnitTestLiteProtoFile.DefaultStringPieceExtensionLite, "123")
          .SetExtension(UnitTestLiteProtoFile.DefaultUint32ExtensionLite, 123u)
          .SetExtension(UnitTestLiteProtoFile.DefaultUint64ExtensionLite, 123u)
        ;

      TestAllExtensionsLite msg = builder.Build();
      byte[] data = msg.ToByteArray();
      TestAllExtensionsLite.Builder copyBuilder = TestAllExtensionsLite.CreateBuilder().MergeFrom(data);
      TestAllExtensionsLite copy = copyBuilder.Build();

      Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
    }
     * 
          //Packed
          .SetExtension(UnitTestLiteProtoFile.PackedBoolExtensionLite, true)
          .SetExtension(UnitTestLiteProtoFile.PackedDoubleExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.PackedFixed32ExtensionLite, 123u)
          .SetExtension(UnitTestLiteProtoFile.PackedFixed64ExtensionLite, 123u)
          .SetExtension(UnitTestLiteProtoFile.PackedFloatExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.PackedInt32ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.PackedInt64ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.PackedSfixed32ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.PackedSfixed64ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.PackedSint32ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.PackedSint64ExtensionLite, 123)
          .SetExtension(UnitTestLiteProtoFile.PackedUint32ExtensionLite, 123u)
          .SetExtension(UnitTestLiteProtoFile.PackedUint64ExtensionLite, 123u)
     */
  }
}
