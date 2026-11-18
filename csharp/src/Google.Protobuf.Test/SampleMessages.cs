#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using Google.Protobuf.TestProtos;
using Proto2 = Google.Protobuf.TestProtos.Proto2;

using static Google.Protobuf.TestProtos.Proto2.UnittestExtensions;

namespace Google.Protobuf
{
    /// <summary>
    /// Helper methods to create sample instances of types generated from unit test messages.
    /// </summary>
    public class SampleMessages
    {
        /// <summary>
        /// Creates a new sample TestAllTypes message with all fields populated.
        /// The "oneof" field is populated with the string property (OneofString).
        /// </summary>
        public static TestAllTypes CreateFullTestAllTypes()
        {
            return new TestAllTypes
            {
                SingleBool = true,
                SingleBytes = ByteString.CopyFrom(1, 2, 3, 4),
                SingleDouble = 23.5,
                SingleFixed32 = 23,
                SingleFixed64 = 1234567890123,
                SingleFloat = 12.25f,
                SingleForeignEnum = ForeignEnum.ForeignBar,
                SingleForeignMessage = new ForeignMessage { C = 10 },
                SingleImportEnum = ImportEnum.ImportBaz,
                SingleImportMessage = new ImportMessage { D = 20 },
                SingleInt32 = 100,
                SingleInt64 = 3210987654321,
                SingleNestedEnum = TestProtos.TestAllTypes.Types.NestedEnum.Foo,
                SingleNestedMessage = new TestAllTypes.Types.NestedMessage { Bb = 35 },
                SinglePublicImportMessage = new PublicImportMessage { E = 54 },
                SingleSfixed32 = -123,
                SingleSfixed64 = -12345678901234,
                SingleSint32 = -456,
                SingleSint64 = -12345678901235,
                SingleString = "test",
                SingleUint32 = UInt32.MaxValue,
                SingleUint64 = UInt64.MaxValue,
                RepeatedBool = { true, false },
                RepeatedBytes = { ByteString.CopyFrom(1, 2, 3, 4), ByteString.CopyFrom(5, 6), ByteString.CopyFrom(new byte[1000]) },
                RepeatedDouble = { -12.25, 23.5 },
                RepeatedFixed32 = { UInt32.MaxValue, 23 },
                RepeatedFixed64 = { UInt64.MaxValue, 1234567890123 },
                RepeatedFloat = { 100f, 12.25f },
                RepeatedForeignEnum = { ForeignEnum.ForeignFoo, ForeignEnum.ForeignBar },
                RepeatedForeignMessage = { new ForeignMessage(), new ForeignMessage { C = 10 } },
                RepeatedImportEnum = { ImportEnum.ImportBaz, ImportEnum.Unspecified },
                RepeatedImportMessage = { new ImportMessage { D = 20 }, new ImportMessage { D = 25 } },
                RepeatedInt32 = { 100, 200 },
                RepeatedInt64 = { 3210987654321, Int64.MaxValue },
                RepeatedNestedEnum = { TestProtos.TestAllTypes.Types.NestedEnum.Foo, TestProtos.TestAllTypes.Types.NestedEnum.Neg },
                RepeatedNestedMessage = { new TestAllTypes.Types.NestedMessage { Bb = 35 }, new TestAllTypes.Types.NestedMessage { Bb = 10 } },
                RepeatedPublicImportMessage = { new PublicImportMessage { E = 54 }, new PublicImportMessage { E = -1 } },
                RepeatedSfixed32 = { -123, 123 },
                RepeatedSfixed64 = { -12345678901234, 12345678901234 },
                RepeatedSint32 = { -456, 100 },
                RepeatedSint64 = { -12345678901235, 123 },
                RepeatedString = { "foo", "bar" },
                RepeatedUint32 = { UInt32.MaxValue, UInt32.MinValue },
                RepeatedUint64 = { UInt64.MaxValue, UInt32.MinValue },
                OneofString = "Oneof string"
            };
        }

