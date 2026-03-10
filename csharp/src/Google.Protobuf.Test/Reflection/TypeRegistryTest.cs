#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using Google.Protobuf.TestProtos;
using Google.Protobuf.WellKnownTypes;
using NUnit.Framework;

namespace Google.Protobuf.Reflection
{
    public class TypeRegistryTest
    {
        // Most of our tests use messages. Simple test that we really can use files...
        [Test]
        public void CreateWithFileDescriptor()
        {
            var registry = TypeRegistry.FromFiles(DurationReflection.Descriptor, StructReflection.Descriptor);
            AssertDescriptorPresent(registry, Duration.Descriptor);
            AssertDescriptorPresent(registry, ListValue.Descriptor);
            AssertDescriptorAbsent(registry, Timestamp.Descriptor);
        }

        [Test]
        public void TypesFromSameFile()
        {
            // Just for kicks, let's start with a nested type
            var registry = TypeRegistry.FromMessages(TestAllTypes.Types.NestedMessage.Descriptor);
            // Top-level...
            AssertDescriptorPresent(registry, TestFieldOrderings.Descriptor);
            // ... and nested (not the same as the original NestedMessage!)
            AssertDescriptorPresent(registry, TestFieldOrderings.Types.NestedMessage.Descriptor);
        }

        [Test]
        public void DependenciesAreIncluded()
        {
            var registry = TypeRegistry.FromMessages(TestAllTypes.Descriptor);
            // Direct dependencies
            AssertDescriptorPresent(registry, ImportMessage.Descriptor);
            // Public dependencies
            AssertDescriptorPresent(registry, PublicImportMessage.Descriptor);
        }

        [Test]
        public void DuplicateFiles()
        {
            // Duplicates via dependencies and simply via repetition
            var registry = TypeRegistry.FromFiles(
                UnittestProto3Reflection.Descriptor, UnittestImportProto3Reflection.Descriptor,
                TimestampReflection.Descriptor, TimestampReflection.Descriptor);
            AssertDescriptorPresent(registry, TestAllTypes.Descriptor);
            AssertDescriptorPresent(registry, ImportMessage.Descriptor);
            AssertDescriptorPresent(registry, Timestamp.Descriptor);
        }

        private static void AssertDescriptorPresent(TypeRegistry registry, MessageDescriptor descriptor)
        {
            Assert.AreSame(descriptor, registry.Find(descriptor.FullName));
        }

        private static void AssertDescriptorAbsent(TypeRegistry registry, MessageDescriptor descriptor)
        {
            Assert.IsNull(registry.Find(descriptor.FullName));
        }
    }
}
