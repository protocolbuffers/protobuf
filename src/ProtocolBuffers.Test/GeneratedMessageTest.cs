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
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class GeneratedMessageTest {
    ReflectionTester reflectionTester;
    ReflectionTester extensionsReflectionTester;
    
    [SetUp]
    public void SetUp() {
      reflectionTester = ReflectionTester.CreateTestAllTypesInstance();
      extensionsReflectionTester = ReflectionTester.CreateTestAllExtensionsInstance();
    }

    [Test]
    public void RepeatedAddPrimitiveBeforeBuild() {
      TestAllTypes message = new TestAllTypes.Builder { RepeatedInt32List = { 1, 2, 3 } }.Build();
      TestUtil.AssertEqual(new int[]{1, 2, 3}, message.RepeatedInt32List);
    }

    [Test]
    public void AddPrimitiveFailsAfterBuild() {
      TestAllTypes.Builder builder = new TestAllTypes.Builder();
      IList<int> list = builder.RepeatedInt32List;
      list.Add(1); // Fine
      builder.Build();

      try {
        list.Add(2);
        Assert.Fail("List should be frozen");
      } catch (NotSupportedException) {
        // Expected
      }
    }

    [Test]
    public void RepeatedAddMessageBeforeBuild() {
      TestAllTypes message = new TestAllTypes.Builder {
          RepeatedNestedMessageList = { new TestAllTypes.Types.NestedMessage.Builder { Bb = 10 }.Build() } }.Build();
      Assert.AreEqual(1, message.RepeatedNestedMessageCount);
      Assert.AreEqual(10, message.RepeatedNestedMessageList[0].Bb);
    }

    [Test]
    public void AddMessageFailsAfterBuild() {
      TestAllTypes.Builder builder = new TestAllTypes.Builder();
      IList<TestAllTypes.Types.NestedMessage> list = builder.RepeatedNestedMessageList;
      builder.Build();

      try {
        list.Add(new TestAllTypes.Types.NestedMessage.Builder { Bb = 10 }.Build());
        Assert.Fail("List should be frozen");
      } catch (NotSupportedException) {
        // Expected
      }
    }

    [Test]
    public void DoubleBuildError() {
      TestAllTypes.Builder builder = new TestAllTypes.Builder();
      builder.Build();
      try {
        builder.Build();
        Assert.Fail("Should have thrown exception.");
      } catch (InvalidOperationException) {
        // Success.
      }
    }

    [Test]
    public void DefaultInstance() {
      Assert.AreSame(TestAllTypes.DefaultInstance, TestAllTypes.DefaultInstance.DefaultInstanceForType);
      Assert.AreSame(TestAllTypes.DefaultInstance, TestAllTypes.CreateBuilder().DefaultInstanceForType);
    }

    [Test]
    public void Accessors() {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      TestUtil.SetAllFields(builder);
      TestAllTypes message = builder.Build();
      TestUtil.AssertAllFieldsSet(message);
    }

    [Test]
    public void SettersRejectNull() {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      TestUtil.AssertArgumentNullException(() => builder.SetOptionalString(null));
      TestUtil.AssertArgumentNullException(() => builder.SetOptionalBytes(null));
      TestUtil.AssertArgumentNullException(() => builder.SetOptionalNestedMessage((TestAllTypes.Types.NestedMessage)null));
      TestUtil.AssertArgumentNullException(() => builder.SetOptionalNestedMessage((TestAllTypes.Types.NestedMessage.Builder)null));
      TestUtil.AssertArgumentNullException(() => builder.AddRepeatedString(null));
      TestUtil.AssertArgumentNullException(() => builder.AddRepeatedBytes(null));
      TestUtil.AssertArgumentNullException(() => builder.AddRepeatedNestedMessage((TestAllTypes.Types.NestedMessage)null));
      TestUtil.AssertArgumentNullException(() => builder.AddRepeatedNestedMessage((TestAllTypes.Types.NestedMessage.Builder)null));
    }

    [Test]
    public void RepeatedSetters() {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      TestUtil.SetAllFields(builder);
      TestUtil.ModifyRepeatedFields(builder);
      TestAllTypes message = builder.Build();
      TestUtil.AssertRepeatedFieldsModified(message);
    }

    [Test]
    public void RepeatedAppend() {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();

      builder.AddRangeRepeatedInt32(new int[]{1, 2, 3, 4});
      builder.AddRangeRepeatedForeignEnum((new ForeignEnum[] { ForeignEnum.FOREIGN_BAZ }));

      ForeignMessage foreignMessage = ForeignMessage.CreateBuilder().SetC(12).Build();
      builder.AddRangeRepeatedForeignMessage(new ForeignMessage[] {foreignMessage});

      TestAllTypes message = builder.Build();
      TestUtil.AssertEqual(message.RepeatedInt32List, new int[]{1, 2, 3, 4});
      TestUtil.AssertEqual(message.RepeatedForeignEnumList, new ForeignEnum[] {ForeignEnum.FOREIGN_BAZ});
      Assert.AreEqual(1, message.RepeatedForeignMessageCount);
      Assert.AreEqual(12, message.GetRepeatedForeignMessage(0).C);
    }

    [Test]
    public void RepeatedAppendRejectsNull() {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();

      ForeignMessage foreignMessage = ForeignMessage.CreateBuilder().SetC(12).Build();
      TestUtil.AssertArgumentNullException(() => builder.AddRangeRepeatedForeignMessage(new[] { foreignMessage, null }));
      TestUtil.AssertArgumentNullException(() => builder.AddRangeRepeatedForeignMessage(null));
      TestUtil.AssertArgumentNullException(() => builder.AddRangeRepeatedForeignEnum(null));
      TestUtil.AssertArgumentNullException(() => builder.AddRangeRepeatedString(new[] { "one", null }));
      TestUtil.AssertArgumentNullException(() => builder.AddRangeRepeatedBytes(new[] { TestUtil.ToBytes("one"), null }));
    }

    [Test]
    public void SettingForeignMessageUsingBuilder() {
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
    public void SettingRepeatedForeignMessageUsingBuilder() {
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
    public void SettingRepeatedValuesUsingRangeInCollectionInitializer() {
      int[] values = { 1, 2, 3 };
      TestAllTypes message = new TestAllTypes.Builder {
        RepeatedSint32List = { values }
      }.Build();
      Assert.IsTrue(Lists.Equals(values, message.RepeatedSint32List));
    }

    [Test]
    public void SettingRepeatedValuesUsingIndividualValuesInCollectionInitializer() {
      TestAllTypes message = new TestAllTypes.Builder {
        RepeatedSint32List = { 6, 7 }
      }.Build();
      Assert.IsTrue(Lists.Equals(new int[] { 6, 7 }, message.RepeatedSint32List));
    }
    
    [Test]
    public void Defaults() {
      TestUtil.AssertClear(TestAllTypes.DefaultInstance);
      TestUtil.AssertClear(TestAllTypes.CreateBuilder().Build());

      Assert.AreEqual("\u1234", TestExtremeDefaultValues.DefaultInstance.Utf8String);
    }

    [Test]
    public void ReflectionGetters() {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      TestUtil.SetAllFields(builder);
      TestAllTypes message = builder.Build();
      reflectionTester.AssertAllFieldsSetViaReflection(message);
    }

    [Test]
    public void ReflectionSetters() {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      reflectionTester.SetAllFieldsViaReflection(builder);
      TestAllTypes message = builder.Build();
      TestUtil.AssertAllFieldsSet(message);
    }

    [Test]
    public void ReflectionSettersRejectNull() {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      reflectionTester.AssertReflectionSettersRejectNull(builder);
    }
    [Test]
    public void ReflectionRepeatedSetters() {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      reflectionTester.SetAllFieldsViaReflection(builder);
      reflectionTester.ModifyRepeatedFieldsViaReflection(builder);
      TestAllTypes message = builder.Build();
      TestUtil.AssertRepeatedFieldsModified(message);
    }

    [Test]
    public void TestReflectionRepeatedSettersRejectNull() {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      reflectionTester.AssertReflectionRepeatedSettersRejectNull(builder);
    }

    [Test]
    public void ReflectionDefaults() {
      TestUtil.TestInMultipleCultures(() => {
        reflectionTester.AssertClearViaReflection(TestAllTypes.DefaultInstance);
        reflectionTester.AssertClearViaReflection(TestAllTypes.CreateBuilder().Build());
      });
    }
    // =================================================================
    // Extensions.

    [Test]
    public void ExtensionAccessors() {
      TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
      TestUtil.SetAllExtensions(builder);
      TestAllExtensions message = builder.Build();
      TestUtil.AssertAllExtensionsSet(message);
    }

    [Test]
    public void ExtensionRepeatedSetters() {
      TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
      TestUtil.SetAllExtensions(builder);
      TestUtil.ModifyRepeatedExtensions(builder);
      TestAllExtensions message = builder.Build();
      TestUtil.AssertRepeatedExtensionsModified(message);
    }

    [Test]
    public void ExtensionDefaults() {
      TestUtil.AssertExtensionsClear(TestAllExtensions.DefaultInstance);
      TestUtil.AssertExtensionsClear(TestAllExtensions.CreateBuilder().Build());
    }

    [Test]
    public void ExtensionReflectionGetters() {
      TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
      TestUtil.SetAllExtensions(builder);
      TestAllExtensions message = builder.Build();
      extensionsReflectionTester.AssertAllFieldsSetViaReflection(message);
    }

    [Test]
    public void ExtensionReflectionSetters() {
      TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
      extensionsReflectionTester.SetAllFieldsViaReflection(builder);
      TestAllExtensions message = builder.Build();
      TestUtil.AssertAllExtensionsSet(message);
    }

    [Test]
    public void ExtensionReflectionSettersRejectNull() {
      TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
      extensionsReflectionTester.AssertReflectionSettersRejectNull(builder);
    }

    [Test]
    public void ExtensionReflectionRepeatedSetters() {
      TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
      extensionsReflectionTester.SetAllFieldsViaReflection(builder);
      extensionsReflectionTester.ModifyRepeatedFieldsViaReflection(builder);
      TestAllExtensions message = builder.Build();
      TestUtil.AssertRepeatedExtensionsModified(message);
    }

    [Test]
    public void ExtensionReflectionRepeatedSettersRejectNull() {
      TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
      extensionsReflectionTester.AssertReflectionRepeatedSettersRejectNull(builder);
    }

    [Test]
    public void ExtensionReflectionDefaults() {
      TestUtil.TestInMultipleCultures(() => {
        extensionsReflectionTester.AssertClearViaReflection(TestAllExtensions.DefaultInstance);
        extensionsReflectionTester.AssertClearViaReflection(TestAllExtensions.CreateBuilder().Build());
      });
    }    
    
    [Test]
    public void ClearExtension() {
      // ClearExtension() is not actually used in TestUtil, so try it manually.
      Assert.IsFalse(TestAllExtensions.CreateBuilder()
          .SetExtension(UnitTestProtoFile.OptionalInt32Extension, 1)
          .ClearExtension(UnitTestProtoFile.OptionalInt32Extension)
          .HasExtension(UnitTestProtoFile.OptionalInt32Extension));
      Assert.AreEqual(0, TestAllExtensions.CreateBuilder()
          .AddExtension(UnitTestProtoFile.RepeatedInt32Extension, 1)
          .ClearExtension(UnitTestProtoFile.RepeatedInt32Extension)
          .GetExtensionCount(UnitTestProtoFile.RepeatedInt32Extension));
    }

    [Test]
    public void ExtensionMergeFrom() {
      TestAllExtensions original = TestAllExtensions.CreateBuilder()
          .SetExtension(UnitTestProtoFile.OptionalInt32Extension, 1).Build();
      TestAllExtensions merged =
          TestAllExtensions.CreateBuilder().MergeFrom(original).Build();
      Assert.IsTrue((merged.HasExtension(UnitTestProtoFile.OptionalInt32Extension)));
      Assert.AreEqual(1, (int)merged.GetExtension(UnitTestProtoFile.OptionalInt32Extension));
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

      Assert.AreEqual(MultiFileProto.Descriptor, MessageWithNoOuter.Descriptor.File);

      FieldDescriptor field = MessageWithNoOuter.Descriptor.FindDescriptor<FieldDescriptor>("foreign_enum");
      Assert.AreEqual(MultiFileProto.Descriptor.FindTypeByName<EnumDescriptor>("EnumWithNoOuter")
        .FindValueByNumber((int)EnumWithNoOuter.BAR), message[field]);

      Assert.AreEqual(MultiFileProto.Descriptor, ServiceWithNoOuter.Descriptor.File);

      Assert.IsFalse(TestAllExtensions.DefaultInstance.HasExtension(MultiFileProto.ExtensionWithOuter));
    }*/

    [Test]
    public void OptionalFieldWithRequiredSubfieldsOptimizedForSize() {      
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
    public void OptimizedForSizeMergeUsesAllFieldsFromTarget() {
      TestOptimizedForSize withFieldSet = new TestOptimizedForSize.Builder { I = 10 }.Build();
      TestOptimizedForSize.Builder builder = new TestOptimizedForSize.Builder();
      builder.MergeFrom(withFieldSet);
      TestOptimizedForSize built = builder.Build();
      Assert.AreEqual(10, built.I);
    }

    [Test]
    public void UninitializedExtensionInOptimizedForSizeMakesMessageUninitialized() {
      TestOptimizedForSize.Builder builder = new TestOptimizedForSize.Builder();
      builder.SetExtension(TestOptimizedForSize.TestExtension2,
          new TestRequiredOptimizedForSize.Builder().BuildPartial());
      Assert.IsFalse(builder.IsInitialized);
      Assert.IsFalse(builder.BuildPartial().IsInitialized);

      builder = new TestOptimizedForSize.Builder();
      builder.SetExtension(TestOptimizedForSize.TestExtension2,
          new TestRequiredOptimizedForSize.Builder { X = 10 }.BuildPartial());
      Assert.IsTrue(builder.IsInitialized);
      Assert.IsTrue(builder.BuildPartial().IsInitialized);
    }

    [Test]
    public void ToBuilder() {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      TestUtil.SetAllFields(builder);
      TestAllTypes message = builder.Build();
      TestUtil.AssertAllFieldsSet(message.ToBuilder().Build());
    }

    [Test]
    public void FieldConstantValues() {
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
    public void ExtensionConstantValues() {
      Assert.AreEqual(TestRequired.SingleFieldNumber, 1000);
      Assert.AreEqual(TestRequired.MultiFieldNumber, 1001);
      Assert.AreEqual(UnitTestProtoFile.OptionalInt32ExtensionFieldNumber, 1);
      Assert.AreEqual(UnitTestProtoFile.OptionalGroupExtensionFieldNumber, 16);
      Assert.AreEqual(UnitTestProtoFile.OptionalNestedMessageExtensionFieldNumber, 18);
      Assert.AreEqual(UnitTestProtoFile.OptionalNestedEnumExtensionFieldNumber, 21);
      Assert.AreEqual(UnitTestProtoFile.RepeatedInt32ExtensionFieldNumber, 31);
      Assert.AreEqual(UnitTestProtoFile.RepeatedGroupExtensionFieldNumber, 46);
      Assert.AreEqual(UnitTestProtoFile.RepeatedNestedMessageExtensionFieldNumber, 48);
      Assert.AreEqual(UnitTestProtoFile.RepeatedNestedEnumExtensionFieldNumber, 51);
    }

    [Test]
    public void EmptyPackedValue() {
      TestPackedTypes empty = new TestPackedTypes.Builder().Build();
      Assert.AreEqual(0, empty.SerializedSize);
    }
  }
}
