using Google.ProtocolBuffers.TestProtos;
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  [TestFixture]
  public class DynamicMessageTest {

    private ReflectionTester reflectionTester;
    private ReflectionTester extensionsReflectionTester;

    [SetUp]
    public void SetUp() {
      reflectionTester = ReflectionTester.CreateTestAllTypesInstance();
      extensionsReflectionTester = ReflectionTester.CreateTestAllExtensionsInstance();
    }

    [Test]
    public void DynamicMessageAccessors() {
      IBuilder builder = DynamicMessage.CreateBuilder(TestAllTypes.Descriptor);
      reflectionTester.SetAllFieldsViaReflection(builder);
      IMessage message = builder.WeakBuild();
      reflectionTester.AssertAllFieldsSetViaReflection(message);
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
    public void DynamicMessageRepeatedSetters() {
      IBuilder builder = DynamicMessage.CreateBuilder(TestAllTypes.Descriptor);
      reflectionTester.SetAllFieldsViaReflection(builder);
      reflectionTester.ModifyRepeatedFieldsViaReflection(builder);
      IMessage message = builder.WeakBuild();
      reflectionTester.AssertRepeatedFieldsModifiedViaReflection(message);
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
    public void DynamicMessageCopy() {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      TestUtil.SetAllFields(builder);
      TestAllTypes message = builder.Build();

      DynamicMessage copy = DynamicMessage.CreateBuilder(message).Build();
      reflectionTester.AssertAllFieldsSetViaReflection(copy);
    }
  }
}
