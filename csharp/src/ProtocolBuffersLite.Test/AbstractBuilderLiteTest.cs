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

using System.IO;
using Google.ProtocolBuffers.TestProtos;
using Xunit;

namespace Google.ProtocolBuffers
{
    public class AbstractBuilderLiteTest
    {
        [Fact]
        public void TestMergeFromCodedInputStream()
        {
            TestAllTypesLite copy,
                             msg = TestAllTypesLite.CreateBuilder()
                                 .SetOptionalUint32(uint.MaxValue).Build();

            copy = TestAllTypesLite.DefaultInstance;
            Assert.NotEqual(msg.ToByteArray(), copy.ToByteArray());

            using (MemoryStream ms = new MemoryStream(msg.ToByteArray()))
            {
                CodedInputStream ci = CodedInputStream.CreateInstance(ms);
                copy = copy.ToBuilder().MergeFrom(ci).Build();
            }

            Assert.Equal(msg.ToByteArray(), copy.ToByteArray());
        }

        [Fact]
        public void TestIBuilderLiteWeakClear()
        {
            TestAllTypesLite copy, msg = TestAllTypesLite.DefaultInstance;

            copy = msg.ToBuilder().SetOptionalString("Should be removed.").Build();
            Assert.NotEqual(msg.ToByteArray(), copy.ToByteArray());

            copy = (TestAllTypesLite) ((IBuilderLite) copy.ToBuilder()).WeakClear().WeakBuild();
            Assert.Equal(msg.ToByteArray(), copy.ToByteArray());
        }

        [Fact]
        public void TestBuilderLiteMergeFromCodedInputStream()
        {
            TestAllTypesLite copy,
                             msg = TestAllTypesLite.CreateBuilder()
                                 .SetOptionalString("Should be merged.").Build();

            copy = TestAllTypesLite.DefaultInstance;
            Assert.NotEqual(msg.ToByteArray(), copy.ToByteArray());

            copy =
                copy.ToBuilder().MergeFrom(CodedInputStream.CreateInstance(new MemoryStream(msg.ToByteArray()))).Build();
            Assert.Equal(msg.ToByteArray(), copy.ToByteArray());
        }

        [Fact]
        public void TestBuilderLiteMergeDelimitedFrom()
        {
            TestAllTypesLite copy,
                             msg = TestAllTypesLite.CreateBuilder()
                                 .SetOptionalString("Should be merged.").Build();

            copy = TestAllTypesLite.DefaultInstance;
            Assert.NotEqual(msg.ToByteArray(), copy.ToByteArray());
            Stream s = new MemoryStream();
            msg.WriteDelimitedTo(s);
            s.Position = 0;
            copy = copy.ToBuilder().MergeDelimitedFrom(s).Build();
            Assert.Equal(msg.ToByteArray(), copy.ToByteArray());
        }

        [Fact]
        public void TestBuilderLiteMergeDelimitedFromExtensions()
        {
            TestAllExtensionsLite copy,
                                  msg = TestAllExtensionsLite.CreateBuilder()
                                      .SetExtension(UnittestLite.OptionalStringExtensionLite,
                                                    "Should be merged.").Build();

            copy = TestAllExtensionsLite.DefaultInstance;
            Assert.NotEqual(msg.ToByteArray(), copy.ToByteArray());

            Stream s = new MemoryStream();
            msg.WriteDelimitedTo(s);
            s.Position = 0;

            ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
            UnittestLite.RegisterAllExtensions(registry);

            copy = copy.ToBuilder().MergeDelimitedFrom(s, registry).Build();
            Assert.Equal(msg.ToByteArray(), copy.ToByteArray());
            Assert.Equal("Should be merged.", copy.GetExtension(UnittestLite.OptionalStringExtensionLite));
        }

        [Fact]
        public void TestBuilderLiteMergeFromStream()
        {
            TestAllTypesLite copy,
                             msg = TestAllTypesLite.CreateBuilder()
                                 .SetOptionalString("Should be merged.").Build();

            copy = TestAllTypesLite.DefaultInstance;
            Assert.NotEqual(msg.ToByteArray(), copy.ToByteArray());
            Stream s = new MemoryStream();
            msg.WriteTo(s);
            s.Position = 0;
            copy = copy.ToBuilder().MergeFrom(s).Build();
            Assert.Equal(msg.ToByteArray(), copy.ToByteArray());
        }

