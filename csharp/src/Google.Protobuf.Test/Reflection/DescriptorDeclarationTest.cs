#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2018 Google Inc.  All rights reserved.
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

using Google.Protobuf.Reflection;
using NUnit.Framework;
using System.Linq;
using System.Reflection;

namespace Google.Protobuf.Test.Reflection
{
    // In reality this isn't a test for DescriptorDeclaration so much as the way they're loaded.
    public class DescriptorDeclarationTest
    {
        static readonly FileDescriptor unitTestProto3Descriptor = LoadProtos();

        // Note: we don't expose a declaration for FileDescriptor as it doesn't have comments
        // at the moment and the locations aren't terribly useful.

        // The tests for most elements are quite basic: we don't test every aspect of every element.
        // The code within the library falls into two categories:
        // - Exposing the properties for *any* declaration
        // - Finding the right declaration for an element from the descriptor data
        // We have a per-element check to make sure we *are* finding the right declaration, and we
        // check every property of declarations in at least one test, but we don't have a cross-product.
        // That would effectively be testing protoc, which seems redundant here.

        [Test]
        public void ServiceComments()
        {
            var service = unitTestProto3Descriptor.FindTypeByName<ServiceDescriptor>("TestService");
            Assert.NotNull(service.Declaration);
            Assert.AreEqual(" This is a test service\n", service.Declaration.LeadingComments);
        }

        [Test]
        public void MethodComments()
        {
            var service = unitTestProto3Descriptor.FindTypeByName<ServiceDescriptor>("TestService");
            var method = service.FindMethodByName("Foo");
            Assert.NotNull(method.Declaration);
            Assert.AreEqual(" This is a test method\n", method.Declaration.LeadingComments);
        }

        [Test]
        public void MessageComments()
        {
            var message = unitTestProto3Descriptor.FindTypeByName<MessageDescriptor>("CommentMessage");
            Assert.NotNull(message.Declaration);
            Assert.AreEqual(" This is a leading comment\n", message.Declaration.LeadingComments);
            Assert.AreEqual(new[] { " This is leading detached comment 1\n", " This is leading detached comment 2\n" },
                message.Declaration.LeadingDetachedComments);
        }

        // Note: this test is somewhat brittle; a change earlier in the proto will break it.
        [Test]
        public void MessageLocations()
        {
            var message = unitTestProto3Descriptor.FindTypeByName<MessageDescriptor>("CommentMessage");
            Assert.NotNull(message.Declaration);
            Assert.AreEqual(389, message.Declaration.StartLine);
            Assert.AreEqual(1, message.Declaration.StartColumn);

            Assert.AreEqual(404, message.Declaration.EndLine);
            Assert.AreEqual(2, message.Declaration.EndColumn);
        }

        [Test]
        public void EnumComments()
        {
            var descriptor = unitTestProto3Descriptor.FindTypeByName<EnumDescriptor>("CommentEnum");
            Assert.NotNull(descriptor.Declaration);
            Assert.AreEqual(" Leading enum comment\n", descriptor.Declaration.LeadingComments);
        }

        [Test]
        public void NestedMessageComments()
        {
            var outer = unitTestProto3Descriptor.FindTypeByName<MessageDescriptor>("CommentMessage");
            var nested = outer.FindDescriptor<MessageDescriptor>("NestedCommentMessage");
            Assert.NotNull(nested.Declaration);
            Assert.AreEqual(" Leading nested message comment\n", nested.Declaration.LeadingComments);
        }

        [Test]
        public void NestedEnumComments()
        {
            var outer = unitTestProto3Descriptor.FindTypeByName<MessageDescriptor>("CommentMessage");
            var nested = outer.FindDescriptor<EnumDescriptor>("NestedCommentEnum");
            Assert.NotNull(nested.Declaration);
            Assert.AreEqual(" Leading nested enum comment\n", nested.Declaration.LeadingComments);
        }

        [Test]
        public void FieldComments()
        {
            var message = unitTestProto3Descriptor.FindTypeByName<MessageDescriptor>("CommentMessage");
            var field = message.FindFieldByName("text");
            Assert.NotNull(field.Declaration);
            Assert.AreEqual(" Leading field comment\n", field.Declaration.LeadingComments);
            Assert.AreEqual(" Trailing field comment\n", field.Declaration.TrailingComments);
        }

        [Test]
        public void NestedMessageFieldComments()
        {
            var outer = unitTestProto3Descriptor.FindTypeByName<MessageDescriptor>("CommentMessage");
            var nested = outer.FindDescriptor<MessageDescriptor>("NestedCommentMessage");
            var field = nested.FindFieldByName("nested_text");
            Assert.NotNull(field.Declaration);
            Assert.AreEqual(" Leading nested message field comment\n", field.Declaration.LeadingComments);
        }

        [Test]
        public void EnumValueComments()
        {
            var enumDescriptor = unitTestProto3Descriptor.FindTypeByName<EnumDescriptor>("CommentEnum");
            var value = enumDescriptor.FindValueByName("ZERO_VALUE");
            Assert.NotNull(value.Declaration);
            Assert.AreEqual(" Zero value comment\n", value.Declaration.LeadingComments);
        }

        [Test]
        public void NestedEnumValueComments()
        {
            var outer = unitTestProto3Descriptor.FindTypeByName<MessageDescriptor>("CommentMessage");
            var nested = outer.FindDescriptor<EnumDescriptor>("NestedCommentEnum");
            var value = nested.FindValueByName("ZERO_VALUE");
            Assert.NotNull(value.Declaration);
            Assert.AreEqual(" Zero value comment\n", value.Declaration.LeadingComments);
        }

        private static FileDescriptor LoadProtos()
        {
            var type = typeof(DescriptorDeclarationTest);
            // TODO: Make this simpler :)
            FileDescriptorSet descriptorSet;
            using (var stream = type.GetTypeInfo().Assembly.GetManifestResourceStream($"Google.Protobuf.Test.testprotos.pb"))
            {
                descriptorSet = FileDescriptorSet.Parser.ParseFrom(stream);
            }
            var byteStrings = descriptorSet.File.Select(f => f.ToByteString()).ToList();
            var descriptors = FileDescriptor.BuildFromByteStrings(byteStrings);
            return descriptors.Single(d => d.Name == "unittest_proto3.proto");
        }
    }
}
