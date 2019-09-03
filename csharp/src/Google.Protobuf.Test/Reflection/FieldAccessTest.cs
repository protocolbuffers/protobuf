#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
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

using Google.Protobuf.TestProtos;
using Proto2 = Google.Protobuf.TestProtos.Proto2;
using NUnit.Framework;
using System;
using System.Collections;
using System.Collections.Generic;

using static Google.Protobuf.TestProtos.Proto2.UnittestExtensions;

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
        public void HasValue_Proto3()
        {
            IMessage message = SampleMessages.CreateFullTestAllTypes();
            var fields = message.Descriptor.Fields;
            Assert.Throws<InvalidOperationException>(() => fields[TestProtos.TestAllTypes.SingleBoolFieldNumber].Accessor.HasValue(message));
        }

        [Test]
        public void HasValue()
        {
            IMessage message = new Proto2.TestAllTypes();
            var fields = message.Descriptor.Fields;
            var accessor = fields[Proto2.TestAllTypes.OptionalBoolFieldNumber].Accessor;

            Assert.False(accessor.HasValue(message));

            accessor.SetValue(message, true);
            Assert.True(accessor.HasValue(message));

            accessor.Clear(message);
            Assert.False(accessor.HasValue(message));
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
    }
}
