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

namespace Google.ProtocolBuffers
{
    public class ExtendableMessageLiteTest
    {
        //The lite framework does not make this assertion
        //[TestMethod, Ignore, ExpectedException(typeof(ArgumentException))]
        //public void ExtensionWriterInvalidExtension()
        //{
        //    TestPackedExtensionsLite.CreateBuilder()[
        //        UnittestLite.OptionalForeignMessageExtensionLite.DescriptorProtoFile] =
        //        ForeignMessageLite.DefaultInstance;
        //}

        [Test]
        public void ExtensionWriterTestMessages()
        {
            TestAllExtensionsLite.Builder b = TestAllExtensionsLite.CreateBuilder().SetExtension(
                UnittestLite.OptionalForeignMessageExtensionLite,
                ForeignMessageLite.CreateBuilder().SetC(123).Build());
            TestAllExtensionsLite copy, msg = b.Build();

            ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
            UnittestLite.RegisterAllExtensions(registry);

            copy = TestAllExtensionsLite.ParseFrom(msg.ToByteArray(), registry);
            Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());
        }

        [Test]
        public void ExtensionWriterIsInitialized()
        {
            Assert.IsTrue(ForeignMessageLite.DefaultInstance.IsInitialized);
            Assert.IsTrue(TestPackedExtensionsLite.CreateBuilder().IsInitialized);
            Assert.IsTrue(TestAllExtensionsLite.CreateBuilder().SetExtension(
                UnittestLite.OptionalForeignMessageExtensionLite, ForeignMessageLite.DefaultInstance)
                              .IsInitialized);
        }

        [Test]
        public void ExtensionWriterTestSetExtensionLists()
        {
            TestAllExtensionsLite msg, copy;
            TestAllExtensionsLite.Builder builder = TestAllExtensionsLite.CreateBuilder()
                .SetExtension(UnittestLite.RepeatedBoolExtensionLite, new[] {true, false})
                .SetExtension(UnittestLite.RepeatedCordExtensionLite, new[] {"123", "456"})
                .SetExtension(UnittestLite.RepeatedForeignEnumExtensionLite,
                              new[] {ForeignEnumLite.FOREIGN_LITE_BAZ, ForeignEnumLite.FOREIGN_LITE_FOO})
                ;

            msg = builder.Build();
            ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
            UnittestLite.RegisterAllExtensions(registry);

            copy = TestAllExtensionsLite.ParseFrom(msg.ToByteArray(), registry);
            Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());

