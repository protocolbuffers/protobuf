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
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers
{
    public class GeneratedMessageTest
    {
        private readonly ReflectionTester reflectionTester;
        private readonly ReflectionTester extensionsReflectionTester;

        public GeneratedMessageTest()
        {
            reflectionTester = ReflectionTester.CreateTestAllTypesInstance();
            extensionsReflectionTester = ReflectionTester.CreateTestAllExtensionsInstance();
        }

        [Test]
        public void RepeatedAddPrimitiveBeforeBuild()
        {
            TestAllTypes message = new TestAllTypes.Builder {RepeatedInt32List = {1, 2, 3}}.Build();
            TestUtil.AssertEqual(new int[] {1, 2, 3}, message.RepeatedInt32List);
        }

        [Test]
        public void AddPrimitiveFailsAfterBuild()
        {
            TestAllTypes.Builder builder = new TestAllTypes.Builder();
            IList<int> list = builder.RepeatedInt32List;
            list.Add(1); // Fine
            builder.Build();

            Assert.Throws<NotSupportedException>(() => list.Add(2));
        }

        [Test]
        public void RepeatedAddMessageBeforeBuild()
        {
            TestAllTypes message = new TestAllTypes.Builder
                                       {
                                           RepeatedNestedMessageList =
                                               {new TestAllTypes.Types.NestedMessage.Builder {Bb = 10}.Build()}
                                       }.Build();
            Assert.AreEqual(1, message.RepeatedNestedMessageCount);
            Assert.AreEqual(10, message.RepeatedNestedMessageList[0].Bb);
        }

        [Test]
        public void AddMessageFailsAfterBuild()
        {
            TestAllTypes.Builder builder = new TestAllTypes.Builder();
            IList<TestAllTypes.Types.NestedMessage> list = builder.RepeatedNestedMessageList;
            builder.Build();

            Assert.Throws<NotSupportedException>(() => list.Add(new TestAllTypes.Types.NestedMessage.Builder { Bb = 10 }.Build()));
        }

        [Test]
        public void DefaultInstance()
        {
            Assert.AreSame(TestAllTypes.DefaultInstance, TestAllTypes.DefaultInstance.DefaultInstanceForType);
            Assert.AreSame(TestAllTypes.DefaultInstance, TestAllTypes.CreateBuilder().DefaultInstanceForType);
        }

        [Test]
        public void Accessors()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            TestUtil.SetAllFields(builder);
            TestAllTypes message = builder.Build();
            TestUtil.AssertAllFieldsSet(message);
        }

        [Test]
        public void SettersRejectNull()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            Assert.Throws<ArgumentNullException>(() => builder.SetOptionalString(null));
            Assert.Throws<ArgumentNullException>(() => builder.SetOptionalBytes(null));
            Assert.Throws<ArgumentNullException>(
                () => builder.SetOptionalNestedMessage((TestAllTypes.Types.NestedMessage) null));
            Assert.Throws<ArgumentNullException>(
                () => builder.SetOptionalNestedMessage((TestAllTypes.Types.NestedMessage.Builder) null));
            Assert.Throws<ArgumentNullException>(() => builder.AddRepeatedString(null));
            Assert.Throws<ArgumentNullException>(() => builder.AddRepeatedBytes(null));
            Assert.Throws<ArgumentNullException>(
                () => builder.AddRepeatedNestedMessage((TestAllTypes.Types.NestedMessage) null));
            Assert.Throws<ArgumentNullException>(
                () => builder.AddRepeatedNestedMessage((TestAllTypes.Types.NestedMessage.Builder) null));
        }

        [Test]
        public void RepeatedSetters()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            TestUtil.SetAllFields(builder);
            TestUtil.ModifyRepeatedFields(builder);
            TestAllTypes message = builder.Build();
            TestUtil.AssertRepeatedFieldsModified(message);
        }

        [Test]
        public void RepeatedAppend()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();

            builder.AddRangeRepeatedInt32(new int[] {1, 2, 3, 4});
            builder.AddRangeRepeatedForeignEnum((new ForeignEnum[] {ForeignEnum.FOREIGN_BAZ}));

            ForeignMessage foreignMessage = ForeignMessage.CreateBuilder().SetC(12).Build();
            builder.AddRangeRepeatedForeignMessage(new ForeignMessage[] {foreignMessage});

            TestAllTypes message = builder.Build();
            TestUtil.AssertEqual(message.RepeatedInt32List, new int[] {1, 2, 3, 4});
            TestUtil.AssertEqual(message.RepeatedForeignEnumList, new ForeignEnum[] {ForeignEnum.FOREIGN_BAZ});
            Assert.AreEqual(1, message.RepeatedForeignMessageCount);
            Assert.AreEqual(12, message.GetRepeatedForeignMessage(0).C);
        }

        [Test]
        public void RepeatedAppendRejectsNull()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();

            ForeignMessage foreignMessage = ForeignMessage.CreateBuilder().SetC(12).Build();
            Assert.Throws<ArgumentNullException>(
                () => builder.AddRangeRepeatedForeignMessage(new[] {foreignMessage, null}));
            Assert.Throws<ArgumentNullException>(() => builder.AddRangeRepeatedForeignMessage(null));
            Assert.Throws<ArgumentNullException>(() => builder.AddRangeRepeatedForeignEnum(null));
            Assert.Throws<ArgumentNullException>(() => builder.AddRangeRepeatedString(new[] {"one", null}));
            Assert.Throws<ArgumentNullException>(
                () => builder.AddRangeRepeatedBytes(new[] {TestUtil.ToBytes("one"), null}));
        }

        [Test]
        public void SettingForeignMessageUsingBuilder()
        {
            TestAllTypes message = TestAllTypes.CreateBuilder()
                // Pass builder for foreign message instance.
                .SetOptionalForeignMessage(ForeignMessage.CreateBuilder().SetC(123))
                .Build();
            TestAllTypes expectedMessage = TestAllTypes.CreateBuilder()
                // Create expected version passing foreign message instance explicitly.
                .SetOptionalForeignMessage(ForeignMessage.CreateBuilder().SetC(123).Build())
                .Build();
            Assert.AreEqual(expectedMessage, message);
        }

        [Test]
        public void SettingRepeatedForeignMessageUsingBuilder()
        {
            TestAllTypes message = TestAllTypes.CreateBuilder()
                // Pass builder for foreign message instance.
                .AddRepeatedForeignMessage(ForeignMessage.CreateBuilder().SetC(456))
                .Build();
            TestAllTypes expectedMessage = TestAllTypes.CreateBuilder()
                // Create expected version passing foreign message instance explicitly.
                .AddRepeatedForeignMessage(ForeignMessage.CreateBuilder().SetC(456).Build())
                .Build();
            Assert.AreEqual(expectedMessage, message);
        }

        [Test]
        public void SettingRepeatedValuesUsingRangeInCollectionInitializer()
        {
            int[] values = {1, 2, 3};
            TestAllTypes message = new TestAllTypes.Builder
                                       {
                                           RepeatedSint32List = {values}
                                       }.Build();
            Assert.IsTrue(Lists.Equals(values, message.RepeatedSint32List));
        }

        [Test]
        public void SettingRepeatedValuesUsingIndividualValuesInCollectionInitializer()
        {
            TestAllTypes message = new TestAllTypes.Builder
                                       {
                                           RepeatedSint32List = {6, 7}
                                       }.Build();
            Assert.IsTrue(Lists.Equals(new int[] {6, 7}, message.RepeatedSint32List));
        }

        [Test]
        public void Defaults()
        {
            TestUtil.AssertClear(TestAllTypes.DefaultInstance);
            TestUtil.AssertClear(TestAllTypes.CreateBuilder().Build());

            Assert.AreEqual("\u1234", TestExtremeDefaultValues.DefaultInstance.Utf8String);
        }

        [Test]
        public void ReflectionGetters()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            TestUtil.SetAllFields(builder);
            TestAllTypes message = builder.Build();
            reflectionTester.AssertAllFieldsSetViaReflection(message);
        }

        [Test]
        public void ReflectionSetters()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            reflectionTester.SetAllFieldsViaReflection(builder);
            TestAllTypes message = builder.Build();
            TestUtil.AssertAllFieldsSet(message);
        }

        [Test]
        public void ReflectionClear()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            reflectionTester.SetAllFieldsViaReflection(builder);
            reflectionTester.ClearAllFieldsViaReflection(builder);
            TestAllTypes message = builder.Build();
            TestUtil.AssertClear(message);
        }

        [Test]
        public void ReflectionSettersRejectNull()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            reflectionTester.AssertReflectionSettersRejectNull(builder);
        }

        [Test]
        public void ReflectionRepeatedSetters()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            reflectionTester.SetAllFieldsViaReflection(builder);
            reflectionTester.ModifyRepeatedFieldsViaReflection(builder);
            TestAllTypes message = builder.Build();
            TestUtil.AssertRepeatedFieldsModified(message);
        }

        [Test]
        public void TestReflectionRepeatedSettersRejectNull()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            reflectionTester.AssertReflectionRepeatedSettersRejectNull(builder);
        }

        [Test]
        public void ReflectionDefaults()
        {
            TestUtil.TestInMultipleCultures(() =>
                                                {
                                                    reflectionTester.AssertClearViaReflection(
                                                        TestAllTypes.DefaultInstance);
                                                    reflectionTester.AssertClearViaReflection(
                                                        TestAllTypes.CreateBuilder().Build());
                                                });
        }

        [Test]
        public void ReflectionGetOneof()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            reflectionTester.SetAllFieldsViaReflection(builder);
            Descriptors.OneofDescriptor oneof = TestAllTypes.Descriptor.Oneofs[0];
            Descriptors.FieldDescriptor field = TestAllTypes.Descriptor.FindFieldByName("oneof_bytes");
            Assert.AreSame(field, builder.OneofFieldDescriptor(oneof));
        }

        [Test]
        public void ReflectionClearOneof()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            reflectionTester.SetAllFieldsViaReflection(builder);
            OneofDescriptor oneof = TestAllTypes.Descriptor.Oneofs[0];
            FieldDescriptor field = TestAllTypes.Descriptor.FindFieldByName("oneof_bytes");

            Assert.IsTrue(builder.HasOneof(oneof));
            Assert.IsTrue(builder.HasField(field));
            builder.ClearOneof(oneof);
            Assert.IsFalse(builder.HasOneof(oneof));
            Assert.IsFalse(builder.HasField(field));
        }

        // =================================================================
        // Extensions.

        [Test]
        public void ExtensionAccessors()
        {
            TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
            TestUtil.SetAllExtensions(builder);
            TestAllExtensions message = builder.Build();
            TestUtil.AssertAllExtensionsSet(message);
        }

        [Test]
        public void ExtensionRepeatedSetters()
        {
            TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
            TestUtil.SetAllExtensions(builder);
            TestUtil.ModifyRepeatedExtensions(builder);
            TestAllExtensions message = builder.Build();
            TestUtil.AssertRepeatedExtensionsModified(message);
        }

        [Test]
        public void ExtensionDefaults()
        {
            TestUtil.AssertExtensionsClear(TestAllExtensions.DefaultInstance);
            TestUtil.AssertExtensionsClear(TestAllExtensions.CreateBuilder().Build());
        }

        [Test]
        public void ExtensionReflectionGetters()
        {
            TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
            TestUtil.SetAllExtensions(builder);
            TestAllExtensions message = builder.Build();
            extensionsReflectionTester.AssertAllFieldsSetViaReflection(message);
        }

        [Test]
        public void ExtensionReflectionSetters()
        {
            TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
            extensionsReflectionTester.SetAllFieldsViaReflection(builder);
            TestAllExtensions message = builder.Build();
            TestUtil.AssertAllExtensionsSet(message);
        }

        [Test]
        public void ExtensionReflectionSettersRejectNull()
        {
            TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
            extensionsReflectionTester.AssertReflectionSettersRejectNull(builder);
        }

        [Test]
        public void ExtensionReflectionRepeatedSetters()
        {
            TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
            extensionsReflectionTester.SetAllFieldsViaReflection(builder);
            extensionsReflectionTester.ModifyRepeatedFieldsViaReflection(builder);
            TestAllExtensions message = builder.Build();
            TestUtil.AssertRepeatedExtensionsModified(message);
        }

        [Test]
        public void ExtensionReflectionRepeatedSettersRejectNull()
        {
            TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
            extensionsReflectionTester.AssertReflectionRepeatedSettersRejectNull(builder);
        }

        [Test]
        public void ExtensionReflectionDefaults()
        {
            TestUtil.TestInMultipleCultures(() =>
                                                {
                                                    extensionsReflectionTester.AssertClearViaReflection(
                                                        TestAllExtensions.DefaultInstance);
                                                    extensionsReflectionTester.AssertClearViaReflection(
                                                        TestAllExtensions.CreateBuilder().Build());
                                                });
        }

        [Test]
        public void ClearExtension()
        {
            // ClearExtension() is not actually used in TestUtil, so try it manually.
            Assert.IsFalse(TestAllExtensions.CreateBuilder()
                               .SetExtension(Unittest.OptionalInt32Extension, 1)
                               .ClearExtension(Unittest.OptionalInt32Extension)
                               .HasExtension(Unittest.OptionalInt32Extension));
            Assert.AreEqual(0, TestAllExtensions.CreateBuilder()
                                   .AddExtension(Unittest.RepeatedInt32Extension, 1)
                                   .ClearExtension(Unittest.RepeatedInt32Extension)
                                   .GetExtensionCount(Unittest.RepeatedInt32Extension));
        }

        [Test]
        public void ExtensionMergeFrom()
        {
            TestAllExtensions original = TestAllExtensions.CreateBuilder()
                .SetExtension(Unittest.OptionalInt32Extension, 1).Build();
            TestAllExtensions merged =
                TestAllExtensions.CreateBuilder().MergeFrom(original).Build();
            Assert.IsTrue((merged.HasExtension(Unittest.OptionalInt32Extension)));
            Assert.AreEqual(1, (int) merged.GetExtension(Unittest.OptionalInt32Extension));
        }

        /* Removed multiple files option for the moment
    [Test]
    public void MultipleFilesOption() {
      // We mostly just want to check that things compile.
      MessageWithNoOuter message = MessageWithNoOuter.CreateBuilder()
          .SetNested(MessageWithNoOuter.Types.NestedMessage.CreateBuilder().SetI(1))
          .AddForeign(TestAllTypes.CreateBuilder().SetOptionalInt32(1))
          .SetNestedEnum(MessageWithNoOuter.Types.NestedEnum.BAZ)
          .SetForeignEnum(EnumWithNoOuter.BAR)
          .Build();
      Assert.AreEqual(message, MessageWithNoOuter.ParseFrom(message.ToByteString()));

      Assert.AreEqual(MultiFileProto.DescriptorProtoFile, MessageWithNoOuter.DescriptorProtoFile.File);

      FieldDescriptor field = MessageWithNoOuter.DescriptorProtoFile.FindDescriptor<FieldDescriptor>("foreign_enum");
      Assert.AreEqual(MultiFileProto.DescriptorProtoFile.FindTypeByName<EnumDescriptor>("EnumWithNoOuter")
        .FindValueByNumber((int)EnumWithNoOuter.BAR), message[field]);

      Assert.AreEqual(MultiFileProto.DescriptorProtoFile, ServiceWithNoOuter.DescriptorProtoFile.File);

      Assert.IsFalse(TestAllExtensions.DefaultInstance.HasExtension(MultiFileProto.ExtensionWithOuter));
    }*/

        [Test]
        public void OptionalFieldWithRequiredSubfieldsOptimizedForSize()
        {
            TestOptionalOptimizedForSize message = TestOptionalOptimizedForSize.DefaultInstance;
            Assert.IsTrue(message.IsInitialized);

            message = TestOptionalOptimizedForSize.CreateBuilder().SetO(
                TestRequiredOptimizedForSize.CreateBuilder().BuildPartial()
                ).BuildPartial();
            Assert.IsFalse(message.IsInitialized);

            message = TestOptionalOptimizedForSize.CreateBuilder().SetO(
                TestRequiredOptimizedForSize.CreateBuilder().SetX(5).BuildPartial()
                ).BuildPartial();
            Assert.IsTrue(message.IsInitialized);
        }

        [Test]
        public void OptimizedForSizeMergeUsesAllFieldsFromTarget()
        {
            TestOptimizedForSize withFieldSet = new TestOptimizedForSize.Builder {I = 10}.Build();
            TestOptimizedForSize.Builder builder = new TestOptimizedForSize.Builder();
            builder.MergeFrom(withFieldSet);
            TestOptimizedForSize built = builder.Build();
            Assert.AreEqual(10, built.I);
        }

        [Test]
        public void UninitializedExtensionInOptimizedForSizeMakesMessageUninitialized()
        {
            TestOptimizedForSize.Builder builder = new TestOptimizedForSize.Builder();
            builder.SetExtension(TestOptimizedForSize.TestExtension2,
                                 new TestRequiredOptimizedForSize.Builder().BuildPartial());
            Assert.IsFalse(builder.IsInitialized);
            Assert.IsFalse(builder.BuildPartial().IsInitialized);

            builder = new TestOptimizedForSize.Builder();
            builder.SetExtension(TestOptimizedForSize.TestExtension2,
                                 new TestRequiredOptimizedForSize.Builder {X = 10}.BuildPartial());
            Assert.IsTrue(builder.IsInitialized);
            Assert.IsTrue(builder.BuildPartial().IsInitialized);
        }

        [Test]
        public void ToBuilder()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            TestUtil.SetAllFields(builder);
            TestAllTypes message = builder.Build();
            TestUtil.AssertAllFieldsSet(message.ToBuilder().Build());
        }

        [Test]
        public void FieldConstantValues()
        {
            Assert.AreEqual(TestAllTypes.Types.NestedMessage.BbFieldNumber, 1);
            Assert.AreEqual(TestAllTypes.OptionalInt32FieldNumber, 1);
            Assert.AreEqual(TestAllTypes.OptionalGroupFieldNumber, 16);
            Assert.AreEqual(TestAllTypes.OptionalNestedMessageFieldNumber, 18);
            Assert.AreEqual(TestAllTypes.OptionalNestedEnumFieldNumber, 21);
            Assert.AreEqual(TestAllTypes.RepeatedInt32FieldNumber, 31);
            Assert.AreEqual(TestAllTypes.RepeatedGroupFieldNumber, 46);
            Assert.AreEqual(TestAllTypes.RepeatedNestedMessageFieldNumber, 48);
            Assert.AreEqual(TestAllTypes.RepeatedNestedEnumFieldNumber, 51);
        }

        [Test]
        public void ExtensionConstantValues()
        {
            Assert.AreEqual(TestRequired.SingleFieldNumber, 1000);
            Assert.AreEqual(TestRequired.MultiFieldNumber, 1001);
            Assert.AreEqual(Unittest.OptionalInt32ExtensionFieldNumber, 1);
            Assert.AreEqual(Unittest.OptionalGroupExtensionFieldNumber, 16);
            Assert.AreEqual(Unittest.OptionalNestedMessageExtensionFieldNumber, 18);
            Assert.AreEqual(Unittest.OptionalNestedEnumExtensionFieldNumber, 21);
            Assert.AreEqual(Unittest.RepeatedInt32ExtensionFieldNumber, 31);
            Assert.AreEqual(Unittest.RepeatedGroupExtensionFieldNumber, 46);
            Assert.AreEqual(Unittest.RepeatedNestedMessageExtensionFieldNumber, 48);
            Assert.AreEqual(Unittest.RepeatedNestedEnumExtensionFieldNumber, 51);
        }

        [Test]
        public void EmptyPackedValue()
        {
            TestPackedTypes empty = new TestPackedTypes.Builder().Build();
            Assert.AreEqual(0, empty.SerializedSize);
        }

        // oneof tests
        [Test]
        public void TestOneofEnumCase()
        {
            TestOneof2 message = TestOneof2.CreateBuilder()
                .SetFooInt(123).SetFooString("foo").SetFooCord("bar").Build();
            TestUtil.AssertAtMostOneFieldSetOneof(message);
        }

        [Test]
        public void TestClearOneof()
        {
            TestOneof2.Builder builder = TestOneof2.CreateBuilder().SetFooInt(123);
            Assert.AreEqual(TestOneof2.FooOneofCase.FooInt, builder.FooCase);
            builder.ClearFoo();
            Assert.AreEqual(TestOneof2.FooOneofCase.None, builder.FooCase);
        }

        [Test]
        public void TestSetOneofClearsOthers()
        {
            TestOneof2.Builder builder = TestOneof2.CreateBuilder();
            TestOneof2 message =
                builder.SetFooInt(123).SetFooString("foo").Build();
            Assert.IsTrue(message.HasFooString);
            TestUtil.AssertAtMostOneFieldSetOneof(message);

            message = builder.SetFooCord("bar").Build();
            Assert.IsTrue(message.HasFooCord);
            TestUtil.AssertAtMostOneFieldSetOneof(message);

            message = builder.SetFooStringPiece("baz").Build();
            Assert.IsTrue(message.HasFooStringPiece);
            TestUtil.AssertAtMostOneFieldSetOneof(message);

            message = builder.SetFooBytes(TestUtil.ToBytes("qux")).Build();
            Assert.IsTrue(message.HasFooBytes);
            TestUtil.AssertAtMostOneFieldSetOneof(message);

            message = builder.SetFooEnum(TestOneof2.Types.NestedEnum.FOO).Build();
            Assert.IsTrue(message.HasFooEnum);
            TestUtil.AssertAtMostOneFieldSetOneof(message);

            message = builder.SetFooMessage(
                TestOneof2.Types.NestedMessage.CreateBuilder().SetQuxInt(234).Build()).Build();
            Assert.IsTrue(message.HasFooMessage);
            TestUtil.AssertAtMostOneFieldSetOneof(message);

            message = builder.SetFooInt(123).Build();
            Assert.IsTrue(message.HasFooInt);
            TestUtil.AssertAtMostOneFieldSetOneof(message);
        }

        [Test]
        public void TestOneofTypes_Primitive()
        {
            TestOneof2.Builder builder = TestOneof2.CreateBuilder();
            Assert.AreEqual(builder.FooInt, 0);
            Assert.IsFalse(builder.HasFooInt);
            Assert.IsTrue(builder.SetFooInt(123).HasFooInt);
            Assert.AreEqual(builder.FooInt, 123);
            TestOneof2 message = builder.BuildPartial();
            Assert.IsTrue(message.HasFooInt);
            Assert.AreEqual(message.FooInt, 123);

            Assert.IsFalse(builder.ClearFooInt().HasFooInt);
            TestOneof2 message2 = builder.Build();
            Assert.IsFalse(message2.HasFooInt);
            Assert.AreEqual(message2.FooInt, 0);
        }

        [Test]
        public void TestOneofTypes_Enum()
        {
            TestOneof2.Builder builder = TestOneof2.CreateBuilder();
            Assert.AreEqual(builder.FooEnum, TestOneof2.Types.NestedEnum.FOO);
            Assert.IsTrue(builder.SetFooEnum(TestOneof2.Types.NestedEnum.BAR).HasFooEnum);
            Assert.AreEqual(builder.FooEnum, TestOneof2.Types.NestedEnum.BAR);
            TestOneof2 message = builder.BuildPartial();
            Assert.IsTrue(message.HasFooEnum);
            Assert.AreEqual(message.FooEnum, TestOneof2.Types.NestedEnum.BAR);

            Assert.IsFalse(builder.ClearFooEnum().HasFooEnum);
            TestOneof2 message2 = builder.Build();
            Assert.IsFalse(message2.HasFooEnum);
            Assert.AreEqual(message2.FooEnum, TestOneof2.Types.NestedEnum.FOO);
        }

        [Test]
        public void TestOneofTypes_String()
        {
            TestOneof2.Builder builder = TestOneof2.CreateBuilder();
            Assert.AreEqual(builder.FooString, "");
            Assert.IsTrue(builder.SetFooString("foo").HasFooString);
            Assert.AreEqual(builder.FooString, "foo");
            TestOneof2 message = builder.BuildPartial();
            Assert.IsTrue(message.HasFooString);
            Assert.AreEqual(message.FooString, "foo");

            Assert.IsFalse(builder.ClearFooString().HasFooString);
            TestOneof2 message2 = builder.Build();
            Assert.IsFalse(message2.HasFooString);
            Assert.AreEqual(message2.FooString, "");

            builder.SetFooInt(123);
            Assert.AreEqual(builder.FooString, "");
            Assert.AreEqual(builder.FooInt, 123);
            message = builder.Build();
            Assert.AreEqual(message.FooString, "");
            Assert.AreEqual(message.FooInt, 123);
        }

        [Test]
        public void TestOneofTypes_Message()
        {
            TestOneof2.Builder builder = TestOneof2.CreateBuilder();
            Assert.AreEqual(builder.FooMessage.QuxInt, 0);
            builder.SetFooMessage(TestOneof2.Types.NestedMessage.CreateBuilder().SetQuxInt(234).Build());
            Assert.IsTrue(builder.HasFooMessage);
            Assert.AreEqual(builder.FooMessage.QuxInt, 234);
            TestOneof2 message = builder.BuildPartial();
            Assert.IsTrue(message.HasFooMessage);
            Assert.AreEqual(message.FooMessage.QuxInt, 234);

            Assert.IsFalse(builder.ClearFooMessage().HasFooMessage);
            message = builder.Build();
            Assert.IsFalse(message.HasFooMessage);
            Assert.AreEqual(message.FooMessage.QuxInt, 0);

            builder = TestOneof2.CreateBuilder();
            Assert.IsFalse(builder.HasFooMessage);
            builder.SetFooMessage(TestOneof2.Types.NestedMessage.CreateBuilder().SetQuxInt(123));
            Assert.IsTrue(builder.HasFooMessage);
            Assert.AreEqual(builder.FooMessage.QuxInt, 123);
            message = builder.Build();
            Assert.IsTrue(message.HasFooMessage);
            Assert.AreEqual(message.FooMessage.QuxInt, 123);
        }

        [Test]
        public void TestOneofMerge_Primitive()
        {
            TestOneof2.Builder builder = TestOneof2.CreateBuilder();
            TestOneof2 message = builder.SetFooInt(123).Build();
            TestOneof2 message2 = TestOneof2.CreateBuilder().MergeFrom(message).Build();
            Assert.IsTrue(message2.HasFooInt);
            Assert.AreEqual(message2.FooInt, 123);
        }

        [Test]
        public void TestOneofMerge_String()
        {
            TestOneof2.Builder builder = TestOneof2.CreateBuilder();
            TestOneof2 message = builder.SetFooString("foo").Build();
            TestOneof2 message2 = TestOneof2.CreateBuilder().MergeFrom(message).Build();
            Assert.IsTrue(message2.HasFooString);
            Assert.AreEqual(message2.FooString, "foo");
        }

        [Test]
        public void TestOneofMerge_Enum()
        {
            TestOneof2.Builder builder = TestOneof2.CreateBuilder();
            TestOneof2 message = builder.SetFooEnum(TestOneof2.Types.NestedEnum.BAR).Build();
            TestOneof2 message2 = TestOneof2.CreateBuilder().MergeFrom(message).Build();
            Assert.IsTrue(message2.HasFooEnum);
            Assert.AreEqual(message2.FooEnum, TestOneof2.Types.NestedEnum.BAR);
        }

        public void TestOneofMerge_message()
        {
            TestOneof2.Builder builder = TestOneof2.CreateBuilder();
            TestOneof2 message = builder.SetFooMessage(
                TestOneof2.Types.NestedMessage.CreateBuilder().SetQuxInt(234).Build()).Build();
            TestOneof2 message2 = TestOneof2.CreateBuilder().MergeFrom(message).Build();
            Assert.IsTrue(message2.HasFooMessage);
            Assert.AreEqual(message2.FooMessage.QuxInt, 234);
        }

        [Test]
        public void TestOneofMergeMixed()
        {
            TestOneof2.Builder builder = TestOneof2.CreateBuilder();
            TestOneof2 message = builder.SetFooEnum(TestOneof2.Types.NestedEnum.BAR).Build();
            TestOneof2 message2 =
                TestOneof2.CreateBuilder().SetFooString("foo").MergeFrom(message).Build();
            Assert.IsFalse(message2.HasFooString);
            Assert.IsTrue(message2.HasFooEnum);
            Assert.AreEqual(message2.FooEnum, TestOneof2.Types.NestedEnum.BAR);
        }

        [Test]
        public void TestOneofSerialization_Primitive()
        {
            TestOneof2.Builder builder = TestOneof2.CreateBuilder();
            TestOneof2 message = builder.SetFooInt(123).Build();
            ByteString serialized = message.ToByteString();
            TestOneof2 message2 = TestOneof2.ParseFrom(serialized);
            Assert.IsTrue(message2.HasFooInt);
            Assert.AreEqual(message2.FooInt, 123);
        }

        [Test]
        public void TestOneofSerialization_String()
        {
            TestOneof2.Builder builder = TestOneof2.CreateBuilder();
            TestOneof2 message = builder.SetFooString("foo").Build();
            ByteString serialized = message.ToByteString();
            TestOneof2 message2 = TestOneof2.ParseFrom(serialized);
            Assert.IsTrue(message2.HasFooString);
            Assert.AreEqual(message2.FooString, "foo");
        }

        [Test]
        public void TestOneofSerialization_Enum()
        {
            TestOneof2.Builder builder = TestOneof2.CreateBuilder();
            TestOneof2 message = builder.SetFooEnum(TestOneof2.Types.NestedEnum.BAR).Build();
            ByteString serialized = message.ToByteString();
            TestOneof2 message2 = TestOneof2.ParseFrom(serialized);
            Assert.IsTrue(message2.HasFooEnum);
            Assert.AreEqual(message2.FooEnum, TestOneof2.Types.NestedEnum.BAR);
        }

        [Test]
        public void TestOneofSerialization_Message()
        {
            TestOneof2.Builder builder = TestOneof2.CreateBuilder();
            TestOneof2 message = builder.SetFooMessage(
                TestOneof2.Types.NestedMessage.CreateBuilder().SetQuxInt(234).Build()).Build();
            ByteString serialized = message.ToByteString();
            TestOneof2 message2 = TestOneof2.ParseFrom(serialized);
            Assert.IsTrue(message2.HasFooMessage);
            Assert.AreEqual(message2.FooMessage.QuxInt, 234);
        }
    }
}