        [Fact]
        public void TestBuilderLiteMergeFromStreamExtensions()
        {
            TestAllExtensionsLite copy,
                                  msg = TestAllExtensionsLite.CreateBuilder()
                                      .SetExtension(UnittestLite.OptionalStringExtensionLite,
                                                    "Should be merged.").Build();

            copy = TestAllExtensionsLite.DefaultInstance;
            Assert.NotEqual(msg.ToByteArray(), copy.ToByteArray());

            Stream s = new MemoryStream();
            msg.WriteTo(s);
            s.Position = 0;

            ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
            UnittestLite.RegisterAllExtensions(registry);

            copy = copy.ToBuilder().MergeFrom(s, registry).Build();
            Assert.Equal(msg.ToByteArray(), copy.ToByteArray());
            Assert.Equal("Should be merged.", copy.GetExtension(UnittestLite.OptionalStringExtensionLite));
        }

        [Fact]
        public void TestIBuilderLiteWeakMergeFromIMessageLite()
        {
            TestAllTypesLite copy,
                             msg = TestAllTypesLite.CreateBuilder()
                                 .SetOptionalString("Should be merged.").Build();

            copy = TestAllTypesLite.DefaultInstance;
            Assert.NotEqual(msg.ToByteArray(), copy.ToByteArray());

            copy = (TestAllTypesLite) ((IBuilderLite) copy.ToBuilder()).WeakMergeFrom((IMessageLite) msg).WeakBuild();
            Assert.Equal(msg.ToByteArray(), copy.ToByteArray());
        }

        [Fact]
        public void TestIBuilderLiteWeakMergeFromByteString()
        {
            TestAllTypesLite copy,
                             msg = TestAllTypesLite.CreateBuilder()
                                 .SetOptionalString("Should be merged.").Build();

            copy = TestAllTypesLite.DefaultInstance;
            Assert.NotEqual(msg.ToByteArray(), copy.ToByteArray());

            copy = (TestAllTypesLite) ((IBuilderLite) copy.ToBuilder()).WeakMergeFrom(msg.ToByteString()).WeakBuild();
            Assert.Equal(msg.ToByteArray(), copy.ToByteArray());
        }

        [Fact]
        public void TestIBuilderLiteWeakMergeFromByteStringExtensions()
        {
            TestAllExtensionsLite copy,
                                  msg = TestAllExtensionsLite.CreateBuilder()
                                      .SetExtension(UnittestLite.OptionalStringExtensionLite,
                                                    "Should be merged.").Build();

            copy = TestAllExtensionsLite.DefaultInstance;
            Assert.NotEqual(msg.ToByteArray(), copy.ToByteArray());

            copy =
                (TestAllExtensionsLite)
                ((IBuilderLite) copy.ToBuilder()).WeakMergeFrom(msg.ToByteString(), ExtensionRegistry.Empty).WeakBuild();
            Assert.NotEqual(msg.ToByteArray(), copy.ToByteArray());

            ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
            UnittestLite.RegisterAllExtensions(registry);

            copy =
                (TestAllExtensionsLite)
                ((IBuilderLite) copy.ToBuilder()).WeakMergeFrom(msg.ToByteString(), registry).WeakBuild();
            Assert.Equal(msg.ToByteArray(), copy.ToByteArray());
            Assert.Equal("Should be merged.", copy.GetExtension(UnittestLite.OptionalStringExtensionLite));
        }

        [Fact]
        public void TestIBuilderLiteWeakMergeFromCodedInputStream()
        {
            TestAllTypesLite copy,
                             msg = TestAllTypesLite.CreateBuilder()
                                 .SetOptionalUint32(uint.MaxValue).Build();

            copy = TestAllTypesLite.DefaultInstance;
            Assert.NotEqual(msg.ToByteArray(), copy.ToByteArray());

            using (MemoryStream ms = new MemoryStream(msg.ToByteArray()))
            {
                CodedInputStream ci = CodedInputStream.CreateInstance(ms);
                copy = (TestAllTypesLite) ((IBuilderLite) copy.ToBuilder()).WeakMergeFrom(ci).WeakBuild();
            }

            Assert.Equal(msg.ToByteArray(), copy.ToByteArray());
        }

