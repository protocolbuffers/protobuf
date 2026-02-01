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

namespace Google.Protobuf
{
    public class FieldMaskTreeTestWithMapFields
    {
        private (TestMapSubmessage source, TestMapSubmessage destination) PrepareTests()
        {
            var destination = new TestMapSubmessage()
            {
                TestMap = new TestMap()
                {
                    MapBoolBool = { [true] = true, [false] = false, },
                    MapInt32Int32 = { [1] = 1, [2] = 2, },
                    MapStringString = { ["a"] = "a", ["b"] = "b", },
                    MapFixed32Fixed32 = { [1] = 1, [2] = 2, },
                    MapFixed64Fixed64 = { [1] = 1, [2] = 2, },
                    MapInt32Bytes =
                    {
                        [1] = ByteString.CopyFromUtf8("a"),
                        [2] = ByteString.CopyFromUtf8("b"),
                    },
                    MapInt32Double = { [1] = 1.0, [2] = 2.0, },
                    MapInt32Enum = { [1] = MapEnum.Bar, [2] = MapEnum.Foo, },
                    MapInt32Float = { [1] = 1.0f, [2] = 2.0f, },
                    MapInt32ForeignMessage =
                    {
                        [1] = new ForeignMessage() { C = 1 },
                        [2] = new ForeignMessage() { C = 2 },
                    },
                    MapInt64Int64 = { [1] = 1, [2] = 2, },
                    MapSfixed32Sfixed32 = { [1] = 1, [2] = 2, },
                    MapSfixed64Sfixed64 = { [1] = 1, [2] = 2, },
                    MapSint32Sint32 = { [1] = 1, [2] = 2, },
                    MapSint64Sint64 = { [1] = 1, [2] = 2, },
                    MapUint32Uint32 = { [1] = 1, [2] = 2, },
                    MapUint64Uint64 = { [1] = 1, [2] = 2, },
                }
            };
            var source = new TestMapSubmessage()
            {
                TestMap = new TestMap()
                {
                    MapBoolBool = { [false] = true, },
                    MapInt32Int32 = { [2] = 2 * 2, [3] = 3 },
                    MapStringString = { ["b"] = "bb", ["c"] = "c", },
                    MapFixed32Fixed32 = { [2] = 2 * 2, [3] = 3 },
                    MapFixed64Fixed64 = { [2] = 2 * 2, [3] = 3 },
                    MapInt32Bytes =
                    {
                        [2] = ByteString.CopyFromUtf8("bb"),
                        [3] = ByteString.CopyFromUtf8("c"),
                    },
                    MapInt32Double = { [2] = 2.0 * 2.5, [3] = 3.0, },
                    MapInt32Enum = { [2] = MapEnum.Foo, [3] = MapEnum.Bar, },
                    MapInt32Float = { [2] = 2.0f * 2.5f, [3] = 3.0f, },
                    MapInt32ForeignMessage =
                    {
                        [2] = new ForeignMessage() { C = 2 * 2 },
                        [3] = new ForeignMessage() { C = 3 },
                    },
                    MapInt64Int64 = { [2] = 2 * 2, [3] = 3 },
                    MapSfixed32Sfixed32 = { [2] = 2 * 2, [3] = 3 },
                    MapSfixed64Sfixed64 = { [2] = 2 * 2, [3] = 3 },
                    MapSint32Sint32 = { [2] = 2 * 2, [3] = 3 },
                    MapSint64Sint64 = { [2] = 2 * 2, [3] = 3 },
                    MapUint32Uint32 = { [2] = 2 * 2, [3] = 3 },
                    MapUint64Uint64 = { [2] = 2 * 2, [3] = 3 },
                }
            };

            return (source, destination);
        }

