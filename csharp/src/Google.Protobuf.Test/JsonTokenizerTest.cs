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
using NUnit.Framework;
using System;
using System.IO;

namespace Google.Protobuf
{
    public class JsonTokenizerTest
    {
        [Test]
        public void EmptyObjectValue()
        {
            AssertTokens("{}", JsonToken.StartObject, JsonToken.EndObject);
        }

        [Test]
        public void EmptyArrayValue()
        {
            AssertTokens("[]", JsonToken.StartArray, JsonToken.EndArray);
        }

        [Test]
        [TestCase("foo", "foo")]
        [TestCase("tab\\t", "tab\t")]
        [TestCase("line\\nfeed", "line\nfeed")]
        [TestCase("carriage\\rreturn", "carriage\rreturn")]
        [TestCase("back\\bspace", "back\bspace")]
        [TestCase("form\\ffeed", "form\ffeed")]
        [TestCase("escaped\\/slash", "escaped/slash")]
        [TestCase("escaped\\\\backslash", "escaped\\backslash")]
        [TestCase("escaped\\\"quote", "escaped\"quote")]
        [TestCase("foo {}[] bar", "foo {}[] bar")]
        [TestCase("foo\\u09aFbar", "foo\u09afbar")] // Digits, upper hex, lower hex
        [TestCase("ab\ud800\udc00cd", "ab\ud800\udc00cd")]
        [TestCase("ab\\ud800\\udc00cd", "ab\ud800\udc00cd")]
        public void StringValue(string json, string expectedValue)
        {
            AssertTokensNoReplacement("\"" + json + "\"", JsonToken.Value(expectedValue));
        }

        // Valid surrogate pairs, with mixed escaping. These test cases can't be expressed
        // using TestCase as they have no valid UTF-8 representation.
        // It's unclear exactly how we should handle a mixture of escaped or not: that can't
        // come from UTF-8 text, but could come from a .NET string. For the moment,
        // treat it as valid in the obvious way.
        [Test]
        public void MixedSurrogatePairs()
        {
            string expected = "\ud800\udc00";
            AssertTokens("'\\ud800\udc00'", JsonToken.Value(expected));
            AssertTokens("'\ud800\\udc00'", JsonToken.Value(expected));
        }

        [Test]
        public void ObjectDepth()
        {
            string json = "{ \"foo\": { \"x\": 1, \"y\": [ 0 ] } }";
            var tokenizer = JsonTokenizer.FromTextReader(new StringReader(json));
            // If we had more tests like this, I'd introduce a helper method... but for one test, it's not worth it.
            Assert.AreEqual(0, tokenizer.ObjectDepth);
            Assert.AreEqual(JsonToken.StartObject, tokenizer.Next());
            Assert.AreEqual(1, tokenizer.ObjectDepth);
            Assert.AreEqual(JsonToken.Name("foo"), tokenizer.Next());
            Assert.AreEqual(1, tokenizer.ObjectDepth);
            Assert.AreEqual(JsonToken.StartObject, tokenizer.Next());
            Assert.AreEqual(2, tokenizer.ObjectDepth);
            Assert.AreEqual(JsonToken.Name("x"), tokenizer.Next());
            Assert.AreEqual(2, tokenizer.ObjectDepth);
            Assert.AreEqual(JsonToken.Value(1), tokenizer.Next());
            Assert.AreEqual(2, tokenizer.ObjectDepth);
            Assert.AreEqual(JsonToken.Name("y"), tokenizer.Next());
            Assert.AreEqual(2, tokenizer.ObjectDepth);
            Assert.AreEqual(JsonToken.StartArray, tokenizer.Next());
            Assert.AreEqual(2, tokenizer.ObjectDepth); // Depth hasn't changed in array
            Assert.AreEqual(JsonToken.Value(0), tokenizer.Next());
            Assert.AreEqual(2, tokenizer.ObjectDepth);
            Assert.AreEqual(JsonToken.EndArray, tokenizer.Next());
            Assert.AreEqual(2, tokenizer.ObjectDepth);
            Assert.AreEqual(JsonToken.EndObject, tokenizer.Next());
            Assert.AreEqual(1, tokenizer.ObjectDepth);
            Assert.AreEqual(JsonToken.EndObject, tokenizer.Next());
            Assert.AreEqual(0, tokenizer.ObjectDepth);
            Assert.AreEqual(JsonToken.EndDocument, tokenizer.Next());
            Assert.AreEqual(0, tokenizer.ObjectDepth);
        }

