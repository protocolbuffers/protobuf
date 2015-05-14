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
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers
{
    public class ExtendableMessageTest
    {
        [Test]
        public void ExtensionWriterInvalidExtension()
        {
            Assert.Throws<ArgumentException>(() =>
                TestPackedExtensions.CreateBuilder()[Unittest.OptionalForeignMessageExtension.Descriptor] =
                    ForeignMessage.DefaultInstance);
        }

        [Test]
        public void ExtensionWriterTest()
        {
            TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder()
                .SetExtension(Unittest.DefaultBoolExtension, true)
                .SetExtension(Unittest.DefaultBytesExtension, ByteString.CopyFromUtf8("123"))
                .SetExtension(Unittest.DefaultCordExtension, "123")
                .SetExtension(Unittest.DefaultDoubleExtension, 123)
                .SetExtension(Unittest.DefaultFixed32Extension, 123u)
                .SetExtension(Unittest.DefaultFixed64Extension, 123u)
                .SetExtension(Unittest.DefaultFloatExtension, 123)
                .SetExtension(Unittest.DefaultForeignEnumExtension, ForeignEnum.FOREIGN_BAZ)
                .SetExtension(Unittest.DefaultImportEnumExtension, ImportEnum.IMPORT_BAZ)
                .SetExtension(Unittest.DefaultInt32Extension, 123)
                .SetExtension(Unittest.DefaultInt64Extension, 123)
                .SetExtension(Unittest.DefaultNestedEnumExtension, TestAllTypes.Types.NestedEnum.FOO)
                .SetExtension(Unittest.DefaultSfixed32Extension, 123)
                .SetExtension(Unittest.DefaultSfixed64Extension, 123)
                .SetExtension(Unittest.DefaultSint32Extension, 123)
                .SetExtension(Unittest.DefaultSint64Extension, 123)
                .SetExtension(Unittest.DefaultStringExtension, "123")
                .SetExtension(Unittest.DefaultStringPieceExtension, "123")
                .SetExtension(Unittest.DefaultUint32Extension, 123u)
                .SetExtension(Unittest.DefaultUint64Extension, 123u)
                //Optional
                .SetExtension(Unittest.OptionalBoolExtension, true)
                .SetExtension(Unittest.OptionalBytesExtension, ByteString.CopyFromUtf8("123"))
                .SetExtension(Unittest.OptionalCordExtension, "123")
                .SetExtension(Unittest.OptionalDoubleExtension, 123)
                .SetExtension(Unittest.OptionalFixed32Extension, 123u)
                .SetExtension(Unittest.OptionalFixed64Extension, 123u)
                .SetExtension(Unittest.OptionalFloatExtension, 123)
                .SetExtension(Unittest.OptionalForeignEnumExtension, ForeignEnum.FOREIGN_BAZ)
                .SetExtension(Unittest.OptionalImportEnumExtension, ImportEnum.IMPORT_BAZ)
                .SetExtension(Unittest.OptionalInt32Extension, 123)
                .SetExtension(Unittest.OptionalInt64Extension, 123)
                .SetExtension(Unittest.OptionalNestedEnumExtension, TestAllTypes.Types.NestedEnum.FOO)
                .SetExtension(Unittest.OptionalSfixed32Extension, 123)
                .SetExtension(Unittest.OptionalSfixed64Extension, 123)
                .SetExtension(Unittest.OptionalSint32Extension, 123)
                .SetExtension(Unittest.OptionalSint64Extension, 123)
                .SetExtension(Unittest.OptionalStringExtension, "123")
                .SetExtension(Unittest.OptionalStringPieceExtension, "123")
                .SetExtension(Unittest.OptionalUint32Extension, 123u)
                .SetExtension(Unittest.OptionalUint64Extension, 123u)
                //Repeated
                .AddExtension(Unittest.RepeatedBoolExtension, true)
                .AddExtension(Unittest.RepeatedBytesExtension, ByteString.CopyFromUtf8("123"))
                .AddExtension(Unittest.RepeatedCordExtension, "123")
                .AddExtension(Unittest.RepeatedDoubleExtension, 123)
                .AddExtension(Unittest.RepeatedFixed32Extension, 123u)
                .AddExtension(Unittest.RepeatedFixed64Extension, 123u)
                .AddExtension(Unittest.RepeatedFloatExtension, 123)
                .AddExtension(Unittest.RepeatedForeignEnumExtension, ForeignEnum.FOREIGN_BAZ)
                .AddExtension(Unittest.RepeatedImportEnumExtension, ImportEnum.IMPORT_BAZ)
                .AddExtension(Unittest.RepeatedInt32Extension, 123)
                .AddExtension(Unittest.RepeatedInt64Extension, 123)
                .AddExtension(Unittest.RepeatedNestedEnumExtension, TestAllTypes.Types.NestedEnum.FOO)
                .AddExtension(Unittest.RepeatedSfixed32Extension, 123)
                .AddExtension(Unittest.RepeatedSfixed64Extension, 123)
                .AddExtension(Unittest.RepeatedSint32Extension, 123)
                .AddExtension(Unittest.RepeatedSint64Extension, 123)
                .AddExtension(Unittest.RepeatedStringExtension, "123")
                .AddExtension(Unittest.RepeatedStringPieceExtension, "123")
                .AddExtension(Unittest.RepeatedUint32Extension, 123u)
                .AddExtension(Unittest.RepeatedUint64Extension, 123u)
                ;
            TestAllExtensions msg = builder.Build();

            ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
            Unittest.RegisterAllExtensions(registry);

            TestAllExtensions.Builder copyBuilder = TestAllExtensions.CreateBuilder().MergeFrom(msg.ToByteArray(),
                                                                                                registry);
            TestAllExtensions copy = copyBuilder.Build();

            Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());

            Assert.AreEqual(true, copy.GetExtension(Unittest.DefaultBoolExtension));
            Assert.AreEqual(ByteString.CopyFromUtf8("123"), copy.GetExtension(Unittest.DefaultBytesExtension));
            Assert.AreEqual("123", copy.GetExtension(Unittest.DefaultCordExtension));
            Assert.AreEqual(123, copy.GetExtension(Unittest.DefaultDoubleExtension));
            Assert.AreEqual(123u, copy.GetExtension(Unittest.DefaultFixed32Extension));
            Assert.AreEqual(123u, copy.GetExtension(Unittest.DefaultFixed64Extension));
            Assert.AreEqual(123, copy.GetExtension(Unittest.DefaultFloatExtension));
            Assert.AreEqual(ForeignEnum.FOREIGN_BAZ, copy.GetExtension(Unittest.DefaultForeignEnumExtension));
            Assert.AreEqual(ImportEnum.IMPORT_BAZ, copy.GetExtension(Unittest.DefaultImportEnumExtension));
            Assert.AreEqual(123, copy.GetExtension(Unittest.DefaultInt32Extension));
            Assert.AreEqual(123, copy.GetExtension(Unittest.DefaultInt64Extension));
            Assert.AreEqual(TestAllTypes.Types.NestedEnum.FOO,
                            copy.GetExtension(Unittest.DefaultNestedEnumExtension));
            Assert.AreEqual(123, copy.GetExtension(Unittest.DefaultSfixed32Extension));
            Assert.AreEqual(123, copy.GetExtension(Unittest.DefaultSfixed64Extension));
            Assert.AreEqual(123, copy.GetExtension(Unittest.DefaultSint32Extension));
            Assert.AreEqual(123, copy.GetExtension(Unittest.DefaultSint64Extension));
            Assert.AreEqual("123", copy.GetExtension(Unittest.DefaultStringExtension));
            Assert.AreEqual("123", copy.GetExtension(Unittest.DefaultStringPieceExtension));
            Assert.AreEqual(123u, copy.GetExtension(Unittest.DefaultUint32Extension));
            Assert.AreEqual(123u, copy.GetExtension(Unittest.DefaultUint64Extension));

            Assert.AreEqual(true, copy.GetExtension(Unittest.OptionalBoolExtension));
            Assert.AreEqual(ByteString.CopyFromUtf8("123"), copy.GetExtension(Unittest.OptionalBytesExtension));
            Assert.AreEqual("123", copy.GetExtension(Unittest.OptionalCordExtension));
            Assert.AreEqual(123, copy.GetExtension(Unittest.OptionalDoubleExtension));
            Assert.AreEqual(123u, copy.GetExtension(Unittest.OptionalFixed32Extension));
            Assert.AreEqual(123u, copy.GetExtension(Unittest.OptionalFixed64Extension));
            Assert.AreEqual(123, copy.GetExtension(Unittest.OptionalFloatExtension));
            Assert.AreEqual(ForeignEnum.FOREIGN_BAZ, copy.GetExtension(Unittest.OptionalForeignEnumExtension));
            Assert.AreEqual(ImportEnum.IMPORT_BAZ, copy.GetExtension(Unittest.OptionalImportEnumExtension));
            Assert.AreEqual(123, copy.GetExtension(Unittest.OptionalInt32Extension));
            Assert.AreEqual(123, copy.GetExtension(Unittest.OptionalInt64Extension));
            Assert.AreEqual(TestAllTypes.Types.NestedEnum.FOO,
                            copy.GetExtension(Unittest.OptionalNestedEnumExtension));
            Assert.AreEqual(123, copy.GetExtension(Unittest.OptionalSfixed32Extension));
            Assert.AreEqual(123, copy.GetExtension(Unittest.OptionalSfixed64Extension));
            Assert.AreEqual(123, copy.GetExtension(Unittest.OptionalSint32Extension));
            Assert.AreEqual(123, copy.GetExtension(Unittest.OptionalSint64Extension));
            Assert.AreEqual("123", copy.GetExtension(Unittest.OptionalStringExtension));
            Assert.AreEqual("123", copy.GetExtension(Unittest.OptionalStringPieceExtension));
            Assert.AreEqual(123u, copy.GetExtension(Unittest.OptionalUint32Extension));
            Assert.AreEqual(123u, copy.GetExtension(Unittest.OptionalUint64Extension));

            Assert.AreEqual(true, copy.GetExtension(Unittest.RepeatedBoolExtension, 0));
            Assert.AreEqual(ByteString.CopyFromUtf8("123"),
                            copy.GetExtension(Unittest.RepeatedBytesExtension, 0));
            Assert.AreEqual("123", copy.GetExtension(Unittest.RepeatedCordExtension, 0));
            Assert.AreEqual(123, copy.GetExtension(Unittest.RepeatedDoubleExtension, 0));
            Assert.AreEqual(123u, copy.GetExtension(Unittest.RepeatedFixed32Extension, 0));
            Assert.AreEqual(123u, copy.GetExtension(Unittest.RepeatedFixed64Extension, 0));
            Assert.AreEqual(123, copy.GetExtension(Unittest.RepeatedFloatExtension, 0));
            Assert.AreEqual(ForeignEnum.FOREIGN_BAZ,
                            copy.GetExtension(Unittest.RepeatedForeignEnumExtension, 0));
            Assert.AreEqual(ImportEnum.IMPORT_BAZ, copy.GetExtension(Unittest.RepeatedImportEnumExtension, 0));
            Assert.AreEqual(123, copy.GetExtension(Unittest.RepeatedInt32Extension, 0));
            Assert.AreEqual(123, copy.GetExtension(Unittest.RepeatedInt64Extension, 0));
            Assert.AreEqual(TestAllTypes.Types.NestedEnum.FOO,
                            copy.GetExtension(Unittest.RepeatedNestedEnumExtension, 0));
            Assert.AreEqual(123, copy.GetExtension(Unittest.RepeatedSfixed32Extension, 0));
            Assert.AreEqual(123, copy.GetExtension(Unittest.RepeatedSfixed64Extension, 0));
            Assert.AreEqual(123, copy.GetExtension(Unittest.RepeatedSint32Extension, 0));
            Assert.AreEqual(123, copy.GetExtension(Unittest.RepeatedSint64Extension, 0));
            Assert.AreEqual("123", copy.GetExtension(Unittest.RepeatedStringExtension, 0));
            Assert.AreEqual("123", copy.GetExtension(Unittest.RepeatedStringPieceExtension, 0));
            Assert.AreEqual(123u, copy.GetExtension(Unittest.RepeatedUint32Extension, 0));
            Assert.AreEqual(123u, copy.GetExtension(Unittest.RepeatedUint64Extension, 0));
        }
    }
}