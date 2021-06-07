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

using Google.Protobuf.TestProtos;
using NUnit.Framework;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Google.Protobuf
{
    /// <summary>
    /// Unit tests for MessageParser.
    /// </summary>
    public class MessageParserTest
    {
        [Test]
        public void ParseJsonUnknownField_NotIgnored()
        {
            var parser = new MessageParser<TestAllTypes>(() => new TestAllTypes());
            string json = "{ \"unknownField\": 10, \"singleString\": \"x\" }";
            Assert.Throws<InvalidProtocolBufferException>(() => parser.ParseJson(json));
        }

        [Test]
        [TestCase("5")]
        [TestCase("\"text\"")]
        [TestCase("[0, 1, 2]")]
        [TestCase("{ \"a\": { \"b\": 10 } }")]
        public void ParseJsonUnknownField_Ignored(string value)
        {
            var parser = new MessageParser<TestAllTypes>(() => new TestAllTypes()).WithDiscardUnknownFields(true);
            string json = "{ \"unknownField\": " + value + ", \"singleString\": \"x\" }";
            var actual = parser.ParseJson(json);
            var expected = new TestAllTypes { SingleString = "x" };
            Assert.AreEqual(expected, actual);
        }
    }
}