        [Fact]
        public void TestIBuilderLiteWeakBuildPartial()
        {
            IBuilderLite builder = TestRequiredLite.CreateBuilder();
            Assert.False(builder.IsInitialized);

            IMessageLite msg = builder.WeakBuildPartial();
            Assert.False(msg.IsInitialized);

            Assert.Equal(msg.ToByteArray(), TestRequiredLite.DefaultInstance.ToByteArray());
        }

        [Fact]
        public void TestIBuilderLiteWeakBuildUninitialized()
        {
            IBuilderLite builder = TestRequiredLite.CreateBuilder();
            Assert.False(builder.IsInitialized);
            Assert.Throws<UninitializedMessageException>(() => builder.WeakBuild());
        }

        [Fact]
        public void TestIBuilderLiteWeakBuild()
        {
            IBuilderLite builder = TestRequiredLite.CreateBuilder()
                .SetD(0)
                .SetEn(ExtraEnum.EXLITE_BAZ);
            Assert.True(builder.IsInitialized);
            builder.WeakBuild();
        }

        [Fact]
        public void TestIBuilderLiteWeakClone()
        {
            TestRequiredLite msg = TestRequiredLite.CreateBuilder()
                .SetD(1).SetEn(ExtraEnum.EXLITE_BAR).Build();
            Assert.True(msg.IsInitialized);

            IMessageLite copy = ((IBuilderLite) msg.ToBuilder()).WeakClone().WeakBuild();
            Assert.Equal(msg.ToByteArray(), copy.ToByteArray());
        }

        [Fact]
        public void TestIBuilderLiteWeakDefaultInstance()
        {
            Assert.True(ReferenceEquals(TestRequiredLite.DefaultInstance,
                                          ((IBuilderLite) TestRequiredLite.CreateBuilder()).WeakDefaultInstanceForType));
        }

        [Fact]
        public void TestGeneratedBuilderLiteAddRange()
        {
            TestAllTypesLite copy,
                             msg = TestAllTypesLite.CreateBuilder()
                                 .SetOptionalUint32(123)
                                 .AddRepeatedInt32(1)
                                 .AddRepeatedInt32(2)
                                 .AddRepeatedInt32(3)
                                 .Build();

            copy = msg.DefaultInstanceForType.ToBuilder().MergeFrom(msg).Build();
            Assert.Equal(msg.ToByteArray(), copy.ToByteArray());
        }

        // ROK 5/7/2013 Issue #54: should retire all bytes in buffer (bufferSize)
        [Fact]
        public void TestBufferRefillIssue()
        {
            var ms = new MemoryStream();
            BucketOfBytes.CreateBuilder()
                .SetValue(ByteString.CopyFrom(new byte[3000]))
                .Build().WriteDelimitedTo(ms);
            BucketOfBytesEx.CreateBuilder()
                .SetValue(ByteString.CopyFrom(new byte[1000]))
                .SetValue2(ByteString.CopyFrom(new byte[1100]))
                .Build().WriteDelimitedTo(ms);
            BucketOfBytes.CreateBuilder()
                .SetValue(ByteString.CopyFrom(new byte[100]))
                .Build().WriteDelimitedTo(ms);

            ms.Position = 0;
            var input = CodedInputStream.CreateInstance(ms);
            var builder = BucketOfBytes.CreateBuilder();
            input.ReadMessage(builder, ExtensionRegistry.Empty);
            Assert.Equal(3005L, input.Position);
            Assert.Equal(3000, builder.Value.Length);
            input.ReadMessage(builder, ExtensionRegistry.Empty);
            Assert.Equal(5114, input.Position);
            Assert.Equal(1000, builder.Value.Length);
            input.ReadMessage(builder, ExtensionRegistry.Empty);
            Assert.Equal(5217L, input.Position);
            Assert.Equal(input.Position, ms.Length);
            Assert.Equal(100, builder.Value.Length);
        }
    }
}