        public static Proto2.TestAllTypes CreateFullTestAllTypesProto2()
        {
            return new Proto2.TestAllTypes
            {
                OptionalBool = true,
                OptionalBytes = ByteString.CopyFrom(1, 2, 3, 4),
                OptionalDouble = 23.5,
                OptionalFixed32 = 23,
                OptionalFixed64 = 1234567890123,
                OptionalFloat = 12.25f,
                OptionalForeignEnum = Proto2.ForeignEnum.ForeignBar,
                OptionalForeignMessage = new Proto2.ForeignMessage { C = 10 },
                OptionalImportEnum = Proto2.ImportEnum.ImportBaz,
                OptionalImportMessage = new Proto2.ImportMessage { D = 20 },
                OptionalInt32 = 100,
                OptionalInt64 = 3210987654321,
                OptionalNestedEnum = Proto2.TestAllTypes.Types.NestedEnum.Foo,
                OptionalNestedMessage = new Proto2.TestAllTypes.Types.NestedMessage { Bb = 35 },
                OptionalPublicImportMessage = new Proto2.PublicImportMessage { E = 54 },
                OptionalSfixed32 = -123,
                OptionalSfixed64 = -12345678901234,
                OptionalSint32 = -456,
                OptionalSint64 = -12345678901235,
                OptionalString = "test",
                OptionalUint32 = UInt32.MaxValue,
                OptionalUint64 = UInt64.MaxValue,
                OptionalGroup = new Proto2.TestAllTypes.Types.OptionalGroup { A = 10 },
                RepeatedBool = { true, false },
                RepeatedBytes = { ByteString.CopyFrom(1, 2, 3, 4), ByteString.CopyFrom(5, 6), ByteString.CopyFrom(new byte[1000]) },
                RepeatedDouble = { -12.25, 23.5 },
                RepeatedFixed32 = { UInt32.MaxValue, 23 },
                RepeatedFixed64 = { UInt64.MaxValue, 1234567890123 },
                RepeatedFloat = { 100f, 12.25f },
                RepeatedForeignEnum = { Proto2.ForeignEnum.ForeignFoo, Proto2.ForeignEnum.ForeignBar },
                RepeatedForeignMessage = { new Proto2.ForeignMessage(), new Proto2.ForeignMessage { C = 10 } },
                RepeatedImportEnum = { Proto2.ImportEnum.ImportBaz, Proto2.ImportEnum.ImportFoo },
                RepeatedImportMessage = { new Proto2.ImportMessage { D = 20 }, new Proto2.ImportMessage { D = 25 } },
                RepeatedInt32 = { 100, 200 },
                RepeatedInt64 = { 3210987654321, Int64.MaxValue },
                RepeatedNestedEnum = { Proto2.TestAllTypes.Types.NestedEnum.Foo, Proto2.TestAllTypes.Types.NestedEnum.Neg },
                RepeatedNestedMessage = { new Proto2.TestAllTypes.Types.NestedMessage { Bb = 35 }, new Proto2.TestAllTypes.Types.NestedMessage { Bb = 10 } },
                RepeatedSfixed32 = { -123, 123 },
                RepeatedSfixed64 = { -12345678901234, 12345678901234 },
                RepeatedSint32 = { -456, 100 },
                RepeatedSint64 = { -12345678901235, 123 },
                RepeatedString = { "foo", "bar" },
                RepeatedUint32 = { UInt32.MaxValue, UInt32.MinValue },
                RepeatedUint64 = { UInt64.MaxValue, UInt32.MinValue },
                RepeatedGroup = { new Proto2.TestAllTypes.Types.RepeatedGroup { A = 10 }, new Proto2.TestAllTypes.Types.RepeatedGroup { A = 20 } },
                OneofString = "Oneof string"
            };
        }

