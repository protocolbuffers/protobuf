#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using Google.Protobuf.TestProtos;
using Proto2 = Google.Protobuf.TestProtos.Proto2;
using NUnit.Framework;
using System;
using System.Collections;
using System.Collections.Generic;

using static Google.Protobuf.TestProtos.Proto2.UnittestExtensions;
using ProtobufUnittest;

namespace Google.Protobuf.Reflection
{
    public class FieldAccessTest
    {
        [Test]
        public void GetValue()
        {
            var message = SampleMessages.CreateFullTestAllTypes();
            var fields = TestProtos.TestAllTypes.Descriptor.Fields;
            Assert.AreEqual(message.SingleBool, fields[TestProtos.TestAllTypes.SingleBoolFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SingleBytes, fields[TestProtos.TestAllTypes.SingleBytesFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SingleDouble, fields[TestProtos.TestAllTypes.SingleDoubleFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SingleFixed32, fields[TestProtos.TestAllTypes.SingleFixed32FieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SingleFixed64, fields[TestProtos.TestAllTypes.SingleFixed64FieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SingleFloat, fields[TestProtos.TestAllTypes.SingleFloatFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SingleForeignEnum, fields[TestProtos.TestAllTypes.SingleForeignEnumFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SingleForeignMessage, fields[TestProtos.TestAllTypes.SingleForeignMessageFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SingleImportEnum, fields[TestProtos.TestAllTypes.SingleImportEnumFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SingleImportMessage, fields[TestProtos.TestAllTypes.SingleImportMessageFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SingleInt32, fields[TestProtos.TestAllTypes.SingleInt32FieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SingleInt64, fields[TestProtos.TestAllTypes.SingleInt64FieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SingleNestedEnum, fields[TestProtos.TestAllTypes.SingleNestedEnumFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SingleNestedMessage, fields[TestProtos.TestAllTypes.SingleNestedMessageFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SinglePublicImportMessage, fields[TestProtos.TestAllTypes.SinglePublicImportMessageFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SingleSint32, fields[TestProtos.TestAllTypes.SingleSint32FieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SingleSint64, fields[TestProtos.TestAllTypes.SingleSint64FieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SingleString, fields[TestProtos.TestAllTypes.SingleStringFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SingleSfixed32, fields[TestProtos.TestAllTypes.SingleSfixed32FieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SingleSfixed64, fields[TestProtos.TestAllTypes.SingleSfixed64FieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SingleUint32, fields[TestProtos.TestAllTypes.SingleUint32FieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.SingleUint64, fields[TestProtos.TestAllTypes.SingleUint64FieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.OneofBytes, fields[TestProtos.TestAllTypes.OneofBytesFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.OneofString, fields[TestProtos.TestAllTypes.OneofStringFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.OneofNestedMessage, fields[TestProtos.TestAllTypes.OneofNestedMessageFieldNumber].Accessor.GetValue(message));
            Assert.AreEqual(message.OneofUint32, fields[TestProtos.TestAllTypes.OneofUint32FieldNumber].Accessor.GetValue(message));

            // Just one example for repeated fields - they're all just returning the list
            var list = (IList) fields[TestProtos.TestAllTypes.RepeatedInt32FieldNumber].Accessor.GetValue(message);
            Assert.AreEqual(message.RepeatedInt32, list);
            Assert.AreEqual(message.RepeatedInt32[0], list[0]); // Just in case there was any doubt...

            // Just a single map field, for the same reason
            var mapMessage = new TestMap { MapStringString = { { "key1", "value1" }, { "key2", "value2" } } };
            fields = TestMap.Descriptor.Fields;
            var dictionary = (IDictionary) fields[TestMap.MapStringStringFieldNumber].Accessor.GetValue(mapMessage);
            Assert.AreEqual(mapMessage.MapStringString, dictionary);
            Assert.AreEqual("value1", dictionary["key1"]);
        }

        [Test]
        public void GetValue_IncorrectType()
        {
            IMessage message = SampleMessages.CreateFullTestAllTypes();
            var fields = message.Descriptor.Fields;
            Assert.Throws<InvalidCastException>(() => fields[TestProtos.TestAllTypes.SingleBoolFieldNumber].Accessor.GetValue(new TestMap()));
        }

        [Test]
        public void HasValue_Proto3_Message()
        {
            var message = new TestAllTypes();
            var accessor = ((IMessage) message).Descriptor.Fields[TestProtos.TestAllTypes.SingleForeignMessageFieldNumber].Accessor;
            Assert.False(accessor.HasValue(message));
            message.SingleForeignMessage = new ForeignMessage();
            Assert.True(accessor.HasValue(message));
            message.SingleForeignMessage = null;
            Assert.False(accessor.HasValue(message));
        }

        [Test]
        public void HasValue_Proto3_Oneof()
        {
            TestAllTypes message = new TestAllTypes();
            var accessor = ((IMessage) message).Descriptor.Fields[TestProtos.TestAllTypes.OneofStringFieldNumber].Accessor;
            Assert.False(accessor.HasValue(message));
            // Even though it's the default value, we still have a value.
            message.OneofString = "";
            Assert.True(accessor.HasValue(message));
            message.OneofString = "hello";
            Assert.True(accessor.HasValue(message));
            message.OneofUint32 = 10;
            Assert.False(accessor.HasValue(message));
        }

        [Test]
        public void HasValue_Proto3_Primitive_Optional()
        {
            var message = new TestProto3Optional();
            var accessor = ((IMessage) message).Descriptor.Fields[TestProto3Optional.OptionalInt64FieldNumber].Accessor;
            Assert.IsFalse(accessor.HasValue(message));
            message.OptionalInt64 = 5L;
            Assert.IsTrue(accessor.HasValue(message));
            message.ClearOptionalInt64();
            Assert.IsFalse(accessor.HasValue(message));
            message.OptionalInt64 = 0L;
            Assert.IsTrue(accessor.HasValue(message));
        }

        [Test]
        public void HasValue_Proto3_Primitive_NotOptional()
        {
            IMessage message = SampleMessages.CreateFullTestAllTypes();
            var fields = message.Descriptor.Fields;
            Assert.Throws<InvalidOperationException>(() => fields[TestProtos.TestAllTypes.SingleBoolFieldNumber].Accessor.HasValue(message));
        }

        [Test]
        public void HasValue_Proto3_Repeated()
        {
            var message = new TestAllTypes();
            var accessor = ((IMessage) message).Descriptor.Fields[TestProtos.TestAllTypes.RepeatedBoolFieldNumber].Accessor;
            Assert.Throws<InvalidOperationException>(() => accessor.HasValue(message));
        }

        [Test]
        public void HasValue_Proto2_Primitive()
        {
            var message = new Proto2.TestAllTypes();
            var accessor = ((IMessage) message).Descriptor.Fields[Proto2.TestAllTypes.OptionalInt64FieldNumber].Accessor;

            Assert.IsFalse(accessor.HasValue(message));
            message.OptionalInt64 = 5L;
            Assert.IsTrue(accessor.HasValue(message));
            message.ClearOptionalInt64();
            Assert.IsFalse(accessor.HasValue(message));
            message.OptionalInt64 = 0L;
            Assert.IsTrue(accessor.HasValue(message));
        }

        [Test]
        public void HasValue_Proto2_Message()
        {
            var message = new Proto2.TestAllTypes();
            var field = ((IMessage) message).Descriptor.Fields[Proto2.TestAllTypes.OptionalForeignMessageFieldNumber];
            Assert.False(field.Accessor.HasValue(message));
            message.OptionalForeignMessage = new Proto2.ForeignMessage();
            Assert.True(field.Accessor.HasValue(message));
            message.OptionalForeignMessage = null;
            Assert.False(field.Accessor.HasValue(message));
        }

        [Test]
        public void HasValue_Proto2_Oneof()
        {
            var message = new Proto2.TestAllTypes();
            var accessor = ((IMessage) message).Descriptor.Fields[Proto2.TestAllTypes.OneofStringFieldNumber].Accessor;
            Assert.False(accessor.HasValue(message));
            // Even though it's the default value, we still have a value.
            message.OneofString = "";
            Assert.True(accessor.HasValue(message));
            message.OneofString = "hello";
            Assert.True(accessor.HasValue(message));
            message.OneofUint32 = 10;
            Assert.False(accessor.HasValue(message));
        }

        [Test]
        public void HasValue_Proto2_Repeated()
        {
            var message = new Proto2.TestAllTypes();
            var accessor = ((IMessage) message).Descriptor.Fields[Proto2.TestAllTypes.RepeatedBoolFieldNumber].Accessor;
            Assert.Throws<InvalidOperationException>(() => accessor.HasValue(message));
        }

        [Test]
        public void SetValue_SingleFields()
        {
            // Just a sample (primitives, messages, enums, strings, byte strings)
            var message = SampleMessages.CreateFullTestAllTypes();
            var fields = TestProtos.TestAllTypes.Descriptor.Fields;
            fields[TestProtos.TestAllTypes.SingleBoolFieldNumber].Accessor.SetValue(message, false);
            fields[TestProtos.TestAllTypes.SingleInt32FieldNumber].Accessor.SetValue(message, 500);
            fields[TestProtos.TestAllTypes.SingleStringFieldNumber].Accessor.SetValue(message, "It's a string");
            fields[TestProtos.TestAllTypes.SingleBytesFieldNumber].Accessor.SetValue(message, ByteString.CopyFrom(99, 98, 97));
            fields[TestProtos.TestAllTypes.SingleForeignEnumFieldNumber].Accessor.SetValue(message, ForeignEnum.ForeignFoo);
            fields[TestProtos.TestAllTypes.SingleForeignMessageFieldNumber].Accessor.SetValue(message, new ForeignMessage { C = 12345 });
            fields[TestProtos.TestAllTypes.SingleDoubleFieldNumber].Accessor.SetValue(message, 20150701.5);

            var expected = new TestAllTypes(SampleMessages.CreateFullTestAllTypes())
            {
                SingleBool = false,
                SingleInt32 = 500,
                SingleString = "It's a string",
                SingleBytes = ByteString.CopyFrom(99, 98, 97),
                SingleForeignEnum = ForeignEnum.ForeignFoo,
                SingleForeignMessage = new ForeignMessage { C = 12345 },
                SingleDouble = 20150701.5
            };

            Assert.AreEqual(expected, message);
        }

        [Test]
        public void SetValue_SingleFields_WrongType()
        {
            IMessage message = SampleMessages.CreateFullTestAllTypes();
            var fields = message.Descriptor.Fields;
            Assert.Throws<InvalidCastException>(() => fields[TestProtos.TestAllTypes.SingleBoolFieldNumber].Accessor.SetValue(message, "This isn't a bool"));
        }

        [Test]
        public void SetValue_MapFields()
        {
            IMessage message = new TestMap();
            var fields = message.Descriptor.Fields;
            Assert.Throws<InvalidOperationException>(() => fields[TestMap.MapStringStringFieldNumber].Accessor.SetValue(message, new Dictionary<string, string>()));
        }

        [Test]
        public void SetValue_RepeatedFields()
        {
            IMessage message = SampleMessages.CreateFullTestAllTypes();
            var fields = message.Descriptor.Fields;
            Assert.Throws<InvalidOperationException>(() => fields[TestProtos.TestAllTypes.RepeatedDoubleFieldNumber].Accessor.SetValue(message, new double[10]));
        }

        [Test]
        public void Oneof()
        {
            var message = new TestAllTypes();
            var descriptor = TestProtos.TestAllTypes.Descriptor;
            Assert.AreEqual(1, descriptor.Oneofs.Count);
            var oneof = descriptor.Oneofs[0];
            Assert.AreEqual("oneof_field", oneof.Name);
            Assert.IsNull(oneof.Accessor.GetCaseFieldDescriptor(message));

            message.OneofString = "foo";
            Assert.AreSame(descriptor.Fields[TestProtos.TestAllTypes.OneofStringFieldNumber], oneof.Accessor.GetCaseFieldDescriptor(message));

            message.OneofUint32 = 10;
            Assert.AreSame(descriptor.Fields[TestProtos.TestAllTypes.OneofUint32FieldNumber], oneof.Accessor.GetCaseFieldDescriptor(message));

            oneof.Accessor.Clear(message);
            Assert.AreEqual(TestProtos.TestAllTypes.OneofFieldOneofCase.None, message.OneofFieldCase);
        }

        [Test]
        public void Clear()
        {
            var message = SampleMessages.CreateFullTestAllTypes();
            var fields = TestProtos.TestAllTypes.Descriptor.Fields;
            fields[TestProtos.TestAllTypes.SingleBoolFieldNumber].Accessor.Clear(message);
            fields[TestProtos.TestAllTypes.SingleInt32FieldNumber].Accessor.Clear(message);
            fields[TestProtos.TestAllTypes.SingleStringFieldNumber].Accessor.Clear(message);
            fields[TestProtos.TestAllTypes.SingleBytesFieldNumber].Accessor.Clear(message);
            fields[TestProtos.TestAllTypes.SingleForeignEnumFieldNumber].Accessor.Clear(message);
            fields[TestProtos.TestAllTypes.SingleForeignMessageFieldNumber].Accessor.Clear(message);
            fields[TestProtos.TestAllTypes.RepeatedDoubleFieldNumber].Accessor.Clear(message);

            var expected = new TestAllTypes(SampleMessages.CreateFullTestAllTypes())
            {
                SingleBool = false,
                SingleInt32 = 0,
                SingleString = "",
                SingleBytes = ByteString.Empty,
                SingleForeignEnum = 0,
                SingleForeignMessage = null,
            };
            expected.RepeatedDouble.Clear();

            Assert.AreEqual(expected, message);

            // Separately, maps.
            var mapMessage = new TestMap { MapStringString = { { "key1", "value1" }, { "key2", "value2" } } };
            fields = TestMap.Descriptor.Fields;
            fields[TestMap.MapStringStringFieldNumber].Accessor.Clear(mapMessage);
            Assert.AreEqual(0, mapMessage.MapStringString.Count);
        }

        [Test]
        public void Clear_Proto3Optional()
        {
            TestProto3Optional message = new TestProto3Optional
            {
                OptionalInt32 = 0,
                OptionalNestedMessage = new TestProto3Optional.Types.NestedMessage()
            };
            var primitiveField = TestProto3Optional.Descriptor.Fields[TestProto3Optional.OptionalInt32FieldNumber];
            var messageField = TestProto3Optional.Descriptor.Fields[TestProto3Optional.OptionalNestedMessageFieldNumber];

            Assert.True(message.HasOptionalInt32);
            Assert.NotNull(message.OptionalNestedMessage);

            primitiveField.Accessor.Clear(message);
            messageField.Accessor.Clear(message);

            Assert.False(message.HasOptionalInt32);
            Assert.Null(message.OptionalNestedMessage);
        }

        [Test]
        public void Clear_Proto3_Oneof()
        {
            var message = new TestAllTypes();
            var accessor = ((IMessage) message).Descriptor.Fields[TestProtos.TestAllTypes.OneofUint32FieldNumber].Accessor;

            // The field accessor Clear method only affects a oneof if the current case is the one being cleared.
            message.OneofString = "hello";
            Assert.AreEqual(TestProtos.TestAllTypes.OneofFieldOneofCase.OneofString, message.OneofFieldCase);
            accessor.Clear(message);
            Assert.AreEqual(TestProtos.TestAllTypes.OneofFieldOneofCase.OneofString, message.OneofFieldCase);

            message.OneofUint32 = 100;
            Assert.AreEqual(TestProtos.TestAllTypes.OneofFieldOneofCase.OneofUint32, message.OneofFieldCase);
            accessor.Clear(message);
            Assert.AreEqual(TestProtos.TestAllTypes.OneofFieldOneofCase.None, message.OneofFieldCase);
        }

        [Test]
        public void Clear_Proto2_Oneof()
        {
            var message = new Proto2.TestAllTypes();
            var accessor = ((IMessage) message).Descriptor.Fields[Proto2.TestAllTypes.OneofUint32FieldNumber].Accessor;

            // The field accessor Clear method only affects a oneof if the current case is the one being cleared.
            message.OneofString = "hello";
            Assert.AreEqual(Proto2.TestAllTypes.OneofFieldOneofCase.OneofString, message.OneofFieldCase);
            accessor.Clear(message);
            Assert.AreEqual(Proto2.TestAllTypes.OneofFieldOneofCase.OneofString, message.OneofFieldCase);

            message.OneofUint32 = 100;
            Assert.AreEqual(Proto2.TestAllTypes.OneofFieldOneofCase.OneofUint32, message.OneofFieldCase);
            accessor.Clear(message);
            Assert.AreEqual(Proto2.TestAllTypes.OneofFieldOneofCase.None, message.OneofFieldCase);
        }

        [Test]
        public void FieldDescriptor_ByName()
        {
            var descriptor = TestProtos.TestAllTypes.Descriptor;
            Assert.AreSame(
                descriptor.Fields[TestProtos.TestAllTypes.SingleBoolFieldNumber],
                descriptor.Fields["single_bool"]);
        }

        [Test]
        public void FieldDescriptor_NotFound()
        {
            var descriptor = TestProtos.TestAllTypes.Descriptor;
            Assert.Throws<KeyNotFoundException>(() => descriptor.Fields[999999].ToString());
            Assert.Throws<KeyNotFoundException>(() => descriptor.Fields["not found"].ToString());
        }

        [Test]
        public void GetExtensionValue()
        {
            var message = SampleMessages.CreateFullTestAllExtensions();

            // test that the reflector works, since the reflector just runs through IExtendableMessage
            Assert.AreEqual(message.GetExtension(OptionalBoolExtension), Proto2.TestAllExtensions.Descriptor.FindFieldByNumber(OptionalBoolExtension.FieldNumber).Accessor.GetValue(message));
        }

        [Test]
        public void GetRepeatedExtensionValue()
        {
            // check to make sure repeated accessor uses GetOrRegister
            var message = new Proto2.TestAllExtensions();

            Assert.IsNull(message.GetExtension(RepeatedBoolExtension));
            Assert.IsNotNull(Proto2.TestAllExtensions.Descriptor.FindFieldByNumber(RepeatedBoolExtension.FieldNumber).Accessor.GetValue(message));
            Assert.IsNotNull(message.GetExtension(RepeatedBoolExtension));

            message.ClearExtension(RepeatedBoolExtension);
            Assert.IsNull(message.GetExtension(RepeatedBoolExtension));
        }

        [Test]
        public void HasPresence()
        {
            // Proto3
            var fields = TestProtos.TestAllTypes.Descriptor.Fields;
            Assert.IsFalse(fields[TestProtos.TestAllTypes.SingleBoolFieldNumber].HasPresence);
            Assert.IsTrue(fields[TestProtos.TestAllTypes.OneofBytesFieldNumber].HasPresence);
            Assert.IsTrue(fields[TestProtos.TestAllTypes.SingleForeignMessageFieldNumber].HasPresence);
            Assert.IsFalse(fields[TestProtos.TestAllTypes.RepeatedBoolFieldNumber].HasPresence);

            fields = TestMap.Descriptor.Fields;
            Assert.IsFalse(fields[TestMap.MapBoolBoolFieldNumber].HasPresence);

            fields = TestProto3Optional.Descriptor.Fields;
            Assert.IsTrue(fields[TestProto3Optional.OptionalBoolFieldNumber].HasPresence);

            // Proto2
            fields = Proto2.TestAllTypes.Descriptor.Fields;
            Assert.IsTrue(fields[Proto2.TestAllTypes.OptionalBoolFieldNumber].HasPresence);
            Assert.IsTrue(fields[Proto2.TestAllTypes.OneofBytesFieldNumber].HasPresence);
            Assert.IsTrue(fields[Proto2.TestAllTypes.OptionalForeignMessageFieldNumber].HasPresence);
            Assert.IsFalse(fields[Proto2.TestAllTypes.RepeatedBoolFieldNumber].HasPresence);

            fields = Proto2.TestRequired.Descriptor.Fields;
            Assert.IsTrue(fields[Proto2.TestRequired.AFieldNumber].HasPresence);
        }
    }
}
