#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

using Google.Protobuf.Reflection;
using UnitTest.Issues.TestProtos;
using NUnit.Framework;
using static UnitTest.Issues.TestProtos.OneofMerging.Types;

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

        [Test]
        public void OneofMerging()
        {
            var message1 = new OneofMerging { Nested = new Nested { X = 10 } };
            var message2 = new OneofMerging { Nested = new Nested { Y = 20 } };
            var expected = new OneofMerging { Nested = new Nested { X = 10, Y = 20 } };

            var merged = message1.Clone();
            merged.MergeFrom(message2);
            Assert.AreEqual(expected, merged);
        }
    }
}