        public static Proto2.TestAllExtensions CreateFullTestAllExtensions()
        {
            var message = new Proto2.TestAllExtensions();
            message.SetExtension(OptionalBoolExtension, true);
            message.SetExtension(OptionalBytesExtension, ByteString.CopyFrom(1, 2, 3, 4));
            message.SetExtension(OptionalDoubleExtension, 23.5);
            message.SetExtension(OptionalFixed32Extension, 23u);
            message.SetExtension(OptionalFixed64Extension, 1234567890123u);
            message.SetExtension(OptionalFloatExtension, 12.25f);
            message.SetExtension(OptionalForeignEnumExtension, Proto2.ForeignEnum.ForeignBar);
            message.SetExtension(OptionalForeignMessageExtension, new Proto2.ForeignMessage { C = 10 });
            message.SetExtension(OptionalImportEnumExtension, Proto2.ImportEnum.ImportBaz);
            message.SetExtension(OptionalImportMessageExtension, new Proto2.ImportMessage { D = 20 });
            message.SetExtension(OptionalInt32Extension, 100);
            message.SetExtension(OptionalInt64Extension, 3210987654321);
            message.SetExtension(OptionalNestedEnumExtension, Proto2.TestAllTypes.Types.NestedEnum.Foo);
            message.SetExtension(OptionalNestedMessageExtension, new Proto2.TestAllTypes.Types.NestedMessage { Bb = 35 });
            message.SetExtension(OptionalPublicImportMessageExtension, new Proto2.PublicImportMessage { E = 54 });
            message.SetExtension(OptionalSfixed32Extension, -123);
            message.SetExtension(OptionalSfixed64Extension, -12345678901234);
            message.SetExtension(OptionalSint32Extension, -456);
            message.SetExtension(OptionalSint64Extension, -12345678901235);
            message.SetExtension(OptionalStringExtension, "test");
            message.SetExtension(OptionalUint32Extension, UInt32.MaxValue);
            message.SetExtension(OptionalUint64Extension, UInt64.MaxValue);
            message.SetExtension(OptionalGroupExtension, new Proto2.OptionalGroup_extension { A = 10 });
            message.GetOrInitializeExtension(RepeatedBoolExtension).AddRange(new[] { true, false });
            message.GetOrInitializeExtension(RepeatedBytesExtension).AddRange(new[] { ByteString.CopyFrom(1, 2, 3, 4), ByteString.CopyFrom(5, 6), ByteString.CopyFrom(new byte[1000]) });
            message.GetOrInitializeExtension(RepeatedDoubleExtension).AddRange(new[] { -12.25, 23.5 });
            message.GetOrInitializeExtension(RepeatedFixed32Extension).AddRange(new[] { UInt32.MaxValue, 23u });
            message.GetOrInitializeExtension(RepeatedFixed64Extension).AddRange(new[] { UInt64.MaxValue, 1234567890123ul });
            message.GetOrInitializeExtension(RepeatedFloatExtension).AddRange(new[] { 100f, 12.25f });
            message.GetOrInitializeExtension(RepeatedForeignEnumExtension).AddRange(new[] { Proto2.ForeignEnum.ForeignFoo, Proto2.ForeignEnum.ForeignBar });
            message.GetOrInitializeExtension(RepeatedForeignMessageExtension).AddRange(new[] { new Proto2.ForeignMessage(), new Proto2.ForeignMessage { C = 10 } });
            message.GetOrInitializeExtension(RepeatedImportEnumExtension).AddRange(new[] { Proto2.ImportEnum.ImportBaz, Proto2.ImportEnum.ImportFoo });
            message.GetOrInitializeExtension(RepeatedImportMessageExtension).AddRange(new[] { new Proto2.ImportMessage { D = 20 }, new Proto2.ImportMessage { D = 25 } });
            message.GetOrInitializeExtension(RepeatedInt32Extension).AddRange(new[] { 100, 200 });
            message.GetOrInitializeExtension(RepeatedInt64Extension).AddRange(new[] { 3210987654321, Int64.MaxValue });
            message.GetOrInitializeExtension(RepeatedNestedEnumExtension).AddRange(new[] { Proto2.TestAllTypes.Types.NestedEnum.Foo, Proto2.TestAllTypes.Types.NestedEnum.Neg });
            message.GetOrInitializeExtension(RepeatedNestedMessageExtension).AddRange(new[] { new Proto2.TestAllTypes.Types.NestedMessage { Bb = 35 }, new Proto2.TestAllTypes.Types.NestedMessage { Bb = 10 } });
            message.GetOrInitializeExtension(RepeatedSfixed32Extension).AddRange(new[] { -123, 123 });
            message.GetOrInitializeExtension(RepeatedSfixed64Extension).AddRange(new[] { -12345678901234, 12345678901234 });
            message.GetOrInitializeExtension(RepeatedSint32Extension).AddRange(new[] { -456, 100 });
            message.GetOrInitializeExtension(RepeatedSint64Extension).AddRange(new[] { -12345678901235, 123 });
            message.GetOrInitializeExtension(RepeatedStringExtension).AddRange(new[] { "foo", "bar" });
            message.GetOrInitializeExtension(RepeatedUint32Extension).AddRange(new[] { UInt32.MaxValue, UInt32.MinValue });
            message.GetOrInitializeExtension(RepeatedUint64Extension).AddRange(new[] { UInt64.MaxValue, UInt32.MinValue });
            message.GetOrInitializeExtension(RepeatedGroupExtension).AddRange(new[] { new Proto2.RepeatedGroup_extension { A = 10 }, new Proto2.RepeatedGroup_extension { A = 20 } });
            message.SetExtension(OneofStringExtension, "Oneof string");
            return message;
        }
    }
}