            Assert.AreEqual(ForeignEnumLite.FOREIGN_LITE_FOO,
                            copy.GetExtension(UnittestLite.RepeatedForeignEnumExtensionLite, 1));
        }

        [Test]
        public void ExtensionWriterTest()
        {
            TestAllExtensionsLite.Builder builder = TestAllExtensionsLite.CreateBuilder()
                .SetExtension(UnittestLite.DefaultBoolExtensionLite, true)
                .SetExtension(UnittestLite.DefaultBytesExtensionLite, ByteString.CopyFromUtf8("123"))
                .SetExtension(UnittestLite.DefaultCordExtensionLite, "123")
                .SetExtension(UnittestLite.DefaultDoubleExtensionLite, 123)
                .SetExtension(UnittestLite.DefaultFixed32ExtensionLite, 123u)
                .SetExtension(UnittestLite.DefaultFixed64ExtensionLite, 123u)
                .SetExtension(UnittestLite.DefaultFloatExtensionLite, 123)
                .SetExtension(UnittestLite.DefaultForeignEnumExtensionLite, ForeignEnumLite.FOREIGN_LITE_BAZ)
                .SetExtension(UnittestLite.DefaultImportEnumExtensionLite, ImportEnumLite.IMPORT_LITE_BAZ)
                .SetExtension(UnittestLite.DefaultInt32ExtensionLite, 123)
                .SetExtension(UnittestLite.DefaultInt64ExtensionLite, 123)
                .SetExtension(UnittestLite.DefaultNestedEnumExtensionLite,
                              TestAllTypesLite.Types.NestedEnum.FOO)
                .SetExtension(UnittestLite.DefaultSfixed32ExtensionLite, 123)
                .SetExtension(UnittestLite.DefaultSfixed64ExtensionLite, 123)
                .SetExtension(UnittestLite.DefaultSint32ExtensionLite, 123)
                .SetExtension(UnittestLite.DefaultSint64ExtensionLite, 123)
                .SetExtension(UnittestLite.DefaultStringExtensionLite, "123")
                .SetExtension(UnittestLite.DefaultStringPieceExtensionLite, "123")
                .SetExtension(UnittestLite.DefaultUint32ExtensionLite, 123u)
                .SetExtension(UnittestLite.DefaultUint64ExtensionLite, 123u)
                //Optional
                .SetExtension(UnittestLite.OptionalBoolExtensionLite, true)
                .SetExtension(UnittestLite.OptionalBytesExtensionLite, ByteString.CopyFromUtf8("123"))
                .SetExtension(UnittestLite.OptionalCordExtensionLite, "123")
                .SetExtension(UnittestLite.OptionalDoubleExtensionLite, 123)
                .SetExtension(UnittestLite.OptionalFixed32ExtensionLite, 123u)
                .SetExtension(UnittestLite.OptionalFixed64ExtensionLite, 123u)
                .SetExtension(UnittestLite.OptionalFloatExtensionLite, 123)
                .SetExtension(UnittestLite.OptionalForeignEnumExtensionLite, ForeignEnumLite.FOREIGN_LITE_BAZ)
                .SetExtension(UnittestLite.OptionalImportEnumExtensionLite, ImportEnumLite.IMPORT_LITE_BAZ)
                .SetExtension(UnittestLite.OptionalInt32ExtensionLite, 123)
                .SetExtension(UnittestLite.OptionalInt64ExtensionLite, 123)
                .SetExtension(UnittestLite.OptionalNestedEnumExtensionLite,
                              TestAllTypesLite.Types.NestedEnum.FOO)
                .SetExtension(UnittestLite.OptionalSfixed32ExtensionLite, 123)
                .SetExtension(UnittestLite.OptionalSfixed64ExtensionLite, 123)
                .SetExtension(UnittestLite.OptionalSint32ExtensionLite, 123)
                .SetExtension(UnittestLite.OptionalSint64ExtensionLite, 123)
                .SetExtension(UnittestLite.OptionalStringExtensionLite, "123")
                .SetExtension(UnittestLite.OptionalStringPieceExtensionLite, "123")
                .SetExtension(UnittestLite.OptionalUint32ExtensionLite, 123u)
                .SetExtension(UnittestLite.OptionalUint64ExtensionLite, 123u)
                //Repeated
                .AddExtension(UnittestLite.RepeatedBoolExtensionLite, true)
                .AddExtension(UnittestLite.RepeatedBytesExtensionLite, ByteString.CopyFromUtf8("123"))
                .AddExtension(UnittestLite.RepeatedCordExtensionLite, "123")
                .AddExtension(UnittestLite.RepeatedDoubleExtensionLite, 123)
                .AddExtension(UnittestLite.RepeatedFixed32ExtensionLite, 123u)
                .AddExtension(UnittestLite.RepeatedFixed64ExtensionLite, 123u)
                .AddExtension(UnittestLite.RepeatedFloatExtensionLite, 123)
                .AddExtension(UnittestLite.RepeatedForeignEnumExtensionLite, ForeignEnumLite.FOREIGN_LITE_BAZ)
                .AddExtension(UnittestLite.RepeatedImportEnumExtensionLite, ImportEnumLite.IMPORT_LITE_BAZ)
                .AddExtension(UnittestLite.RepeatedInt32ExtensionLite, 123)
                .AddExtension(UnittestLite.RepeatedInt64ExtensionLite, 123)
                .AddExtension(UnittestLite.RepeatedNestedEnumExtensionLite,
                              TestAllTypesLite.Types.NestedEnum.FOO)
                .AddExtension(UnittestLite.RepeatedSfixed32ExtensionLite, 123)
                .AddExtension(UnittestLite.RepeatedSfixed64ExtensionLite, 123)
                .AddExtension(UnittestLite.RepeatedSint32ExtensionLite, 123)
                .AddExtension(UnittestLite.RepeatedSint64ExtensionLite, 123)
                .AddExtension(UnittestLite.RepeatedStringExtensionLite, "123")
                .AddExtension(UnittestLite.RepeatedStringPieceExtensionLite, "123")
                .AddExtension(UnittestLite.RepeatedUint32ExtensionLite, 123u)
                .AddExtension(UnittestLite.RepeatedUint64ExtensionLite, 123u)
                ;
            TestAllExtensionsLite msg = builder.Build();

            ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
            UnittestLite.RegisterAllExtensions(registry);

            TestAllExtensionsLite.Builder copyBuilder =
                TestAllExtensionsLite.CreateBuilder().MergeFrom(msg.ToByteArray(), registry);
            TestAllExtensionsLite copy = copyBuilder.Build();

            Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());

            Assert.AreEqual(true, copy.GetExtension(UnittestLite.DefaultBoolExtensionLite));
            Assert.AreEqual(ByteString.CopyFromUtf8("123"),
                            copy.GetExtension(UnittestLite.DefaultBytesExtensionLite));
            Assert.AreEqual("123", copy.GetExtension(UnittestLite.DefaultCordExtensionLite));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.DefaultDoubleExtensionLite));
            Assert.AreEqual(123u, copy.GetExtension(UnittestLite.DefaultFixed32ExtensionLite));
            Assert.AreEqual(123u, copy.GetExtension(UnittestLite.DefaultFixed64ExtensionLite));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.DefaultFloatExtensionLite));
            Assert.AreEqual(ForeignEnumLite.FOREIGN_LITE_BAZ,
                            copy.GetExtension(UnittestLite.DefaultForeignEnumExtensionLite));
            Assert.AreEqual(ImportEnumLite.IMPORT_LITE_BAZ,
                            copy.GetExtension(UnittestLite.DefaultImportEnumExtensionLite));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.DefaultInt32ExtensionLite));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.DefaultInt64ExtensionLite));
            Assert.AreEqual(TestAllTypesLite.Types.NestedEnum.FOO,
                            copy.GetExtension(UnittestLite.DefaultNestedEnumExtensionLite));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.DefaultSfixed32ExtensionLite));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.DefaultSfixed64ExtensionLite));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.DefaultSint32ExtensionLite));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.DefaultSint64ExtensionLite));
            Assert.AreEqual("123", copy.GetExtension(UnittestLite.DefaultStringExtensionLite));
            Assert.AreEqual("123", copy.GetExtension(UnittestLite.DefaultStringPieceExtensionLite));
            Assert.AreEqual(123u, copy.GetExtension(UnittestLite.DefaultUint32ExtensionLite));
            Assert.AreEqual(123u, copy.GetExtension(UnittestLite.DefaultUint64ExtensionLite));

            Assert.AreEqual(true, copy.GetExtension(UnittestLite.OptionalBoolExtensionLite));
            Assert.AreEqual(ByteString.CopyFromUtf8("123"),
                            copy.GetExtension(UnittestLite.OptionalBytesExtensionLite));
            Assert.AreEqual("123", copy.GetExtension(UnittestLite.OptionalCordExtensionLite));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.OptionalDoubleExtensionLite));
            Assert.AreEqual(123u, copy.GetExtension(UnittestLite.OptionalFixed32ExtensionLite));
            Assert.AreEqual(123u, copy.GetExtension(UnittestLite.OptionalFixed64ExtensionLite));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.OptionalFloatExtensionLite));
            Assert.AreEqual(ForeignEnumLite.FOREIGN_LITE_BAZ,
                            copy.GetExtension(UnittestLite.OptionalForeignEnumExtensionLite));
            Assert.AreEqual(ImportEnumLite.IMPORT_LITE_BAZ,
                            copy.GetExtension(UnittestLite.OptionalImportEnumExtensionLite));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.OptionalInt32ExtensionLite));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.OptionalInt64ExtensionLite));
            Assert.AreEqual(TestAllTypesLite.Types.NestedEnum.FOO,
                            copy.GetExtension(UnittestLite.OptionalNestedEnumExtensionLite));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.OptionalSfixed32ExtensionLite));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.OptionalSfixed64ExtensionLite));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.OptionalSint32ExtensionLite));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.OptionalSint64ExtensionLite));
            Assert.AreEqual("123", copy.GetExtension(UnittestLite.OptionalStringExtensionLite));
            Assert.AreEqual("123", copy.GetExtension(UnittestLite.OptionalStringPieceExtensionLite));
            Assert.AreEqual(123u, copy.GetExtension(UnittestLite.OptionalUint32ExtensionLite));
            Assert.AreEqual(123u, copy.GetExtension(UnittestLite.OptionalUint64ExtensionLite));

            Assert.AreEqual(true, copy.GetExtension(UnittestLite.RepeatedBoolExtensionLite, 0));
            Assert.AreEqual(ByteString.CopyFromUtf8("123"),
                            copy.GetExtension(UnittestLite.RepeatedBytesExtensionLite, 0));
            Assert.AreEqual("123", copy.GetExtension(UnittestLite.RepeatedCordExtensionLite, 0));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.RepeatedDoubleExtensionLite, 0));
            Assert.AreEqual(123u, copy.GetExtension(UnittestLite.RepeatedFixed32ExtensionLite, 0));
            Assert.AreEqual(123u, copy.GetExtension(UnittestLite.RepeatedFixed64ExtensionLite, 0));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.RepeatedFloatExtensionLite, 0));
            Assert.AreEqual(ForeignEnumLite.FOREIGN_LITE_BAZ,
                            copy.GetExtension(UnittestLite.RepeatedForeignEnumExtensionLite, 0));
            Assert.AreEqual(ImportEnumLite.IMPORT_LITE_BAZ,
                            copy.GetExtension(UnittestLite.RepeatedImportEnumExtensionLite, 0));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.RepeatedInt32ExtensionLite, 0));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.RepeatedInt64ExtensionLite, 0));
            Assert.AreEqual(TestAllTypesLite.Types.NestedEnum.FOO,
                            copy.GetExtension(UnittestLite.RepeatedNestedEnumExtensionLite, 0));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.RepeatedSfixed32ExtensionLite, 0));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.RepeatedSfixed64ExtensionLite, 0));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.RepeatedSint32ExtensionLite, 0));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.RepeatedSint64ExtensionLite, 0));
            Assert.AreEqual("123", copy.GetExtension(UnittestLite.RepeatedStringExtensionLite, 0));
            Assert.AreEqual("123", copy.GetExtension(UnittestLite.RepeatedStringPieceExtensionLite, 0));
            Assert.AreEqual(123u, copy.GetExtension(UnittestLite.RepeatedUint32ExtensionLite, 0));
            Assert.AreEqual(123u, copy.GetExtension(UnittestLite.RepeatedUint64ExtensionLite, 0));
        }

        private TestPackedExtensionsLite BuildPackedExtensions()
        {
            TestPackedExtensionsLite.Builder builder = TestPackedExtensionsLite.CreateBuilder()
                   .AddExtension(UnittestLite.PackedBoolExtensionLite, true)
                   .AddExtension(UnittestLite.PackedDoubleExtensionLite, 123)
                   .AddExtension(UnittestLite.PackedFixed32ExtensionLite, 123u)
                   .AddExtension(UnittestLite.PackedFixed64ExtensionLite, 123u)
                   .AddExtension(UnittestLite.PackedFloatExtensionLite, 123)
                   .AddExtension(UnittestLite.PackedInt32ExtensionLite, 123)
                   .AddExtension(UnittestLite.PackedInt64ExtensionLite, 123)
                   .AddExtension(UnittestLite.PackedSfixed32ExtensionLite, 123)
                   .AddExtension(UnittestLite.PackedSfixed64ExtensionLite, 123)
                   .AddExtension(UnittestLite.PackedSint32ExtensionLite, 123)
                   .AddExtension(UnittestLite.PackedSint64ExtensionLite, 123)
                   .AddExtension(UnittestLite.PackedUint32ExtensionLite, 123u)
                   .AddExtension(UnittestLite.PackedUint64ExtensionLite, 123u)
                   .AddExtension(UnittestLite.PackedBoolExtensionLite, true)
                   .AddExtension(UnittestLite.PackedDoubleExtensionLite, 123)
                   .AddExtension(UnittestLite.PackedFixed32ExtensionLite, 123u)
                   .AddExtension(UnittestLite.PackedFixed64ExtensionLite, 123u)
                   .AddExtension(UnittestLite.PackedFloatExtensionLite, 123)
                   .AddExtension(UnittestLite.PackedInt32ExtensionLite, 123)
                   .AddExtension(UnittestLite.PackedInt64ExtensionLite, 123)
                   .AddExtension(UnittestLite.PackedSfixed32ExtensionLite, 123)
                   .AddExtension(UnittestLite.PackedSfixed64ExtensionLite, 123)
                   .AddExtension(UnittestLite.PackedSint32ExtensionLite, 123)
                   .AddExtension(UnittestLite.PackedSint64ExtensionLite, 123)
                   .AddExtension(UnittestLite.PackedUint32ExtensionLite, 123u)
                   .AddExtension(UnittestLite.PackedUint64ExtensionLite, 123u);

            TestPackedExtensionsLite msg = builder.Build();
            return msg;
        }

        private void AssertPackedExtensions(TestPackedExtensionsLite copy)
        {
            Assert.AreEqual(true, copy.GetExtension(UnittestLite.PackedBoolExtensionLite, 0));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.PackedDoubleExtensionLite, 0));
            Assert.AreEqual(123u, copy.GetExtension(UnittestLite.PackedFixed32ExtensionLite, 0));
            Assert.AreEqual(123u, copy.GetExtension(UnittestLite.PackedFixed64ExtensionLite, 0));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.PackedFloatExtensionLite, 0));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.PackedInt32ExtensionLite, 0));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.PackedInt64ExtensionLite, 0));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.PackedSfixed32ExtensionLite, 0));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.PackedSfixed64ExtensionLite, 0));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.PackedSint32ExtensionLite, 0));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.PackedSint64ExtensionLite, 0));
            Assert.AreEqual(123u, copy.GetExtension(UnittestLite.PackedUint32ExtensionLite, 0));
            Assert.AreEqual(123u, copy.GetExtension(UnittestLite.PackedUint64ExtensionLite, 0));

            Assert.AreEqual(true, copy.GetExtension(UnittestLite.PackedBoolExtensionLite, 1));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.PackedDoubleExtensionLite, 1));
            Assert.AreEqual(123u, copy.GetExtension(UnittestLite.PackedFixed32ExtensionLite, 1));
            Assert.AreEqual(123u, copy.GetExtension(UnittestLite.PackedFixed64ExtensionLite, 1));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.PackedFloatExtensionLite, 1));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.PackedInt32ExtensionLite, 1));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.PackedInt64ExtensionLite, 1));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.PackedSfixed32ExtensionLite, 1));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.PackedSfixed64ExtensionLite, 1));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.PackedSint32ExtensionLite, 1));
            Assert.AreEqual(123, copy.GetExtension(UnittestLite.PackedSint64ExtensionLite, 1));
            Assert.AreEqual(123u, copy.GetExtension(UnittestLite.PackedUint32ExtensionLite, 1));
            Assert.AreEqual(123u, copy.GetExtension(UnittestLite.PackedUint64ExtensionLite, 1));
        }

        [Test]
        public void ExtensionWriterTestPacked()
        {
            TestPackedExtensionsLite msg = BuildPackedExtensions();

            ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
            UnittestLite.RegisterAllExtensions(registry);

            TestPackedExtensionsLite.Builder copyBuilder =
                TestPackedExtensionsLite.CreateBuilder().MergeFrom(msg.ToByteArray(), registry);
            TestPackedExtensionsLite copy = copyBuilder.Build();

            Assert.AreEqual(msg.ToByteArray(), copy.ToByteArray());

            AssertPackedExtensions(copy);
        }

        [Test]
        public void TestUnpackedAndPackedExtensions()
        {
            TestPackedExtensionsLite original = BuildPackedExtensions();
            AssertPackedExtensions(original);

            ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
            UnittestLite.RegisterAllExtensions(registry);
            UnittestExtrasLite.RegisterAllExtensions(registry);

            TestUnpackedExtensionsLite unpacked = TestUnpackedExtensionsLite.ParseFrom(original.ToByteArray(), registry);

            TestPackedExtensionsLite packed = TestPackedExtensionsLite.ParseFrom(unpacked.ToByteArray(), registry);

            Assert.AreEqual(original, packed);
            Assert.AreEqual(original.ToByteArray(), packed.ToByteArray());
            AssertPackedExtensions(packed);
        }

        [Test]
        public void TestUnpackedFromPackedInput()
        {
            byte[] packedData = BuildPackedExtensions().ToByteArray();

            TestUnpackedTypesLite unpacked = TestUnpackedTypesLite.ParseFrom(packedData);
            TestPackedTypesLite packed = TestPackedTypesLite.ParseFrom(unpacked.ToByteArray());
            Assert.AreEqual(packedData, packed.ToByteArray());
            
            unpacked = TestUnpackedTypesLite.ParseFrom(packed.ToByteArray());

            ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
            UnittestLite.RegisterAllExtensions(registry);
            AssertPackedExtensions(TestPackedExtensionsLite.ParseFrom(unpacked.ToByteArray(), registry));
        }
    }
}