        [Test]
        public void ObjectDepth_WithPushBack()
        {
            string json = "{}";
            var tokenizer = JsonTokenizer.FromTextReader(new StringReader(json));
            Assert.AreEqual(0, tokenizer.ObjectDepth);
            var token = tokenizer.Next();
            Assert.AreEqual(1, tokenizer.ObjectDepth);
            // When we push back a "start object", we should effectively be back to the previous depth.
            tokenizer.PushBack(token);
            Assert.AreEqual(0, tokenizer.ObjectDepth);
            // Read the same token again, and get back to depth 1
            token = tokenizer.Next();
            Assert.AreEqual(1, tokenizer.ObjectDepth);

            // Now the same in reverse, with EndObject
            token = tokenizer.Next();
            Assert.AreEqual(0, tokenizer.ObjectDepth);
            tokenizer.PushBack(token);
            Assert.AreEqual(1, tokenizer.ObjectDepth);
            tokenizer.Next();
            Assert.AreEqual(0, tokenizer.ObjectDepth);
        }

        [Test]
        [TestCase("embedded tab\t")]
        [TestCase("embedded CR\r")]
        [TestCase("embedded LF\n")]
        [TestCase("embedded bell\u0007")]
        [TestCase("bad escape\\a")]
        [TestCase("incomplete escape\\")]
        [TestCase("incomplete Unicode escape\\u000")]
        [TestCase("invalid Unicode escape\\u000H")]
        // Surrogate pair handling, both in raw .NET strings and escaped. We only need
        // to detect this in strings, as non-ASCII characters anywhere other than in strings
        // will already lead to parsing errors.
        [TestCase("\\ud800")]
        [TestCase("\\udc00")]
        [TestCase("\\ud800x")]
        [TestCase("\\udc00x")]
        [TestCase("\\udc00\\ud800y")]
        public void InvalidStringValue(string json)
        {
            AssertThrowsAfter("\"" + json + "\"");
        }

        // Tests for invalid strings that can't be expressed in attributes,
        // as the constants can't be expressed as UTF-8 strings.
        [Test]
        public void InvalidSurrogatePairs()
        {
            AssertThrowsAfter("\"\ud800x\"");
            AssertThrowsAfter("\"\udc00y\"");
            AssertThrowsAfter("\"\udc00\ud800y\"");
        }

        [Test]
        [TestCase("0", 0)]
        [TestCase("-0", 0)] // We don't distinguish between positive and negative 0
        [TestCase("1", 1)]
        [TestCase("-1", -1)]
        // From here on, assume leading sign is okay...
        [TestCase("1.125", 1.125)]
        [TestCase("1.0", 1)]
        [TestCase("1e5", 100000)]
        [TestCase("1e000000", 1)] // Weird, but not prohibited by the spec
        [TestCase("1E5", 100000)]
        [TestCase("1e+5", 100000)]
        [TestCase("1E-5", 0.00001)]
        [TestCase("123E-2", 1.23)]
        [TestCase("123.45E3", 123450)]
        [TestCase("   1   ", 1)]
        public void NumberValue(string json, double expectedValue)
        {
            AssertTokens(json, JsonToken.Value(expectedValue));
        }

        [Test]
        [TestCase("00")]
        [TestCase(".5")]
        [TestCase("1.")]
        [TestCase("1e")]
        [TestCase("1e-")]
        [TestCase("--")]
        [TestCase("--1")]
        [TestCase("-1.7977e308")]
        [TestCase("1.7977e308")]
        public void InvalidNumberValue(string json)
        {
            AssertThrowsAfter(json);
        }

