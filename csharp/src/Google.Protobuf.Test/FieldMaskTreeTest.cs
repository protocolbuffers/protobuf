#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using Google.Protobuf.Collections;
using Google.Protobuf.TestProtos;
using NUnit.Framework;
using Google.Protobuf.WellKnownTypes;

namespace Google.Protobuf
{
    public class FieldMaskTreeTest
    {
        [Test]
        public void AddFieldPath()
        {
            FieldMaskTree tree = new FieldMaskTree();
            RepeatedField<string> paths = tree.ToFieldMask().Paths;
            Assert.AreEqual(0, paths.Count);

            tree.AddFieldPath("");
            paths = tree.ToFieldMask().Paths;
            Assert.AreEqual(1, paths.Count);
            Assert.Contains("", paths);

            // New branch.
            tree.AddFieldPath("foo");
            paths = tree.ToFieldMask().Paths;
            Assert.AreEqual(2, paths.Count);
            Assert.Contains("foo", paths);

            // Redundant path.
            tree.AddFieldPath("foo");
            paths = tree.ToFieldMask().Paths;
            Assert.AreEqual(2, paths.Count);

            // New branch.
            tree.AddFieldPath("bar.baz");
            paths = tree.ToFieldMask().Paths;
            Assert.AreEqual(3, paths.Count);
            Assert.Contains("bar.baz", paths);

            // Redundant sub-path.
            tree.AddFieldPath("foo.bar");
            paths = tree.ToFieldMask().Paths;
            Assert.AreEqual(3, paths.Count);

            // New branch from a non-root node.
            tree.AddFieldPath("bar.quz");
            paths = tree.ToFieldMask().Paths;
            Assert.AreEqual(4, paths.Count);
            Assert.Contains("bar.quz", paths);

            // A path that matches several existing sub-paths.
            tree.AddFieldPath("bar");
            paths = tree.ToFieldMask().Paths;
            Assert.AreEqual(3, paths.Count);
            Assert.Contains("foo", paths);
            Assert.Contains("bar", paths);
        }

        [Test]
        public void MergeFromFieldMask()
        {
            FieldMaskTree tree = new FieldMaskTree();
            tree.MergeFromFieldMask(new FieldMask
            {
                Paths = {"foo", "bar.baz", "bar.quz"}
            });
            RepeatedField<string> paths = tree.ToFieldMask().Paths;
            Assert.AreEqual(3, paths.Count);
            Assert.Contains("foo", paths);
            Assert.Contains("bar.baz", paths);
            Assert.Contains("bar.quz", paths);

            tree.MergeFromFieldMask(new FieldMask
            {
                Paths = {"foo.bar", "bar"}
            });
            paths = tree.ToFieldMask().Paths;
            Assert.AreEqual(2, paths.Count);
            Assert.Contains("foo", paths);
            Assert.Contains("bar", paths);
        }

        [Test]
        public void IntersectFieldPath()
        {
            FieldMaskTree tree = new FieldMaskTree();
            FieldMaskTree result = new FieldMaskTree();
            tree.MergeFromFieldMask(new FieldMask
            {
                Paths = {"foo", "bar.baz", "bar.quz"}
            });

            // Empty path.
            tree.IntersectFieldPath("", result);
            RepeatedField<string> paths = result.ToFieldMask().Paths;
            Assert.AreEqual(0, paths.Count);

            // Non-exist path.
            tree.IntersectFieldPath("quz", result);
            paths = result.ToFieldMask().Paths;
            Assert.AreEqual(0, paths.Count);

            // Sub-path of an existing leaf.
            tree.IntersectFieldPath("foo.bar", result);
            paths = result.ToFieldMask().Paths;
            Assert.AreEqual(1, paths.Count);
            Assert.Contains("foo.bar", paths);

            // Match an existing leaf node.
            tree.IntersectFieldPath("foo", result);
            paths = result.ToFieldMask().Paths;
            Assert.AreEqual(1, paths.Count);
            Assert.Contains("foo", paths);

            // Non-exist path.
            tree.IntersectFieldPath("bar.foo", result);
            paths = result.ToFieldMask().Paths;
            Assert.AreEqual(1, paths.Count);
            Assert.Contains("foo", paths);

            // Match a non-leaf node.
            tree.IntersectFieldPath("bar", result);
            paths = result.ToFieldMask().Paths;
            Assert.AreEqual(3, paths.Count);
            Assert.Contains("foo", paths);
            Assert.Contains("bar.baz", paths);
            Assert.Contains("bar.quz", paths);
        }

