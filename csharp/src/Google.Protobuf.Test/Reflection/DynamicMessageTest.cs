#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using Google.Protobuf.TestProtos;
using NUnit.Framework;
using UnitTest.Issues.TestProtos;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Tests for <see cref="DynamicMessage"/>.
    /// </summary>
    [TestFixture]
    public class DynamicMessageTest
    {
        // Dynamic descriptors loaded from serialized data (no CLR type association)
        // These must be used with DynamicMessage since generated descriptors have ClrType set.
        private static readonly IReadOnlyList<FileDescriptor> DynamicProto3Files;
        private static readonly MessageDescriptor DynamicTestAllTypes;
        private static readonly MessageDescriptor DynamicNestedMessage;
        private static readonly MessageDescriptor DynamicForeignMessage;
        private static readonly MessageDescriptor DynamicTestOneof; // Message with oneof fields, for "wrong descriptor" tests
        private static readonly MessageDescriptor DynamicTestMap;

        // Proto2 dynamic descriptors for extension tests
        private static readonly IReadOnlyList<FileDescriptor> DynamicProto2Files;
        private static readonly MessageDescriptor DynamicProto2TestAllExtensions;
        private static readonly FileDescriptor DynamicProto2UnittestFile;

        static DynamicMessageTest()
        {
            // Load proto3 test descriptors dynamically
            var proto3Data = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            DynamicProto3Files = FileDescriptor.BuildFromByteStrings(proto3Data);
            DynamicTestAllTypes = DynamicProto3Files[2].FindTypeByName<MessageDescriptor>("TestAllTypes");
            DynamicNestedMessage = DynamicTestAllTypes.NestedTypes.First(t => t.Name == "NestedMessage");
            DynamicForeignMessage = DynamicProto3Files[2].FindTypeByName<MessageDescriptor>("ForeignMessage");
            DynamicTestOneof = DynamicProto3Files[2].FindTypeByName<MessageDescriptor>("TestOneof");

            // Load map test descriptor (depends on unittest_proto3)
            var mapData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData,
                MapUnittestProto3Reflection.Descriptor.SerializedData
            };
            var mapFiles = FileDescriptor.BuildFromByteStrings(mapData);
            DynamicTestMap = mapFiles[3].FindTypeByName<MessageDescriptor>("TestMap");

            // Load proto2 test descriptors for extension tests
            // Order: public first (no deps), then import (depends on public), then unittest (depends on import)
            var proto2Data = new List<ByteString>
            {
                Google.Protobuf.TestProtos.Proto2.UnittestImportPublicReflection.Descriptor.SerializedData,
                Google.Protobuf.TestProtos.Proto2.UnittestImportReflection.Descriptor.SerializedData,
                Google.Protobuf.TestProtos.Proto2.UnittestReflection.Descriptor.SerializedData
            };
            DynamicProto2Files = FileDescriptor.BuildFromByteStrings(proto2Data);
            DynamicProto2UnittestFile = DynamicProto2Files[2];
            DynamicProto2TestAllExtensions = DynamicProto2UnittestFile.FindTypeByName<MessageDescriptor>("TestAllExtensions");
        }

        [Test]
        public void EmptyMessage()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            Assert.AreEqual(descriptor, message.Descriptor);
            Assert.AreEqual(0, message.CalculateSize());
        }

        [Test]
        public void SetField_SingularPrimitive()
        {
            var descriptor = DynamicTestAllTypes;

            var message = new DynamicMessage(descriptor);
            var int32Field = descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);
            message.SetField(int32Field, 42);

            Assert.AreEqual(42, message.GetField(int32Field));
            Assert.IsTrue(message.HasField(int32Field));
        }

        [Test]
        public void SetField_String()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            var stringField = descriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber);
            message.SetField(stringField, "hello");

            Assert.AreEqual("hello", message.GetField(stringField));
        }

        [Test]
        public void SetField_NestedMessage()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            var nestedField = descriptor.FindFieldByNumber(TestAllTypes.SingleNestedMessageFieldNumber);
            var nestedDescriptor = nestedField.MessageType;

            var nested = new DynamicMessage(nestedDescriptor);
            nested.SetField(nestedDescriptor.FindFieldByNumber(TestAllTypes.Types.NestedMessage.BbFieldNumber), 123);

            message.SetField(nestedField, nested);

            var result = message.GetField(nestedField) as DynamicMessage;
            Assert.NotNull(result);
            Assert.AreEqual(123, result.GetField(nestedDescriptor.FindFieldByNumber(TestAllTypes.Types.NestedMessage.BbFieldNumber)));
        }

        [Test]
        public void AddRepeatedField()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);
            message.AddRepeatedField(repeatedField, 1);
            message.AddRepeatedField(repeatedField, 2);
            message.AddRepeatedField(repeatedField, 3);

            Assert.AreEqual(3, message.GetRepeatedFieldCount(repeatedField));
            Assert.AreEqual(1, message.GetRepeatedField(repeatedField, 0));
            Assert.AreEqual(2, message.GetRepeatedField(repeatedField, 1));
            Assert.AreEqual(3, message.GetRepeatedField(repeatedField, 2));
        }

        [Test]
        public void SetField_Bytes()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            var bytesField = descriptor.FindFieldByNumber(TestAllTypes.SingleBytesFieldNumber);
            var bytesValue = ByteString.CopyFromUtf8("test bytes");
            message.SetField(bytesField, bytesValue);

            Assert.AreEqual(bytesValue, message.GetField(bytesField));
        }

        [Test]
        public void RoundTrip_Serialization()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 42);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber), "hello");
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleBoolFieldNumber), true);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleDoubleFieldNumber), 3.14);

            // Serialize to bytes
            var bytes = message.ToByteArray();

            // Parse back
            var parsed = DynamicMessage.ParseFrom(descriptor, bytes);

            Assert.AreEqual(42, parsed.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber)));
            Assert.AreEqual("hello", parsed.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber)));
            Assert.AreEqual(true, parsed.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleBoolFieldNumber)));
            Assert.AreEqual(3.14, parsed.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleDoubleFieldNumber)));
        }

        [Test]
        public void RoundTrip_WithNestedMessage()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            var nestedField = descriptor.FindFieldByNumber(TestAllTypes.SingleNestedMessageFieldNumber);
            var nestedDescriptor = nestedField.MessageType;
            var nested = new DynamicMessage(nestedDescriptor);
            nested.SetField(nestedDescriptor.FindFieldByNumber(TestAllTypes.Types.NestedMessage.BbFieldNumber), 999);

            message.SetField(nestedField, nested);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 42);

            var bytes = message.ToByteArray();
            var parsed = DynamicMessage.ParseFrom(descriptor, bytes);

            Assert.AreEqual(42, parsed.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber)));
            var parsedNested = parsed.GetField(nestedField) as DynamicMessage;
            Assert.NotNull(parsedNested);
            Assert.AreEqual(999, parsedNested.GetField(nestedDescriptor.FindFieldByNumber(TestAllTypes.Types.NestedMessage.BbFieldNumber)));
        }

        [Test]
        public void RoundTrip_WithRepeatedField()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);
            message.AddRepeatedField(repeatedField, 10);
            message.AddRepeatedField(repeatedField, 20);
            message.AddRepeatedField(repeatedField, 30);

            var bytes = message.ToByteArray();
            var parsed = DynamicMessage.ParseFrom(descriptor, bytes);

            Assert.AreEqual(3, parsed.GetRepeatedFieldCount(repeatedField));
            Assert.AreEqual(10, parsed.GetRepeatedField(repeatedField, 0));
            Assert.AreEqual(20, parsed.GetRepeatedField(repeatedField, 1));
            Assert.AreEqual(30, parsed.GetRepeatedField(repeatedField, 2));
        }

        [Test]
        public void Equality()
        {
            var descriptor = DynamicTestAllTypes;

            var message1 = new DynamicMessage(descriptor);
            message1.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 42);
            message1.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber), "test");

            var message2 = new DynamicMessage(descriptor);
            message2.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 42);
            message2.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber), "test");

            Assert.AreEqual(message1, message2);
            Assert.AreEqual(message1.GetHashCode(), message2.GetHashCode());
        }

        [Test]
        public void EmptyRepeatedAndMapFieldAccess_DoesNotAffectEqualityOrGetAllFields()
        {
            {
                var descriptor = DynamicTestAllTypes;
                var message1 = new DynamicMessage(descriptor);
                var message2 = new DynamicMessage(descriptor);

                var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);
                _ = message1.GetField(repeatedField);

                Assert.AreEqual(message2, message1);
                Assert.AreEqual(message2.GetHashCode(), message1.GetHashCode());
                Assert.IsEmpty(message1.GetAllFields());
            }

            {
                var descriptor = DynamicTestMap;
                var message1 = new DynamicMessage(descriptor);
                var message2 = new DynamicMessage(descriptor);

                var mapField = descriptor.FindFieldByName("map_int32_int32");
                _ = message1.GetField(mapField);

                Assert.AreEqual(message2, message1);
                Assert.AreEqual(message2.GetHashCode(), message1.GetHashCode());
                Assert.IsEmpty(message1.GetAllFields());
            }
        }

        [Test]
        public void Inequality()
        {
            var descriptor = DynamicTestAllTypes;

            var message1 = new DynamicMessage(descriptor);
            message1.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 42);

            var message2 = new DynamicMessage(descriptor);
            message2.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 99);

            Assert.AreNotEqual(message1, message2);
        }

        [Test]
        public void Clone_ReturnsDeepCopy()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 42);

            var clone = message.Clone();

            // Clone should be a different instance
            Assert.AreNotSame(message, clone);
            Assert.AreEqual(42, clone.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber)));

            // Modifying clone should not affect original
            clone.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 99);
            Assert.AreEqual(42, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber)));
            Assert.AreEqual(99, clone.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber)));
        }

        [Test]
        public void Mutability()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 42);

            // Should be able to modify directly
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 99);
            Assert.AreEqual(99, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber)));

            // Clear field
            message.ClearField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber));
            Assert.IsFalse(message.HasField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber)));
        }

        [Test]
        public void InteroperabilityWithGeneratedMessage()
        {
            // Create a generated message
            var generated = new TestAllTypes
            {
                SingleInt32 = 42,
                SingleString = "hello",
                SingleBool = true
            };

            // Serialize it
            var bytes = generated.ToByteArray();

            // Parse as DynamicMessage
            var descriptor = DynamicTestAllTypes;
            var dynamic = DynamicMessage.ParseFrom(descriptor, bytes);

            Assert.AreEqual(42, dynamic.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber)));
            Assert.AreEqual("hello", dynamic.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber)));
            Assert.AreEqual(true, dynamic.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleBoolFieldNumber)));

            // Re-serialize the dynamic message
            var dynamicBytes = dynamic.ToByteArray();

            // Parse as generated message
            var regenerated = TestAllTypes.Parser.ParseFrom(dynamicBytes);

            Assert.AreEqual(42, regenerated.SingleInt32);
            Assert.AreEqual("hello", regenerated.SingleString);
            Assert.AreEqual(true, regenerated.SingleBool);
        }

        [Test]
        public void Parser_ReturnsDynamicParserForDynamicDescriptor()
        {
            // Load descriptors dynamically
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);
            foreach (var fileDescriptor in converted)
            {
                foreach (var messageDescriptor in fileDescriptor.MessageTypes)
                {
                    Assert.NotNull(messageDescriptor.Parser, "Parser should not be null");
                }
            }

            var descriptor = converted[2].FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));
            // Parser should return a working parser for dynamic descriptors
            var parser = descriptor.Parser;
            Assert.NotNull(parser);

            var generated = new TestAllTypes { SingleInt32 = 123 };
            var bytes = generated.ToByteArray();

            var parsed = parser.ParseFrom(bytes);
            Assert.IsInstanceOf<DynamicMessage>(parsed);
            Assert.AreEqual(
                123,
                ((DynamicMessage)parsed).GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber))
            );
        }

        [Test]
        public void FieldAccessor_WithDynamicDescriptor()
        {
            // Load descriptors dynamically
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var testAllTypesDescriptor = converted[2]
                .FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));
            Assert.NotNull(testAllTypesDescriptor);

            // Create a message using the new API
            var message = new DynamicMessage(testAllTypesDescriptor);
            var int32Field = testAllTypesDescriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);

            // Use field accessor
            var accessor = int32Field.Accessor;
            Assert.NotNull(accessor);

            message.SetField(int32Field, 42);

            // GetValue should work
            var value = accessor.GetValue(message);
            Assert.AreEqual(42, value);

            // SetValue should also work since DynamicMessage is mutable
            accessor.SetValue(message, 99);
            Assert.AreEqual(99, message.GetField(int32Field));
        }

        [Test]
        public void BuildFromByteStrings_WithDynamicMessage()
        {
            // Load descriptors dynamically (without generated types)
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var testAllTypesDescriptor = converted[2]
                .FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));

            // Create a DynamicMessage using the dynamic descriptor
            var message = new DynamicMessage(testAllTypesDescriptor);
            message.SetField(testAllTypesDescriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 42);
            message.SetField(testAllTypesDescriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber), "dynamic");

            // Verify it serializes correctly
            var bytes = message.ToByteArray();

            // Parse it back using Parser (which returns DynamicMessage for dynamic descriptors)
            var parsed = (DynamicMessage)testAllTypesDescriptor.Parser.ParseFrom(bytes);

            Assert.AreEqual(
                42,
                parsed.GetField(testAllTypesDescriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber))
            );
            Assert.AreEqual(
                "dynamic",
                parsed.GetField(testAllTypesDescriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber))
            );
        }

        [Test]
        public void Oneof_SingleFieldSet()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            var oneofDescriptor = descriptor.Oneofs.FirstOrDefault(o => o.Name == "oneof_field");
            Assert.NotNull(oneofDescriptor);

            var oneofStringField = descriptor.FindFieldByNumber(TestAllTypes.OneofStringFieldNumber);
            message.SetField(oneofStringField, "test");

            Assert.IsTrue(message.HasOneof(oneofDescriptor));
            Assert.AreEqual(oneofStringField, message.GetOneofFieldDescriptor(oneofDescriptor));
            Assert.AreEqual("test", message.GetField(oneofStringField));
        }

        [Test]
        public void Oneof_SwitchingFields()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            var oneofDescriptor = descriptor.Oneofs.FirstOrDefault(o => o.Name == "oneof_field");
            var oneofStringField = descriptor.FindFieldByNumber(TestAllTypes.OneofStringFieldNumber);
            var oneofUint32Field = descriptor.FindFieldByNumber(TestAllTypes.OneofUint32FieldNumber);

            // Set string first
            message.SetField(oneofStringField, "test");
            Assert.AreEqual("test", message.GetField(oneofStringField));
            Assert.AreEqual(oneofStringField, message.GetOneofFieldDescriptor(oneofDescriptor));

            // Set uint32, which should clear string
            message.SetField(oneofUint32Field, 42u);
            Assert.AreEqual(42u, message.GetField(oneofUint32Field));
            Assert.AreEqual(oneofUint32Field, message.GetOneofFieldDescriptor(oneofDescriptor));

            // String should now return default
            Assert.AreEqual("", message.GetField(oneofStringField));
        }

        [Test]
        public void Enum_StoredAsInt()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            var enumField = descriptor.FindFieldByNumber(TestAllTypes.SingleNestedEnumFieldNumber);
            // Enums are stored as integers in DynamicMessage
            message.SetField(enumField, 1); // FOO = 1 in NestedEnum

            Assert.AreEqual(1, message.GetField(enumField));

            // Verify serialization round-trip
            var bytes = message.ToByteArray();
            var parsed = DynamicMessage.ParseFrom(descriptor, bytes);
            Assert.AreEqual(1, parsed.GetField(enumField));
        }

        [Test]
        public void CalculateSize_EmptyMessage()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            Assert.AreEqual(0, message.CalculateSize());
        }

        [Test]
        public void CalculateSize_WithFields()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 42);

            // Should match the generated message's size
            var generated = new TestAllTypes { SingleInt32 = 42 };
            Assert.AreEqual(generated.CalculateSize(), message.CalculateSize());
        }

        [Test]
        public void MergeFrom_CombinesMessages()
        {
            var descriptor = DynamicTestAllTypes;

            var message1 = new DynamicMessage(descriptor);
            message1.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 42);

            var message2 = new DynamicMessage(descriptor);
            message2.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber), "hello");

            // Merge message2 into message1
            message1.MergeFrom(message2);

            Assert.AreEqual(42, message1.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber)));
            Assert.AreEqual(
                "hello",
                message1.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber))
            );
        }

        [Test]
        public void GetAllFields_ReturnsSetFields()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            var int32Field = descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);
            var stringField = descriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber);

            message.SetField(int32Field, 42);
            message.SetField(stringField, "test");

            var allFields = message.GetAllFields();

            Assert.AreEqual(2, allFields.Count);
            Assert.IsTrue(allFields.ContainsKey(int32Field));
            Assert.IsTrue(allFields.ContainsKey(stringField));
            Assert.AreEqual(42, allFields[int32Field]);
            Assert.AreEqual("test", allFields[stringField]);
        }

        [Test]
        public void ClearOneof()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            var oneofDescriptor = descriptor.Oneofs.FirstOrDefault(o => o.Name == "oneof_field");
            var oneofStringField = descriptor.FindFieldByNumber(TestAllTypes.OneofStringFieldNumber);

            message.SetField(oneofStringField, "test");
            Assert.IsTrue(message.HasOneof(oneofDescriptor));

            message.ClearOneof(oneofDescriptor);
            Assert.IsFalse(message.HasOneof(oneofDescriptor));
            Assert.AreEqual("", message.GetField(oneofStringField));
        }

        [Test]
        public void SetRepeatedField()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);
            message.AddRepeatedField(repeatedField, 1);
            message.AddRepeatedField(repeatedField, 2);
            message.AddRepeatedField(repeatedField, 3);

            // Modify an element
            message.SetRepeatedField(repeatedField, 1, 20);

            Assert.AreEqual(1, message.GetRepeatedField(repeatedField, 0));
            Assert.AreEqual(20, message.GetRepeatedField(repeatedField, 1));
            Assert.AreEqual(3, message.GetRepeatedField(repeatedField, 2));
        }

        [Test]
        public void Oneof_TestOneof_Switching()
        {
            // TestOneof has one oneof "foo" with fields: foo_int, foo_string, foo_message
            var descriptor = DynamicTestOneof;
            var message = new DynamicMessage(descriptor);

            // Get the oneof descriptor
            var fooOneof = descriptor.Oneofs.FirstOrDefault(o => o.Name == "foo");
            Assert.NotNull(fooOneof);

            // Get fields for the oneof
            var fooIntField = descriptor.FindFieldByNumber(TestOneof.FooIntFieldNumber);
            var fooStringField = descriptor.FindFieldByNumber(TestOneof.FooStringFieldNumber);

            // Initially oneof should not be set
            Assert.IsFalse(message.HasOneof(fooOneof));

            // Set foo_int
            message.SetField(fooIntField, 42);
            Assert.IsTrue(message.HasOneof(fooOneof));
            Assert.AreEqual(fooIntField, message.GetOneofFieldDescriptor(fooOneof));
            Assert.AreEqual(42, message.GetField(fooIntField));

            // Switch to foo_string - should clear foo_int
            message.SetField(fooStringField, "hello");
            Assert.AreEqual(fooStringField, message.GetOneofFieldDescriptor(fooOneof));
            Assert.AreEqual("hello", message.GetField(fooStringField));
            Assert.AreEqual(0, message.GetField(fooIntField)); // Cleared by switching

            // Clear the oneof
            message.ClearOneof(fooOneof);
            Assert.IsFalse(message.HasOneof(fooOneof));
            Assert.AreEqual("", message.GetField(fooStringField));
        }

        [Test]
        public void Oneof_TestOneof_RoundTrip()
        {
            // Test serialization round-trip with oneof
            var descriptor = DynamicTestOneof;
            var message = new DynamicMessage(descriptor);

            var fooIntField = descriptor.FindFieldByNumber(TestOneof.FooIntFieldNumber);
            var fooStringField = descriptor.FindFieldByNumber(TestOneof.FooStringFieldNumber);

            message.SetField(fooStringField, "test value");

            // Serialize and parse back
            var bytes = message.ToByteArray();
            var parsed = DynamicMessage.ParseFrom(descriptor, bytes);

            // Verify field preserved
            Assert.AreEqual("test value", parsed.GetField(fooStringField));
            Assert.AreEqual(0, parsed.GetField(fooIntField)); // Not set

            // Verify oneof state preserved
            var fooOneof = descriptor.Oneofs.FirstOrDefault(o => o.Name == "foo");
            Assert.AreEqual(fooStringField, parsed.GetOneofFieldDescriptor(fooOneof));
        }

        #region Validation and edge case tests

        [Test]
        public void SetField_NullValue_Throws()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            var stringField = descriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber);

            Assert.Throws<ArgumentNullException>(() => message.SetField(stringField, null));
        }

        [Test]
        public void SetField_WrongType_Throws()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            var int32Field = descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);

            // Try to set a string value to an int32 field
            Assert.Throws<ArgumentException>(() => message.SetField(int32Field, "not an int"));
        }

        [Test]
        public void SetField_WrongDescriptor_Throws()
        {
            var descriptor1 = DynamicTestAllTypes;
            var descriptor2 = DynamicForeignMessage;
            var message = new DynamicMessage(descriptor1);

            // Try to use a field from a different descriptor
            var wrongField = descriptor2.FindFieldByNumber(ForeignMessage.CFieldNumber);

            Assert.Throws<ArgumentException>(() => message.SetField(wrongField, 42));
        }

        [Test]
        public void AddRepeatedField_NullValue_Throws()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            var repeatedStringField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedStringFieldNumber);

            Assert.Throws<ArgumentNullException>(() => message.AddRepeatedField(repeatedStringField, null));
        }

        [Test]
        public void AddRepeatedField_WrongType_Throws()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            var repeatedInt32Field = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);

            Assert.Throws<ArgumentException>(() => message.AddRepeatedField(repeatedInt32Field, "not an int"));
        }

        [Test]
        public void DefaultValues_ForUnsetFields()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            // Scalars should return default values
            Assert.AreEqual(0, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber)));
            Assert.AreEqual(0L, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt64FieldNumber)));
            Assert.AreEqual(0u, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleUint32FieldNumber)));
            Assert.AreEqual(0UL, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleUint64FieldNumber)));
            Assert.AreEqual(0f, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleFloatFieldNumber)));
            Assert.AreEqual(0d, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleDoubleFieldNumber)));
            Assert.AreEqual(false, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleBoolFieldNumber)));
            Assert.AreEqual("", message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber)));
            Assert.AreEqual(ByteString.Empty, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleBytesFieldNumber)));
            Assert.AreEqual(0, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleNestedEnumFieldNumber)));

            // Repeated fields should return empty lists
            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);
            var repeatedValue = message.GetField(repeatedField);
            Assert.IsInstanceOf<IList>(repeatedValue);
            Assert.AreEqual(0, ((IList)repeatedValue).Count);
        }

        [Test]
        public void CalculateSize_MatchesGeneratedMessage()
        {
            var descriptor = DynamicTestAllTypes;

            // Create identical messages
            var generated = new TestAllTypes
            {
                SingleInt32 = 42,
                SingleInt64 = 123456789L,
                SingleString = "hello world",
                SingleBool = true,
                SingleDouble = 3.14159
            };
            generated.RepeatedInt32.AddRange(new[] { 1, 2, 3, 4, 5 });

            var dynamic = new DynamicMessage(descriptor);
            dynamic.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 42);
            dynamic.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt64FieldNumber), 123456789L);
            dynamic.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber), "hello world");
            dynamic.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleBoolFieldNumber), true);
            dynamic.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleDoubleFieldNumber), 3.14159);

            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);
            foreach (var val in new[] { 1, 2, 3, 4, 5 })
            {
                dynamic.AddRepeatedField(repeatedField, val);
            }

            Assert.AreEqual(generated.CalculateSize(), dynamic.CalculateSize());
        }

        [Test]
        public void Serialization_MatchesGeneratedMessage()
        {
            var descriptor = DynamicTestAllTypes;

            var generated = new TestAllTypes
            {
                SingleInt32 = 42,
                SingleString = "hello"
            };

            var dynamic = new DynamicMessage(descriptor);
            dynamic.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 42);
            dynamic.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber), "hello");

            // Serialized bytes should be identical
            Assert.AreEqual(generated.ToByteArray(), dynamic.ToByteArray());
        }

        [Test]
        public void PackedRepeatedField_Serialization()
        {
            var descriptor = DynamicTestAllTypes;
            var dynamic = new DynamicMessage(descriptor);

            // RepeatedInt32 is packed by default in proto3
            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);
            dynamic.AddRepeatedField(repeatedField, 1);
            dynamic.AddRepeatedField(repeatedField, 2);
            dynamic.AddRepeatedField(repeatedField, 3);

            var bytes = dynamic.ToByteArray();

            // Parse as generated message and verify
            var generated = TestAllTypes.Parser.ParseFrom(bytes);
            Assert.AreEqual(3, generated.RepeatedInt32.Count);
            Assert.AreEqual(1, generated.RepeatedInt32[0]);
            Assert.AreEqual(2, generated.RepeatedInt32[1]);
            Assert.AreEqual(3, generated.RepeatedInt32[2]);
        }

        [Test]
        public void EmptyPackedRepeatedField()
        {
            var descriptor = DynamicTestAllTypes;
            var dynamic = new DynamicMessage(descriptor);

            // Don't add any elements to packed field
            var bytes = dynamic.ToByteArray();

            // Should parse correctly as empty
            var generated = TestAllTypes.Parser.ParseFrom(bytes);
            Assert.AreEqual(0, generated.RepeatedInt32.Count);
        }

        [Test]
        public void UnknownFields_PreservedOnClone()
        {
            var descriptor = DynamicTestAllTypes;

            // Create message with unknown field by parsing
            // We simulate unknown fields by creating a message with more fields
            // then parsing it with a descriptor that doesn't know about some fields
            var generated = new TestAllTypes { SingleInt32 = 42 };
            var bytes = generated.ToByteArray();

            var dynamic = DynamicMessage.ParseFrom(descriptor, bytes);

            // Clone should preserve unknown fields
            var clone = dynamic.Clone();

            // Both should serialize identically
            Assert.AreEqual(dynamic.ToByteArray(), clone.ToByteArray());
        }

        [Test]
        public void Clone_WithNestedMessage()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            var nestedField = descriptor.FindFieldByNumber(TestAllTypes.SingleNestedMessageFieldNumber);
            var nestedDescriptor = nestedField.MessageType;
            var bbField = nestedDescriptor.FindFieldByNumber(TestAllTypes.Types.NestedMessage.BbFieldNumber);

            var nested = new DynamicMessage(nestedDescriptor);
            nested.SetField(bbField, 42);
            message.SetField(nestedField, nested);

            var clone = message.Clone();

            // Modify the original nested message
            nested.SetField(bbField, 999);

            // Clone's nested message should be unaffected
            var clonedNested = (DynamicMessage)clone.GetField(nestedField);
            Assert.AreEqual(42, clonedNested.GetField(bbField));
        }

        [Test]
        public void Clone_WithRepeatedField()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);
            message.AddRepeatedField(repeatedField, 1);
            message.AddRepeatedField(repeatedField, 2);

            var clone = message.Clone();

            // Modify original
            message.AddRepeatedField(repeatedField, 3);

            // Clone should be unaffected
            Assert.AreEqual(3, message.GetRepeatedFieldCount(repeatedField));
            Assert.AreEqual(2, clone.GetRepeatedFieldCount(repeatedField));
        }

        [Test]
        public void MergeFrom_RepeatedFieldsConcatenate()
        {
            var descriptor = DynamicTestAllTypes;
            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);

            var message1 = new DynamicMessage(descriptor);
            message1.AddRepeatedField(repeatedField, 1);
            message1.AddRepeatedField(repeatedField, 2);

            var message2 = new DynamicMessage(descriptor);
            message2.AddRepeatedField(repeatedField, 3);
            message2.AddRepeatedField(repeatedField, 4);

            message1.MergeFrom(message2);

            Assert.AreEqual(4, message1.GetRepeatedFieldCount(repeatedField));
            Assert.AreEqual(1, message1.GetRepeatedField(repeatedField, 0));
            Assert.AreEqual(2, message1.GetRepeatedField(repeatedField, 1));
            Assert.AreEqual(3, message1.GetRepeatedField(repeatedField, 2));
            Assert.AreEqual(4, message1.GetRepeatedField(repeatedField, 3));
        }

        [Test]
        public void MergeFrom_NestedMessagesMerge()
        {
            var descriptor = DynamicTestAllTypes;
            var nestedField = descriptor.FindFieldByNumber(TestAllTypes.SingleNestedMessageFieldNumber);
            var nestedDescriptor = nestedField.MessageType;
            var bbField = nestedDescriptor.FindFieldByNumber(TestAllTypes.Types.NestedMessage.BbFieldNumber);

            var message1 = new DynamicMessage(descriptor);
            var nested1 = new DynamicMessage(nestedDescriptor);
            nested1.SetField(bbField, 1);
            message1.SetField(nestedField, nested1);

            var message2 = new DynamicMessage(descriptor);
            var nested2 = new DynamicMessage(nestedDescriptor);
            nested2.SetField(bbField, 2);
            message2.SetField(nestedField, nested2);

            message1.MergeFrom(message2);

            // Nested message should have merged (overwritten in this case since it's scalar)
            var mergedNested = (DynamicMessage)message1.GetField(nestedField);
            Assert.AreEqual(2, mergedNested.GetField(bbField));
        }

        [Test]
        public void MergeFrom_OneofFieldsRespected()
        {
            var descriptor = DynamicTestAllTypes;
            var oneofDescriptor = descriptor.Oneofs.FirstOrDefault(o => o.Name == "oneof_field");
            var oneofStringField = descriptor.FindFieldByNumber(TestAllTypes.OneofStringFieldNumber);
            var oneofUint32Field = descriptor.FindFieldByNumber(TestAllTypes.OneofUint32FieldNumber);

            var message1 = new DynamicMessage(descriptor);
            message1.SetField(oneofStringField, "original");

            var message2 = new DynamicMessage(descriptor);
            message2.SetField(oneofUint32Field, 42u);

            message1.MergeFrom(message2);

            // The oneof should now have the uint32 value from message2
            Assert.AreEqual(oneofUint32Field, message1.GetOneofFieldDescriptor(oneofDescriptor));
            Assert.AreEqual(42u, message1.GetField(oneofUint32Field));
            Assert.AreEqual("", message1.GetField(oneofStringField));
        }

        [Test]
        public void HasField_ThrowsForRepeatedField()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);

            Assert.Throws<ArgumentException>(() => message.HasField(repeatedField));
        }

        [Test]
        public void GetRepeatedFieldCount_ThrowsForSingularField()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            var singularField = descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);

            Assert.Throws<ArgumentException>(() => message.GetRepeatedFieldCount(singularField));
        }

        [Test]
        public void GetRepeatedField_ThrowsForSingularField()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            var singularField = descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);

            Assert.Throws<ArgumentException>(() => message.GetRepeatedField(singularField, 0));
        }

        [Test]
        public void AddRepeatedField_ThrowsForSingularField()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            var singularField = descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);

            Assert.Throws<ArgumentException>(() => message.AddRepeatedField(singularField, 42));
        }

        [Test]
        public void EqualsAndHashCode_WithSameUnknownFields()
        {
            var descriptor = DynamicTestAllTypes;

            // Create two messages from the same bytes (will have same unknown fields)
            var generated = new TestAllTypes { SingleInt32 = 42 };
            var bytes = generated.ToByteArray();

            var message1 = DynamicMessage.ParseFrom(descriptor, bytes);
            var message2 = DynamicMessage.ParseFrom(descriptor, bytes);

            Assert.AreEqual(message1, message2);
            Assert.AreEqual(message1.GetHashCode(), message2.GetHashCode());
        }

        [Test]
        public void Proto3_ImplicitPresence_DefaultValueNotStored()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            // In proto3, fields without explicit presence don't distinguish between
            // "not set" and "set to default value"
            var int32Field = descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);

            // Set to default value
            message.SetField(int32Field, 0);

            // The field should not have explicit presence
            Assert.IsFalse(message.HasField(int32Field));

            // GetAllFields should not include fields set to default
            var allFields = message.GetAllFields();
            Assert.IsFalse(allFields.ContainsKey(int32Field));
        }

        [Test]
        public void RepeatedNestedMessage_RoundTrip()
        {
            var descriptor = DynamicTestAllTypes;

            // First create using generated code to get expected bytes
            var generated = new TestAllTypes();
            generated.RepeatedNestedMessage.Add(new TestAllTypes.Types.NestedMessage { Bb = 1 });
            generated.RepeatedNestedMessage.Add(new TestAllTypes.Types.NestedMessage { Bb = 2 });
            var expectedBytes = generated.ToByteArray();

            // Now create the same using DynamicMessage
            var message = new DynamicMessage(descriptor);

            var repeatedNestedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedNestedMessageFieldNumber);
            var nestedDescriptor = repeatedNestedField.MessageType;
            var bbField = nestedDescriptor.FindFieldByNumber(TestAllTypes.Types.NestedMessage.BbFieldNumber);

            var nested1 = new DynamicMessage(nestedDescriptor);
            nested1.SetField(bbField, 1);

            var nested2 = new DynamicMessage(nestedDescriptor);
            nested2.SetField(bbField, 2);

            message.AddRepeatedField(repeatedNestedField, nested1);
            message.AddRepeatedField(repeatedNestedField, nested2);

            var actualBytes = message.ToByteArray();

            // Compare bytes
            Assert.AreEqual(expectedBytes, actualBytes, "Serialized bytes should match");

            // Now test parsing
            var parsed = DynamicMessage.ParseFrom(descriptor, actualBytes);

            Assert.AreEqual(2, parsed.GetRepeatedFieldCount(repeatedNestedField));
            Assert.AreEqual(1, ((DynamicMessage)parsed.GetRepeatedField(repeatedNestedField, 0)).GetField(bbField));
            Assert.AreEqual(2, ((DynamicMessage)parsed.GetRepeatedField(repeatedNestedField, 1)).GetField(bbField));
        }

        [Test]
        public void Equality_NaN_BitwiseComparison()
        {
            // NaN values should be compared bitwise - same NaN representation should be equal
            var descriptor = DynamicTestAllTypes;
            var doubleField = descriptor.FindFieldByNumber(TestAllTypes.SingleDoubleFieldNumber);
            var floatField = descriptor.FindFieldByNumber(TestAllTypes.SingleFloatFieldNumber);

            // Test double NaN
            var msg1 = new DynamicMessage(descriptor);
            msg1.SetField(doubleField, double.NaN);

            var msg2 = new DynamicMessage(descriptor);
            msg2.SetField(doubleField, double.NaN);

            Assert.AreEqual(msg1, msg2, "Two DynamicMessages with NaN double values should be equal");
            Assert.AreEqual(msg1.GetHashCode(), msg2.GetHashCode(), "Hash codes should match for NaN values");

            // Test float NaN
            var msg3 = new DynamicMessage(descriptor);
            msg3.SetField(floatField, float.NaN);

            var msg4 = new DynamicMessage(descriptor);
            msg4.SetField(floatField, float.NaN);

            Assert.AreEqual(msg3, msg4, "Two DynamicMessages with NaN float values should be equal");
            Assert.AreEqual(msg3.GetHashCode(), msg4.GetHashCode(), "Hash codes should match for NaN values");

            // Test that different types with NaN are not equal (different field numbers)
            Assert.AreNotEqual(msg1, msg3, "Messages with NaN in different fields should not be equal");
        }

        [Test]
        public void Equality_PositiveAndNegativeZero()
        {
            // For proto3 implicit presence fields, both +0.0 and -0.0 are considered
            // default values and are not stored. This is consistent with protobuf semantics
            // where zero values are not serialized.
            var descriptor = DynamicTestAllTypes;
            var doubleField = descriptor.FindFieldByNumber(TestAllTypes.SingleDoubleFieldNumber);

            var msg1 = new DynamicMessage(descriptor);
            msg1.SetField(doubleField, 0.0);

            var msg2 = new DynamicMessage(descriptor);
            msg2.SetField(doubleField, -0.0);

            // Both are considered default values for proto3 implicit presence,
            // so they are not stored and the messages are equal (both empty)
            Assert.AreEqual(msg1, msg2, "Messages with +0.0 and -0.0 should be equal for implicit presence fields");

            // Verify that neither value was stored (both are treated as default)
            Assert.IsFalse(msg1.GetAllFields().ContainsKey(doubleField), "+0.0 should not be stored");
            Assert.IsFalse(msg2.GetAllFields().ContainsKey(doubleField), "-0.0 should not be stored");
        }

        #endregion

        #region Roundtrip byte-for-byte tests

        [Test]
        public void Roundtrip_SimpleFields_BytesMatch()
        {
            // Create a generated message with various simple fields
            var generated = new TestAllTypes
            {
                SingleInt32 = 42,
                SingleInt64 = 123456789L,
                SingleUint32 = 100,
                SingleUint64 = 999999UL,
                SingleSint32 = -50,
                SingleSint64 = -12345L,
                SingleFixed32 = 1000,
                SingleFixed64 = 2000UL,
                SingleSfixed32 = -500,
                SingleSfixed64 = -1000L,
                SingleFloat = 3.14f,
                SingleDouble = 2.71828,
                SingleBool = true,
                SingleString = "hello world",
                SingleBytes = ByteString.CopyFromUtf8("binary data"),
                SingleNestedEnum = TestAllTypes.Types.NestedEnum.Bar
            };

            // Serialize original
            var originalBytes = generated.ToByteArray();

            // Parse as DynamicMessage
            var dynamic = DynamicMessage.ParseFrom(DynamicTestAllTypes, originalBytes);

            // Re-serialize
            var roundtripBytes = dynamic.ToByteArray();

            // Bytes should match exactly
            Assert.AreEqual(originalBytes, roundtripBytes,
                "Roundtrip serialization should produce identical bytes");
        }

        [Test]
        public void Roundtrip_NestedMessage_BytesMatch()
        {
            var generated = new TestAllTypes
            {
                SingleNestedMessage = new TestAllTypes.Types.NestedMessage { Bb = 42 },
                SingleForeignMessage = new ForeignMessage { C = 100 }
            };

            var originalBytes = generated.ToByteArray();
            var dynamic = DynamicMessage.ParseFrom(DynamicTestAllTypes, originalBytes);
            var roundtripBytes = dynamic.ToByteArray();

            Assert.AreEqual(originalBytes, roundtripBytes,
                "Roundtrip serialization of nested messages should produce identical bytes");
        }

        [Test]
        public void Roundtrip_RepeatedPrimitives_BytesMatch()
        {
            var generated = new TestAllTypes();
            generated.RepeatedInt32.AddRange(new[] { 1, 2, 3, 4, 5 });
            generated.RepeatedInt64.AddRange(new[] { 10L, 20L, 30L });
            generated.RepeatedString.AddRange(new[] { "a", "b", "c" });
            generated.RepeatedBool.AddRange(new[] { true, false, true });
            generated.RepeatedDouble.AddRange(new[] { 1.1, 2.2, 3.3 });

            var originalBytes = generated.ToByteArray();
            var dynamic = DynamicMessage.ParseFrom(DynamicTestAllTypes, originalBytes);
            var roundtripBytes = dynamic.ToByteArray();

            Assert.AreEqual(originalBytes, roundtripBytes,
                "Roundtrip serialization of repeated primitives should produce identical bytes");
        }

        [Test]
        public void Roundtrip_RepeatedNestedMessages_BytesMatch()
        {
            var generated = new TestAllTypes();
            generated.RepeatedNestedMessage.Add(new TestAllTypes.Types.NestedMessage { Bb = 1 });
            generated.RepeatedNestedMessage.Add(new TestAllTypes.Types.NestedMessage { Bb = 2 });
            generated.RepeatedNestedMessage.Add(new TestAllTypes.Types.NestedMessage { Bb = 3 });

            var originalBytes = generated.ToByteArray();
            var dynamic = DynamicMessage.ParseFrom(DynamicTestAllTypes, originalBytes);
            var roundtripBytes = dynamic.ToByteArray();

            Assert.AreEqual(originalBytes, roundtripBytes,
                "Roundtrip serialization of repeated nested messages should produce identical bytes");
        }

        [Test]
        public void Roundtrip_MapFields_BytesMatch()
        {
            var generated = new TestMap();
            generated.MapInt32Int32.Add(1, 100);
            generated.MapInt32Int32.Add(2, 200);
            generated.MapStringString.Add("key1", "value1");
            generated.MapStringString.Add("key2", "value2");
            generated.MapInt32ForeignMessage.Add(1, new ForeignMessage { C = 10 });
            generated.MapInt32ForeignMessage.Add(2, new ForeignMessage { C = 20 });

            var originalBytes = generated.ToByteArray();
            var dynamic = DynamicMessage.ParseFrom(DynamicTestMap, originalBytes);
            var roundtripBytes = dynamic.ToByteArray();

            Assert.AreEqual(originalBytes, roundtripBytes,
                "Roundtrip serialization of map fields should produce identical bytes");
        }

        [Test]
        public void Roundtrip_Oneof_BytesMatch()
        {
            // Test with int value
            var generated1 = new TestAllTypes { OneofUint32 = 12345 };
            var originalBytes1 = generated1.ToByteArray();
            var dynamic1 = DynamicMessage.ParseFrom(DynamicTestAllTypes, originalBytes1);
            var roundtripBytes1 = dynamic1.ToByteArray();
            Assert.AreEqual(originalBytes1, roundtripBytes1, "Oneof uint32 roundtrip should produce identical bytes");

            // Test with string value
            var generated2 = new TestAllTypes { OneofString = "test string" };
            var originalBytes2 = generated2.ToByteArray();
            var dynamic2 = DynamicMessage.ParseFrom(DynamicTestAllTypes, originalBytes2);
            var roundtripBytes2 = dynamic2.ToByteArray();
            Assert.AreEqual(originalBytes2, roundtripBytes2, "Oneof string roundtrip should produce identical bytes");

            // Test with nested message
            var generated3 = new TestAllTypes
            {
                OneofNestedMessage = new TestAllTypes.Types.NestedMessage { Bb = 999 }
            };
            var originalBytes3 = generated3.ToByteArray();
            var dynamic3 = DynamicMessage.ParseFrom(DynamicTestAllTypes, originalBytes3);
            var roundtripBytes3 = dynamic3.ToByteArray();
            Assert.AreEqual(originalBytes3, roundtripBytes3, "Oneof nested message roundtrip should produce identical bytes");
        }

        [Test]
        public void Roundtrip_ComplexMessage_BytesMatch()
        {
            // Create a complex message with many field types
            var generated = new TestAllTypes
            {
                SingleInt32 = int.MaxValue,
                SingleInt64 = long.MinValue,
                SingleUint32 = uint.MaxValue,
                SingleUint64 = ulong.MaxValue,
                SingleSint32 = int.MinValue,
                SingleSint64 = long.MaxValue,
                SingleFixed32 = 0xDEADBEEF,
                SingleFixed64 = 0xCAFEBABEDEADBEEF,
                SingleSfixed32 = -0x7FFFFFFF,
                SingleSfixed64 = -0x7FFFFFFFFFFFFFFF,
                SingleFloat = float.MaxValue,
                SingleDouble = double.MinValue,
                SingleBool = true,
                SingleString = "Complex test with unicode: \u4e2d\u6587",
                SingleBytes = ByteString.CopyFrom(new byte[] { 0x00, 0xFF, 0x01, 0xFE }),
                SingleNestedMessage = new TestAllTypes.Types.NestedMessage { Bb = 42 },
                SingleNestedEnum = TestAllTypes.Types.NestedEnum.Baz,
                OneofUint32 = 999
            };

            generated.RepeatedInt32.AddRange(new[] { 1, -1, int.MaxValue, int.MinValue });
            generated.RepeatedString.AddRange(new[] { "", "a", "longer string here" });
            generated.RepeatedNestedMessage.Add(new TestAllTypes.Types.NestedMessage { Bb = 1 });
            generated.RepeatedNestedMessage.Add(new TestAllTypes.Types.NestedMessage { Bb = 2 });

            var originalBytes = generated.ToByteArray();
            var dynamic = DynamicMessage.ParseFrom(DynamicTestAllTypes, originalBytes);
            var roundtripBytes = dynamic.ToByteArray();

            Assert.AreEqual(originalBytes, roundtripBytes,
                "Roundtrip serialization of complex message should produce identical bytes");

            // Also verify we can parse the roundtrip bytes back to a generated message
            var reparsed = TestAllTypes.Parser.ParseFrom(roundtripBytes);
            Assert.AreEqual(generated, reparsed, "Reparsed message should equal original");
        }

        [Test]
        public void Roundtrip_SpecialFloatValues_BytesMatch()
        {
            // Test special float/double values
            var generated = new TestAllTypes
            {
                SingleFloat = float.NaN,
                SingleDouble = double.PositiveInfinity
            };

            var originalBytes = generated.ToByteArray();
            var dynamic = DynamicMessage.ParseFrom(DynamicTestAllTypes, originalBytes);
            var roundtripBytes = dynamic.ToByteArray();

            Assert.AreEqual(originalBytes, roundtripBytes,
                "Roundtrip serialization of special float values should produce identical bytes");

            // Test negative infinity
            generated = new TestAllTypes
            {
                SingleFloat = float.NegativeInfinity,
                SingleDouble = double.NaN
            };

            originalBytes = generated.ToByteArray();
            dynamic = DynamicMessage.ParseFrom(DynamicTestAllTypes, originalBytes);
            roundtripBytes = dynamic.ToByteArray();

            Assert.AreEqual(originalBytes, roundtripBytes,
                "Roundtrip serialization of NaN and negative infinity should produce identical bytes");
        }

        [Test]
        public void Roundtrip_EmptyMessage_BytesMatch()
        {
            var generated = new TestAllTypes();

            var originalBytes = generated.ToByteArray();
            var dynamic = DynamicMessage.ParseFrom(DynamicTestAllTypes, originalBytes);
            var roundtripBytes = dynamic.ToByteArray();

            Assert.AreEqual(originalBytes, roundtripBytes,
                "Roundtrip serialization of empty message should produce identical bytes");
            Assert.AreEqual(0, originalBytes.Length, "Empty message should serialize to zero bytes");
        }

        [Test]
        public void Roundtrip_EmptyStringsAndBytes_BytesMatch()
        {
            var generated = new TestAllTypes
            {
                SingleString = "",
                SingleBytes = ByteString.Empty
            };

            // Empty strings/bytes are default values in proto3, so they won't be serialized
            var originalBytes = generated.ToByteArray();
            var dynamic = DynamicMessage.ParseFrom(DynamicTestAllTypes, originalBytes);
            var roundtripBytes = dynamic.ToByteArray();

            Assert.AreEqual(originalBytes, roundtripBytes,
                "Roundtrip serialization with empty strings/bytes should produce identical bytes");
        }

        [Test]
        public void Roundtrip_MultipleRoundtrips_BytesRemainStable()
        {
            // Ensure bytes remain stable across multiple roundtrips
            var generated = new TestAllTypes
            {
                SingleInt32 = 42,
                SingleString = "test"
            };
            generated.RepeatedInt32.AddRange(new[] { 1, 2, 3 });

            var bytes = generated.ToByteArray();

            // Do multiple roundtrips
            for (int i = 0; i < 5; i++)
            {
                var dynamic = DynamicMessage.ParseFrom(DynamicTestAllTypes, bytes);
                bytes = dynamic.ToByteArray();
            }

            // Should still match original
            Assert.AreEqual(generated.ToByteArray(), bytes,
                "Bytes should remain stable after multiple roundtrips");
        }

        #endregion

        #region Public API coverage tests

        [Test]
        public void ParseFrom_ByteString()
        {
            var generated = new TestAllTypes { SingleInt32 = 42, SingleString = "hello" };
            var byteString = ByteString.CopyFrom(generated.ToByteArray());

            var parsed = DynamicMessage.ParseFrom(DynamicTestAllTypes, byteString);

            Assert.AreEqual(42, parsed.GetField(DynamicTestAllTypes.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber)));
            Assert.AreEqual("hello", parsed.GetField(DynamicTestAllTypes.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber)));
        }

        [Test]
        public void ParseFrom_ByteString_NullDescriptor_Throws()
        {
            var byteString = ByteString.Empty;
            Assert.Throws<ArgumentNullException>(() => DynamicMessage.ParseFrom(null, byteString));
        }

        [Test]
        public void ParseFrom_ByteString_NullData_Throws()
        {
            Assert.Throws<ArgumentNullException>(() => DynamicMessage.ParseFrom(DynamicTestAllTypes, (ByteString)null));
        }

        [Test]
        public void ParseFrom_ByteArray_NullDescriptor_Throws()
        {
            var bytes = new byte[0];
            Assert.Throws<ArgumentNullException>(() => DynamicMessage.ParseFrom(null, bytes));
        }

        [Test]
        public void ParseFrom_ByteArray_NullData_Throws()
        {
            Assert.Throws<ArgumentNullException>(() => DynamicMessage.ParseFrom(DynamicTestAllTypes, (byte[])null));
        }

        [Test]
        public void ParseFrom_CodedInputStream()
        {
            var generated = new TestAllTypes { SingleInt32 = 99, SingleBool = true };
            var bytes = generated.ToByteArray();
            var input = new CodedInputStream(bytes);

            var parsed = DynamicMessage.ParseFrom(DynamicTestAllTypes, input);

            Assert.AreEqual(99, parsed.GetField(DynamicTestAllTypes.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber)));
            Assert.AreEqual(true, parsed.GetField(DynamicTestAllTypes.FindFieldByNumber(TestAllTypes.SingleBoolFieldNumber)));
        }

        [Test]
        public void ParseFrom_CodedInputStream_NullDescriptor_Throws()
        {
            var input = new CodedInputStream(new byte[0]);
            Assert.Throws<ArgumentNullException>(() => DynamicMessage.ParseFrom(null, input));
        }

        [Test]
        public void ParseFrom_CodedInputStream_NullInput_Throws()
        {
            Assert.Throws<ArgumentNullException>(() => DynamicMessage.ParseFrom(DynamicTestAllTypes, (CodedInputStream)null));
        }

        [Test]
        public void MergeFrom_ByteArray()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 1);

            var generated = new TestAllTypes { SingleString = "merged" };
            message.MergeFrom(generated.ToByteArray());

            Assert.AreEqual(1, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber)));
            Assert.AreEqual("merged", message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber)));
        }

        [Test]
        public void MergeFrom_ByteArray_NullData_Throws()
        {
            var message = new DynamicMessage(DynamicTestAllTypes);
            Assert.Throws<ArgumentNullException>(() => message.MergeFrom((byte[])null));
        }

        [Test]
        public void MergeFrom_CodedInputStream()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleBoolFieldNumber), true);

            var generated = new TestAllTypes { SingleInt64 = 12345L };
            var input = new CodedInputStream(generated.ToByteArray());
            message.MergeFrom(input);

            Assert.AreEqual(true, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleBoolFieldNumber)));
            Assert.AreEqual(12345L, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt64FieldNumber)));
        }

        [Test]
        public void MergeFrom_CodedInputStream_NullInput_Throws()
        {
            var message = new DynamicMessage(DynamicTestAllTypes);
            Assert.Throws<ArgumentNullException>(() => message.MergeFrom((CodedInputStream)null));
        }

        [Test]
        public void MergeFrom_DifferentDescriptor_Throws()
        {
            var message1 = new DynamicMessage(DynamicTestAllTypes);
            var message2 = new DynamicMessage(DynamicTestOneof);

            Assert.Throws<ArgumentException>(() => message1.MergeFrom(message2));
        }

        [Test]
        public void MergeFrom_Null_NoOp()
        {
            var message = new DynamicMessage(DynamicTestAllTypes);
            message.SetField(DynamicTestAllTypes.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 42);

            message.MergeFrom((DynamicMessage)null);

            // Should be unchanged
            Assert.AreEqual(42, message.GetField(DynamicTestAllTypes.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber)));
        }

        [Test]
        public void WriteTo_CodedOutputStream()
        {
            var message = new DynamicMessage(DynamicTestAllTypes);
            message.SetField(DynamicTestAllTypes.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 42);
            message.SetField(DynamicTestAllTypes.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber), "hello");

            var stream = new System.IO.MemoryStream();
            var output = new CodedOutputStream(stream);
            message.WriteTo(output);
            output.Flush();

            var bytes = stream.ToArray();

            // Parse back and verify
            var parsed = TestAllTypes.Parser.ParseFrom(bytes);
            Assert.AreEqual(42, parsed.SingleInt32);
            Assert.AreEqual("hello", parsed.SingleString);
        }

        [Test]
        public void CreateParser_ReturnsWorkingParser()
        {
            var parser = DynamicMessage.CreateParser(DynamicTestAllTypes);
            Assert.NotNull(parser);

            var generated = new TestAllTypes { SingleInt32 = 123 };
            var parsed = parser.ParseFrom(generated.ToByteArray());

            Assert.IsInstanceOf<DynamicMessage>(parsed);
            Assert.AreEqual(123, parsed.GetField(DynamicTestAllTypes.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber)));
        }

        [Test]
        public void Constructor_NullDescriptor_Throws()
        {
            Assert.Throws<ArgumentNullException>(() => new DynamicMessage(null));
        }

        #endregion

        #region All numeric types tests

        [Test]
        public void SetField_AllIntegerTypes()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            // int32 variants
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), -42);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleSint32FieldNumber), -100);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleSfixed32FieldNumber), -200);

            // uint32 variants
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleUint32FieldNumber), 42u);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleFixed32FieldNumber), 100u);

            // int64 variants
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt64FieldNumber), -123456789L);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleSint64FieldNumber), -999999999L);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleSfixed64FieldNumber), -111111111L);

            // uint64 variants
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleUint64FieldNumber), 123456789UL);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleFixed64FieldNumber), 999999999UL);

            // Verify all values
            Assert.AreEqual(-42, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber)));
            Assert.AreEqual(-100, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleSint32FieldNumber)));
            Assert.AreEqual(-200, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleSfixed32FieldNumber)));
            Assert.AreEqual(42u, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleUint32FieldNumber)));
            Assert.AreEqual(100u, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleFixed32FieldNumber)));
            Assert.AreEqual(-123456789L, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt64FieldNumber)));
            Assert.AreEqual(-999999999L, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleSint64FieldNumber)));
            Assert.AreEqual(-111111111L, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleSfixed64FieldNumber)));
            Assert.AreEqual(123456789UL, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleUint64FieldNumber)));
            Assert.AreEqual(999999999UL, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleFixed64FieldNumber)));
        }

        [Test]
        public void SetField_AllIntegerTypes_RoundTrip()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), int.MinValue);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleSint32FieldNumber), int.MaxValue);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleSfixed32FieldNumber), -1);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleUint32FieldNumber), uint.MaxValue);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleFixed32FieldNumber), uint.MinValue);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt64FieldNumber), long.MinValue);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleSint64FieldNumber), long.MaxValue);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleSfixed64FieldNumber), -1L);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleUint64FieldNumber), ulong.MaxValue);
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleFixed64FieldNumber), ulong.MinValue);

            var bytes = message.ToByteArray();
            var parsed = DynamicMessage.ParseFrom(descriptor, bytes);

            Assert.AreEqual(int.MinValue, parsed.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber)));
            Assert.AreEqual(int.MaxValue, parsed.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleSint32FieldNumber)));
            Assert.AreEqual(-1, parsed.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleSfixed32FieldNumber)));
            Assert.AreEqual(uint.MaxValue, parsed.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleUint32FieldNumber)));
            Assert.AreEqual(uint.MinValue, parsed.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleFixed32FieldNumber)));
            Assert.AreEqual(long.MinValue, parsed.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt64FieldNumber)));
            Assert.AreEqual(long.MaxValue, parsed.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleSint64FieldNumber)));
            Assert.AreEqual(-1L, parsed.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleSfixed64FieldNumber)));
            Assert.AreEqual(ulong.MaxValue, parsed.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleUint64FieldNumber)));
            Assert.AreEqual(ulong.MinValue, parsed.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleFixed64FieldNumber)));
        }

        [Test]
        public void SetField_FloatAndDouble_SpecialValues()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            // Test special float values
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleFloatFieldNumber), float.PositiveInfinity);
            Assert.AreEqual(float.PositiveInfinity, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleFloatFieldNumber)));

            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleFloatFieldNumber), float.NegativeInfinity);
            Assert.AreEqual(float.NegativeInfinity, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleFloatFieldNumber)));

            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleFloatFieldNumber), float.MinValue);
            Assert.AreEqual(float.MinValue, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleFloatFieldNumber)));

            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleFloatFieldNumber), float.MaxValue);
            Assert.AreEqual(float.MaxValue, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleFloatFieldNumber)));

            // Test special double values
            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleDoubleFieldNumber), double.PositiveInfinity);
            Assert.AreEqual(double.PositiveInfinity, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleDoubleFieldNumber)));

            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleDoubleFieldNumber), double.NegativeInfinity);
            Assert.AreEqual(double.NegativeInfinity, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleDoubleFieldNumber)));

            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleDoubleFieldNumber), double.MinValue);
            Assert.AreEqual(double.MinValue, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleDoubleFieldNumber)));

            message.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleDoubleFieldNumber), double.MaxValue);
            Assert.AreEqual(double.MaxValue, message.GetField(descriptor.FindFieldByNumber(TestAllTypes.SingleDoubleFieldNumber)));
        }

        #endregion

        #region Map field tests

        [Test]
        public void MapField_SetAndGet()
        {
            var descriptor = DynamicTestMap;
            var message = new DynamicMessage(descriptor);

            var mapField = descriptor.FindFieldByNumber(TestMap.MapInt32Int32FieldNumber);
            var mapValue = new Dictionary<object, object>
            {
                { 1, 100 },
                { 2, 200 },
                { 3, 300 }
            };

            message.SetField(mapField, mapValue);
            var result = (IDictionary)message.GetField(mapField);

            Assert.AreEqual(3, result.Count);
            Assert.AreEqual(100, result[1]);
            Assert.AreEqual(200, result[2]);
            Assert.AreEqual(300, result[3]);
        }

        [Test]
        public void MapField_StringKeys()
        {
            var descriptor = DynamicTestMap;
            var message = new DynamicMessage(descriptor);

            var mapField = descriptor.FindFieldByNumber(TestMap.MapStringStringFieldNumber);
            var mapValue = new Dictionary<object, object>
            {
                { "key1", "value1" },
                { "key2", "value2" }
            };

            message.SetField(mapField, mapValue);
            var result = (IDictionary)message.GetField(mapField);

            Assert.AreEqual(2, result.Count);
            Assert.AreEqual("value1", result["key1"]);
            Assert.AreEqual("value2", result["key2"]);
        }

        [Test]
        public void MapField_MessageValues()
        {
            var descriptor = DynamicTestMap;
            var message = new DynamicMessage(descriptor);

            var mapField = descriptor.FindFieldByNumber(TestMap.MapInt32ForeignMessageFieldNumber);
            // Get the ForeignMessage descriptor from the map field's message type
            var foreignDescriptor = mapField.MessageType.Fields[2].MessageType;

            var nested1 = new DynamicMessage(foreignDescriptor);
            nested1.SetField(foreignDescriptor.FindFieldByNumber(ForeignMessage.CFieldNumber), 10);

            var nested2 = new DynamicMessage(foreignDescriptor);
            nested2.SetField(foreignDescriptor.FindFieldByNumber(ForeignMessage.CFieldNumber), 20);

            var mapValue = new Dictionary<object, object>
            {
                { 1, nested1 },
                { 2, nested2 }
            };

            message.SetField(mapField, mapValue);

            // Serialize and parse back
            var bytes = message.ToByteArray();
            var parsed = DynamicMessage.ParseFrom(descriptor, bytes);

            var resultMap = (IDictionary)parsed.GetField(mapField);
            Assert.AreEqual(2, resultMap.Count);
            Assert.AreEqual(10, ((DynamicMessage)resultMap[1]).GetField(foreignDescriptor.FindFieldByNumber(ForeignMessage.CFieldNumber)));
            Assert.AreEqual(20, ((DynamicMessage)resultMap[2]).GetField(foreignDescriptor.FindFieldByNumber(ForeignMessage.CFieldNumber)));
        }

        [Test]
        public void MapField_Empty_DefaultValue()
        {
            var descriptor = DynamicTestMap;
            var message = new DynamicMessage(descriptor);

            var mapField = descriptor.FindFieldByNumber(TestMap.MapInt32Int32FieldNumber);
            var result = (IDictionary)message.GetField(mapField);

            Assert.NotNull(result);
            Assert.AreEqual(0, result.Count);
        }

        [Test]
        public void MapField_Equality()
        {
            var descriptor = DynamicTestMap;
            var mapField = descriptor.FindFieldByNumber(TestMap.MapInt32Int32FieldNumber);

            var message1 = new DynamicMessage(descriptor);
            message1.SetField(mapField, new Dictionary<object, object> { { 1, 100 }, { 2, 200 } });

            var message2 = new DynamicMessage(descriptor);
            message2.SetField(mapField, new Dictionary<object, object> { { 1, 100 }, { 2, 200 } });

            Assert.AreEqual(message1, message2);
            Assert.AreEqual(message1.GetHashCode(), message2.GetHashCode());
        }

        [Test]
        public void MapField_Inequality()
        {
            var descriptor = DynamicTestMap;
            var mapField = descriptor.FindFieldByNumber(TestMap.MapInt32Int32FieldNumber);

            var message1 = new DynamicMessage(descriptor);
            message1.SetField(mapField, new Dictionary<object, object> { { 1, 100 } });

            var message2 = new DynamicMessage(descriptor);
            message2.SetField(mapField, new Dictionary<object, object> { { 1, 999 } });

            Assert.AreNotEqual(message1, message2);
        }

        [Test]
        public void MapField_Clone()
        {
            var descriptor = DynamicTestMap;
            var mapField = descriptor.FindFieldByNumber(TestMap.MapInt32Int32FieldNumber);

            var message = new DynamicMessage(descriptor);
            message.SetField(mapField, new Dictionary<object, object> { { 1, 100 }, { 2, 200 } });

            var clone = message.Clone();

            // Modify original
            var originalMap = (Dictionary<object, object>)message.GetField(mapField);
            originalMap[3] = 300;

            // Clone should be unaffected
            var clonedMap = (IDictionary)clone.GetField(mapField);
            Assert.AreEqual(2, clonedMap.Count);
            Assert.IsFalse(clonedMap.Contains(3));
        }

        [Test]
        public void MapField_MergeFrom()
        {
            var descriptor = DynamicTestMap;
            var mapField = descriptor.FindFieldByNumber(TestMap.MapInt32Int32FieldNumber);

            var message1 = new DynamicMessage(descriptor);
            message1.SetField(mapField, new Dictionary<object, object> { { 1, 100 }, { 2, 200 } });

            var message2 = new DynamicMessage(descriptor);
            message2.SetField(mapField, new Dictionary<object, object> { { 2, 999 }, { 3, 300 } });

            message1.MergeFrom(message2);

            var result = (IDictionary)message1.GetField(mapField);
            Assert.AreEqual(3, result.Count);
            Assert.AreEqual(100, result[1]);
            Assert.AreEqual(999, result[2]); // Overwritten by message2
            Assert.AreEqual(300, result[3]);
        }

        #endregion

        #region Error case tests

        [Test]
        public void SetRepeatedField_ThrowsForSingularField()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            var singularField = descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);

            Assert.Throws<ArgumentException>(() => message.SetRepeatedField(singularField, 0, 42));
        }

        [Test]
        public void SetRepeatedField_WrongType_Throws()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);
            message.AddRepeatedField(repeatedField, 1);

            Assert.Throws<ArgumentException>(() => message.SetRepeatedField(repeatedField, 0, "not an int"));
        }

        [Test]
        public void SetRepeatedField_NullValue_Throws()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedStringFieldNumber);
            message.AddRepeatedField(repeatedField, "initial");

            Assert.Throws<ArgumentNullException>(() => message.SetRepeatedField(repeatedField, 0, null));
        }

        [Test]
        public void SetRepeatedField_IndexOutOfRange_Throws()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);
            message.AddRepeatedField(repeatedField, 1);

            Assert.Throws<ArgumentOutOfRangeException>(() => message.SetRepeatedField(repeatedField, 5, 42));
        }

        [Test]
        public void SetRepeatedField_NoList_Throws()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);

            // Don't add any elements first
            Assert.Throws<ArgumentOutOfRangeException>(() => message.SetRepeatedField(repeatedField, 0, 42));
        }

        [Test]
        public void GetRepeatedField_IndexOutOfRange_Throws()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);
            message.AddRepeatedField(repeatedField, 1);

            Assert.Throws<ArgumentOutOfRangeException>(() => message.GetRepeatedField(repeatedField, 5));
        }

        [Test]
        public void GetRepeatedField_NoList_Throws()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);

            Assert.Throws<ArgumentOutOfRangeException>(() => message.GetRepeatedField(repeatedField, 0));
        }

        [Test]
        public void HasOneof_WrongDescriptor_Throws()
        {
            var message = new DynamicMessage(DynamicTestAllTypes);
            var wrongOneof = DynamicTestOneof.Oneofs[0];

            Assert.Throws<ArgumentException>(() => message.HasOneof(wrongOneof));
        }

        [Test]
        public void GetOneofFieldDescriptor_WrongDescriptor_Throws()
        {
            var message = new DynamicMessage(DynamicTestAllTypes);
            var wrongOneof = DynamicTestOneof.Oneofs[0];

            Assert.Throws<ArgumentException>(() => message.GetOneofFieldDescriptor(wrongOneof));
        }

        [Test]
        public void ClearOneof_WrongDescriptor_Throws()
        {
            var message = new DynamicMessage(DynamicTestAllTypes);
            var wrongOneof = DynamicTestOneof.Oneofs[0];

            Assert.Throws<ArgumentException>(() => message.ClearOneof(wrongOneof));
        }

        [Test]
        public void GetField_WrongDescriptor_Throws()
        {
            var message = new DynamicMessage(DynamicTestAllTypes);
            var wrongField = DynamicForeignMessage.FindFieldByNumber(ForeignMessage.CFieldNumber);

            Assert.Throws<ArgumentException>(() => message.GetField(wrongField));
        }

        [Test]
        public void ClearField_WrongDescriptor_Throws()
        {
            var message = new DynamicMessage(DynamicTestAllTypes);
            var wrongField = DynamicForeignMessage.FindFieldByNumber(ForeignMessage.CFieldNumber);

            Assert.Throws<ArgumentException>(() => message.ClearField(wrongField));
        }

        [Test]
        public void HasField_WrongDescriptor_Throws()
        {
            var message = new DynamicMessage(DynamicTestAllTypes);
            var wrongField = DynamicForeignMessage.FindFieldByNumber(ForeignMessage.CFieldNumber);

            Assert.Throws<ArgumentException>(() => message.HasField(wrongField));
        }

        [Test]
        public void GetRepeatedFieldCount_WrongDescriptor_Throws()
        {
            var message = new DynamicMessage(DynamicTestAllTypes);
            var wrongField = DynamicForeignMessage.FindFieldByNumber(ForeignMessage.CFieldNumber);

            Assert.Throws<ArgumentException>(() => message.GetRepeatedFieldCount(wrongField));
        }

        [Test]
        public void AddRepeatedField_WrongDescriptor_Throws()
        {
            var message = new DynamicMessage(DynamicTestAllTypes);
            // Use a field from a different descriptor
            var wrongField = DynamicForeignMessage.FindFieldByNumber(ForeignMessage.CFieldNumber);

            Assert.Throws<ArgumentException>(() => message.AddRepeatedField(wrongField, "value"));
        }

        #endregion

        #region Equality edge cases

        [Test]
        public void Equals_Null_ReturnsFalse()
        {
            var message = new DynamicMessage(DynamicTestAllTypes);
            Assert.IsFalse(message.Equals(null));
            Assert.IsFalse(message.Equals((object)null));
        }

        [Test]
        public void Equals_SameInstance_ReturnsTrue()
        {
            var message = new DynamicMessage(DynamicTestAllTypes);
            message.SetField(DynamicTestAllTypes.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 42);

            Assert.IsTrue(message.Equals(message));
            Assert.IsTrue(message.Equals((object)message));
        }

        [Test]
        public void Equals_DifferentDescriptor_ReturnsFalse()
        {
            var message1 = new DynamicMessage(DynamicTestAllTypes);
            var message2 = new DynamicMessage(DynamicTestOneof);

            Assert.IsFalse(message1.Equals(message2));
        }

        [Test]
        public void Equals_DifferentFieldCount_ReturnsFalse()
        {
            var descriptor = DynamicTestAllTypes;
            var message1 = new DynamicMessage(descriptor);
            message1.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 42);

            var message2 = new DynamicMessage(descriptor);
            message2.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 42);
            message2.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber), "extra");

            Assert.IsFalse(message1.Equals(message2));
        }

        [Test]
        public void Equals_DifferentFieldNumbers_ReturnsFalse()
        {
            var descriptor = DynamicTestAllTypes;
            var message1 = new DynamicMessage(descriptor);
            message1.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 42);

            var message2 = new DynamicMessage(descriptor);
            message2.SetField(descriptor.FindFieldByNumber(TestAllTypes.SingleSint32FieldNumber), 42);

            Assert.IsFalse(message1.Equals(message2));
        }

        [Test]
        public void Equals_WrongObjectType_ReturnsFalse()
        {
            var message = new DynamicMessage(DynamicTestAllTypes);
            Assert.IsFalse(message.Equals("not a message"));
            Assert.IsFalse(message.Equals(42));
        }

        [Test]
        public void Equals_WithNestedMessages()
        {
            var descriptor = DynamicTestAllTypes;
            var nestedField = descriptor.FindFieldByNumber(TestAllTypes.SingleNestedMessageFieldNumber);
            var nestedDescriptor = nestedField.MessageType;
            var bbField = nestedDescriptor.FindFieldByNumber(TestAllTypes.Types.NestedMessage.BbFieldNumber);

            var message1 = new DynamicMessage(descriptor);
            var nested1 = new DynamicMessage(nestedDescriptor);
            nested1.SetField(bbField, 42);
            message1.SetField(nestedField, nested1);

            var message2 = new DynamicMessage(descriptor);
            var nested2 = new DynamicMessage(nestedDescriptor);
            nested2.SetField(bbField, 42);
            message2.SetField(nestedField, nested2);

            Assert.AreEqual(message1, message2);
            Assert.AreEqual(message1.GetHashCode(), message2.GetHashCode());

            // Change nested message in message2
            nested2.SetField(bbField, 99);
            message2.SetField(nestedField, nested2);
            Assert.AreNotEqual(message1, message2);
        }

        [Test]
        public void Equals_WithRepeatedFields()
        {
            var descriptor = DynamicTestAllTypes;
            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);

            var message1 = new DynamicMessage(descriptor);
            message1.AddRepeatedField(repeatedField, 1);
            message1.AddRepeatedField(repeatedField, 2);

            var message2 = new DynamicMessage(descriptor);
            message2.AddRepeatedField(repeatedField, 1);
            message2.AddRepeatedField(repeatedField, 2);

            Assert.AreEqual(message1, message2);
            Assert.AreEqual(message1.GetHashCode(), message2.GetHashCode());

            // Different order
            var message3 = new DynamicMessage(descriptor);
            message3.AddRepeatedField(repeatedField, 2);
            message3.AddRepeatedField(repeatedField, 1);

            Assert.AreNotEqual(message1, message3);

            // Different count
            var message4 = new DynamicMessage(descriptor);
            message4.AddRepeatedField(repeatedField, 1);

            Assert.AreNotEqual(message1, message4);
        }

        [Test]
        public void Equals_ByteStringField()
        {
            var descriptor = DynamicTestAllTypes;
            var bytesField = descriptor.FindFieldByNumber(TestAllTypes.SingleBytesFieldNumber);

            var message1 = new DynamicMessage(descriptor);
            message1.SetField(bytesField, ByteString.CopyFromUtf8("hello"));

            var message2 = new DynamicMessage(descriptor);
            message2.SetField(bytesField, ByteString.CopyFromUtf8("hello"));

            Assert.AreEqual(message1, message2);

            var message3 = new DynamicMessage(descriptor);
            message3.SetField(bytesField, ByteString.CopyFromUtf8("world"));

            Assert.AreNotEqual(message1, message3);
        }

        #endregion

        #region UnknownFields tests

        [Test]
        public void UnknownFields_InitiallyNull()
        {
            var message = new DynamicMessage(DynamicTestAllTypes);
            Assert.IsNull(message.UnknownFields);
        }

        [Test]
        public void UnknownFields_PreservedOnRoundTrip()
        {
            // Create a message using a descriptor that doesn't know about some fields
            // by using one message type's descriptor to parse another's bytes

            // First create a TestAllTypes with some fields set
            var generated = new TestAllTypes { SingleInt32 = 42, SingleString = "hello" };
            var bytes = generated.ToByteArray();

            // Parse with correct descriptor
            var parsed = DynamicMessage.ParseFrom(DynamicTestAllTypes, bytes);

            // Known fields should be present
            Assert.AreEqual(42, parsed.GetField(DynamicTestAllTypes.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber)));
            Assert.AreEqual("hello", parsed.GetField(DynamicTestAllTypes.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber)));

            // Re-serialize should produce same bytes
            Assert.AreEqual(bytes, parsed.ToByteArray());
        }

        [Test]
        public void UnknownFields_ClonePreserves()
        {
            var generated = new TestAllTypes { SingleInt32 = 42 };
            var bytes = generated.ToByteArray();

            var parsed = DynamicMessage.ParseFrom(DynamicTestAllTypes, bytes);
            var clone = parsed.Clone();

            // Clone should serialize identically
            Assert.AreEqual(parsed.ToByteArray(), clone.ToByteArray());
        }

        [Test]
        public void UnknownFields_EqualityConsidersUnknownFields()
        {
            var generated = new TestAllTypes { SingleInt32 = 42 };
            var bytes = generated.ToByteArray();

            var message1 = DynamicMessage.ParseFrom(DynamicTestAllTypes, bytes);
            var message2 = DynamicMessage.ParseFrom(DynamicTestAllTypes, bytes);

            Assert.AreEqual(message1, message2);
            Assert.AreEqual(message1.GetHashCode(), message2.GetHashCode());
        }

        [Test]
        public void UnknownFields_MergeFromPreserves()
        {
            var generated = new TestAllTypes { SingleInt32 = 42 };
            var bytes = generated.ToByteArray();

            var message1 = DynamicMessage.ParseFrom(DynamicTestAllTypes, bytes);
            var message2 = new DynamicMessage(DynamicTestAllTypes);
            message2.SetField(DynamicTestAllTypes.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber), "added");

            message2.MergeFrom(message1);

            // Should have both fields
            Assert.AreEqual(42, message2.GetField(DynamicTestAllTypes.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber)));
            Assert.AreEqual("added", message2.GetField(DynamicTestAllTypes.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber)));
        }

        #endregion

        #region ClearField tests

        [Test]
        public void ClearField_SingularField()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            var int32Field = descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);

            message.SetField(int32Field, 42);
            Assert.IsTrue(message.HasField(int32Field));

            message.ClearField(int32Field);
            Assert.IsFalse(message.HasField(int32Field));
            Assert.AreEqual(0, message.GetField(int32Field)); // Returns default
        }

        [Test]
        public void ClearField_RepeatedField()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);

            message.AddRepeatedField(repeatedField, 1);
            message.AddRepeatedField(repeatedField, 2);
            Assert.AreEqual(2, message.GetRepeatedFieldCount(repeatedField));

            message.ClearField(repeatedField);
            Assert.AreEqual(0, message.GetRepeatedFieldCount(repeatedField));
        }

        [Test]
        public void ClearField_OneofField()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            var oneofDescriptor = descriptor.Oneofs.FirstOrDefault(o => o.Name == "oneof_field");
            var oneofStringField = descriptor.FindFieldByNumber(TestAllTypes.OneofStringFieldNumber);

            message.SetField(oneofStringField, "test");
            Assert.IsTrue(message.HasOneof(oneofDescriptor));
            Assert.AreEqual(oneofStringField, message.GetOneofFieldDescriptor(oneofDescriptor));

            message.ClearField(oneofStringField);
            Assert.IsFalse(message.HasOneof(oneofDescriptor));
            Assert.IsNull(message.GetOneofFieldDescriptor(oneofDescriptor));
        }

        [Test]
        public void ClearField_MapField()
        {
            var descriptor = DynamicTestMap;
            var message = new DynamicMessage(descriptor);
            var mapField = descriptor.FindFieldByNumber(TestMap.MapInt32Int32FieldNumber);

            message.SetField(mapField, new Dictionary<object, object> { { 1, 100 } });
            Assert.AreEqual(1, ((IDictionary)message.GetField(mapField)).Count);

            message.ClearField(mapField);
            var result = message.GetAllFields();
            Assert.IsFalse(result.ContainsKey(mapField));
        }

        #endregion

        #region Descriptor property test

        [Test]
        public void Descriptor_ReturnsCorrectDescriptor()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            Assert.AreSame(descriptor, message.Descriptor);
        }

        #endregion

        #region GetAllFields tests

        [Test]
        public void GetAllFields_EmptyMessage_ReturnsEmptyDictionary()
        {
            var message = new DynamicMessage(DynamicTestAllTypes);
            var allFields = message.GetAllFields();

            Assert.NotNull(allFields);
            Assert.AreEqual(0, allFields.Count);
        }

        [Test]
        public void GetAllFields_WithRepeatedField()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);
            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);

            message.AddRepeatedField(repeatedField, 1);
            message.AddRepeatedField(repeatedField, 2);

            var allFields = message.GetAllFields();

            Assert.AreEqual(1, allFields.Count);
            Assert.IsTrue(allFields.ContainsKey(repeatedField));
            var list = (IList)allFields[repeatedField];
            Assert.AreEqual(2, list.Count);
        }

        [Test]
        public void GetAllFields_WithMapField()
        {
            var descriptor = DynamicTestMap;
            var message = new DynamicMessage(descriptor);
            var mapField = descriptor.FindFieldByNumber(TestMap.MapInt32Int32FieldNumber);

            message.SetField(mapField, new Dictionary<object, object> { { 1, 100 }, { 2, 200 } });

            var allFields = message.GetAllFields();

            Assert.AreEqual(1, allFields.Count);
            Assert.IsTrue(allFields.ContainsKey(mapField));
            var dict = (IDictionary)allFields[mapField];
            Assert.AreEqual(2, dict.Count);
        }

        [Test]
        public void GetAllFields_DoesNotIncludeDefaultValues()
        {
            var descriptor = DynamicTestAllTypes;
            var message = new DynamicMessage(descriptor);

            // Set a field to its default value
            var int32Field = descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);
            message.SetField(int32Field, 0);

            var allFields = message.GetAllFields();

            // Should not include the field since it's default for proto3
            Assert.IsFalse(allFields.ContainsKey(int32Field));
        }

        #endregion

        #region DynamicFieldAccessor tests

        [Test]
        public void FieldAccessor_GetValue_SingularField()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var descriptor = converted[2].FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));
            var message = new DynamicMessage(descriptor);

            var int32Field = descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);
            message.SetField(int32Field, 42);

            var accessor = int32Field.Accessor;
            Assert.AreEqual(42, accessor.GetValue(message));
        }

        [Test]
        public void FieldAccessor_GetValue_UnsetField_ReturnsDefault()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var descriptor = converted[2].FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));
            var message = new DynamicMessage(descriptor);

            var int32Field = descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);
            var stringField = descriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber);

            Assert.AreEqual(0, int32Field.Accessor.GetValue(message));
            Assert.AreEqual("", stringField.Accessor.GetValue(message));
        }

        [Test]
        public void FieldAccessor_SetValue_SingularField()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var descriptor = converted[2].FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));
            var message = new DynamicMessage(descriptor);

            var int32Field = descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);
            var accessor = int32Field.Accessor;

            accessor.SetValue(message, 99);

            Assert.AreEqual(99, message.GetField(int32Field));
        }

        [Test]
        public void FieldAccessor_Clear_SingularField()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var descriptor = converted[2].FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));
            var message = new DynamicMessage(descriptor);

            var int32Field = descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);
            message.SetField(int32Field, 42);

            var accessor = int32Field.Accessor;
            accessor.Clear(message);

            Assert.IsFalse(message.HasField(int32Field));
            Assert.AreEqual(0, message.GetField(int32Field));
        }

        [Test]
        public void FieldAccessor_HasValue_SingularField()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var descriptor = converted[2].FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));
            var message = new DynamicMessage(descriptor);

            var int32Field = descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);
            var accessor = int32Field.Accessor;

            Assert.IsFalse(accessor.HasValue(message));

            message.SetField(int32Field, 42);
            Assert.IsTrue(accessor.HasValue(message));

            message.ClearField(int32Field);
            Assert.IsFalse(accessor.HasValue(message));
        }

        [Test]
        public void FieldAccessor_RepeatedField_GetValue()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var descriptor = converted[2].FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));
            var message = new DynamicMessage(descriptor);

            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);
            message.AddRepeatedField(repeatedField, 1);
            message.AddRepeatedField(repeatedField, 2);
            message.AddRepeatedField(repeatedField, 3);

            var accessor = repeatedField.Accessor;
            var value = (IList)accessor.GetValue(message);

            Assert.AreEqual(3, value.Count);
            Assert.AreEqual(1, value[0]);
            Assert.AreEqual(2, value[1]);
            Assert.AreEqual(3, value[2]);
        }

        [Test]
        public void FieldAccessor_RepeatedField_SetValue()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var descriptor = converted[2].FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));
            var message = new DynamicMessage(descriptor);

            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);

            // Add some initial values
            message.AddRepeatedField(repeatedField, 1);

            // SetValue should replace the entire list
            var accessor = repeatedField.Accessor;
            accessor.SetValue(message, new List<object> { 10, 20, 30 });

            Assert.AreEqual(3, message.GetRepeatedFieldCount(repeatedField));
            Assert.AreEqual(10, message.GetRepeatedField(repeatedField, 0));
            Assert.AreEqual(20, message.GetRepeatedField(repeatedField, 1));
            Assert.AreEqual(30, message.GetRepeatedField(repeatedField, 2));
        }

        [Test]
        public void FieldAccessor_RepeatedField_Clear()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var descriptor = converted[2].FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));
            var message = new DynamicMessage(descriptor);

            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);
            message.AddRepeatedField(repeatedField, 1);
            message.AddRepeatedField(repeatedField, 2);

            var accessor = repeatedField.Accessor;
            accessor.Clear(message);

            Assert.AreEqual(0, message.GetRepeatedFieldCount(repeatedField));
        }

        [Test]
        public void FieldAccessor_RepeatedField_HasValue()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var descriptor = converted[2].FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));
            var message = new DynamicMessage(descriptor);

            var repeatedField = descriptor.FindFieldByNumber(TestAllTypes.RepeatedInt32FieldNumber);
            var accessor = repeatedField.Accessor;

            // Empty repeated field
            Assert.IsFalse(accessor.HasValue(message));

            message.AddRepeatedField(repeatedField, 1);
            Assert.IsTrue(accessor.HasValue(message));

            message.ClearField(repeatedField);
            Assert.IsFalse(accessor.HasValue(message));
        }

        [Test]
        public void FieldAccessor_MapField_GetValue()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData,
                MapUnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var descriptor = converted[3].FindTypeByName<MessageDescriptor>(nameof(TestMap));
            var message = new DynamicMessage(descriptor);

            var mapField = descriptor.FindFieldByNumber(TestMap.MapInt32Int32FieldNumber);
            message.SetField(mapField, new Dictionary<object, object> { { 1, 100 }, { 2, 200 } });

            var accessor = mapField.Accessor;
            var value = (IDictionary)accessor.GetValue(message);

            Assert.AreEqual(2, value.Count);
            Assert.AreEqual(100, value[1]);
            Assert.AreEqual(200, value[2]);
        }

        [Test]
        public void FieldAccessor_MapField_SetValue()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData,
                MapUnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var descriptor = converted[3].FindTypeByName<MessageDescriptor>(nameof(TestMap));
            var message = new DynamicMessage(descriptor);

            var mapField = descriptor.FindFieldByNumber(TestMap.MapInt32Int32FieldNumber);

            // Set initial value
            message.SetField(mapField, new Dictionary<object, object> { { 1, 100 } });

            // SetValue should replace the entire map
            var accessor = mapField.Accessor;
            accessor.SetValue(message, new Dictionary<object, object> { { 10, 1000 }, { 20, 2000 } });

            var result = (IDictionary)message.GetField(mapField);
            Assert.AreEqual(2, result.Count);
            Assert.AreEqual(1000, result[10]);
            Assert.AreEqual(2000, result[20]);
            Assert.IsFalse(result.Contains(1));
        }

        [Test]
        public void FieldAccessor_MapField_Clear()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData,
                MapUnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var descriptor = converted[3].FindTypeByName<MessageDescriptor>(nameof(TestMap));
            var message = new DynamicMessage(descriptor);

            var mapField = descriptor.FindFieldByNumber(TestMap.MapInt32Int32FieldNumber);
            message.SetField(mapField, new Dictionary<object, object> { { 1, 100 } });

            var accessor = mapField.Accessor;
            accessor.Clear(message);

            Assert.IsFalse(message.GetAllFields().ContainsKey(mapField));
        }

        [Test]
        public void FieldAccessor_MapField_HasValue()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData,
                MapUnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var descriptor = converted[3].FindTypeByName<MessageDescriptor>(nameof(TestMap));
            var message = new DynamicMessage(descriptor);

            var mapField = descriptor.FindFieldByNumber(TestMap.MapInt32Int32FieldNumber);
            var accessor = mapField.Accessor;

            // Empty map
            Assert.IsFalse(accessor.HasValue(message));

            message.SetField(mapField, new Dictionary<object, object> { { 1, 100 } });
            Assert.IsTrue(accessor.HasValue(message));

            message.ClearField(mapField);
            Assert.IsFalse(accessor.HasValue(message));
        }

        [Test]
        public void FieldAccessor_Descriptor_Property()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var descriptor = converted[2].FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));
            var int32Field = descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);

            var accessor = int32Field.Accessor;
            Assert.AreSame(int32Field, accessor.Descriptor);
        }

        [Test]
        public void FieldAccessor_GetValue_NonDynamicMessage_Throws()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var descriptor = converted[2].FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));
            var int32Field = descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);

            var accessor = int32Field.Accessor;
            var generatedMessage = new TestAllTypes { SingleInt32 = 42 };

            // Using dynamic accessor with generated message should throw
            Assert.Throws<ArgumentException>(() => accessor.GetValue(generatedMessage));
        }

        [Test]
        public void FieldAccessor_SetValue_NonDynamicMessage_Throws()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var descriptor = converted[2].FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));
            var int32Field = descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);

            var accessor = int32Field.Accessor;
            var generatedMessage = new TestAllTypes();

            Assert.Throws<ArgumentException>(() => accessor.SetValue(generatedMessage, 42));
        }

        [Test]
        public void FieldAccessor_Clear_NonDynamicMessage_Throws()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var descriptor = converted[2].FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));
            var int32Field = descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);

            var accessor = int32Field.Accessor;
            var generatedMessage = new TestAllTypes();

            Assert.Throws<ArgumentException>(() => accessor.Clear(generatedMessage));
        }

        [Test]
        public void FieldAccessor_HasValue_NonDynamicMessage_Throws()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var descriptor = converted[2].FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));
            var int32Field = descriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber);

            var accessor = int32Field.Accessor;
            var generatedMessage = new TestAllTypes();

            Assert.Throws<ArgumentException>(() => accessor.HasValue(generatedMessage));
        }

        [Test]
        public void FieldAccessor_NestedMessage()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var descriptor = converted[2].FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));
            var nestedField = descriptor.FindFieldByNumber(TestAllTypes.SingleNestedMessageFieldNumber);
            var nestedDescriptor = nestedField.MessageType;
            var bbField = nestedDescriptor.FindFieldByNumber(TestAllTypes.Types.NestedMessage.BbFieldNumber);

            var message = new DynamicMessage(descriptor);
            var nested = new DynamicMessage(nestedDescriptor);
            nested.SetField(bbField, 42);

            var accessor = nestedField.Accessor;
            accessor.SetValue(message, nested);

            var result = (DynamicMessage)accessor.GetValue(message);
            Assert.AreEqual(42, result.GetField(bbField));
        }

        #endregion

        #region BuildFromByteStrings comprehensive tests

        [Test]
        public void BuildFromByteStrings_ReturnsCorrectNumberOfDescriptors()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            Assert.AreEqual(3, converted.Count);
        }

        [Test]
        public void BuildFromByteStrings_PreservesDependencyOrder()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            // The first file should have no dependencies from this list
            Assert.AreEqual(UnittestImportPublicProto3Reflection.Descriptor.Name, converted[0].Name);
            Assert.AreEqual(UnittestImportProto3Reflection.Descriptor.Name, converted[1].Name);
            Assert.AreEqual(UnittestProto3Reflection.Descriptor.Name, converted[2].Name);
        }

        [Test]
        public void BuildFromByteStrings_AllMessageTypesHaveParser()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            foreach (var file in converted)
            {
                foreach (var messageType in file.MessageTypes)
                {
                    Assert.NotNull(messageType.Parser, $"Parser should not be null for {messageType.FullName}");
                }
            }
        }

        [Test]
        public void BuildFromByteStrings_AllFieldsHaveAccessor()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            foreach (var file in converted)
            {
                foreach (var messageType in file.MessageTypes)
                {
                    foreach (var field in messageType.Fields.InDeclarationOrder())
                    {
                        Assert.NotNull(field.Accessor, $"Accessor should not be null for {field.FullName}");
                    }
                }
            }
        }

        [Test]
        public void BuildFromByteStrings_CanFindMessageTypes()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var testAllTypes = converted[2].FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));
            Assert.NotNull(testAllTypes);
            Assert.AreEqual("protobuf_unittest3.TestAllTypes", testAllTypes.FullName);
        }

        [Test]
        public void BuildFromByteStrings_CanFindEnumTypes()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var foreignEnum = converted[2].FindTypeByName<EnumDescriptor>("ForeignEnum");
            Assert.NotNull(foreignEnum);
        }

        [Test]
        public void BuildFromByteStrings_NestedTypesAccessible()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var testAllTypes = converted[2].FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));
            Assert.NotNull(testAllTypes);

            var nestedMessage = testAllTypes.NestedTypes.FirstOrDefault(t => t.Name == "NestedMessage");
            Assert.NotNull(nestedMessage);

            // Nested type should also have working parser
            Assert.NotNull(nestedMessage.Parser);
        }

        [Test]
        public void BuildFromByteStrings_ParseAndSerialize_RoundTrip()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var testAllTypes = converted[2].FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));

            // Create with generated code
            var generated = new TestAllTypes
            {
                SingleInt32 = 42,
                SingleString = "test",
                SingleBool = true
            };
            generated.RepeatedInt32.AddRange(new[] { 1, 2, 3 });

            var originalBytes = generated.ToByteArray();

            // Parse with dynamic descriptor
            var parsed = testAllTypes.Parser.ParseFrom(originalBytes);
            Assert.IsInstanceOf<DynamicMessage>(parsed);

            // Serialize back
            var roundtripBytes = parsed.ToByteArray();

            // Should be identical
            Assert.AreEqual(originalBytes, roundtripBytes);
        }

        [Test]
        public void BuildFromByteStrings_WithComplexTypes()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData,
                MapUnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var testMap = converted[3].FindTypeByName<MessageDescriptor>(nameof(TestMap));
            Assert.NotNull(testMap);

            // Find map field
            var mapField = testMap.FindFieldByNumber(TestMap.MapInt32Int32FieldNumber);
            Assert.NotNull(mapField);
            Assert.IsTrue(mapField.IsMap);
        }

        [Test]
        public void BuildFromByteStrings_InteroperabilityWithGeneratedMessages()
        {
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);

            var testAllTypesDescriptor = converted[2].FindTypeByName<MessageDescriptor>(nameof(TestAllTypes));

            // Create a DynamicMessage
            var dynamic = new DynamicMessage(testAllTypesDescriptor);
            dynamic.SetField(testAllTypesDescriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber), 42);
            dynamic.SetField(testAllTypesDescriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber), "dynamic");

            // Serialize
            var bytes = dynamic.ToByteArray();

            // Parse as generated message
            var generated = TestAllTypes.Parser.ParseFrom(bytes);
            Assert.AreEqual(42, generated.SingleInt32);
            Assert.AreEqual("dynamic", generated.SingleString);

            // Now go the other way
            generated.SingleBool = true;
            bytes = generated.ToByteArray();

            // Parse as DynamicMessage
            var parsed = (DynamicMessage)testAllTypesDescriptor.Parser.ParseFrom(bytes);
            Assert.AreEqual(42, parsed.GetField(testAllTypesDescriptor.FindFieldByNumber(TestAllTypes.SingleInt32FieldNumber)));
            Assert.AreEqual("dynamic", parsed.GetField(testAllTypesDescriptor.FindFieldByNumber(TestAllTypes.SingleStringFieldNumber)));
            Assert.AreEqual(true, parsed.GetField(testAllTypesDescriptor.FindFieldByNumber(TestAllTypes.SingleBoolFieldNumber)));
        }

        #endregion

        #region Extension support tests

        /// <summary>
        /// Helper method to find an extension field by number.
        /// Extensions are stored in FileDescriptor.Extensions, not in the message descriptor.
        /// </summary>
        private static FieldDescriptor FindExtensionByNumber(FileDescriptor fileDescriptor, MessageDescriptor extendeeType, int fieldNumber)
        {
            var extensions = fileDescriptor.Extensions.GetExtensionsInNumberOrder(extendeeType);
            return extensions.FirstOrDefault(e => e.FieldNumber == fieldNumber);
        }

        [Test]
        public void Extension_SetAndGetField()
        {
            // Get the descriptor with extensions (proto2 with extensions)
            var fileDescriptor = DynamicProto2UnittestFile;
            var testAllExtensions = fileDescriptor.FindTypeByName<MessageDescriptor>("TestAllExtensions");
            Assert.NotNull(testAllExtensions);

            // Find extension field by number (OptionalInt32Extension has field number 1)
            var extensionField = FindExtensionByNumber(fileDescriptor, testAllExtensions, 1);
            Assert.NotNull(extensionField, "OptionalInt32Extension not found");
            Assert.IsTrue(extensionField.IsExtension);

            // Create a DynamicMessage and set the extension field
            var message = new DynamicMessage(testAllExtensions);
            message.SetField(extensionField, 42);

            // Verify the value
            Assert.AreEqual(42, message.GetField(extensionField));
            Assert.IsTrue(message.HasField(extensionField));
        }

        [Test]
        public void Extension_ClearField()
        {
            var fileDescriptor = DynamicProto2UnittestFile;
            var testAllExtensions = fileDescriptor.FindTypeByName<MessageDescriptor>("TestAllExtensions");
            var extensionField = FindExtensionByNumber(fileDescriptor, testAllExtensions, 1); // OptionalInt32Extension

            var message = new DynamicMessage(testAllExtensions);
            message.SetField(extensionField, 42);
            Assert.IsTrue(message.HasField(extensionField));

            message.ClearField(extensionField);
            Assert.IsFalse(message.HasField(extensionField));
        }

        [Test]
        public void Extension_MultipleExtensions()
        {
            var fileDescriptor = DynamicProto2UnittestFile;
            var testAllExtensions = fileDescriptor.FindTypeByName<MessageDescriptor>("TestAllExtensions");

            // OptionalInt32Extension = 1, OptionalInt64Extension = 2, OptionalStringExtension = 14
            var int32Extension = FindExtensionByNumber(fileDescriptor, testAllExtensions, 1);
            var int64Extension = FindExtensionByNumber(fileDescriptor, testAllExtensions, 2);
            var stringExtension = FindExtensionByNumber(fileDescriptor, testAllExtensions, 14);

            Assert.NotNull(int32Extension, "int32Extension not found");
            Assert.NotNull(int64Extension, "int64Extension not found");
            Assert.NotNull(stringExtension, "stringExtension not found");
            Assert.IsTrue(int32Extension.IsExtension);
            Assert.IsTrue(int64Extension.IsExtension);
            Assert.IsTrue(stringExtension.IsExtension);

            var message = new DynamicMessage(testAllExtensions);
            message.SetField(int32Extension, 42);
            message.SetField(int64Extension, 123L);
            message.SetField(stringExtension, "hello");

            Assert.AreEqual(42, message.GetField(int32Extension));
            Assert.AreEqual(123L, message.GetField(int64Extension));
            Assert.AreEqual("hello", message.GetField(stringExtension));
        }

        [Test]
        public void Extension_GetAllFields()
        {
            var fileDescriptor = DynamicProto2UnittestFile;
            var testAllExtensions = fileDescriptor.FindTypeByName<MessageDescriptor>("TestAllExtensions");
            var extensionField = FindExtensionByNumber(fileDescriptor, testAllExtensions, 1); // OptionalInt32Extension

            var message = new DynamicMessage(testAllExtensions);
            message.SetField(extensionField, 42);

            var allFields = message.GetAllFields();
            Assert.AreEqual(1, allFields.Count);
            Assert.IsTrue(allFields.ContainsKey(extensionField));
            Assert.AreEqual(42, allFields[extensionField]);
        }

        [Test]
        public void Extension_SerializeAndParse()
        {
            var fileDescriptor = DynamicProto2UnittestFile;
            var testAllExtensions = fileDescriptor.FindTypeByName<MessageDescriptor>("TestAllExtensions");

            var int32Extension = FindExtensionByNumber(fileDescriptor, testAllExtensions, 1);
            var stringExtension = FindExtensionByNumber(fileDescriptor, testAllExtensions, 14);

            var message = new DynamicMessage(testAllExtensions);
            message.SetField(int32Extension, 42);
            message.SetField(stringExtension, "hello");

            // Serialize
            var bytes = message.ToByteArray();
            Assert.Greater(bytes.Length, 0);

            // Parse back using a new DynamicMessage and MergeFrom
            // Note: We can't use Parser.ParseFrom because that returns a generated message
            var reparsed = new DynamicMessage(testAllExtensions);
            reparsed.MergeFrom(bytes);

            // The values should be accessible via the extension fields we set
            Assert.AreEqual(42, reparsed.GetField(int32Extension));
            Assert.AreEqual("hello", reparsed.GetField(stringExtension));
        }

        [Test]
        public void Extension_Clone()
        {
            var fileDescriptor = DynamicProto2UnittestFile;
            var testAllExtensions = fileDescriptor.FindTypeByName<MessageDescriptor>("TestAllExtensions");
            var extensionField = FindExtensionByNumber(fileDescriptor, testAllExtensions, 1);

            var message = new DynamicMessage(testAllExtensions);
            message.SetField(extensionField, 42);

            var clone = message.Clone();

            Assert.AreEqual(42, clone.GetField(extensionField));
            Assert.AreNotSame(message, clone);

            // Modifying clone shouldn't affect original
            clone.SetField(extensionField, 100);
            Assert.AreEqual(42, message.GetField(extensionField));
            Assert.AreEqual(100, clone.GetField(extensionField));
        }

        [Test]
        public void Extension_MergeFrom()
        {
            var fileDescriptor = DynamicProto2UnittestFile;
            var testAllExtensions = fileDescriptor.FindTypeByName<MessageDescriptor>("TestAllExtensions");

            var int32Extension = FindExtensionByNumber(fileDescriptor, testAllExtensions, 1);
            var stringExtension = FindExtensionByNumber(fileDescriptor, testAllExtensions, 14);

            var message1 = new DynamicMessage(testAllExtensions);
            message1.SetField(int32Extension, 42);

            var message2 = new DynamicMessage(testAllExtensions);
            message2.SetField(stringExtension, "hello");

            message1.MergeFrom(message2);

            Assert.AreEqual(42, message1.GetField(int32Extension));
            Assert.AreEqual("hello", message1.GetField(stringExtension));
        }

        [Test]
        public void Extension_Equals()
        {
            var fileDescriptor = DynamicProto2UnittestFile;
            var testAllExtensions = fileDescriptor.FindTypeByName<MessageDescriptor>("TestAllExtensions");
            var extensionField = FindExtensionByNumber(fileDescriptor, testAllExtensions, 1);

            var message1 = new DynamicMessage(testAllExtensions);
            message1.SetField(extensionField, 42);

            var message2 = new DynamicMessage(testAllExtensions);
            message2.SetField(extensionField, 42);

            var message3 = new DynamicMessage(testAllExtensions);
            message3.SetField(extensionField, 100);

            Assert.IsTrue(message1.Equals(message2));
            Assert.IsFalse(message1.Equals(message3));
        }

        [Test]
        public void Extension_CalculateSize()
        {
            var fileDescriptor = DynamicProto2UnittestFile;
            var testAllExtensions = fileDescriptor.FindTypeByName<MessageDescriptor>("TestAllExtensions");
            var extensionField = FindExtensionByNumber(fileDescriptor, testAllExtensions, 1);

            var message = new DynamicMessage(testAllExtensions);
            Assert.AreEqual(0, message.CalculateSize());

            message.SetField(extensionField, 42);
            Assert.Greater(message.CalculateSize(), 0);
        }

        // NOTE: Extension accessor tests are not included here because:
        // When extensions are loaded via generated code (Extension != null), they use
        // ExtensionAccessor which requires IExtensionMessage interface.
        // DynamicMessage doesn't implement IExtensionMessage, so the accessor can't be used directly.
        // Instead, use SetField/GetField/HasField/ClearField for extensions on DynamicMessage.

        [Test]
        public void Extension_RepeatedField_AddAndGet()
        {
            var fileDescriptor = DynamicProto2UnittestFile;
            var testAllExtensions = fileDescriptor.FindTypeByName<MessageDescriptor>("TestAllExtensions");

            // RepeatedInt32Extension has field number 31
            var repeatedExtension = FindExtensionByNumber(fileDescriptor, testAllExtensions, 31);
            Assert.NotNull(repeatedExtension, "RepeatedInt32Extension not found");
            Assert.IsTrue(repeatedExtension.IsExtension);
            Assert.IsTrue(repeatedExtension.IsRepeated);

            var message = new DynamicMessage(testAllExtensions);
            message.AddRepeatedField(repeatedExtension, 1);
            message.AddRepeatedField(repeatedExtension, 2);
            message.AddRepeatedField(repeatedExtension, 3);

            Assert.AreEqual(3, message.GetRepeatedFieldCount(repeatedExtension));
            Assert.AreEqual(1, message.GetRepeatedField(repeatedExtension, 0));
            Assert.AreEqual(2, message.GetRepeatedField(repeatedExtension, 1));
            Assert.AreEqual(3, message.GetRepeatedField(repeatedExtension, 2));
        }

        [Test]
        public void Extension_RepeatedField_SerializeAndParse()
        {
            var fileDescriptor = DynamicProto2UnittestFile;
            var testAllExtensions = fileDescriptor.FindTypeByName<MessageDescriptor>("TestAllExtensions");
            var repeatedExtension = FindExtensionByNumber(fileDescriptor, testAllExtensions, 31); // RepeatedInt32Extension

            var message = new DynamicMessage(testAllExtensions);
            message.AddRepeatedField(repeatedExtension, 1);
            message.AddRepeatedField(repeatedExtension, 2);
            message.AddRepeatedField(repeatedExtension, 3);

            var bytes = message.ToByteArray();

            // Parse back using MergeFrom with extension tracking
            var parsed = new DynamicMessage(testAllExtensions);
            parsed.MergeFrom(bytes);

            Assert.AreEqual(3, parsed.GetRepeatedFieldCount(repeatedExtension));
            Assert.AreEqual(1, parsed.GetRepeatedField(repeatedExtension, 0));
            Assert.AreEqual(2, parsed.GetRepeatedField(repeatedExtension, 1));
            Assert.AreEqual(3, parsed.GetRepeatedField(repeatedExtension, 2));
        }

        [Test]
        public void Extension_WrongType_ThrowsException()
        {
            // Get descriptors for two different message types
            var fileDescriptor = DynamicProto2UnittestFile;
            var testAllExtensions = fileDescriptor.FindTypeByName<MessageDescriptor>("TestAllExtensions");
            var foreignMessage = fileDescriptor.FindTypeByName<MessageDescriptor>("ForeignMessage");

            Assert.NotNull(testAllExtensions);
            Assert.NotNull(foreignMessage);

            // Get an extension for TestAllExtensions
            var extensionField = FindExtensionByNumber(fileDescriptor, testAllExtensions, 1); // OptionalInt32Extension
            Assert.NotNull(extensionField, "OptionalInt32Extension not found");
            Assert.IsTrue(extensionField.IsExtension);

            // Create a DynamicMessage for a different type (ForeignMessage)
            var message = new DynamicMessage(foreignMessage);

            // Trying to set an extension on the wrong message type should throw
            Assert.Throws<ArgumentException>(() => message.SetField(extensionField, 42));
        }

        [Test]
        public void Extension_InteroperabilityWithGeneratedMessages()
        {
            // Get the dynamic descriptor
            var fileDescriptor = DynamicProto2UnittestFile;
            var testAllExtensions = fileDescriptor.FindTypeByName<MessageDescriptor>("TestAllExtensions");
            var int32Extension = FindExtensionByNumber(fileDescriptor, testAllExtensions, 1);

            // Create with generated code
            var generated = new Google.Protobuf.TestProtos.Proto2.TestAllExtensions();
            generated.SetExtension(Google.Protobuf.TestProtos.Proto2.UnittestExtensions.OptionalInt32Extension, 42);
            var originalBytes = generated.ToByteArray();

            // Parse as DynamicMessage using MergeFrom for extension tracking
            var dynamic = new DynamicMessage(testAllExtensions);
            dynamic.MergeFrom(originalBytes);

            // Verify the extension was parsed correctly
            Assert.AreEqual(42, dynamic.GetField(int32Extension));

            // Modify and serialize back
            dynamic.SetField(int32Extension, 100);
            var modifiedBytes = dynamic.ToByteArray();

            // Parse back as generated message
            var parsedGenerated = Google.Protobuf.TestProtos.Proto2.TestAllExtensions.Parser
                .WithExtensionRegistry(new ExtensionRegistry { Google.Protobuf.TestProtos.Proto2.UnittestExtensions.OptionalInt32Extension })
                .ParseFrom(modifiedBytes);

            Assert.AreEqual(100, parsedGenerated.GetExtension(Google.Protobuf.TestProtos.Proto2.UnittestExtensions.OptionalInt32Extension));
        }

        #endregion

        #region JSON serialization tests

        [Test]
        public void JsonFormatter_DynamicMessage_WithDynamicDescriptor()
        {
            // JSON serialization works with dynamically loaded descriptors (not generated code descriptors)
            // because they get DynamicFieldAccessors that work with DynamicMessage.
            var descriptorData = new List<ByteString>
            {
                UnittestImportPublicProto3Reflection.Descriptor.SerializedData,
                UnittestImportProto3Reflection.Descriptor.SerializedData,
                UnittestProto3Reflection.Descriptor.SerializedData
            };
            var converted = FileDescriptor.BuildFromByteStrings(descriptorData);
            var dynamicDescriptor = converted[2].FindTypeByName<MessageDescriptor>("TestAllTypes");

            var message = new DynamicMessage(dynamicDescriptor);
            var singleInt32Field = dynamicDescriptor.FindFieldByName("single_int32");
            var singleStringField = dynamicDescriptor.FindFieldByName("single_string");
            message.SetField(singleInt32Field, 42);
            message.SetField(singleStringField, "hello");

            var json = JsonFormatter.Default.Format(message);

            Assert.That(json, Does.Contain("\"singleInt32\": 42"));
            Assert.That(json, Does.Contain("\"singleString\": \"hello\""));
        }

        [Test]
        public void JsonParser_DynamicMessage_RoundTrip()
        {
            // JSON parsing works with dynamically loaded descriptors
            var json = "{ \"singleInt32\": 42, \"singleString\": \"hello\" }";

            var message = JsonParser.Default.Parse(json, DynamicTestAllTypes);

            Assert.That(message, Is.TypeOf<DynamicMessage>());
            var dynamicMessage = (DynamicMessage)message;
            Assert.That(dynamicMessage.GetField(DynamicTestAllTypes.FindFieldByName("single_int32")), Is.EqualTo(42));
            Assert.That(dynamicMessage.GetField(DynamicTestAllTypes.FindFieldByName("single_string")), Is.EqualTo("hello"));

            // Verify round-trip
            var formattedJson = JsonFormatter.Default.Format(dynamicMessage);
            Assert.That(formattedJson, Does.Contain("\"singleInt32\": 42"));
            Assert.That(formattedJson, Does.Contain("\"singleString\": \"hello\""));
        }

        [Test]
        public void JsonFormatter_DynamicMessage_FieldMask_UsesAccessorContractOnly()
        {
            // JsonFormatter has a special-case for google.protobuf.FieldMask. When formatting a DynamicMessage,
            // the repeated "paths" field is only guaranteed (by IFieldAccessor) to be an untyped IList.
            // This test ensures the formatter doesn't assume IList<string>.

            var fileProto = new FileDescriptorProto
            {
                // MessageDescriptor.IsWellKnownType is based on the canonical well-known type file name.
                Name = "google/protobuf/field_mask.proto",
                Package = "google.protobuf",
                Syntax = "proto3",
                MessageType =
                {
                    new DescriptorProto
                    {
                        Name = "FieldMask",
                        Field =
                        {
                            new FieldDescriptorProto
                            {
                                Name = "paths",
                                Number = 1,
                                Label = FieldDescriptorProto.Types.Label.Repeated,
                                Type = FieldDescriptorProto.Types.Type.String
                            }
                        }
                    }
                }
            };

            var converted = FileDescriptor.BuildFromByteStrings(new[] { fileProto.ToByteString() });
            var dynamicFieldMaskDescriptor = converted[0].FindTypeByName<MessageDescriptor>("FieldMask");
            var message = new DynamicMessage(dynamicFieldMaskDescriptor);

            var pathsField = dynamicFieldMaskDescriptor.FindFieldByName("paths");
            var paths = (IList)message.GetField(pathsField);
            paths.Add("foo_bar");
            paths.Add("bar_baz");

            var json = JsonFormatter.Default.Format(message);

            Assert.AreEqual("\"fooBar,barBaz\"", json);
        }

        [Test]
        public void JsonParser_DynamicMessage_NestedMessage()
        {
            // JSON parsing handles nested messages
            var json = "{ \"singleNestedMessage\": { \"bb\": 123 } }";

            var message = JsonParser.Default.Parse(json, DynamicTestAllTypes);

            Assert.That(message, Is.TypeOf<DynamicMessage>());
            var dynamicMessage = (DynamicMessage)message;
            var nestedField = DynamicTestAllTypes.FindFieldByName("single_nested_message");
            var nested = dynamicMessage.GetField(nestedField) as DynamicMessage;
            Assert.That(nested, Is.Not.Null);
            Assert.That(nested.GetField(DynamicNestedMessage.FindFieldByName("bb")), Is.EqualTo(123));
        }

        [Test]
        public void JsonParser_DynamicMessage_RepeatedField()
        {
            // JSON parsing handles repeated fields
            var json = "{ \"repeatedInt32\": [1, 2, 3] }";

            var message = JsonParser.Default.Parse(json, DynamicTestAllTypes);

            Assert.That(message, Is.TypeOf<DynamicMessage>());
            var dynamicMessage = (DynamicMessage)message;
            var repeatedField = DynamicTestAllTypes.FindFieldByName("repeated_int32");
            var values = dynamicMessage.GetField(repeatedField) as IList;
            Assert.That(values, Is.Not.Null);
            Assert.That(values.Count, Is.EqualTo(3));
            Assert.That(values[0], Is.EqualTo(1));
            Assert.That(values[1], Is.EqualTo(2));
            Assert.That(values[2], Is.EqualTo(3));
        }

        [Test]
        public void JsonParser_DynamicMessage_MapField()
        {
            // JSON parsing handles map fields
            var json = "{ \"mapInt32Int32\": { \"1\": 10, \"2\": 20 } }";

            var message = JsonParser.Default.Parse(json, DynamicTestMap);

            Assert.That(message, Is.TypeOf<DynamicMessage>());
            var dynamicMessage = (DynamicMessage)message;
            var mapField = DynamicTestMap.FindFieldByName("map_int32_int32");
            var map = dynamicMessage.GetField(mapField) as IDictionary;
            Assert.That(map, Is.Not.Null);
            Assert.That(map.Count, Is.EqualTo(2));
            Assert.That(map[1], Is.EqualTo(10));
            Assert.That(map[2], Is.EqualTo(20));
        }

        [Test]
        public void JsonFormatter_Struct_RoundTrip()
        {
            // Create a dynamic descriptor for Struct
            var descriptorData = Google.Protobuf.WellKnownTypes.Struct.Descriptor.File.SerializedData;
            var fileDescriptors = FileDescriptor.BuildFromByteStrings(new[] { descriptorData });
            var fileDescriptor = fileDescriptors[0];
            var structDescriptor = fileDescriptor.MessageTypes.Single(mt => mt.Name == "Struct");
            var valueDescriptor = fileDescriptor.MessageTypes.Single(mt => mt.Name == "Value");

            // Create a DynamicMessage for Struct
            var message = new DynamicMessage(structDescriptor);
            var fieldsField = structDescriptor.FindFieldByName("fields");

            // Create a Value message (string)
            var stringValue = new DynamicMessage(valueDescriptor);
            var stringValueField = valueDescriptor.FindFieldByName("string_value");
            stringValue.SetField(stringValueField, "bar");

            // Create a Value message (number)
            var numberValue = new DynamicMessage(valueDescriptor);
            var numberValueField = valueDescriptor.FindFieldByName("number_value");
            numberValue.SetField(numberValueField, 123.0);

            // Add to map
            var fields = message.GetField(fieldsField);
            // DynamicMessage returns a mutable IDictionary for map fields
            var fieldsDict = (System.Collections.IDictionary)fields;
            fieldsDict["foo"] = stringValue;
            fieldsDict["baz"] = numberValue;

            var formatter = JsonFormatter.Default;
            var json = formatter.Format(message);

            // Round-trip
            var parsed = JsonParser.Default.Parse(json, structDescriptor);
            Assert.That(parsed, Is.EqualTo(message));
        }

        [Test]
        public void JsonFormatter_Value_Null_RoundTrip()
        {
            var descriptorData = Google.Protobuf.WellKnownTypes.Struct.Descriptor.File.SerializedData;
            var fileDescriptors = FileDescriptor.BuildFromByteStrings(new[] { descriptorData });
            var fileDescriptor = fileDescriptors[0];
            var valueDescriptor = fileDescriptor.MessageTypes.Single(mt => mt.Name == "Value");

            var message = new DynamicMessage(valueDescriptor);
            var nullValueField = valueDescriptor.FindFieldByName("null_value");
            message.SetField(nullValueField, 0); // NullValue.NullValue is 0

            var formatter = JsonFormatter.Default;
            var json = formatter.Format(message);

            Assert.AreEqual("null", json);

            // Round-trip
            var parsed = JsonParser.Default.Parse(json, valueDescriptor);
            Assert.That(parsed, Is.EqualTo(message));
        }

        #endregion
    }
}