        [Test]
        [TestCase("nul")]
        [TestCase("nothing")]
        [TestCase("truth")]
        [TestCase("fALSEhood")]
        public void InvalidLiterals(string json)
        {
            AssertThrowsAfter(json);
        }

        [Test]
        public void NullValue()
        {
            AssertTokens("null", JsonToken.Null);
        }

        [Test]
        public void TrueValue()
        {
            AssertTokens("true", JsonToken.True);
        }

        [Test]
        public void FalseValue()
        {
            AssertTokens("false", JsonToken.False);
        }

        [Test]
        public void SimpleObject()
        {
            AssertTokens("{'x': 'y'}",
                JsonToken.StartObject, JsonToken.Name("x"), JsonToken.Value("y"), JsonToken.EndObject);
        }
        
        [Test]
        [TestCase("[10, 20", 3)]
        [TestCase("[10,", 2)]
        [TestCase("[10:20]", 2)]
        [TestCase("[", 1)]
        [TestCase("[,", 1)]
        [TestCase("{", 1)]
        [TestCase("{,", 1)]
        [TestCase("{[", 1)]
        [TestCase("{{", 1)]
        [TestCase("{0", 1)]
        [TestCase("{null", 1)]
        [TestCase("{false", 1)]
        [TestCase("{true", 1)]
        [TestCase("}", 0)]
        [TestCase("]", 0)]
        [TestCase(",", 0)]
        [TestCase("'foo' 'bar'", 1)]
        [TestCase(":", 0)]
        [TestCase("'foo", 0)] // Incomplete string
        [TestCase("{ 'foo' }", 2)]
        [TestCase("{ x:1", 1)] // Property names must be quoted
        [TestCase("{]", 1)]
        [TestCase("[}", 1)]
        [TestCase("[1,", 2)]
        [TestCase("{'x':0]", 3)]
        [TestCase("{ 'foo': }", 2)]
        [TestCase("{ 'foo':'bar', }", 3)]
        public void InvalidStructure(string json, int expectedValidTokens)
        {
            // Note: we don't test that the earlier tokens are exactly as expected,
            // partly because that's hard to parameterize.
            var reader = new StringReader(json.Replace('\'', '"'));
            var tokenizer = JsonTokenizer.FromTextReader(reader);
            for (int i = 0; i < expectedValidTokens; i++)
            {
                Assert.IsNotNull(tokenizer.Next());
            }
            Assert.Throws<InvalidJsonException>(() => tokenizer.Next());
        }

        [Test]
        public void ArrayMixedType()
        {
            AssertTokens("[1, 'foo', null, false, true, [2], {'x':'y' }]",
                JsonToken.StartArray,
                JsonToken.Value(1),
                JsonToken.Value("foo"),
                JsonToken.Null,
                JsonToken.False,
                JsonToken.True,
                JsonToken.StartArray,
                JsonToken.Value(2),
                JsonToken.EndArray,
                JsonToken.StartObject,
                JsonToken.Name("x"),
                JsonToken.Value("y"),
                JsonToken.EndObject,
                JsonToken.EndArray);
        }

        [Test]
        public void ObjectMixedType()
        {
            AssertTokens(@"{'a': 1, 'b': 'bar', 'c': null, 'd': false, 'e': true, 
                           'f': [2], 'g': {'x':'y' }}",
                JsonToken.StartObject,
                JsonToken.Name("a"),
                JsonToken.Value(1),
                JsonToken.Name("b"),
                JsonToken.Value("bar"),
                JsonToken.Name("c"),
                JsonToken.Null,
                JsonToken.Name("d"),
                JsonToken.False,
                JsonToken.Name("e"),
                JsonToken.True,
                JsonToken.Name("f"),
                JsonToken.StartArray,
                JsonToken.Value(2),
                JsonToken.EndArray,
                JsonToken.Name("g"),
                JsonToken.StartObject,
                JsonToken.Name("x"),
                JsonToken.Value("y"),
                JsonToken.EndObject,
                JsonToken.EndObject);
        }

