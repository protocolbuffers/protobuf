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
using Xunit;

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

        [Fact]
        public void RepeatedAddPrimitiveBeforeBuild()
        {
            TestAllTypes message = new TestAllTypes.Builder {RepeatedInt32List = {1, 2, 3}}.Build();
            TestUtil.AssertEqual(new int[] {1, 2, 3}, message.RepeatedInt32List);
        }

        [Fact]
        public void AddPrimitiveFailsAfterBuild()
        {
            TestAllTypes.Builder builder = new TestAllTypes.Builder();
            IList<int> list = builder.RepeatedInt32List;
            list.Add(1); // Fine
            builder.Build();

            Assert.Throws<NotSupportedException>(() => list.Add(2));
        }

        [Fact]
        public void RepeatedAddMessageBeforeBuild()
        {
            TestAllTypes message = new TestAllTypes.Builder
                                       {
                                           RepeatedNestedMessageList =
                                               {new TestAllTypes.Types.NestedMessage.Builder {Bb = 10}.Build()}
                                       }.Build();
            Assert.Equal(1, message.RepeatedNestedMessageCount);
            Assert.Equal(10, message.RepeatedNestedMessageList[0].Bb);
        }

        [Fact]
        public void AddMessageFailsAfterBuild()
        {
            TestAllTypes.Builder builder = new TestAllTypes.Builder();
            IList<TestAllTypes.Types.NestedMessage> list = builder.RepeatedNestedMessageList;
            builder.Build();

            Assert.Throws<NotSupportedException>(() => list.Add(new TestAllTypes.Types.NestedMessage.Builder { Bb = 10 }.Build()));
        }

        [Fact]
        public void DefaultInstance()
        {
            Assert.Same(TestAllTypes.DefaultInstance, TestAllTypes.DefaultInstance.DefaultInstanceForType);
            Assert.Same(TestAllTypes.DefaultInstance, TestAllTypes.CreateBuilder().DefaultInstanceForType);
        }

        [Fact]
        public void Accessors()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            TestUtil.SetAllFields(builder);
            TestAllTypes message = builder.Build();
            TestUtil.AssertAllFieldsSet(message);
        }

        [Fact]
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

        [Fact]
        public void RepeatedSetters()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            TestUtil.SetAllFields(builder);
            TestUtil.ModifyRepeatedFields(builder);
            TestAllTypes message = builder.Build();
            TestUtil.AssertRepeatedFieldsModified(message);
        }

        [Fact]
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
            Assert.Equal(1, message.RepeatedForeignMessageCount);
            Assert.Equal(12, message.GetRepeatedForeignMessage(0).C);
        }

        [Fact]
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

        [Fact]
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
            Assert.Equal(expectedMessage, message);
        }

        [Fact]
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
            Assert.Equal(expectedMessage, message);
        }

        [Fact]
        public void SettingRepeatedValuesUsingRangeInCollectionInitializer()
        {
            int[] values = {1, 2, 3};
            TestAllTypes message = new TestAllTypes.Builder
                                       {
                                           RepeatedSint32List = {values}
                                       }.Build();
            Assert.True(Lists.Equals(values, message.RepeatedSint32List));
        }

        [Fact]
        public void SettingRepeatedValuesUsingIndividualValuesInCollectionInitializer()
        {
            TestAllTypes message = new TestAllTypes.Builder
                                       {
                                           RepeatedSint32List = {6, 7}
                                       }.Build();
            Assert.True(Lists.Equals(new int[] {6, 7}, message.RepeatedSint32List));
        }

        [Fact]
        public void Defaults()
        {
            TestUtil.AssertClear(TestAllTypes.DefaultInstance);
            TestUtil.AssertClear(TestAllTypes.CreateBuilder().Build());

            Assert.Equal("\u1234", TestExtremeDefaultValues.DefaultInstance.Utf8String);
        }

        [Fact]
        public void ReflectionGetters()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            TestUtil.SetAllFields(builder);
            TestAllTypes message = builder.Build();
            reflectionTester.AssertAllFieldsSetViaReflection(message);
        }

        [Fact]
        public void ReflectionSetters()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            reflectionTester.SetAllFieldsViaReflection(builder);
            TestAllTypes message = builder.Build();
            TestUtil.AssertAllFieldsSet(message);
        }

        [Fact]
        public void ReflectionClear()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            reflectionTester.SetAllFieldsViaReflection(builder);
            reflectionTester.ClearAllFieldsViaReflection(builder);
            TestAllTypes message = builder.Build();
            TestUtil.AssertClear(message);
        }

        [Fact]
        public void ReflectionSettersRejectNull()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            reflectionTester.AssertReflectionSettersRejectNull(builder);
        }

        [Fact]
        public void ReflectionRepeatedSetters()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            reflectionTester.SetAllFieldsViaReflection(builder);
            reflectionTester.ModifyRepeatedFieldsViaReflection(builder);
            TestAllTypes message = builder.Build();
            TestUtil.AssertRepeatedFieldsModified(message);
        }

        [Fact]
        public void TestReflectionRepeatedSettersRejectNull()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            reflectionTester.AssertReflectionRepeatedSettersRejectNull(builder);
        }

        [Fact]
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

        [Fact]
        public void ReflectionGetOneof()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            reflectionTester.SetAllFieldsViaReflection(builder);
            Descriptors.OneofDescriptor oneof = TestAllTypes.Descriptor.Oneofs[0];
            Descriptors.FieldDescriptor field = TestAllTypes.Descriptor.FindFieldByName("oneof_bytes");
            Assert.Same(field, builder.OneofFieldDescriptor(oneof));
        }

        [Fact]
        public void ReflectionClearOneof()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            reflectionTester.SetAllFieldsViaReflection(builder);
            OneofDescriptor oneof = TestAllTypes.Descriptor.Oneofs[0];
            FieldDescriptor field = TestAllTypes.Descriptor.FindFieldByName("oneof_bytes");

            Assert.True(builder.HasOneof(oneof));
            Assert.True(builder.HasField(field));
            builder.ClearOneof(oneof);
            Assert.False(builder.HasOneof(oneof));
            Assert.False(builder.HasField(field));
        }

        // =================================================================
        // Extensions.

        [Fact]
        public void ExtensionAccessors()
        {
            TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
            TestUtil.SetAllExtensions(builder);
            TestAllExtensions message = builder.Build();
            TestUtil.AssertAllExtensionsSet(message);
        }

        [Fact]
        public void ExtensionRepeatedSetters()
        {
            TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
            TestUtil.SetAllExtensions(builder);
            TestUtil.ModifyRepeatedExtensions(builder);
            TestAllExtensions message = builder.Build();
            TestUtil.AssertRepeatedExtensionsModified(message);
        }

        [Fact]
        public void ExtensionDefaults()
        {
            TestUtil.AssertExtensionsClear(TestAllExtensions.DefaultInstance);
            TestUtil.AssertExtensionsClear(TestAllExtensions.CreateBuilder().Build());
        }

        [Fact]
        public void ExtensionReflectionGetters()
        {
            TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
            TestUtil.SetAllExtensions(builder);
            TestAllExtensions message = builder.Build();
            extensionsReflectionTester.AssertAllFieldsSetViaReflection(message);
        }

        [Fact]
        public void ExtensionReflectionSetters()
        {
            TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
            extensionsReflectionTester.SetAllFieldsViaReflection(builder);
            TestAllExtensions message = builder.Build();
            TestUtil.AssertAllExtensionsSet(message);
        }

        [Fact]
        public void ExtensionReflectionSettersRejectNull()
        {
            TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
            extensionsReflectionTester.AssertReflectionSettersRejectNull(builder);
        }

        [Fact]
        public void ExtensionReflectionRepeatedSetters()
        {
            TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
            extensionsReflectionTester.SetAllFieldsViaReflection(builder);
            extensionsReflectionTester.ModifyRepeatedFieldsViaReflection(builder);
            TestAllExtensions message = builder.Build();
            TestUtil.AssertRepeatedExtensionsModified(message);
        }

        [Fact]
        public void ExtensionReflectionRepeatedSettersRejectNull()
        {
            TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
            extensionsReflectionTester.AssertReflectionRepeatedSettersRejectNull(builder);
        }

        [Fact]
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

        [Fact]
        public void ClearExtension()
        {
            // ClearExtension() is not actually used in TestUtil, so try it manually.
            Assert.False(TestAllExtensions.CreateBuilder()
                               .SetExtension(Unittest.OptionalInt32Extension, 1)
                               .ClearExtension(Unittest.OptionalInt32Extension)
                               .HasExtension(Unittest.OptionalInt32Extension));
            Assert.Equal(0, TestAllExtensions.CreateBuilder()
                                   .AddExtension(Unittest.RepeatedInt32Extension, 1)
                                   .ClearExtension(Unittest.RepeatedInt32Extension)
                                   .GetExtensionCount(Unittest.RepeatedInt32Extension));
        }

        [Fact]
        public void ExtensionMergeFrom()
        {
            TestAllExtensions original = TestAllExtensions.CreateBuilder()
                .SetExtension(Unittest.OptionalInt32Extension, 1).Build();
            TestAllExtensions merged =
                TestAllExtensions.CreateBuilder().MergeFrom(original).Build();
            Assert.True((merged.HasExtension(Unittest.OptionalInt32Extension)));
            Assert.Equal(1, (int) merged.GetExtension(Unittest.OptionalInt32Extension));
        }

        /* Removed multiple files option for the moment
    [Fact]
    public void MultipleFilesOption() {
      // We mostly just want to check that things compile.
      MessageWithNoOuter message = MessageWithNoOuter.CreateBuilder()
          .SetNested(MessageWithNoOuter.Types.NestedMessage.CreateBuilder().SetI(1))
          .AddForeign(TestAllTypes.CreateBuilder().SetOptionalInt32(1))
          .SetNestedEnum(MessageWithNoOuter.Types.NestedEnum.BAZ)
          .SetForeignEnum(EnumWithNoOuter.BAR)
          .Build();
      Assert.Equal(message, MessageWithNoOuter.ParseFrom(message.ToByteString()));

      Assert.Equal(MultiFileProto.DescriptorProtoFile, MessageWithNoOuter.DescriptorProtoFile.File);

      FieldDescriptor field = MessageWithNoOuter.DescriptorProtoFile.FindDescriptor<FieldDescriptor>("foreign_enum");
      Assert.Equal(MultiFileProto.DescriptorProtoFile.FindTypeByName<EnumDescriptor>("EnumWithNoOuter")
        .FindValueByNumber((int)EnumWithNoOuter.BAR), message[field]);

      Assert.Equal(MultiFileProto.DescriptorProtoFile, ServiceWithNoOuter.DescriptorProtoFile.File);

      Assert.False(TestAllExtensions.DefaultInstance.HasExtension(MultiFileProto.ExtensionWithOuter));
    }*/

        [Fact]
        public void OptionalFieldWithRequiredSubfieldsOptimizedForSize()
        {
            TestOptionalOptimizedForSize message = TestOptionalOptimizedForSize.DefaultInstance;
            Assert.True(message.IsInitialized);

            message = TestOptionalOptimizedForSize.CreateBuilder().SetO(
                TestRequiredOptimizedForSize.CreateBuilder().BuildPartial()
                ).BuildPartial();
            Assert.False(message.IsInitialized);

            message = TestOptionalOptimizedForSize.CreateBuilder().SetO(
                TestRequiredOptimizedForSize.CreateBuilder().SetX(5).BuildPartial()
                ).BuildPartial();
            Assert.True(message.IsInitialized);
        }

        [Fact]
        public void OptimizedForSizeMergeUsesAllFieldsFromTarget()
        {
            TestOptimizedForSize withFieldSet = new TestOptimizedForSize.Builder {I = 10}.Build();
            TestOptimizedForSize.Builder builder = new TestOptimizedForSize.Builder();
            builder.MergeFrom(withFieldSet);
            TestOptimizedForSize built = builder.Build();
            Assert.Equal(10, built.I);
        }

        [Fact]
        public void UninitializedExtensionInOptimizedForSizeMakesMessageUninitialized()
        {
            TestOptimizedForSize.Builder builder = new TestOptimizedForSize.Builder();
            builder.SetExtension(TestOptimizedForSize.TestExtension2,
                                 new TestRequiredOptimizedForSize.Builder().BuildPartial());
            Assert.False(builder.IsInitialized);
            Assert.False(builder.BuildPartial().IsInitialized);

            builder = new TestOptimizedForSize.Builder();
            builder.SetExtension(TestOptimizedForSize.TestExtension2,
                                 new TestRequiredOptimizedForSize.Builder {X = 10}.BuildPartial());
            Assert.True(builder.IsInitialized);
            Assert.True(builder.BuildPartial().IsInitialized);
        }

        [Fact]
        public void ToBuilder()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            TestUtil.SetAllFields(builder);
            TestAllTypes message = builder.Build();
            TestUtil.AssertAllFieldsSet(message.ToBuilder().Build());
        }

        [Fact]
        public void FieldConstantValues()
        {
            Assert.Equal(TestAllTypes.Types.NestedMessage.BbFieldNumber, 1);
            Assert.Equal(TestAllTypes.OptionalInt32FieldNumber, 1);
            Assert.Equal(TestAllTypes.OptionalGroupFieldNumber, 16);
            Assert.Equal(TestAllTypes.OptionalNestedMessageFieldNumber, 18);
            Assert.Equal(TestAllTypes.OptionalNestedEnumFieldNumber, 21);
            Assert.Equal(TestAllTypes.RepeatedInt32FieldNumber, 31);
            Assert.Equal(TestAllTypes.RepeatedGroupFieldNumber, 46);
            Assert.Equal(TestAllTypes.RepeatedNestedMessageFieldNumber, 48);
            Assert.Equal(TestAllTypes.RepeatedNestedEnumFieldNumber, 51);
        }

        [Fact]
        public void ExtensionConstantValues()
        {
            Assert.Equal(TestRequired.SingleFieldNumber, 1000);
            Assert.Equal(TestRequired.MultiFieldNumber, 1001);
            Assert.Equal(Unittest.OptionalInt32ExtensionFieldNumber, 1);
            Assert.Equal(Unittest.OptionalGroupExtensionFieldNumber, 16);
            Assert.Equal(Unittest.OptionalNestedMessageExtensionFieldNumber, 18);
            Assert.Equal(Unittest.OptionalNestedEnumExtensionFieldNumber, 21);
            Assert.Equal(Unittest.RepeatedInt32ExtensionFieldNumber, 31);
            Assert.Equal(Unittest.RepeatedGroupExtensionFieldNumber, 46);
            Assert.Equal(Unittest.RepeatedNestedMessageExtensionFieldNumber, 48);
            Assert.Equal(Unittest.RepeatedNestedEnumExtensionFieldNumber, 51);
        }

        [Fact]
        public void EmptyPackedValue()
        {
            TestPackedTypes empty = new TestPackedTypes.Builder().Build();
            Assert.Equal(0, empty.SerializedSize);
        }

        // oneof tests
        [Fact]
        public void TestOneofEmumCase()
        {
            TestOneof2 message = TestOneof2.CreateBuilder()
                .SetFooInt(123).SetFooString("foo").SetFooCord("bar").Build();
            TestUtil.AssertAtMostOneFieldSetOneof(message);
        }

        [Fact]
        public void TestClearOneof()
        {
            TestOneof2.Builder builder = TestOneof2.CreateBuilder().SetFooInt(123);
            Assert.Equal(TestOneof2.FooOneofCase.FooInt, builder.FooCase);
            builder.ClearFoo();
            Assert.Equal(TestOneof2.FooOneofCase.FooNotSet, builder.FooCase);
        }

        [Fact]
        public void TestSetOneofClearsOthers()
        {
            TestOneof2.Builder builder = TestOneof2.CreateBuilder();
            TestOneof2 message =
                builder.SetFooInt(123).SetFooString("foo").BuildPartial();
            Assert.True(message.HasFooString);
            TestUtil.AssertAtMostOneFieldSetOneof(message);

            message = builder.SetFooCord("bar").BuildPartial();
            Assert.True(message.HasFooCord);
            TestUtil.AssertAtMostOneFieldSetOneof(message);

            message = builder.SetFooStringPiece("baz").BuildPartial();
            Assert.True(message.HasFooStringPiece);
            TestUtil.AssertAtMostOneFieldSetOneof(message);

            message = builder.SetFooBytes(TestUtil.ToBytes("qux")).BuildPartial();
            Assert.True(message.HasFooBytes);
            TestUtil.AssertAtMostOneFieldSetOneof(message);

            message = builder.SetFooEnum(TestOneof2.Types.NestedEnum.FOO).BuildPartial();
            Assert.True(message.HasFooEnum);
            TestUtil.AssertAtMostOneFieldSetOneof(message);

            message = builder.SetFooMessage(
                TestOneof2.Types.NestedMessage.CreateBuilder().SetQuxInt(234).Build()).BuildPartial();
            Assert.True(message.HasFooMessage);
            TestUtil.AssertAtMostOneFieldSetOneof(message);

            message = builder.SetFooInt(123).BuildPartial();
            Assert.True(message.HasFooInt);
            TestUtil.AssertAtMostOneFieldSetOneof(message);
        }

        [Fact]
        public void TestOneofTypes()
        {   
            // Primitive
            {
                TestOneof2.Builder builder = TestOneof2.CreateBuilder();
                Assert.Equal(builder.FooInt, 0);
                Assert.False(builder.HasFooInt);
                Assert.True(builder.SetFooInt(123).HasFooInt);
                Assert.Equal(builder.FooInt, 123);
                TestOneof2 message = builder.BuildPartial();
                Assert.True(message.HasFooInt);
                Assert.Equal(message.FooInt, 123);

                Assert.False(builder.ClearFooInt().HasFooInt);
                TestOneof2 message2 = builder.Build();
                Assert.False(message2.HasFooInt);
                Assert.Equal(message2.FooInt, 0);
            }
            // Enum
            {
                TestOneof2.Builder builder = TestOneof2.CreateBuilder();
                Assert.Equal(builder.FooEnum, TestOneof2.Types.NestedEnum.FOO);
                Assert.True(builder.SetFooEnum(TestOneof2.Types.NestedEnum.BAR).HasFooEnum);
                Assert.Equal(builder.FooEnum, TestOneof2.Types.NestedEnum.BAR);
                TestOneof2 message = builder.BuildPartial();
                Assert.True(message.HasFooEnum);
                Assert.Equal(message.FooEnum, TestOneof2.Types.NestedEnum.BAR);

                Assert.False(builder.ClearFooEnum().HasFooEnum);
                TestOneof2 message2 = builder.Build();
                Assert.False(message2.HasFooEnum);
                Assert.Equal(message2.FooEnum, TestOneof2.Types.NestedEnum.FOO);
            }
            // String
            {
                TestOneof2.Builder builder = TestOneof2.CreateBuilder();
                Assert.Equal(builder.FooString, "");
                Assert.True(builder.SetFooString("foo").HasFooString);
                Assert.Equal(builder.FooString, "foo");
                TestOneof2 message = builder.BuildPartial();
                Assert.True(message.HasFooString);
                Assert.Equal(message.FooString, "foo");

                Assert.False(builder.ClearFooString().HasFooString);
                TestOneof2 message2 = builder.Build();
                Assert.False(message2.HasFooString);
                Assert.Equal(message2.FooString, "");

                builder.SetFooInt(123);
                Assert.Equal(builder.FooString, "");
                Assert.Equal(builder.FooInt, 123);
                message = builder.Build();
                Assert.Equal(message.FooString, "");
                Assert.Equal(message.FooInt, 123);
            }
            // Message
            {
                TestOneof2.Builder builder = TestOneof2.CreateBuilder();
                Assert.Equal(builder.FooMessage.QuxInt, 0);
                builder.SetFooMessage(TestOneof2.Types.NestedMessage.CreateBuilder().SetQuxInt(234).Build());
                Assert.True(builder.HasFooMessage);
                Assert.Equal(builder.FooMessage.QuxInt, 234);
                TestOneof2 message = builder.BuildPartial();
                Assert.True(message.HasFooMessage);
                Assert.Equal(message.FooMessage.QuxInt, 234);

                Assert.False(builder.ClearFooMessage().HasFooMessage);
                message = builder.Build();
                Assert.False(message.HasFooMessage);
                Assert.Equal(message.FooMessage.QuxInt, 0);

                builder = TestOneof2.CreateBuilder();
                Assert.False(builder.HasFooMessage);
                builder.SetFooMessage(TestOneof2.Types.NestedMessage.CreateBuilder().SetQuxInt(123));
                Assert.True(builder.HasFooMessage);
                Assert.Equal(builder.FooMessage.QuxInt, 123);
                message = builder.BuildPartial();
                Assert.True(message.HasFooMessage);
                Assert.Equal(message.FooMessage.QuxInt, 123);
            }
        }

        [Fact]
        public void TestOneofMerge()
        {
            {
                // Primitive Type
                TestOneof2.Builder builder = TestOneof2.CreateBuilder();
                TestOneof2 message = builder.SetFooInt(123).Build();
                TestOneof2 message2 = TestOneof2.CreateBuilder().MergeFrom(message).Build();
                Assert.True(message2.HasFooInt);
                Assert.Equal(message2.FooInt, 123);
            }
            {
                // String
                TestOneof2.Builder builder = TestOneof2.CreateBuilder();
                TestOneof2 message = builder.SetFooString("foo").Build();
                TestOneof2 message2 = TestOneof2.CreateBuilder().MergeFrom(message).Build();
                Assert.True(message2.HasFooString);
                Assert.Equal(message2.FooString, "foo");
            }
            {
                // Enum
                TestOneof2.Builder builder = TestOneof2.CreateBuilder();
                TestOneof2 message = builder.SetFooEnum(TestOneof2.Types.NestedEnum.BAR).Build();
                TestOneof2 message2 = TestOneof2.CreateBuilder().MergeFrom(message).Build();
                Assert.True(message2.HasFooEnum);
                Assert.Equal(message2.FooEnum, TestOneof2.Types.NestedEnum.BAR);
            }
            {
                // message
                TestOneof2.Builder builder = TestOneof2.CreateBuilder();
                TestOneof2 message = builder.SetFooMessage(
                    TestOneof2.Types.NestedMessage.CreateBuilder().SetQuxInt(234).Build()).Build();
                TestOneof2 message2 = TestOneof2.CreateBuilder().MergeFrom(message).Build();
                Assert.True(message2.HasFooMessage);
                Assert.Equal(message2.FooMessage.QuxInt, 234);
            }
        }

        [Fact]
        public void TestOneofSerialization()
        {
            {
                // Primisive
                TestOneof2.Builder builder = TestOneof2.CreateBuilder();
                TestOneof2 message = builder.SetFooInt(123).Build();
                ByteString serialized = message.ToByteString();
                TestOneof2 message2 = TestOneof2.ParseFrom(serialized);
                Assert.True(message2.HasFooInt);
                Assert.Equal(message2.FooInt, 123);
            }
            {
                // String
                TestOneof2.Builder builder = TestOneof2.CreateBuilder();
                TestOneof2 message = builder.SetFooString("foo").Build();
                ByteString serialized = message.ToByteString();
                TestOneof2 message2 = TestOneof2.ParseFrom(serialized);
                Assert.True(message2.HasFooString);
                Assert.Equal(message2.FooString, "foo");
            }
            {
                // Enum
                TestOneof2.Builder builder = TestOneof2.CreateBuilder();
                TestOneof2 message = builder.SetFooEnum(TestOneof2.Types.NestedEnum.BAR).Build();
                ByteString serialized = message.ToByteString();
                TestOneof2 message2 = TestOneof2.ParseFrom(serialized);
                Assert.True(message2.HasFooEnum);
                Assert.Equal(message2.FooEnum, TestOneof2.Types.NestedEnum.BAR);
            }
            {
                // message
                TestOneof2.Builder builder = TestOneof2.CreateBuilder();
                TestOneof2 message = builder.SetFooMessage(
                    TestOneof2.Types.NestedMessage.CreateBuilder().SetQuxInt(234).Build()).Build();
                ByteString serialized = message.ToByteString();
                TestOneof2 message2 = TestOneof2.ParseFrom(serialized);
                Assert.True(message2.HasFooMessage);
                Assert.Equal(message2.FooMessage.QuxInt, 234);
            }
        }
    }
}