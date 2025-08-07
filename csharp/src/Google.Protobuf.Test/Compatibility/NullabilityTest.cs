#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using NUnit.Framework;
using System;
using System.Reflection;
using Google.Protobuf.TestProtos;

namespace Google.Protobuf.Compatibility
{
    public class NullabilityTest
    {
        private static readonly NullabilityInfoContext NullabilityContext = new();

        [Test]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleBool))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleDouble))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleFixed32))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleFixed64))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleFloat))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleForeignEnum))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleImportEnum))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleInt32))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleInt64))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleNestedEnum))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleSfixed32))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleSfixed64))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleSint32))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleSint64))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleUint32))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleUint64))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.OneofFieldCase))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.OneofUint32))]
        public void GetProperty_IsNotNull(Type type, string name)
        {
            // These must always be NotNull, independent of using NRT or not.
            GetProperty_IsNullabilityState(type, name, NullabilityState.NotNull, false);
        }

        [Test]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleBytes))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleString))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.RepeatedBool))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.RepeatedBytes))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.RepeatedDouble))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.RepeatedFixed32))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.RepeatedFixed64))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.RepeatedFloat))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.RepeatedForeignEnum))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.RepeatedForeignMessage))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.RepeatedImportEnum))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.RepeatedImportMessage))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.RepeatedNestedEnum))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.RepeatedNestedMessage))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.RepeatedPublicImportMessage))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.RepeatedSfixed32))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.RepeatedSfixed64))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.RepeatedSint32))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.RepeatedSint64))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.RepeatedString))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.RepeatedUint32))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.RepeatedUint64))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.OneofString))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.OneofBytes))]
        [TestCase(typeof(RepeatedWellKnownTypes), nameof(RepeatedWellKnownTypes.AnyField))]
        [TestCase(typeof(RepeatedWellKnownTypes), nameof(RepeatedWellKnownTypes.ApiField))]
        [TestCase(typeof(RepeatedWellKnownTypes), nameof(RepeatedWellKnownTypes.DurationField))]
        [TestCase(typeof(RepeatedWellKnownTypes), nameof(RepeatedWellKnownTypes.EmptyField))]
        [TestCase(typeof(RepeatedWellKnownTypes), nameof(RepeatedWellKnownTypes.FieldMaskField))]
        [TestCase(typeof(RepeatedWellKnownTypes), nameof(RepeatedWellKnownTypes.SourceContextField))]
        [TestCase(typeof(RepeatedWellKnownTypes), nameof(RepeatedWellKnownTypes.StructField))]
        [TestCase(typeof(RepeatedWellKnownTypes), nameof(RepeatedWellKnownTypes.TimestampField))]
        [TestCase(typeof(RepeatedWellKnownTypes), nameof(RepeatedWellKnownTypes.TypeField))]
        [TestCase(typeof(RepeatedWellKnownTypes), nameof(RepeatedWellKnownTypes.DoubleField))]
        [TestCase(typeof(RepeatedWellKnownTypes), nameof(RepeatedWellKnownTypes.FloatField))]
        [TestCase(typeof(RepeatedWellKnownTypes), nameof(RepeatedWellKnownTypes.Int64Field))]
        [TestCase(typeof(RepeatedWellKnownTypes), nameof(RepeatedWellKnownTypes.Uint64Field))]
        [TestCase(typeof(RepeatedWellKnownTypes), nameof(RepeatedWellKnownTypes.Int32Field))]
        [TestCase(typeof(RepeatedWellKnownTypes), nameof(RepeatedWellKnownTypes.Uint32Field))]
        [TestCase(typeof(RepeatedWellKnownTypes), nameof(RepeatedWellKnownTypes.BoolField))]
        [TestCase(typeof(RepeatedWellKnownTypes), nameof(RepeatedWellKnownTypes.StringField))]
        [TestCase(typeof(RepeatedWellKnownTypes), nameof(RepeatedWellKnownTypes.BytesField))]
        [TestCase(typeof(MapWellKnownTypes), nameof(MapWellKnownTypes.AnyField))]
        [TestCase(typeof(MapWellKnownTypes), nameof(MapWellKnownTypes.ApiField))]
        [TestCase(typeof(MapWellKnownTypes), nameof(MapWellKnownTypes.DurationField))]
        [TestCase(typeof(MapWellKnownTypes), nameof(MapWellKnownTypes.EmptyField))]
        [TestCase(typeof(MapWellKnownTypes), nameof(MapWellKnownTypes.FieldMaskField))]
        [TestCase(typeof(MapWellKnownTypes), nameof(MapWellKnownTypes.SourceContextField))]
        [TestCase(typeof(MapWellKnownTypes), nameof(MapWellKnownTypes.StructField))]
        [TestCase(typeof(MapWellKnownTypes), nameof(MapWellKnownTypes.TimestampField))]
        [TestCase(typeof(MapWellKnownTypes), nameof(MapWellKnownTypes.TypeField))]
        [TestCase(typeof(MapWellKnownTypes), nameof(MapWellKnownTypes.DoubleField))]
        [TestCase(typeof(MapWellKnownTypes), nameof(MapWellKnownTypes.FloatField))]
        [TestCase(typeof(MapWellKnownTypes), nameof(MapWellKnownTypes.Int64Field))]
        [TestCase(typeof(MapWellKnownTypes), nameof(MapWellKnownTypes.Uint64Field))]
        [TestCase(typeof(MapWellKnownTypes), nameof(MapWellKnownTypes.Int32Field))]
        [TestCase(typeof(MapWellKnownTypes), nameof(MapWellKnownTypes.Uint32Field))]
        [TestCase(typeof(MapWellKnownTypes), nameof(MapWellKnownTypes.BoolField))]
        [TestCase(typeof(MapWellKnownTypes), nameof(MapWellKnownTypes.StringField))]
        [TestCase(typeof(MapWellKnownTypes), nameof(MapWellKnownTypes.BytesField))]
        public void GetProperty_IsNotNullInNrt(Type type, string name)
        {
            // These must be NotNull for NRT or Unknown if not.
            GetProperty_IsNullabilityState(type, name, NullabilityState.NotNull, true);
        }

        [Test]
        [TestCase(typeof(TestWellKnownTypes), nameof(TestWellKnownTypes.BoolField))]
        [TestCase(typeof(TestWellKnownTypes), nameof(TestWellKnownTypes.FloatField))]
        [TestCase(typeof(TestWellKnownTypes), nameof(TestWellKnownTypes.DoubleField))]
        [TestCase(typeof(TestWellKnownTypes), nameof(TestWellKnownTypes.Int32Field))]
        [TestCase(typeof(TestWellKnownTypes), nameof(TestWellKnownTypes.Int64Field))]
        [TestCase(typeof(TestWellKnownTypes), nameof(TestWellKnownTypes.Uint32Field))]
        [TestCase(typeof(TestWellKnownTypes), nameof(TestWellKnownTypes.Uint64Field))]
        public void GetProperty_IsNullable(Type type, string name)
        {
            // These must be Nullable for NRT or Unknown if not.
            GetProperty_IsNullabilityState(type, name, NullabilityState.Nullable, false);
        }

        [Test]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleForeignMessage))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleImportMessage))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SingleNestedMessage))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.SinglePublicImportMessage))]
        [TestCase(typeof(TestAllTypes), nameof(TestAllTypes.OneofNestedMessage))]
        [TestCase(typeof(TestWellKnownTypes), nameof(TestWellKnownTypes.AnyField))]
        [TestCase(typeof(TestWellKnownTypes), nameof(TestWellKnownTypes.ApiField))]
        [TestCase(typeof(TestWellKnownTypes), nameof(TestWellKnownTypes.DurationField))]
        [TestCase(typeof(TestWellKnownTypes), nameof(TestWellKnownTypes.EmptyField))]
        [TestCase(typeof(TestWellKnownTypes), nameof(TestWellKnownTypes.FieldMaskField))]
        [TestCase(typeof(TestWellKnownTypes), nameof(TestWellKnownTypes.SourceContextField))]
        [TestCase(typeof(TestWellKnownTypes), nameof(TestWellKnownTypes.StructField))]
        [TestCase(typeof(TestWellKnownTypes), nameof(TestWellKnownTypes.TimestampField))]
        [TestCase(typeof(TestWellKnownTypes), nameof(TestWellKnownTypes.TypeField))]
        [TestCase(typeof(TestWellKnownTypes), nameof(TestWellKnownTypes.StringField))]
        [TestCase(typeof(TestWellKnownTypes), nameof(TestWellKnownTypes.BytesField))]
        [TestCase(typeof(TestWellKnownTypes), nameof(TestWellKnownTypes.ValueField))]
        [TestCase(typeof(OneofWellKnownTypes), nameof(OneofWellKnownTypes.DoubleField))]
        [TestCase(typeof(OneofWellKnownTypes), nameof(OneofWellKnownTypes.FloatField))]
        [TestCase(typeof(OneofWellKnownTypes), nameof(OneofWellKnownTypes.Int64Field))]
        [TestCase(typeof(OneofWellKnownTypes), nameof(OneofWellKnownTypes.Uint64Field))]
        [TestCase(typeof(OneofWellKnownTypes), nameof(OneofWellKnownTypes.Int32Field))]
        [TestCase(typeof(OneofWellKnownTypes), nameof(OneofWellKnownTypes.Uint32Field))]
        [TestCase(typeof(OneofWellKnownTypes), nameof(OneofWellKnownTypes.BoolField))]
        [TestCase(typeof(OneofWellKnownTypes), nameof(OneofWellKnownTypes.StringField))]
        [TestCase(typeof(OneofWellKnownTypes), nameof(OneofWellKnownTypes.BytesField))]
        [TestCase(typeof(OneofWellKnownTypes), nameof(OneofWellKnownTypes.AnyField))]
        [TestCase(typeof(OneofWellKnownTypes), nameof(OneofWellKnownTypes.ApiField))]
        [TestCase(typeof(OneofWellKnownTypes), nameof(OneofWellKnownTypes.DurationField))]
        [TestCase(typeof(OneofWellKnownTypes), nameof(OneofWellKnownTypes.EmptyField))]
        [TestCase(typeof(OneofWellKnownTypes), nameof(OneofWellKnownTypes.FieldMaskField))]
        [TestCase(typeof(OneofWellKnownTypes), nameof(OneofWellKnownTypes.SourceContextField))]
        [TestCase(typeof(OneofWellKnownTypes), nameof(OneofWellKnownTypes.StructField))]
        [TestCase(typeof(OneofWellKnownTypes), nameof(OneofWellKnownTypes.TimestampField))]
        [TestCase(typeof(OneofWellKnownTypes), nameof(OneofWellKnownTypes.TypeField))]
        public void GetProperty_IsNullableInNrt(Type type, string name)
        {
            // These must be Nullable for NRT or Unknown if not.
            GetProperty_IsNullabilityState(type, name, NullabilityState.Nullable, true);
        }

        // ReSharper disable once RedundantAssignment
        private void GetProperty_IsNullabilityState(Type type, string name, NullabilityState state, bool unknownIfNotNrt)
        {
            var property = TypeExtensions.GetProperty(type, name);
            var propertyType = property.PropertyType;
            if (propertyType.IsGenericType && propertyType.GetGenericTypeDefinition() == typeof(Nullable<>))
            {
                // It's an explicit nullable struct type, so we need to check a bit differently.
                Assert.IsTrue(state == NullabilityState.Nullable);
                return;
            }

            #if !NET8_0

            // If we are not using the NRT protos, these MUST be Unknown!
            if (unknownIfNotNrt)
            {
                state = NullabilityState.Unknown;
            }

            #endif

            #if !NET6_0_OR_GREATER

            // Cases other than Nullable<> are not able to check prior to .NET 6 without fully backporting the feature,
            // so Polyglot will always return Unknown. The non-NRTs will still be checked on .NET 6!
            state = NullabilityState.Unknown;

            #endif

            var nullabilityInfo = NullabilityContext.Create(property);
            if (property.CanRead) Assert.AreEqual(state, nullabilityInfo.ReadState);
            if (property.CanWrite) Assert.AreEqual(state, nullabilityInfo.WriteState);
        }
    }
}