        [Test]
        public void NextAfterEndDocumentThrows()
        {
            var tokenizer = JsonTokenizer.FromTextReader(new StringReader("null"));
            Assert.AreEqual(JsonToken.Null, tokenizer.Next());
            Assert.AreEqual(JsonToken.EndDocument, tokenizer.Next());
            Assert.Throws<InvalidOperationException>(() => tokenizer.Next());
        }

        [Test]
        public void CanPushBackEndDocument()
        {
            var tokenizer = JsonTokenizer.FromTextReader(new StringReader("null"));
            Assert.AreEqual(JsonToken.Null, tokenizer.Next());
            Assert.AreEqual(JsonToken.EndDocument, tokenizer.Next());
            tokenizer.PushBack(JsonToken.EndDocument);
            Assert.AreEqual(JsonToken.EndDocument, tokenizer.Next());
            Assert.Throws<InvalidOperationException>(() => tokenizer.Next());
        }

        [Test]
        [TestCase("{ 'skip': 0, 'next': 1")]
        [TestCase("{ 'skip': [0, 1, 2], 'next': 1")]
        [TestCase("{ 'skip': 'x', 'next': 1")]
        [TestCase("{ 'skip': ['x', 'y'], 'next': 1")]
        [TestCase("{ 'skip': {'a': 0}, 'next': 1")]
        [TestCase("{ 'skip': {'a': [0, {'b':[]}]}, 'next': 1")]
        public void SkipValue(string json)
        {
            var tokenizer = JsonTokenizer.FromTextReader(new StringReader(json.Replace('\'', '"')));
            Assert.AreEqual(JsonToken.StartObject, tokenizer.Next());
            Assert.AreEqual("skip", tokenizer.Next().StringValue);
            tokenizer.SkipValue();
            Assert.AreEqual("next", tokenizer.Next().StringValue);
        }
       
        /// <summary>
        /// Asserts that the specified JSON is tokenized into the given sequence of tokens.
        /// All apostrophes are first converted to double quotes, allowing any tests
        /// that don't need to check actual apostrophe handling to use apostrophes in the JSON, avoiding
        /// messy string literal escaping. The "end document" token is not specified in the list of 
        /// expected tokens, but is implicit.
        /// </summary>
        private static void AssertTokens(string json, params JsonToken[] expectedTokens)
        {
            AssertTokensNoReplacement(json.Replace('\'', '"'), expectedTokens);
        }

        /// <summary>
        /// Asserts that the specified JSON is tokenized into the given sequence of tokens.
        /// Unlike <see cref="AssertTokens(string, JsonToken[])"/>, this does not perform any character
        /// replacement on the specified JSON, and should be used when the text contains apostrophes which
        /// are expected to be used *as* apostrophes. The "end document" token is not specified in the list of 
        /// expected tokens, but is implicit.
        /// </summary>
        private static void AssertTokensNoReplacement(string json, params JsonToken[] expectedTokens)
        {
            var reader = new StringReader(json);
            var tokenizer = JsonTokenizer.FromTextReader(reader);
            for (int i = 0; i < expectedTokens.Length; i++)
            {
                var actualToken = tokenizer.Next();
                if (actualToken == JsonToken.EndDocument)
                {
                    Assert.Fail("Expected {0} but reached end of token stream", expectedTokens[i]);
                }
                Assert.AreEqual(expectedTokens[i], actualToken);
            }
            var finalToken = tokenizer.Next();
            if (finalToken != JsonToken.EndDocument)
            {
                Assert.Fail("Expected token stream to be exhausted; received {0}", finalToken);
            }
        }

        private static void AssertThrowsAfter(string json, params JsonToken[] expectedTokens)
        {
            var reader = new StringReader(json);
            var tokenizer = JsonTokenizer.FromTextReader(reader);
            for (int i = 0; i < expectedTokens.Length; i++)
            {
                var actualToken = tokenizer.Next();
                if (actualToken == JsonToken.EndDocument)
                {
                    Assert.Fail("Expected {0} but reached end of document", expectedTokens[i]);
                }
                Assert.AreEqual(expectedTokens[i], actualToken);
            }
            Assert.Throws<InvalidJsonException>(() => tokenizer.Next());
        }
    }
}
