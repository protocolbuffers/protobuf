#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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
using Google.Protobuf.WellKnownTypes;

namespace Google.Protobuf.Util
{
    public class FieldMaskUtilTest
    {
        [Test]
        public void IsValid()
        {
            Assert.IsTrue(FieldMaskUtil.IsValid<NestedTestAllTypes>("payload"));
            Assert.IsFalse(FieldMaskUtil.IsValid<NestedTestAllTypes>("nonexist"));
            Assert.IsTrue(FieldMaskUtil.IsValid<NestedTestAllTypes>("payload.single_int32"));
            Assert.IsTrue(FieldMaskUtil.IsValid<NestedTestAllTypes>("payload.repeated_int32"));
            Assert.IsTrue(FieldMaskUtil.IsValid<NestedTestAllTypes>("payload.single_nested_message"));
            Assert.IsTrue(FieldMaskUtil.IsValid<NestedTestAllTypes>("payload.repeated_nested_message"));
            Assert.IsFalse(FieldMaskUtil.IsValid<NestedTestAllTypes>("payload.nonexist"));

            Assert.IsTrue(FieldMaskUtil.IsValid<NestedTestAllTypes>(FieldMaskUtil.FromString("payload")));
            Assert.IsFalse(FieldMaskUtil.IsValid<NestedTestAllTypes>(FieldMaskUtil.FromString("nonexist")));
            Assert.IsFalse(FieldMaskUtil.IsValid<NestedTestAllTypes>(FieldMaskUtil.FromString("payload,nonexist")));

            Assert.IsTrue(FieldMaskUtil.IsValid(NestedTestAllTypes.Descriptor, "payload"));
            Assert.IsFalse(FieldMaskUtil.IsValid(NestedTestAllTypes.Descriptor, "nonexist"));

            Assert.IsTrue(FieldMaskUtil.IsValid(NestedTestAllTypes.Descriptor, FieldMaskUtil.FromString("payload")));
            Assert.IsFalse(FieldMaskUtil.IsValid(NestedTestAllTypes.Descriptor, FieldMaskUtil.FromString("nonexist")));

            Assert.IsTrue(FieldMaskUtil.IsValid<NestedTestAllTypes>("payload.single_nested_message.bb"));

            // Repeated fields cannot have sub-paths.
            Assert.IsFalse(FieldMaskUtil.IsValid<NestedTestAllTypes>("payload.repeated_nested_message.bb"));

            // Non-message fields cannot have sub-paths.
            Assert.IsFalse(FieldMaskUtil.IsValid<NestedTestAllTypes>("payload.single_int32.bb"));
        }

        [Test]
        [TestCase(new string[] { }, "")]
        [TestCase(new string[] {"foo"}, "foo")]
        [TestCase(new string[] {"foo", "bar"}, "foo,bar")]
        [TestCase(new string[] {"", "foo", "", "bar", ""}, "foo,bar")]
        public void ToString(string[] input, string expectedOutput)
        {
            FieldMask mask = new FieldMask();
            mask.Paths.AddRange(input);
            Assert.AreEqual(expectedOutput, FieldMaskUtil.ToString(mask));
        }

        [Test]
        [TestCase("", new string[] { })]
        [TestCase("foo", new string[] {"foo"})]
        [TestCase("foo,bar.baz", new string[] {"foo", "bar.baz"})]
        [TestCase(",foo,,bar,", new string[] {"foo", "bar"})]
        public void FromString(string input, string[] expectedOutput)
        {
            FieldMask mask = FieldMaskUtil.FromString(input);
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
            Assert.DoesNotThrow(() => FieldMaskUtil.FromString<NestedTestAllTypes>(",payload"));
            Assert.Throws<InvalidProtocolBufferException>(() => FieldMaskUtil.FromString<NestedTestAllTypes>("payload,nonexist"));
        }

