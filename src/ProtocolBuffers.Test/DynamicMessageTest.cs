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
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class DynamicMessageTest {

    private ReflectionTester reflectionTester;
    private ReflectionTester extensionsReflectionTester;
    private ReflectionTester packedReflectionTester;

    [SetUp]
    public void SetUp() {
      reflectionTester = ReflectionTester.CreateTestAllTypesInstance();
      extensionsReflectionTester = ReflectionTester.CreateTestAllExtensionsInstance();
      packedReflectionTester = ReflectionTester.CreateTestPackedTypesInstance();
    }

    [Test]
    public void DynamicMessageAccessors() {
      IBuilder builder = DynamicMessage.CreateBuilder(TestAllTypes.Descriptor);
      reflectionTester.SetAllFieldsViaReflection(builder);
      IMessage message = builder.WeakBuild();
      reflectionTester.AssertAllFieldsSetViaReflection(message);
    }

    [Test]
    public void DoubleBuildError() {
      DynamicMessage.Builder builder = DynamicMessage.CreateBuilder(TestAllTypes.Descriptor);
      builder.Build();
      try {
        builder.Build();
        Assert.Fail("Should have thrown exception.");
      } catch (InvalidOperationException) {
        // Success.
      }
    }

    [Test]
    public void DynamicMessageSettersRejectNull() {
      IBuilder builder = DynamicMessage.CreateBuilder(TestAllTypes.Descriptor);
      reflectionTester.AssertReflectionSettersRejectNull(builder);
	  }

    [Test]
    public void DynamicMessageExtensionAccessors() {
      // We don't need to extensively test DynamicMessage's handling of
      // extensions because, frankly, it doesn't do anything special with them.
      // It treats them just like any other fields.
      IBuilder builder = DynamicMessage.CreateBuilder(TestAllExtensions.Descriptor);
      extensionsReflectionTester.SetAllFieldsViaReflection(builder);
      IMessage message = builder.WeakBuild();
      extensionsReflectionTester.AssertAllFieldsSetViaReflection(message);
    }

    [Test]
    public void DynamicMessageExtensionSettersRejectNull() {
      IBuilder builder = DynamicMessage.CreateBuilder(TestAllExtensions.Descriptor);
      extensionsReflectionTester.AssertReflectionSettersRejectNull(builder);
	  }

    [Test]
    public void DynamicMessageRepeatedSetters() {
      IBuilder builder = DynamicMessage.CreateBuilder(TestAllTypes.Descriptor);
      reflectionTester.SetAllFieldsViaReflection(builder);
      reflectionTester.ModifyRepeatedFieldsViaReflection(builder);
      IMessage message = builder.WeakBuild();
      reflectionTester.AssertRepeatedFieldsModifiedViaReflection(message);
    }

    [Test]
    public void DynamicMessageRepeatedSettersRejectNull() {
      IBuilder builder = DynamicMessage.CreateBuilder(TestAllTypes.Descriptor);
      reflectionTester.AssertReflectionRepeatedSettersRejectNull(builder);
    }

    [Test]
    public void DynamicMessageDefaults() {
      reflectionTester.AssertClearViaReflection(DynamicMessage.GetDefaultInstance(TestAllTypes.Descriptor));
      reflectionTester.AssertClearViaReflection(DynamicMessage.CreateBuilder(TestAllTypes.Descriptor).Build());
    }

    [Test]
    public void DynamicMessageSerializedSize() {
      TestAllTypes message = TestUtil.GetAllSet();

      IBuilder dynamicBuilder = DynamicMessage.CreateBuilder(TestAllTypes.Descriptor);
      reflectionTester.SetAllFieldsViaReflection(dynamicBuilder);
      IMessage dynamicMessage = dynamicBuilder.WeakBuild();

      Assert.AreEqual(message.SerializedSize, dynamicMessage.SerializedSize);
    }

    [Test]
    public void DynamicMessageSerialization() {
      IBuilder builder =  DynamicMessage.CreateBuilder(TestAllTypes.Descriptor);
      reflectionTester.SetAllFieldsViaReflection(builder);
      IMessage message = builder.WeakBuild();

      ByteString rawBytes = message.ToByteString();
      TestAllTypes message2 = TestAllTypes.ParseFrom(rawBytes);

      TestUtil.AssertAllFieldsSet(message2);

      // In fact, the serialized forms should be exactly the same, byte-for-byte.
      Assert.AreEqual(TestUtil.GetAllSet().ToByteString(), rawBytes);
    }

    [Test]
    public void DynamicMessageParsing() {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      TestUtil.SetAllFields(builder);
      TestAllTypes message = builder.Build();

      ByteString rawBytes = message.ToByteString();

      IMessage message2 = DynamicMessage.ParseFrom(TestAllTypes.Descriptor, rawBytes);
      reflectionTester.AssertAllFieldsSetViaReflection(message2);
    }

    [Test]
      public void DynamicMessagePackedSerialization() {
    IBuilder builder = DynamicMessage.CreateBuilder(TestPackedTypes.Descriptor);
    packedReflectionTester.SetPackedFieldsViaReflection(builder);
    IMessage message = builder.WeakBuild();

    ByteString rawBytes = message.ToByteString();
    TestPackedTypes message2 = TestPackedTypes.ParseFrom(rawBytes);

    TestUtil.AssertPackedFieldsSet(message2);

    // In fact, the serialized forms should be exactly the same, byte-for-byte.
    Assert.AreEqual(TestUtil.GetPackedSet().ToByteString(), rawBytes);
    }

    [Test]
  public void testDynamicMessagePackedParsing() {
    TestPackedTypes.Builder builder = TestPackedTypes.CreateBuilder();
    TestUtil.SetPackedFields(builder);
    TestPackedTypes message = builder.Build();

    ByteString rawBytes = message.ToByteString();

    IMessage message2 = DynamicMessage.ParseFrom(TestPackedTypes.Descriptor, rawBytes);
    packedReflectionTester.AssertPackedFieldsSetViaReflection(message2);
  }

    [Test]
    public void DynamicMessageCopy() {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      TestUtil.SetAllFields(builder);
      TestAllTypes message = builder.Build();

      DynamicMessage copy = DynamicMessage.CreateBuilder(message).Build();
      reflectionTester.AssertAllFieldsSetViaReflection(copy);
    }

    [Test]
    public void ToBuilder() {
      DynamicMessage.Builder builder =
          DynamicMessage.CreateBuilder(TestAllTypes.Descriptor);
      reflectionTester.SetAllFieldsViaReflection(builder);
      int unknownFieldNum = 9;
      ulong unknownFieldVal = 90;
      builder.SetUnknownFields(UnknownFieldSet.CreateBuilder()
          .AddField(unknownFieldNum,
              UnknownField.CreateBuilder().AddVarint(unknownFieldVal).Build())
          .Build());
      DynamicMessage message = builder.Build();

      DynamicMessage derived = message.ToBuilder().Build();
      reflectionTester.AssertAllFieldsSetViaReflection(derived);

      IList<ulong> values = derived.UnknownFields.FieldDictionary[unknownFieldNum].VarintList;
      Assert.AreEqual(1, values.Count);
      Assert.AreEqual(unknownFieldVal, values[0]);
    }
  }
}
