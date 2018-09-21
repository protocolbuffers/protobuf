#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2016 Google Inc.  All rights reserved.
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

using System;
using Google.Protobuf.TestProtos;
using NUnit.Framework;

namespace Google.Protobuf.WellKnownTypes
{
    public class FieldMaskTest
    {
        [Test]
        [TestCase("foo__bar")]
        [TestCase("foo_3_ar")]
        [TestCase("fooBar")]
        public void ToString_Invalid(string input)
        {
            var mask = new FieldMask { Paths = { input } };
            var text = mask.ToString();
            // More specific test below
            Assert.That(text, Does.Contain("@warning"));
            Assert.That(text, Does.Contain(input));
        }

        [Test]
        public void ToString_Invalid_Precise()
        {
            var mask = new FieldMask { Paths = { "x", "foo__bar", @"x\y" } };
            Assert.AreEqual(
                "{ \"@warning\": \"Invalid FieldMask\", \"paths\": [ \"x\", \"foo__bar\", \"x\\\\y\" ] }",
                mask.ToString());
        }

        [Test]
        public void IsValid()
        {
            Assert.IsTrue(FieldMask.IsValid<NestedTestAllTypes>("payload"));
            Assert.IsFalse(FieldMask.IsValid<NestedTestAllTypes>("nonexist"));
            Assert.IsTrue(FieldMask.IsValid<NestedTestAllTypes>("payload.single_int32"));
            Assert.IsTrue(FieldMask.IsValid<NestedTestAllTypes>("payload.repeated_int32"));
            Assert.IsTrue(FieldMask.IsValid<NestedTestAllTypes>("payload.single_nested_message"));
            Assert.IsTrue(FieldMask.IsValid<NestedTestAllTypes>("payload.repeated_nested_message"));
            Assert.IsFalse(FieldMask.IsValid<NestedTestAllTypes>("payload.nonexist"));

            Assert.IsTrue(FieldMask.IsValid<NestedTestAllTypes>(FieldMask.FromString("payload")));
            Assert.IsFalse(FieldMask.IsValid<NestedTestAllTypes>(FieldMask.FromString("nonexist")));
            Assert.IsFalse(FieldMask.IsValid<NestedTestAllTypes>(FieldMask.FromString("payload,nonexist")));

            Assert.IsTrue(FieldMask.IsValid(NestedTestAllTypes.Descriptor, "payload"));
            Assert.IsFalse(FieldMask.IsValid(NestedTestAllTypes.Descriptor, "nonexist"));

            Assert.IsTrue(FieldMask.IsValid(NestedTestAllTypes.Descriptor, FieldMask.FromString("payload")));
            Assert.IsFalse(FieldMask.IsValid(NestedTestAllTypes.Descriptor, FieldMask.FromString("nonexist")));

            Assert.IsTrue(FieldMask.IsValid<NestedTestAllTypes>("payload.single_nested_message.bb"));

            // Repeated fields cannot have sub-paths.
            Assert.IsFalse(FieldMask.IsValid<NestedTestAllTypes>("payload.repeated_nested_message.bb"));

            // Non-message fields cannot have sub-paths.
            Assert.IsFalse(FieldMask.IsValid<NestedTestAllTypes>("payload.single_int32.bb"));
        }

        [Test]
        [TestCase(new string[] { }, "\"\"")]
        [TestCase(new string[] { "foo" }, "\"foo\"")]
        [TestCase(new string[] { "foo", "bar" }, "\"foo,bar\"")]
        [TestCase(new string[] { "", "foo", "", "bar", "" }, "\",foo,,bar,\"")]
        public void ToString(string[] input, string expectedOutput)
        {
            FieldMask mask = new FieldMask();
            mask.Paths.AddRange(input);
            Assert.AreEqual(expectedOutput, mask.ToString());
        }

        [Test]
        [TestCase("", new string[] { })]
        [TestCase("foo", new string[] { "foo" })]
        [TestCase("foo,bar.baz", new string[] { "foo", "bar.baz" })]
        [TestCase(",foo,,bar,", new string[] { "foo", "bar" })]
        public void FromString(string input, string[] expectedOutput)
        {
            FieldMask mask = FieldMask.FromString(input);
            Assert.AreEqual(expectedOutput.Length, mask.Paths.Count);
            for (int i = 0; i < expectedOutput.Length; i++)
            {
                Assert.AreEqual(expectedOutput[i], mask.Paths[i]);
            }
        }

        [Test]
        public void FromString_Validated()
        {
            // Check whether the field paths are valid if a class parameter is provided.
            Assert.DoesNotThrow(() => FieldMask.FromString<NestedTestAllTypes>(",payload"));
            Assert.Throws<InvalidProtocolBufferException>(() => FieldMask.FromString<NestedTestAllTypes>("payload,nonexist"));
        }