        public void AssertFullMerge(
            TestMapSubmessage source,
            TestMapSubmessage destination,
            TestMapSubmessage originalDestination
        )
        {
            Assert.AreEqual(
                originalDestination.TestMap.MapBoolBool[true],
                destination.TestMap.MapBoolBool[true]
            );
            Assert.AreEqual(
                source.TestMap.MapBoolBool[false],
                destination.TestMap.MapBoolBool[false]
            );

            Assert.AreEqual(
                originalDestination.TestMap.MapInt32Int32[1],
                destination.TestMap.MapInt32Int32[1]
            );
            Assert.AreEqual(source.TestMap.MapInt32Int32[2], destination.TestMap.MapInt32Int32[2]);
            Assert.AreEqual(source.TestMap.MapInt32Int32[3], destination.TestMap.MapInt32Int32[3]);

            Assert.AreEqual(
                originalDestination.TestMap.MapStringString["a"],
                destination.TestMap.MapStringString["a"]
            );
            Assert.AreEqual(
                source.TestMap.MapStringString["b"],
                destination.TestMap.MapStringString["b"]
            );
            Assert.AreEqual(
                source.TestMap.MapStringString["c"],
                destination.TestMap.MapStringString["c"]
            );

            Assert.AreEqual(
                originalDestination.TestMap.MapFixed32Fixed32[1],
                destination.TestMap.MapFixed32Fixed32[1]
            );
            Assert.AreEqual(
                source.TestMap.MapFixed32Fixed32[2],
                destination.TestMap.MapFixed32Fixed32[2]
            );
            Assert.AreEqual(
                source.TestMap.MapFixed32Fixed32[3],
                destination.TestMap.MapFixed32Fixed32[3]
            );

            Assert.AreEqual(
                originalDestination.TestMap.MapFixed64Fixed64[1],
                destination.TestMap.MapFixed64Fixed64[1]
            );
            Assert.AreEqual(
                source.TestMap.MapFixed64Fixed64[2],
                destination.TestMap.MapFixed64Fixed64[2]
            );
            Assert.AreEqual(
                source.TestMap.MapFixed64Fixed64[3],
                destination.TestMap.MapFixed64Fixed64[3]
            );

            Assert.AreEqual(
                originalDestination.TestMap.MapInt32Bytes[1],
                destination.TestMap.MapInt32Bytes[1]
            );
            Assert.AreEqual(source.TestMap.MapInt32Bytes[2], destination.TestMap.MapInt32Bytes[2]);
            Assert.AreEqual(source.TestMap.MapInt32Bytes[3], destination.TestMap.MapInt32Bytes[3]);

            // follow same pattern as above for the rest of the fields
            Assert.AreEqual(
                originalDestination.TestMap.MapInt32Double[1],
                destination.TestMap.MapInt32Double[1]
            );
            Assert.AreEqual(
                source.TestMap.MapInt32Double[2],
                destination.TestMap.MapInt32Double[2]
            );
            Assert.AreEqual(
                source.TestMap.MapInt32Double[3],
                destination.TestMap.MapInt32Double[3]
            );

            Assert.AreEqual(
                originalDestination.TestMap.MapInt32Enum[1],
                destination.TestMap.MapInt32Enum[1]
            );
            Assert.AreEqual(source.TestMap.MapInt32Enum[2], destination.TestMap.MapInt32Enum[2]);
            Assert.AreEqual(source.TestMap.MapInt32Enum[3], destination.TestMap.MapInt32Enum[3]);

            Assert.AreEqual(
                originalDestination.TestMap.MapInt32Float[1],
                destination.TestMap.MapInt32Float[1]
            );
            Assert.AreEqual(source.TestMap.MapInt32Float[2], destination.TestMap.MapInt32Float[2]);
            Assert.AreEqual(source.TestMap.MapInt32Float[3], destination.TestMap.MapInt32Float[3]);

            Assert.AreEqual(
                originalDestination.TestMap.MapInt32ForeignMessage[1].C,
                destination.TestMap.MapInt32ForeignMessage[1].C
            );
            Assert.AreEqual(
                source.TestMap.MapInt32ForeignMessage[2].C,
                destination.TestMap.MapInt32ForeignMessage[2].C
            );
            Assert.AreEqual(
                source.TestMap.MapInt32ForeignMessage[3].C,
                destination.TestMap.MapInt32ForeignMessage[3].C
            );

            Assert.AreEqual(
                originalDestination.TestMap.MapInt64Int64[1],
                destination.TestMap.MapInt64Int64[1]
            );
            Assert.AreEqual(source.TestMap.MapInt64Int64[2], destination.TestMap.MapInt64Int64[2]);
            Assert.AreEqual(source.TestMap.MapInt64Int64[3], destination.TestMap.MapInt64Int64[3]);

            Assert.AreEqual(
                originalDestination.TestMap.MapSfixed32Sfixed32[1],
                destination.TestMap.MapSfixed32Sfixed32[1]
            );
            Assert.AreEqual(
                source.TestMap.MapSfixed32Sfixed32[2],
                destination.TestMap.MapSfixed32Sfixed32[2]
            );
            Assert.AreEqual(
                source.TestMap.MapSfixed32Sfixed32[3],
                destination.TestMap.MapSfixed32Sfixed32[3]
            );

            Assert.AreEqual(
                originalDestination.TestMap.MapSfixed64Sfixed64[1],
                destination.TestMap.MapSfixed64Sfixed64[1]
            );
            Assert.AreEqual(
                source.TestMap.MapSfixed64Sfixed64[2],
                destination.TestMap.MapSfixed64Sfixed64[2]
            );
            Assert.AreEqual(
                source.TestMap.MapSfixed64Sfixed64[3],
                destination.TestMap.MapSfixed64Sfixed64[3]
            );

            Assert.AreEqual(
                originalDestination.TestMap.MapSint32Sint32[1],
                destination.TestMap.MapSint32Sint32[1]
            );
            Assert.AreEqual(
                source.TestMap.MapSint32Sint32[2],
                destination.TestMap.MapSint32Sint32[2]
            );
            Assert.AreEqual(
                source.TestMap.MapSint32Sint32[3],
                destination.TestMap.MapSint32Sint32[3]
            );

            Assert.AreEqual(
                originalDestination.TestMap.MapSint64Sint64[1],
                destination.TestMap.MapSint64Sint64[1]
            );
            Assert.AreEqual(
                source.TestMap.MapSint64Sint64[2],
                destination.TestMap.MapSint64Sint64[2]
            );
            Assert.AreEqual(
                source.TestMap.MapSint64Sint64[3],
                destination.TestMap.MapSint64Sint64[3]
            );

            Assert.AreEqual(
                originalDestination.TestMap.MapUint32Uint32[1],
                destination.TestMap.MapUint32Uint32[1]
            );
            Assert.AreEqual(
                source.TestMap.MapUint32Uint32[2],
                destination.TestMap.MapUint32Uint32[2]
            );
            Assert.AreEqual(
                source.TestMap.MapUint32Uint32[3],
                destination.TestMap.MapUint32Uint32[3]
            );

            Assert.AreEqual(
                originalDestination.TestMap.MapUint64Uint64[1],
                destination.TestMap.MapUint64Uint64[1]
            );
            Assert.AreEqual(
                source.TestMap.MapUint64Uint64[2],
                destination.TestMap.MapUint64Uint64[2]
            );
            Assert.AreEqual(
                source.TestMap.MapUint64Uint64[3],
                destination.TestMap.MapUint64Uint64[3]
            );
        }

