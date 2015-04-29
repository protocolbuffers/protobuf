#region Copyright notice and license

// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
// Author: jieluo@google.com (Jie Luo)
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
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#endregion

using System;
using System.Reflection;
using System.Collections.Generic;
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.TestProtos;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Google.ProtocolBuffers
{
    [TestClass]
    class FieldPresenceTest
    {
        private void CheckHasMethodRemoved(Type proto2Type, Type proto3Type, string name)
        {
            Assert.NotNull(proto2Type.GetProperty(name));
            Assert.NotNull(proto2Type.GetProperty("Has" + name));
            Assert.NotNull(proto3Type.GetProperty(name));
            Assert.Null(proto3Type.GetProperty("Has" + name));
        }

        [TestMethod]
        public void TestHasMethod()
        {
            // Optional non-message fields don't have HasFoo method generated
            Type proto2Type = typeof(TestAllTypes);
            Type proto3Type = typeof(FieldPresence.TestAllTypes);
            CheckHasMethodRemoved(proto2Type, proto3Type, "OptionalInt32");
            CheckHasMethodRemoved(proto2Type, proto3Type, "OptionalString");
            CheckHasMethodRemoved(proto2Type, proto3Type, "OptionalBytes");
            CheckHasMethodRemoved(proto2Type, proto3Type, "OptionalNestedEnum");

            proto2Type = typeof(TestAllTypes.Builder);
            proto3Type = typeof(FieldPresence.TestAllTypes.Builder);
            CheckHasMethodRemoved(proto2Type, proto3Type, "OptionalInt32");
            CheckHasMethodRemoved(proto2Type, proto3Type, "OptionalString");
            CheckHasMethodRemoved(proto2Type, proto3Type, "OptionalBytes");
            CheckHasMethodRemoved(proto2Type, proto3Type, "OptionalNestedEnum");

            // message fields still have the HasFoo method generated
            Assert.False(FieldPresence.TestAllTypes.CreateBuilder().Build().HasOptionalNestedMessage);
            Assert.False(FieldPresence.TestAllTypes.CreateBuilder().HasOptionalNestedMessage);
        }

        [TestMethod]
        public void TestFieldPresence()
        {
            // Optional non-message fields set to their default value are treated the same
            // way as not set.

            // Serialization will ignore such fields.
            FieldPresence.TestAllTypes.Builder builder = FieldPresence.TestAllTypes.CreateBuilder();
            builder.SetOptionalInt32(0);
            builder.SetOptionalString("");
            builder.SetOptionalBytes(ByteString.Empty);
            builder.SetOptionalNestedEnum(FieldPresence.TestAllTypes.Types.NestedEnum.FOO);
            FieldPresence.TestAllTypes message = builder.Build();
            Assert.AreEqual(0, message.SerializedSize);

            // Test merge
            FieldPresence.TestAllTypes.Builder a = FieldPresence.TestAllTypes.CreateBuilder();
            a.SetOptionalInt32(1);
            a.SetOptionalString("x");
            a.SetOptionalBytes(ByteString.CopyFromUtf8("y"));
            a.SetOptionalNestedEnum(FieldPresence.TestAllTypes.Types.NestedEnum.BAR);
            a.MergeFrom(message);
            FieldPresence.TestAllTypes messageA = a.Build();
            Assert.AreEqual(1, messageA.OptionalInt32);
            Assert.AreEqual("x", messageA.OptionalString);
            Assert.AreEqual(ByteString.CopyFromUtf8("y"), messageA.OptionalBytes);
            Assert.AreEqual(FieldPresence.TestAllTypes.Types.NestedEnum.BAR, messageA.OptionalNestedEnum);

            // equals/hashCode should produce the same results
            FieldPresence.TestAllTypes empty = FieldPresence.TestAllTypes.CreateBuilder().Build();
            Assert.True(empty.Equals(message));
            Assert.True(message.Equals(empty));
            Assert.AreEqual(empty.GetHashCode(), message.GetHashCode());
        }

        [TestMethod]
        public void TestFieldPresenceReflection()
        {
            MessageDescriptor descriptor = FieldPresence.TestAllTypes.Descriptor;
            FieldDescriptor optionalInt32Field = descriptor.FindFieldByName("optional_int32");
            FieldDescriptor optionalStringField = descriptor.FindFieldByName("optional_string");
            FieldDescriptor optionalBytesField = descriptor.FindFieldByName("optional_bytes");
            FieldDescriptor optionalNestedEnumField = descriptor.FindFieldByName("optional_nested_enum");

            FieldPresence.TestAllTypes message = FieldPresence.TestAllTypes.CreateBuilder().Build();
            Assert.False(message.HasField(optionalInt32Field));
            Assert.False(message.HasField(optionalStringField));
            Assert.False(message.HasField(optionalBytesField));
            Assert.False(message.HasField(optionalNestedEnumField));

            // Set to default value is seen as not present
            message = FieldPresence.TestAllTypes.CreateBuilder()
                .SetOptionalInt32(0)
                .SetOptionalString("")
                .SetOptionalBytes(ByteString.Empty)
                .SetOptionalNestedEnum(FieldPresence.TestAllTypes.Types.NestedEnum.FOO)
                .Build();
            Assert.False(message.HasField(optionalInt32Field));
            Assert.False(message.HasField(optionalStringField));
            Assert.False(message.HasField(optionalBytesField));
            Assert.False(message.HasField(optionalNestedEnumField));
            Assert.AreEqual(0, message.AllFields.Count);
            
            // Set t0 non-defalut value is seen as present
            message = FieldPresence.TestAllTypes.CreateBuilder()
                .SetOptionalInt32(1)
                .SetOptionalString("x")
                .SetOptionalBytes(ByteString.CopyFromUtf8("y"))
                .SetOptionalNestedEnum(FieldPresence.TestAllTypes.Types.NestedEnum.BAR)
                .Build();
            Assert.True(message.HasField(optionalInt32Field));
            Assert.True(message.HasField(optionalStringField));
            Assert.True(message.HasField(optionalBytesField));
            Assert.True(message.HasField(optionalNestedEnumField));
            Assert.AreEqual(4, message.AllFields.Count);
        }

        [TestMethod]
        public void TestMessageField()
        {
            FieldPresence.TestAllTypes.Builder builder = FieldPresence.TestAllTypes.CreateBuilder();
            Assert.False(builder.HasOptionalNestedMessage);
            Assert.False(builder.Build().HasOptionalNestedMessage);

            // Unlike non-message fields, if we set default value to message field, the field
            // shoule be seem as present.
            builder.SetOptionalNestedMessage(FieldPresence.TestAllTypes.Types.NestedMessage.DefaultInstance);
            Assert.True(builder.HasOptionalNestedMessage);
            Assert.True(builder.Build().HasOptionalNestedMessage);

        }

        [TestMethod]
        public void TestSeralizeAndParese()
        {
            FieldPresence.TestAllTypes.Builder builder = FieldPresence.TestAllTypes.CreateBuilder();
            builder.SetOptionalInt32(1234);
            builder.SetOptionalString("hello");
            builder.SetOptionalNestedMessage(FieldPresence.TestAllTypes.Types.NestedMessage.DefaultInstance);
            ByteString data = builder.Build().ToByteString();

            FieldPresence.TestAllTypes message = FieldPresence.TestAllTypes.ParseFrom(data);
            Assert.AreEqual(1234, message.OptionalInt32);
            Assert.AreEqual("hello", message.OptionalString);
            Assert.AreEqual(ByteString.Empty, message.OptionalBytes);
            Assert.AreEqual(FieldPresence.TestAllTypes.Types.NestedEnum.FOO, message.OptionalNestedEnum);
            Assert.True(message.HasOptionalNestedMessage);
            Assert.AreEqual(0, message.OptionalNestedMessage.Value);
        }
    }
}