        private void Merge(FieldMaskTree tree, IMessage source, IMessage destination, FieldMask.MergeOptions options, bool useDynamicMessage)
        {
            if (useDynamicMessage)
            {
                var newSource = source.Descriptor.Parser.CreateTemplate();
                newSource.MergeFrom(source.ToByteString());

                var newDestination = source.Descriptor.Parser.CreateTemplate();
                newDestination.MergeFrom(destination.ToByteString());

                tree.Merge(newSource, newDestination, options);

                // Clear before merging:
                foreach (var fieldDescriptor in destination.Descriptor.Fields.InFieldNumberOrder())
                {
                    fieldDescriptor.Accessor.Clear(destination);
                }
                destination.MergeFrom(newDestination.ToByteString());
            }
            else
            {
                tree.Merge(source, destination, options);
            }
        }

        [Test]
        [TestCase(true)]
        [TestCase(false)]
        public void Merge(bool useDynamicMessage)
        {
            TestAllTypes value = new TestAllTypes
            {
                SingleInt32 = 1234,
                SingleNestedMessage = new TestAllTypes.Types.NestedMessage {Bb = 5678},
                RepeatedInt32 = {4321},
                RepeatedNestedMessage = {new TestAllTypes.Types.NestedMessage {Bb = 8765}}
            };

            NestedTestAllTypes source = new NestedTestAllTypes
            {
                Payload = value,
                Child = new NestedTestAllTypes {Payload = value}
            };
            // Now we have a message source with the following structure:
            //   [root] -+- payload -+- single_int32
            //           |           +- single_nested_message
            //           |           +- repeated_int32
            //           |           +- repeated_nested_message
            //           |
            //           +- child --- payload -+- single_int32
            //                                 +- single_nested_message
            //                                 +- repeated_int32
            //                                 +- repeated_nested_message

            FieldMask.MergeOptions options = new FieldMask.MergeOptions();

            // Test merging each individual field.
            NestedTestAllTypes destination = new NestedTestAllTypes();
            Merge(new FieldMaskTree().AddFieldPath("payload.single_int32"),
                source, destination, options, useDynamicMessage);
            NestedTestAllTypes expected = new NestedTestAllTypes
            {
                Payload = new TestAllTypes
                {
                    SingleInt32 = 1234
                }
            };
            Assert.AreEqual(expected, destination);

            destination = new NestedTestAllTypes();
            Merge(new FieldMaskTree().AddFieldPath("payload.single_nested_message"),
                source, destination, options, useDynamicMessage);
            expected = new NestedTestAllTypes
            {
                Payload = new TestAllTypes
                {
                    SingleNestedMessage = new TestAllTypes.Types.NestedMessage {Bb = 5678}
                }
            };
            Assert.AreEqual(expected, destination);

            destination = new NestedTestAllTypes();
            Merge(new FieldMaskTree().AddFieldPath("payload.repeated_int32"),
                source, destination, options, useDynamicMessage);
            expected = new NestedTestAllTypes
            {
                Payload = new TestAllTypes
                {
                    RepeatedInt32 = {4321}
                }
            };
            Assert.AreEqual(expected, destination);

            destination = new NestedTestAllTypes();
            Merge(new FieldMaskTree().AddFieldPath("payload.repeated_nested_message"),
                source, destination, options, useDynamicMessage);
            expected = new NestedTestAllTypes
            {
                Payload = new TestAllTypes
                {
                    RepeatedNestedMessage = {new TestAllTypes.Types.NestedMessage {Bb = 8765}}
                }
            };
            Assert.AreEqual(expected, destination);

            destination = new NestedTestAllTypes();
            Merge(
                new FieldMaskTree().AddFieldPath("child.payload.single_int32"),
                source,
                destination,
                options,
                useDynamicMessage);
            expected = new NestedTestAllTypes
            {
                Child = new NestedTestAllTypes
                {
                    Payload = new TestAllTypes
                    {
                        SingleInt32 = 1234
                    }
                }
            };
            Assert.AreEqual(expected, destination);

            destination = new NestedTestAllTypes();
            Merge(
                new FieldMaskTree().AddFieldPath("child.payload.single_nested_message"),
                source,
                destination,
                options,
                useDynamicMessage);
            expected = new NestedTestAllTypes
            {
                Child = new NestedTestAllTypes
                {
                    Payload = new TestAllTypes
                    {
                        SingleNestedMessage = new TestAllTypes.Types.NestedMessage {Bb = 5678}
                    }
                }
            };
            Assert.AreEqual(expected, destination);

            destination = new NestedTestAllTypes();
            Merge(new FieldMaskTree().AddFieldPath("child.payload.repeated_int32"),
                source, destination, options, useDynamicMessage);
            expected = new NestedTestAllTypes
            {
                Child = new NestedTestAllTypes
                {
                    Payload = new TestAllTypes
                    {
                        RepeatedInt32 = {4321}
                    }
                }
            };
            Assert.AreEqual(expected, destination);

            destination = new NestedTestAllTypes();
            Merge(new FieldMaskTree().AddFieldPath("child.payload.repeated_nested_message"),
                source, destination, options, useDynamicMessage);
            expected = new NestedTestAllTypes
            {
                Child = new NestedTestAllTypes
                {
                    Payload = new TestAllTypes
                    {
                        RepeatedNestedMessage = {new TestAllTypes.Types.NestedMessage {Bb = 8765}}
                    }
                }
            };
            Assert.AreEqual(expected, destination);

            destination = new NestedTestAllTypes();
            Merge(new FieldMaskTree().AddFieldPath("child").AddFieldPath("payload"),
                source, destination, options, useDynamicMessage);
            Assert.AreEqual(source, destination);

            // Test repeated options.
            destination = new NestedTestAllTypes
            {
                Payload = new TestAllTypes
                {
                    RepeatedInt32 = { 1000 }
                }
            };
            Merge(new FieldMaskTree().AddFieldPath("payload.repeated_int32"),
                    source, destination, options, useDynamicMessage);
            // Default behavior is to append repeated fields.
            Assert.AreEqual(2, destination.Payload.RepeatedInt32.Count);
            Assert.AreEqual(1000, destination.Payload.RepeatedInt32[0]);
            Assert.AreEqual(4321, destination.Payload.RepeatedInt32[1]);
            // Change to replace repeated fields.
            options.ReplaceRepeatedFields = true;
            Merge(new FieldMaskTree().AddFieldPath("payload.repeated_int32"),
                source, destination, options, useDynamicMessage);
            Assert.AreEqual(1, destination.Payload.RepeatedInt32.Count);
            Assert.AreEqual(4321, destination.Payload.RepeatedInt32[0]);

            // Test message options.
            destination = new NestedTestAllTypes
            {
                Payload = new TestAllTypes
                {
                    SingleInt32 = 1000,
                    SingleUint32 = 2000
                }
            };
            Merge(new FieldMaskTree().AddFieldPath("payload"),
                    source, destination, options, useDynamicMessage);
            // Default behavior is to merge message fields.
            Assert.AreEqual(1234, destination.Payload.SingleInt32);
            Assert.AreEqual(2000, destination.Payload.SingleUint32);

            // Test merging unset message fields.
            NestedTestAllTypes clearedSource = source.Clone();
            clearedSource.Payload = null;
            destination = new NestedTestAllTypes();
            Merge(new FieldMaskTree().AddFieldPath("payload"),
                clearedSource, destination, options, useDynamicMessage);
            Assert.IsNull(destination.Payload);

            // Skip a message field if they are unset in both source and target.
            destination = new NestedTestAllTypes();
            Merge(new FieldMaskTree().AddFieldPath("payload.single_int32"),
                clearedSource, destination, options, useDynamicMessage);
            Assert.IsNull(destination.Payload);

            // Change to replace message fields.
            options.ReplaceMessageFields = true;
            destination = new NestedTestAllTypes
            {
                Payload = new TestAllTypes
                {
                    SingleInt32 = 1000,
                    SingleUint32 = 2000
                }
            };
            Merge(new FieldMaskTree().AddFieldPath("payload"),
                    source, destination, options, useDynamicMessage);
            Assert.AreEqual(1234, destination.Payload.SingleInt32);
            Assert.AreEqual(0, destination.Payload.SingleUint32);

            // Test merging unset message fields.
            destination = new NestedTestAllTypes
            {
                Payload = new TestAllTypes
                {
                    SingleInt32 = 1000,
                    SingleUint32 = 2000
                }
            };
            Merge(new FieldMaskTree().AddFieldPath("payload"),
                    clearedSource, destination, options, useDynamicMessage);
            Assert.IsNull(destination.Payload);

            // Test merging unset primitive fields.
            destination = source.Clone();
            destination.Payload.SingleInt32 = 0;
            NestedTestAllTypes sourceWithPayloadInt32Unset = destination;
            destination = source.Clone();
            Merge(new FieldMaskTree().AddFieldPath("payload.single_int32"),
                sourceWithPayloadInt32Unset, destination, options, useDynamicMessage);
            Assert.AreEqual(0, destination.Payload.SingleInt32);

            // Change to clear unset primitive fields.
            options.ReplacePrimitiveFields = true;
            destination = source.Clone();
            Merge(new FieldMaskTree().AddFieldPath("payload.single_int32"),
                sourceWithPayloadInt32Unset, destination, options, useDynamicMessage);
            Assert.IsNotNull(destination.Payload);
        }