        [Test]
        [TestCase(true, true, true)]
        [TestCase(true, true, false)]
        [TestCase(true, false, true)]
        [TestCase(true, false, false)]
        // treeTargetsEachField and replaceMessageFields can't be both set to true.
        // [TestCase(false, true, true)]
        [TestCase(false, true, false)]
        // [TestCase(false, false, true)]
        [TestCase(false, false, false)]
        public void MergeMapMessagesWithFieldMaskTargetingMap(
            bool treeTargetsEachField,
            bool useDynamicMessage,
            bool replaceMessageFields
        )
        {
            var (source, destination) = PrepareTests();

            var tree = new FieldMaskTree();
            if (treeTargetsEachField)
            {
                tree.AddFieldPath("test_map.map_bool_bool")
                    .AddFieldPath("test_map.map_int32_int32")
                    .AddFieldPath("test_map.map_string_string")
                    .AddFieldPath("test_map.map_fixed32_fixed32")
                    .AddFieldPath("test_map.map_fixed64_fixed64")
                    .AddFieldPath("test_map.map_int32_bytes")
                    .AddFieldPath("test_map.map_int32_double")
                    .AddFieldPath("test_map.map_int32_enum")
                    .AddFieldPath("test_map.map_int32_float")
                    .AddFieldPath("test_map.map_int32_foreign_message")
                    .AddFieldPath("test_map.map_int64_int64")
                    .AddFieldPath("test_map.map_sfixed32_sfixed32")
                    .AddFieldPath("test_map.map_sfixed64_sfixed64")
                    .AddFieldPath("test_map.map_sint32_sint32")
                    .AddFieldPath("test_map.map_sint64_sint64")
                    .AddFieldPath("test_map.map_uint32_uint32")
                    .AddFieldPath("test_map.map_uint64_uint64");
            }
            else
            {
                tree.AddFieldPath("test_map");
            }
            var originalDestination = destination.Clone();

            Merge(
                tree,
                source,
                destination,
                new FieldMask.MergeOptions() { ReplaceMessageFields = replaceMessageFields },
                useDynamicMessage
            );

            AssertFullMerge(source, destination, originalDestination);
        }

