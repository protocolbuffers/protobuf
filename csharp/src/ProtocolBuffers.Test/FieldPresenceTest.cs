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
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.TestProtos.FieldPresence;
using Xunit;

namespace Google.ProtocolBuffers
{
    public class FieldPresenceTest
    {
        private void CheckHasMethodRemoved(Type proto2Type, Type proto3Type, string name)
        {
            Assert.NotNull(proto2Type.GetProperty(name));
            Assert.NotNull(proto2Type.GetProperty("Has" + name));
            Assert.NotNull(proto3Type.GetProperty(name));
            Assert.Null(proto3Type.GetProperty("Has" + name));
        }

        [Fact]
        public void TestHasMethod()
        {
            // Optional non-message fields don't have HasFoo method generated
            Type proto2Type = typeof(Google.ProtocolBuffers.TestProtos.TestAllTypes);
            Type proto3Type = typeof(TestAllTypes);
            CheckHasMethodRemoved(proto2Type, proto3Type, "OptionalInt32");
            CheckHasMethodRemoved(proto2Type, proto3Type, "OptionalString");
            CheckHasMethodRemoved(proto2Type, proto3Type, "OptionalBytes");
            CheckHasMethodRemoved(proto2Type, proto3Type, "OptionalNestedEnum");
            
            proto2Type = typeof(Google.ProtocolBuffers.TestProtos.TestAllTypes.Builder);
            proto3Type = typeof(TestAllTypes.Builder);
            CheckHasMethodRemoved(proto2Type, proto3Type, "OptionalInt32");
            CheckHasMethodRemoved(proto2Type, proto3Type, "OptionalString");
            CheckHasMethodRemoved(proto2Type, proto3Type, "OptionalBytes");
            CheckHasMethodRemoved(proto2Type, proto3Type, "OptionalNestedEnum");

            // message fields still have the HasFoo method generated
            Assert.False(TestAllTypes.CreateBuilder().Build().HasOptionalNestedMessage);
            Assert.False(TestAllTypes.CreateBuilder().HasOptionalNestedMessage);
        }

        [Fact]
        public void TestFieldPresence()
        {
            // Optional non-message fields set to their default value are treated the same
            // way as not set.

            // Serialization will ignore such fields.
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            builder.SetOptionalInt32(0);
            builder.SetOptionalString("");
            builder.SetOptionalBytes(ByteString.Empty);
            builder.SetOptionalNestedEnum(TestAllTypes.Types.NestedEnum.FOO);
            TestAllTypes message = builder.Build();
            Assert.Equal(0, message.SerializedSize);

            // Test merge
            TestAllTypes.Builder a = TestAllTypes.CreateBuilder();
            a.SetOptionalInt32(1);
            a.SetOptionalString("x");
            a.SetOptionalBytes(ByteString.CopyFromUtf8("y"));
            a.SetOptionalNestedEnum(TestAllTypes.Types.NestedEnum.BAR);
            a.MergeFrom(message);
            TestAllTypes messageA = a.Build();
            Assert.Equal(1, messageA.OptionalInt32);
            Assert.Equal("x", messageA.OptionalString);
            Assert.Equal(ByteString.CopyFromUtf8("y"), messageA.OptionalBytes);
            Assert.Equal(TestAllTypes.Types.NestedEnum.BAR, messageA.OptionalNestedEnum);

            // equals/hashCode should produce the same results
            TestAllTypes empty = TestAllTypes.CreateBuilder().Build();
            Assert.True(empty.Equals(message));
            Assert.True(message.Equals(empty));
            Assert.Equal(empty.GetHashCode(), message.GetHashCode());
        }

        [Fact]
        public void TestFieldPresenceReflection()
        {
            MessageDescriptor descriptor = TestAllTypes.Descriptor;
            FieldDescriptor optionalInt32Field = descriptor.FindFieldByName("optional_int32");
            FieldDescriptor optionalStringField = descriptor.FindFieldByName("optional_string");
            FieldDescriptor optionalBytesField = descriptor.FindFieldByName("optional_bytes");
            FieldDescriptor optionalNestedEnumField = descriptor.FindFieldByName("optional_nested_enum");

            TestAllTypes message = TestAllTypes.CreateBuilder().Build();
            Assert.False(message.HasField(optionalInt32Field));
            Assert.False(message.HasField(optionalStringField));
            Assert.False(message.HasField(optionalBytesField));
            Assert.False(message.HasField(optionalNestedEnumField));

            // Set to default value is seen as not present
            message = TestAllTypes.CreateBuilder()
                .SetOptionalInt32(0)
                .SetOptionalString("")
                .SetOptionalBytes(ByteString.Empty)
                .SetOptionalNestedEnum(TestAllTypes.Types.NestedEnum.FOO)
                .Build();
            Assert.False(message.HasField(optionalInt32Field));
            Assert.False(message.HasField(optionalStringField));
            Assert.False(message.HasField(optionalBytesField));
            Assert.False(message.HasField(optionalNestedEnumField));
            Assert.Equal(0, message.AllFields.Count);
            
            // Set t0 non-defalut value is seen as present
            message = TestAllTypes.CreateBuilder()
                .SetOptionalInt32(1)
                .SetOptionalString("x")
                .SetOptionalBytes(ByteString.CopyFromUtf8("y"))
                .SetOptionalNestedEnum(TestAllTypes.Types.NestedEnum.BAR)
                .Build();
            Assert.True(message.HasField(optionalInt32Field));
            Assert.True(message.HasField(optionalStringField));
            Assert.True(message.HasField(optionalBytesField));
            Assert.True(message.HasField(optionalNestedEnumField));
            Assert.Equal(4, message.AllFields.Count);
        }

        [Fact]
        public void TestMessageField()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            Assert.False(builder.HasOptionalNestedMessage);
            Assert.False(builder.Build().HasOptionalNestedMessage);

            // Unlike non-message fields, if we set default value to message field, the field
            // shoule be seem as present.
            builder.SetOptionalNestedMessage(TestAllTypes.Types.NestedMessage.DefaultInstance);
            Assert.True(builder.HasOptionalNestedMessage);
            Assert.True(builder.Build().HasOptionalNestedMessage);
        }

        [Fact]
        public void TestSeralizeAndParse()
        {
            TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
            builder.SetOptionalInt32(1234);
            builder.SetOptionalString("hello");
            builder.SetOptionalNestedMessage(TestAllTypes.Types.NestedMessage.DefaultInstance);
            ByteString data = builder.Build().ToByteString();

            TestAllTypes message = TestAllTypes.ParseFrom(data);
            Assert.Equal(1234, message.OptionalInt32);
            Assert.Equal("hello", message.OptionalString);
            Assert.Equal(ByteString.Empty, message.OptionalBytes);
            Assert.Equal(TestAllTypes.Types.NestedEnum.FOO, message.OptionalNestedEnum);
            Assert.True(message.HasOptionalNestedMessage);
            Assert.Equal(0, message.OptionalNestedMessage.Value);
        }
    }
}
