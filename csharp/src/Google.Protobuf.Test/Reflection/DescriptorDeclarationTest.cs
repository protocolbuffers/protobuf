#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2018 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
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

        [Test]
        public void MessageLocations()
        {
            var message = unitTestProto3Descriptor.FindTypeByName<MessageDescriptor>("CommentMessage");
            Assert.NotNull(message.Declaration);
            Assert.AreEqual(1, message.Declaration.StartColumn);

            Assert.AreEqual(2, message.Declaration.EndColumn);

            // This is slightly brittle, but allows a reasonable amount of change.
            var diff = message.Declaration.EndLine - message.Declaration.StartLine;
            Assert.That(diff, Is.GreaterThanOrEqualTo(10));
            Assert.That(diff, Is.LessThanOrEqualTo(50));
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
            return descriptors.Single(d => d.Name == "csharp/protos/unittest_proto3.proto");
        }
    }
}