        [Test]
        [TestCase(true, true)]
        [TestCase(true, false)]
        [TestCase(false, true)]
        [TestCase(false, false)]
        public void MergeMapMessagesWithFieldMaskTargetingMapKey(
            bool useDynamicMessage,
            bool replaceMessageFields
        )
        {
            var (source, destination) = PrepareTests();

            var tree = new FieldMaskTree();
            tree.AddFieldPath("test_map.map_bool_bool.false")
                .AddFieldPath("test_map.map_int32_int32.2")
                .AddFieldPath("test_map.map_int32_int32.3")
                .AddFieldPath("test_map.map_string_string.b")
                .AddFieldPath("test_map.map_string_string.c")
                .AddFieldPath("test_map.map_fixed32_fixed32.2")
                .AddFieldPath("test_map.map_fixed32_fixed32.3")
                .AddFieldPath("test_map.map_fixed64_fixed64.2")
                .AddFieldPath("test_map.map_fixed64_fixed64.3")
                .AddFieldPath("test_map.map_int32_bytes.2")
                .AddFieldPath("test_map.map_int32_bytes.3")
                .AddFieldPath("test_map.map_int32_double.2")
                .AddFieldPath("test_map.map_int32_double.3")
                .AddFieldPath("test_map.map_int32_enum.2")
                .AddFieldPath("test_map.map_int32_enum.3")
                .AddFieldPath("test_map.map_int32_float.2")
                .AddFieldPath("test_map.map_int32_float.3")
                .AddFieldPath("test_map.map_int32_foreign_message.2")
                .AddFieldPath("test_map.map_int32_foreign_message.3")
                .AddFieldPath("test_map.map_int64_int64.2")
                .AddFieldPath("test_map.map_int64_int64.3")
                .AddFieldPath("test_map.map_sfixed32_sfixed32.2")
                .AddFieldPath("test_map.map_sfixed32_sfixed32.3")
                .AddFieldPath("test_map.map_sfixed64_sfixed64.2")
                .AddFieldPath("test_map.map_sfixed64_sfixed64.3")
                .AddFieldPath("test_map.map_sint32_sint32.2")
                .AddFieldPath("test_map.map_sint32_sint32.3")
                .AddFieldPath("test_map.map_sint64_sint64.2")
                .AddFieldPath("test_map.map_sint64_sint64.3")
                .AddFieldPath("test_map.map_uint32_uint32.2")
                .AddFieldPath("test_map.map_uint32_uint32.3")
                .AddFieldPath("test_map.map_uint64_uint64.2")
                .AddFieldPath("test_map.map_uint64_uint64.3");

            var originalDestination = destination.Clone();

            Merge(
                tree,
                source,
                destination,
                new FieldMask.MergeOptions() { ReplaceMessageFields = replaceMessageFields },
                useDynamicMessage
            );

            AssertFullMerge(source, destination, originalDestination);
        }

