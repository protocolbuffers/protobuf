#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2016 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion


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
    }
}
