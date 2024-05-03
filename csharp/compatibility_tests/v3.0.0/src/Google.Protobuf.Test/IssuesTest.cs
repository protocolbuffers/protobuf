#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using Google.Protobuf.Reflection;
using UnitTest.Issues.TestProtos;
using NUnit.Framework;


namespace Google.Protobuf
{
    /// <summary>
    /// Tests for issues which aren't easily compartmentalized into other unit tests.
    /// </summary>
    public class IssuesTest
    {
        // Issue 45
        [Test]
        public void FieldCalledItem()
        {
            ItemField message = new ItemField { Item = 3 };
            FieldDescriptor field = ItemField.Descriptor.FindFieldByName("item");
            Assert.NotNull(field);
            Assert.AreEqual(3, (int)field.Accessor.GetValue(message));
        }

        [Test]
        public void ReservedNames()
        {
            var message = new ReservedNames { Types_ = 10, Descriptor_ = 20 };
            // Underscores aren't reflected in the JSON.
            Assert.AreEqual("{ \"types\": 10, \"descriptor\": 20 }", message.ToString());
        }

        [Test]
        public void JsonNameParseTest()
        {
            var settings = new JsonParser.Settings(10, TypeRegistry.FromFiles(UnittestIssuesReflection.Descriptor));
            var parser = new JsonParser(settings);

            // It is safe to use either original field name or explicitly specified json_name
            Assert.AreEqual(new TestJsonName { Name = "test", Description = "test2", Guid = "test3" },
                parser.Parse<TestJsonName>("{ \"name\": \"test\", \"desc\": \"test2\", \"guid\": \"test3\" }"));
        }

        [Test]
        public void JsonNameFormatTest()
        {
            var message = new TestJsonName { Name = "test", Description = "test2", Guid = "test3" };
            Assert.AreEqual("{ \"name\": \"test\", \"desc\": \"test2\", \"exid\": \"test3\" }",
                JsonFormatter.Default.Format(message));
        }
    }
}