        [Test]
        [TestCase(true, true)]
        [TestCase(true, false)]
        [TestCase(false, true)]
        [TestCase(false, false)]
        public void MergeMapMessagesWithFieldMaskTargetingMapKeyWithInvalidSubKey(
            bool useDynamicMessage,
            bool replaceMessageFields
        )
        {
            var (source, destination) = PrepareTests();

            var tree = new FieldMaskTree();
            tree.AddFieldPath("test_map.map_int32_foreign_message.1.c.invalid_sub_key")
                .AddFieldPath("test_map.map_int32_foreign_message.999.c.invalid_sub_key");
            var originalDestination = destination.Clone();
            Merge(
                tree,
                source,
                destination,
                new FieldMask.MergeOptions() { ReplaceMessageFields = replaceMessageFields },
                useDynamicMessage
            );

            // these are all invalid paths so nothing should change.
            Assert.AreEqual(destination, originalDestination);
        }

        [Test]
        [TestCase(true, true)]
        [TestCase(true, false)]
        [TestCase(false, true)]
        [TestCase(false, false)]
        public void MergeMapMessagesWithFieldMaskTargetingMapKeyWithSubkey(
            bool useDynamicMessage,
            bool replaceMessageFields
        )
        {
            var (source, destination) = PrepareTests();

            var tree = new FieldMaskTree();
            tree.AddFieldPath("test_map.map_int32_foreign_message.3.c")
                .AddFieldPath("test_map.map_int32_foreign_message.1.c")
                .AddFieldPath("test_map.map_int32_foreign_message.999.c");
            var originalDestination = destination.Clone();
            Merge(
                tree,
                source,
                destination,
                new FieldMask.MergeOptions() { ReplaceMessageFields = replaceMessageFields },
                useDynamicMessage
            );

            // map_int32_foreign_message[3].c exists in source, but doesn't exist in the destination.
            // so it should set the destination to the source value.
            Assert.AreEqual(
                source.TestMap.MapInt32ForeignMessage[3],
                destination.TestMap.MapInt32ForeignMessage[3]
            );
            // map_int32_foreign_message[1].c doesn't exist in source, but it exists in the destination,
            // so it should set the destination to an empty message with a default value.
            Assert.AreEqual(new ForeignMessage(), destination.TestMap.MapInt32ForeignMessage[1]);
            // map_int32_foreign_message[1].c doesn't exist in source nor in the destination,
            // so it should be ignored.
            Assert.IsFalse(destination.TestMap.MapInt32ForeignMessage.ContainsKey(999));
        }

        [Test]
        [TestCase(true, true)]
        [TestCase(true, false)]
        [TestCase(false, true)]
        [TestCase(false, false)]
        public void MergeMapMessagesWithFieldMaskTargetingMapKey_InvalidCases_InvalidSubKey(
            bool useDynamicMessage,
            bool replaceMessageFields
        )
        {
            var (source, destination) = PrepareTests();

            var tree = new FieldMaskTree();
            tree.AddFieldPath("test_map.map_bool_bool.invalid_boolean.invalid_sub_key")
                .AddFieldPath("test_map.map_int32_int32.invalid_number.invalid_sub_key")
                .AddFieldPath("test_map.map_int32_int32.999.invalid_sub_key")
                .AddFieldPath("test_map.map_string_string..invalid_sub_key")
                .AddFieldPath("test_map.map_string_string.non_existent_key.invalid_sub_key")
                .AddFieldPath("test_map.map_fixed32_fixed32.invalid_number.invalid_sub_key")
                .AddFieldPath("test_map.map_fixed32_fixed32.999.invalid_sub_key")
                .AddFieldPath("test_map.map_fixed64_fixed64.invalid_number.invalid_sub_key")
                .AddFieldPath("test_map.map_fixed64_fixed64.999.invalid_sub_key")
                .AddFieldPath("test_map.map_int64_int64.invalid_number.invalid_sub_key")
                .AddFieldPath("test_map.map_int64_int64.999.invalid_sub_key")
                .AddFieldPath("test_map.map_sfixed32_sfixed32.invalid_number.invalid_sub_key")
                .AddFieldPath("test_map.map_sfixed32_sfixed32.999.invalid_sub_key")
                .AddFieldPath("test_map.map_sfixed64_sfixed64.invalid_number.invalid_sub_key")
                .AddFieldPath("test_map.map_sint32_sint32.invalid_number.invalid_sub_key")
                .AddFieldPath("test_map.map_sint32_sint32.999.invalid_sub_key")
                .AddFieldPath("test_map.map_sint64_sint64.invalid_number.invalid_sub_key")
                .AddFieldPath("test_map.map_sint64_sint64.999.invalid_sub_key")
                .AddFieldPath("test_map.map_sfixed64_sfixed64.999.invalid_sub_key")
                .AddFieldPath("test_map.map_uint32_uint32.invalid_number.invalid_sub_key")
                .AddFieldPath("test_map.map_uint32_uint32.999.invalid_sub_key")
                .AddFieldPath("test_map.map_uint64_uint64.invalid_number.invalid_sub_key")
                .AddFieldPath("test_map.map_uint64_uint64.999.invalid_sub_key");
            var originalDestination = destination.Clone();
            Merge(
                tree,
                source,
                destination,
                new FieldMask.MergeOptions() { ReplaceMessageFields = replaceMessageFields },
                useDynamicMessage
            );
            // all of these are invalid paths so nothing should change.
            Assert.AreEqual(destination, originalDestination);
        }