        [Test]
        [TestCase(new int[] { }, new string[] { })]
        [TestCase(new int[] { TestAllTypes.SingleInt32FieldNumber }, new string[] { "single_int32" })]
        [TestCase(new int[] { TestAllTypes.SingleInt32FieldNumber, TestAllTypes.SingleInt64FieldNumber }, new string[] { "single_int32", "single_int64" })]
        public void FromFieldNumbers(int[] input, string[] expectedOutput)
        {
            FieldMask mask = FieldMask.FromFieldNumbers<TestAllTypes>(input);
            Assert.AreEqual(expectedOutput.Length, mask.Paths.Count);
            for (int i = 0; i < expectedOutput.Length; i++)
            {
                Assert.AreEqual(expectedOutput[i], mask.Paths[i]);
            }
        }

        [Test]
        public void FromFieldNumbers_Invalid()
        {
            Assert.Throws<ArgumentNullException>(() =>
            {
                int invalidFieldNumber = 1000;
                FieldMask.FromFieldNumbers<TestAllTypes>(invalidFieldNumber);
            });
        }

        [Test]
        [TestCase(new string[] { }, "\"\"")]
        [TestCase(new string[] { "foo" }, "\"foo\"")]
        [TestCase(new string[] { "foo", "bar" }, "\"foo,bar\"")]
        [TestCase(new string[] { "", "foo", "", "bar", "" }, "\",foo,bar\"")]
        public void Normalize(string[] input, string expectedOutput)
        {
            FieldMask mask = new FieldMask();
            mask.Paths.AddRange(input);
            FieldMask result = mask.Normalize();
            Assert.AreEqual(expectedOutput, result.ToString());
        }

        [Test]
        public void Union()
        {
            // Only test a simple case here and expect
            // {@link FieldMaskTreeTest#AddFieldPath} to cover all scenarios.
            FieldMask mask1 = FieldMask.FromString("foo,bar.baz,bar.quz");
            FieldMask mask2 = FieldMask.FromString("foo.bar,bar");
            FieldMask result = mask1.Union(mask2);
            Assert.AreEqual(2, result.Paths.Count);
            Assert.Contains("bar", result.Paths);
            Assert.Contains("foo", result.Paths);
            Assert.That(result.Paths, Has.No.Member("bar.baz"));
            Assert.That(result.Paths, Has.No.Member("bar.quz"));
            Assert.That(result.Paths, Has.No.Member("foo.bar"));
        }

        [Test]
        public void Union_UsingVarArgs()
        {
            FieldMask mask1 = FieldMask.FromString("foo");
            FieldMask mask2 = FieldMask.FromString("foo.bar,bar.quz");
            FieldMask mask3 = FieldMask.FromString("bar.quz");
            FieldMask mask4 = FieldMask.FromString("bar");
            FieldMask result = mask1.Union(mask2, mask3, mask4);
            Assert.AreEqual(2, result.Paths.Count);
            Assert.Contains("bar", result.Paths);
            Assert.Contains("foo", result.Paths);
            Assert.That(result.Paths, Has.No.Member("foo.bar"));
            Assert.That(result.Paths, Has.No.Member("bar.quz"));
        }

        [Test]
        public void Intersection()
        {
            // Only test a simple case here and expect
            // {@link FieldMaskTreeTest#IntersectFieldPath} to cover all scenarios.
            FieldMask mask1 = FieldMask.FromString("foo,bar.baz,bar.quz");
            FieldMask mask2 = FieldMask.FromString("foo.bar,bar");
            FieldMask result = mask1.Intersection(mask2);
            Assert.AreEqual(3, result.Paths.Count);
            Assert.Contains("foo.bar", result.Paths);
            Assert.Contains("bar.baz", result.Paths);
            Assert.Contains("bar.quz", result.Paths);
            Assert.That(result.Paths, Has.No.Member("foo"));
            Assert.That(result.Paths, Has.No.Member("bar"));
        }

        [Test]
        public void Merge()
        {
            // Only test a simple case here and expect
            // {@link FieldMaskTreeTest#Merge} to cover all scenarios.
            FieldMask fieldMask = FieldMask.FromString("payload");
            NestedTestAllTypes source = new NestedTestAllTypes
            {
                Payload = new TestAllTypes
                {
                    SingleInt32 = 1234,
                    SingleFixed64 = 4321
                }
            };
            NestedTestAllTypes destination = new NestedTestAllTypes();
            fieldMask.Merge(source, destination);
            Assert.AreEqual(1234, destination.Payload.SingleInt32);
            Assert.AreEqual(4321, destination.Payload.SingleFixed64);

            destination = new NestedTestAllTypes
            {
                Payload = new TestAllTypes
                {
                    SingleInt32 = 4321,
                    SingleInt64 = 5678
                }
            };
            fieldMask.Merge(source, destination);
            Assert.AreEqual(1234, destination.Payload.SingleInt32);
            Assert.AreEqual(5678, destination.Payload.SingleInt64);
            Assert.AreEqual(4321, destination.Payload.SingleFixed64);
        }
    }
}