        [Test]
        public void MergeWrapperFieldsWithNonNullFieldsInSource()
        {
            // Instantiate a destination with wrapper-based field types.
            var destination = new TestWellKnownTypes()
            {
                StringField = "Hello",
                Int32Field = 12,
                Int64Field = 24,
                BoolField = true,
            };

            // Set up a targeted update.
            var source = new TestWellKnownTypes()
            {
                StringField = "Hi",
                Int64Field = 240
            };

            Merge(new FieldMaskTree().AddFieldPath("string_field").AddFieldPath("int64_field"),
                source,
                destination,
                new FieldMask.MergeOptions(),
                false);

            // Make sure the targeted fields changed.
            Assert.AreEqual("Hi", destination.StringField);
            Assert.AreEqual(240, destination.Int64Field);

            // Prove that non-targeted fields stay intact...
            Assert.AreEqual(12, destination.Int32Field);
            Assert.IsTrue(destination.BoolField);

            // ...including default values which were not explicitly set in the destination object.
            Assert.IsNull(destination.FloatField);
        }

        [Test]
        [TestCase(false, "Hello", 24)]
        [TestCase(true, null, null)]
        public void MergeWrapperFieldsWithNullFieldsInSource(
            bool replaceMessageFields,
            string expectedStringValue,
            long? expectedInt64Value)
        {
            // Instantiate a destination with wrapper-based field types.
            var destination = new TestWellKnownTypes()
            {
                StringField = "Hello",
                Int32Field = 12,
                Int64Field = 24,
                BoolField = true,
            };

            // Set up a targeted update with null valued fields.
            var source = new TestWellKnownTypes()
            {
                StringField = null,
                Int64Field = null
            };

            Merge(new FieldMaskTree().AddFieldPath("string_field").AddFieldPath("int64_field"),
                source,
                destination,
                new FieldMask.MergeOptions()
                {
                    ReplaceMessageFields = replaceMessageFields
                },
                false);

            // Make sure the targeted fields changed according to our expectations, depending on the value of ReplaceMessageFields.
            // When ReplaceMessageFields is false, the null values are not applied to the destination, because, although wrapped types
            // are semantically primitives, FieldMaskTree.Merge still treats them as message types in order to maintain consistency with other Protobuf
            // libraries such as Java and C++.
            Assert.AreEqual(expectedStringValue, destination.StringField);
            Assert.AreEqual(expectedInt64Value, destination.Int64Field);

            // Prove that non-targeted fields stay intact...
            Assert.AreEqual(12, destination.Int32Field);
            Assert.IsTrue(destination.BoolField);

            // ...including default values which were not explicitly set in the destination object.
            Assert.IsNull(destination.FloatField);
        }
    }
}