        [Test]
        [TestCase(true, true)]
        [TestCase(true, false)]
        [TestCase(false, true)]
        [TestCase(false, false)]
        public void MergeMapMessagesWithFieldMaskTargetingMapKey_InvalidCases(
            bool useDynamicMessage,
            bool replaceMessageFields
        )
        {
            var (source, destination) = PrepareTests();

            var tree = new FieldMaskTree();
            tree.AddFieldPath("test_map.map_bool_bool.invalid_boolean")
                .AddFieldPath("test_map.map_int32_int32.invalid_number")
                .AddFieldPath("test_map.map_int32_int32.999")
                .AddFieldPath("test_map.map_fixed32_fixed32.invalid_number")
                .AddFieldPath("test_map.map_fixed32_fixed32.999")
                .AddFieldPath("test_map.map_fixed64_fixed64.invalid_number")
                .AddFieldPath("test_map.map_fixed64_fixed64.999")
                .AddFieldPath("test_map.map_int64_int64.invalid_number")
                .AddFieldPath("test_map.map_int64_int64.999")
                .AddFieldPath("test_map.map_sfixed32_sfixed32.invalid_number")
                .AddFieldPath("test_map.map_sfixed32_sfixed32.999")
                .AddFieldPath("test_map.map_sfixed64_sfixed64.invalid_number")
                .AddFieldPath("test_map.map_sfixed64_sfixed64.999")
                .AddFieldPath("test_map.map_sint32_sint32.invalid_number")
                .AddFieldPath("test_map.map_sint32_sint32.999")
                .AddFieldPath("test_map.map_uint32_uint32.invalid_number")
                .AddFieldPath("test_map.map_uint32_uint32.999")
                .AddFieldPath("test_map.map_uint64_uint64.invalid_number")
                .AddFieldPath("test_map.map_uint64_uint64.999");
            var originalDestination = destination.Clone();
            Merge(
                tree,
                source,
                destination,
                new FieldMask.MergeOptions() { ReplaceMessageFields = replaceMessageFields },
                useDynamicMessage
            );
            // all of these are invalid paths so nothing should change.
            Assert.AreEqual(destination, originalDestination);
        }

        private (Struct source, Struct destination) PrepareStructTests()
        {
            var destination = new Struct()
            {
                Fields =
                {
                    ["a"] = new Value() { StringValue = "a" },
                    ["b"] = new Value() { NumberValue = 10.5 },
                    ["c"] = new Value() { BoolValue = true },
                    ["d"] = new Value() { NullValue = NullValue.NullValue },
                    ["e"] = new Value()
                    {
                        ListValue = new()
                        {
                            Values =
                            {
                                new Value() { NumberValue = 1 },
                                new Value() { NumberValue = 2 },
                                new Value() { NumberValue = 3 },
                            }
                        }
                    },
                    ["f"] = new Value()
                    {
                        StructValue = new()
                        {
                            Fields =
                            {
                                ["aa"] = new Value() { NumberValue = 1 },
                                ["bb"] = new Value() { NumberValue = 2 },
                                ["cc"] = new Value() { NumberValue = 3 },
                            }
                        }
                    },
                    ["h"] = new Value() { NumberValue = 1 },
                }
            };

            var source = new Struct()
            {
                Fields =
                {
                    ["a"] = new Value() { NumberValue = 300 },
                    ["b"] = new Value() { StringValue = "b" },
                    ["c"] = new Value()
                    {
                        ListValue = new() { Values = { new Value() { NumberValue = 1 } } }
                    },
                    ["d"] = new Value() { BoolValue = false },
                    ["e"] = new Value()
                    {
                        StructValue = new()
                        {
                            Fields =
                            {
                                ["aa"] = new Value() { NumberValue = 1 },
                                ["bb"] = new Value() { NumberValue = 2 },
                            }
                        }
                    },
                    ["f"] = new Value() { NullValue = NullValue.NullValue },
                    ["g"] = new Value() { StringValue = "g" },
                }
            };

            return (source, destination);
        }