        [Test]
        [TestCase(new int[] { }, new string[] { })]
        [TestCase(new int[] {TestAllTypes.SingleInt32FieldNumber}, new string[] {"single_int32"})]
        [TestCase(new int[] {TestAllTypes.SingleInt32FieldNumber, TestAllTypes.SingleInt64FieldNumber}, new string[] {"single_int32", "single_int64"})]
        public void FromFieldNumbers(int[] input, string[] expectedOutput)
        {
            FieldMask mask = FieldMaskUtil.FromFieldNumbers<TestAllTypes>(input);
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
                FieldMaskUtil.FromFieldNumbers<TestAllTypes>(invalidFieldNumber);
            });
        }

        [Test]
        [TestCase(new string[] { }, "")]
        [TestCase(new string[] {"foo"}, "foo")]
        [TestCase(new string[] {"foo.bar_baz", ""}, "foo.barBaz")]
        [TestCase(new string[] {"foo", "bar_baz"}, "foo,barBaz")]
        public void ToJsonString(string[] input, string expectedOutput)
        {
            FieldMask mask = new FieldMask();
            mask.Paths.AddRange(input);
            Assert.AreEqual(expectedOutput, FieldMaskUtil.ToJsonString(mask));
        }

        [Test]
        [TestCase("", new string[] { })]
        [TestCase("foo", new string[] {"foo"})]
        [TestCase("foo.barBaz", new string[] {"foo.bar_baz"})]
        [TestCase("foo,barBaz", new string[] {"foo", "bar_baz"})]
        public void FromJsonString(string input, string[] expectedOutput)
        {
            FieldMask mask = FieldMaskUtil.FromJsonString(input);
            Assert.AreEqual(expectedOutput.Length, mask.Paths.Count);
            for (int i = 0; i < expectedOutput.Length; i++)
            {
                Assert.AreEqual(expectedOutput[i], mask.Paths[i]);
            }
        }

        [Test]
        public void Union()
        {
            // Only test a simple case here and expect
            // {@link FieldMaskTreeTest#AddFieldPath} to cover all scenarios.
            FieldMask mask1 = FieldMaskUtil.FromString("foo,bar.baz,bar.quz");
            FieldMask mask2 = FieldMaskUtil.FromString("foo.bar,bar");
            FieldMask result = FieldMaskUtil.Union(mask1, mask2);
            Assert.Contains("bar", result.Paths);
            Assert.Contains("foo", result.Paths);
        }

        [Test]
        public void Union_usingVarArgs()
        {
            FieldMask mask1 = FieldMaskUtil.FromString("foo");
            FieldMask mask2 = FieldMaskUtil.FromString("foo.bar,bar.quz");
            FieldMask mask3 = FieldMaskUtil.FromString("bar.quz");
            FieldMask mask4 = FieldMaskUtil.FromString("bar");
            FieldMask result = FieldMaskUtil.Union(mask1, mask2, mask3, mask4);
            Assert.Contains("bar", result.Paths);
            Assert.Contains("foo", result.Paths);
        }

        [Test]
        public void Intersection()
        {
            // Only test a simple case here and expect
            // {@link FieldMaskTreeTest#IntersectFieldPath} to cover all scenarios.
            FieldMask mask1 = FieldMaskUtil.FromString("foo,bar.baz,bar.quz");
            FieldMask mask2 = FieldMaskUtil.FromString("foo.bar,bar");
            FieldMask result = FieldMaskUtil.Intersection(mask1, mask2);
            Assert.Contains("foo.bar", result.Paths);
            Assert.Contains("bar.baz", result.Paths);
            Assert.Contains("bar.quz", result.Paths);
        }

        [Test]
        public void Merge()
        {
            // Only test a simple case here and expect
            // {@link FieldMaskTreeTest#Merge} to cover all scenarios.
            NestedTestAllTypes source = new NestedTestAllTypes
            {
                Payload = new TestAllTypes
                {
                    SingleInt32 = 1234
                }
            };
            NestedTestAllTypes destination = new NestedTestAllTypes();
            FieldMaskUtil.Merge(FieldMaskUtil.FromString("payload"), source, destination);
            Assert.AreEqual(1234, destination.Payload.SingleInt32);

            destination = new NestedTestAllTypes
            {
                Payload = new TestAllTypes
                {
                    SingleInt32 = 4321,
                    SingleInt64 = 5678
                }
            };
            FieldMaskUtil.Merge(FieldMaskUtil.FromString("payload"), source, destination);
            Assert.AreEqual(1234, destination.Payload.SingleInt32);
            Assert.AreEqual(5678, destination.Payload.SingleInt64);
        }
    }
}