        /// <summary>
        /// Asserts that a full merge from source to destination works.
        /// </summary>
        /// <param name="source"></param>
        /// <param name="destination"></param>
        /// <param name="originalDestination"></param>
        private void AssertFullMerge(Struct source, Struct destination, Struct originalDestination)
        {
            Assert.AreEqual(source.Fields["a"], destination.Fields["a"]);
            Assert.AreEqual(source.Fields["b"], destination.Fields["b"]);
            Assert.AreEqual(source.Fields["c"], destination.Fields["c"]);
            Assert.AreEqual(source.Fields["d"], destination.Fields["d"]);
            Assert.AreEqual(source.Fields["e"], destination.Fields["e"]);
            Assert.AreEqual(source.Fields["f"], destination.Fields["f"]);
            Assert.AreEqual(source.Fields["g"], destination.Fields["g"]);
            Assert.AreEqual(originalDestination.Fields["h"], destination.Fields["h"]);
        }

        [Test]
        [TestCase(true, true)]
        [TestCase(true, false)]
        [TestCase(false, true)]
        [TestCase(false, false)]
        public void MergeWellKnownStruct(bool useDynamicMessage, bool replaceMessageFields)
        {
            var (source, destination) = PrepareStructTests();
            var originalDestination = destination.Clone();

            Merge(
                new FieldMaskTree().AddFieldPath("fields"),
                source,
                destination,
                new FieldMask.MergeOptions() { ReplaceMessageFields = replaceMessageFields },
                useDynamicMessage
            );

            AssertFullMerge(source, destination, originalDestination);
        }

        [Test]
        [TestCase(true, true)]
        [TestCase(true, false)]
        [TestCase(false, true)]
        [TestCase(false, false)]
        public void MergeWellKnownStructTargetingMapKeys(
            bool useDynamicMessage,
            bool replaceMessageFields
        )
        {
            var (source, destination) = PrepareStructTests();
            var originalDestination = destination.Clone();

            Merge(
                new FieldMaskTree()
                    .AddFieldPath("fields.a")
                    .AddFieldPath("fields.b")
                    .AddFieldPath("fields.c")
                    .AddFieldPath("fields.d")
                    .AddFieldPath("fields.e")
                    .AddFieldPath("fields.f")
                    .AddFieldPath("fields.g"),
                source,
                destination,
                new FieldMask.MergeOptions() { ReplaceMessageFields = replaceMessageFields },
                useDynamicMessage
            );

            AssertFullMerge(source, destination, originalDestination);
        }

        [Test]
        [TestCase(true, true)]
        [TestCase(true, false)]
        [TestCase(false, true)]
        [TestCase(false, false)]
        public void MergeWellKnownStructTargetingMapKeysWithSubkey_InSource(
            bool useDynamicMessage,
            bool replaceMessageFields
        )
        {
            var (source, destination) = PrepareStructTests();
            var originalDestination = destination.Clone();

            Merge(
                new FieldMaskTree().AddFieldPath("fields.a.number_value"),
                source,
                destination,
                new FieldMask.MergeOptions() { ReplaceMessageFields = replaceMessageFields },
                useDynamicMessage
            );

            // source has a "number_value" for "a", destination has a "string_value" for "a", mask is targeting "number_value",
            // so we should respect the mask and set the destination to the source value (number_value 300).
            Assert.AreEqual(Value.KindOneofCase.NumberValue, destination.Fields["a"].KindCase);
            Assert.AreEqual(source.Fields["a"], destination.Fields["a"]);
            // the rest of the fields should remain the same.
            Assert.AreEqual(originalDestination.Fields["b"], destination.Fields["b"]);
        }

        [Test]
        [TestCase(true, true)]
        [TestCase(true, false)]
        [TestCase(false, true)]
        [TestCase(false, false)]
        public void MergeWellKnownStructTargetingMapKeysWithSubkey_InDestination(
            bool useDynamicMessage,
            bool replaceMessageFields
        )
        {
            var (source, destination) = PrepareStructTests();
            var originalDestination = destination.Clone();

            Merge(
                new FieldMaskTree().AddFieldPath("fields.a.string_value"),
                source,
                destination,
                new FieldMask.MergeOptions() { ReplaceMessageFields = replaceMessageFields },
                useDynamicMessage
            );

            // source has a "number_value" for "a", destination has a "string_value" for "a", mask is targeting "string_value",
            // so we should respect the mask and set the destination to the source value (empty string).
            Assert.AreEqual(destination.Fields["a"].KindCase, Value.KindOneofCase.StringValue);
            Assert.AreEqual(string.Empty, destination.Fields["a"].StringValue);
            // the rest of the fields should remain the same.
            Assert.AreEqual(originalDestination.Fields["b"], destination.Fields["b"]);
        }

        [Test]
        [TestCase(true, true)]
        [TestCase(true, false)]
        [TestCase(false, true)]
        [TestCase(false, false)]
        public void MergeWellKnownStructTargetingMapKeysWithSubkey_OneOfConflict_StringLast(
            bool useDynamicMessage,
            bool replaceMessageFields
        )
        {
            var (source, destination) = PrepareStructTests();
            var originalDestination = destination.Clone();

            Merge(
                new FieldMaskTree()
                .AddFieldPath("fields.a.number_value")
                .AddFieldPath("fields.a.string_value"),
                source,
                destination,
                new FieldMask.MergeOptions() { ReplaceMessageFields = replaceMessageFields },
                useDynamicMessage
            );

            // source has a "number_value" for "a", destination has a "string_value" for "a",
            // mask is targeting "number_value" first, then "string_value",
            // we should respect the last path and set the destination to the source value (empty string).
            Assert.AreEqual(destination.Fields["a"].KindCase, Value.KindOneofCase.StringValue);
            Assert.AreEqual(string.Empty, destination.Fields["a"].StringValue);
            // the rest of the fields should remain the same.
            Assert.AreEqual(originalDestination.Fields["b"], destination.Fields["b"]);
        }

        [Test]
        [TestCase(true, true)]
        [TestCase(true, false)]
        [TestCase(false, true)]
        [TestCase(false, false)]
        public void MergeWellKnownStructTargetingMapKeysWithSubkey_OneOfConflict_NumberLast(
            bool useDynamicMessage,
            bool replaceMessageFields
        )
        {
            var (source, destination) = PrepareStructTests();
            var originalDestination = destination.Clone();

            Merge(
                new FieldMaskTree()
                .AddFieldPath("fields.a.string_value")
                .AddFieldPath("fields.a.number_value"),
                source,
                destination,
                new FieldMask.MergeOptions() { ReplaceMessageFields = replaceMessageFields },
                useDynamicMessage
            );

            // source has a "number_value" for "a", destination has a "string_value" for "a",
            // mask is targeting "string_value" first, then "number_value"
            // so we should respect the last path and set the destination to the source value (number 300).
            Assert.AreEqual(destination.Fields["a"].KindCase, Value.KindOneofCase.NumberValue);
            Assert.AreEqual(source.Fields["a"].NumberValue, destination.Fields["a"].NumberValue);
            // the rest of the fields should remain the same.
            Assert.AreEqual(originalDestination.Fields["b"], destination.Fields["b"]);
        }

        private void Merge(
            FieldMaskTree tree,
            IMessage source,
            IMessage destination,
            FieldMask.MergeOptions options,
            bool useDynamicMessage
        )
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
    }
}
