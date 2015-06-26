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
using System.Collections.Generic;
using Google.Protobuf.TestProtos;
using NUnit.Framework;

namespace Google.Protobuf.Collections
{
    /// <summary>
    /// Tests for MapField which aren't reliant on the encoded format -
    /// tests for serialization/deserialization are part of GeneratedMessageTest.
    /// </summary>
    public class MapFieldTest
    {
        // Protobuf-specific tests
        [Test]
        public void Freeze_FreezesMessages()
        {
            var message = new ForeignMessage { C = 20 };
            var map = new MapField<string, ForeignMessage> { { "x", message } };
            map.Freeze();
            Assert.IsTrue(message.IsFrozen);
        }

        [Test]
        public void Freeze_PreventsMutation()
        {
            var map = new MapField<string, string>();
            map.Freeze();
            Assert.IsTrue(map.IsFrozen);
            Assert.IsTrue(map.IsReadOnly);
            ICollection<KeyValuePair<string, string>> collection = map;
            Assert.Throws<InvalidOperationException>(() => map["x"] = "y");
            Assert.Throws<InvalidOperationException>(() => map.Add("x", "y"));
            Assert.Throws<InvalidOperationException>(() => map.Remove("x"));
            Assert.Throws<InvalidOperationException>(() => map.Clear());
            Assert.Throws<InvalidOperationException>(() => collection.Add(NewKeyValuePair("x", "y")));
            Assert.Throws<InvalidOperationException>(() => collection.Remove(NewKeyValuePair("x", "y")));
        }

        [Test]
        public void Clone_ReturnsNonFrozen()
        {
            var map = new MapField<string, string>();
            map.Freeze();
            var clone = map.Clone();
            clone.Add("x", "y");
        }

        [Test]
        public void Clone_ClonesMessages()
        {
            var message = new ForeignMessage { C = 20 };
            var map = new MapField<string, ForeignMessage> { { "x", message } };
            var clone = map.Clone();
            map["x"].C = 30;
            Assert.AreEqual(20, clone["x"].C);
        }

        [Test]
        public void Add_ForbidsNullKeys()
        {
            var map = new MapField<string, ForeignMessage>();
            Assert.Throws<ArgumentNullException>(() => map.Add(null, new ForeignMessage()));
        }

        [Test]
        public void Add_AcceptsNullMessageValues()
        {
            var map = new MapField<string, ForeignMessage>();
            map.Add("missing", null);
            Assert.IsNull(map["missing"]);
        }

        [Test]
        public void Add_ForbidsNullStringValues()
        {
            var map = new MapField<string, string>();
            Assert.Throws<ArgumentNullException>(() => map.Add("missing", null));
        }

        [Test]
        public void Add_ForbidsNullByteStringValues()
        {
            var map = new MapField<string, ByteString>();
            Assert.Throws<ArgumentNullException>(() => map.Add("missing", null));
        }

        [Test]
        public void Indexer_ForbidsNullKeys()
        {
            var map = new MapField<string, ForeignMessage>();
            Assert.Throws<ArgumentNullException>(() => map[null] = new ForeignMessage());
        }

        [Test]
        public void Indexer_AcceptsNullMessageValues()
        {
            var map = new MapField<string, ForeignMessage>();
            map["missing"] = null;
            Assert.IsNull(map["missing"]);
        }

        [Test]
        public void Indexer_ForbidsNullStringValues()
        {
            var map = new MapField<string, string>();
            Assert.Throws<ArgumentNullException>(() => map["missing"] = null);
        }

        [Test]
        public void Indexer_ForbidsNullByteStringValues()
        {
            var map = new MapField<string, ByteString>();
            Assert.Throws<ArgumentNullException>(() => map["missing"] = null);
        }

        [Test]
        public void AddPreservesInsertionOrder()
        {
            var map = new MapField<string, string>();
            map.Add("a", "v1");
            map.Add("b", "v2");
            map.Add("c", "v3");
            map.Remove("b");
            map.Add("d", "v4");
            CollectionAssert.AreEqual(new[] { "a", "c", "d" }, map.Keys);
            CollectionAssert.AreEqual(new[] { "v1", "v3", "v4" }, map.Values);
        }

        [Test]
        public void EqualityIsOrderInsensitive()
        {
            var map1 = new MapField<string, string>();
            map1.Add("a", "v1");
            map1.Add("b", "v2");

            var map2 = new MapField<string, string>();
            map2.Add("b", "v2");
            map2.Add("a", "v1");

            EqualityTester.AssertEquality(map1, map2);
        }

        [Test]
        public void EqualityIsKeySensitive()
        {
            var map1 = new MapField<string, string>();
            map1.Add("first key", "v1");
            map1.Add("second key", "v2");

            var map2 = new MapField<string, string>();
            map2.Add("third key", "v1");
            map2.Add("fourth key", "v2");

            EqualityTester.AssertInequality(map1, map2);
        }

        [Test]
        public void EqualityIsValueSensitive()
        {
            // Note: Without some care, it's a little easier than one might
            // hope to see hash collisions, but only in some environments...
            var map1 = new MapField<string, string>();
            map1.Add("a", "first value");
            map1.Add("b", "second value");

            var map2 = new MapField<string, string>();
            map2.Add("a", "third value");
            map2.Add("b", "fourth value");

            EqualityTester.AssertInequality(map1, map2);
        }

        [Test]
        public void EqualityHandlesNullValues()
        {
            var map1 = new MapField<string, ForeignMessage>();
            map1.Add("a", new ForeignMessage { C = 10 });
            map1.Add("b", null);

            var map2 = new MapField<string, ForeignMessage>();
            map2.Add("a", new ForeignMessage { C = 10 });
            map2.Add("b", null);

            EqualityTester.AssertEquality(map1, map2);
            // Check the null value isn't ignored entirely...
            Assert.IsTrue(map1.Remove("b"));
            EqualityTester.AssertInequality(map1, map2);
            map1.Add("b", new ForeignMessage());
            EqualityTester.AssertInequality(map1, map2);
            map1["b"] = null;
            EqualityTester.AssertEquality(map1, map2);
        }

        [Test]
        public void Add_Dictionary()
        {
            var map1 = new MapField<string, string>
            {
                { "x", "y" },
                { "a", "b" }
            };
            var map2 = new MapField<string, string>
            {
                { "before", "" },
                map1,
                { "after", "" }
            };
            var expected = new MapField<string, string>
            {
                { "before", "" },
                { "x", "y" },
                { "a", "b" },
                { "after", "" }
            };
            Assert.AreEqual(expected, map2);
            CollectionAssert.AreEqual(new[] { "before", "x", "a", "after" }, map2.Keys);
        }

        // General IDictionary<TKey, TValue> behavior tests
        [Test]
        public void Add_KeyAlreadyExists()
        {
            var map = new MapField<string, string>();
            map.Add("foo", "bar");
            Assert.Throws<ArgumentException>(() => map.Add("foo", "baz"));
        }

        [Test]
        public void Add_Pair()
        {
            var map = new MapField<string, string>();
            ICollection<KeyValuePair<string, string>> collection = map;
            collection.Add(NewKeyValuePair("x", "y"));
            Assert.AreEqual("y", map["x"]);
            Assert.Throws<ArgumentException>(() => collection.Add(NewKeyValuePair("x", "z")));
        }

        [Test]
        public void Contains_Pair()
        {
            var map = new MapField<string, string> { { "x", "y" } };
            ICollection<KeyValuePair<string, string>> collection = map;
            Assert.IsTrue(collection.Contains(NewKeyValuePair("x", "y")));
            Assert.IsFalse(collection.Contains(NewKeyValuePair("x", "z")));
            Assert.IsFalse(collection.Contains(NewKeyValuePair("z", "y")));
        }

        [Test]
        public void Remove_Key()
        {
            var map = new MapField<string, string>();
            map.Add("foo", "bar");
            Assert.AreEqual(1, map.Count);
            Assert.IsFalse(map.Remove("missing"));
            Assert.AreEqual(1, map.Count);
            Assert.IsTrue(map.Remove("foo"));
            Assert.AreEqual(0, map.Count);           
        }

        [Test]
        public void Remove_Pair()
        {
            var map = new MapField<string, string>();
            map.Add("foo", "bar");
            ICollection<KeyValuePair<string, string>> collection = map;
            Assert.AreEqual(1, map.Count);
            Assert.IsFalse(collection.Remove(NewKeyValuePair("wrong key", "bar")));
            Assert.AreEqual(1, map.Count);
            Assert.IsFalse(collection.Remove(NewKeyValuePair("foo", "wrong value")));
            Assert.AreEqual(1, map.Count);
            Assert.IsTrue(collection.Remove(NewKeyValuePair("foo", "bar")));
            Assert.AreEqual(0, map.Count);
            Assert.Throws<ArgumentException>(() => collection.Remove(new KeyValuePair<string, string>(null, "")));
        }

        [Test]
        public void CopyTo_Pair()
        {
            var map = new MapField<string, string>();
            map.Add("foo", "bar");
            ICollection<KeyValuePair<string, string>> collection = map;
            KeyValuePair<string, string>[] array = new KeyValuePair<string, string>[3];
            collection.CopyTo(array, 1);
            Assert.AreEqual(NewKeyValuePair("foo", "bar"), array[1]);
        }

        [Test]
        public void Clear()
        {
            var map = new MapField<string, string> { { "x", "y" } };
            Assert.AreEqual(1, map.Count);
            map.Clear();
            Assert.AreEqual(0, map.Count);
            map.Add("x", "y");
            Assert.AreEqual(1, map.Count);
        }

        [Test]
        public void Indexer_Get()
        {
            var map = new MapField<string, string> { { "x", "y" } };
            Assert.AreEqual("y", map["x"]);
            Assert.Throws<KeyNotFoundException>(() => { var ignored = map["z"]; });
        }

        [Test]
        public void Indexer_Set()
        {
            var map = new MapField<string, string>();
            map["x"] = "y";
            Assert.AreEqual("y", map["x"]);
            map["x"] = "z"; // This won't throw, unlike Add.
            Assert.AreEqual("z", map["x"]);
        }

        private static KeyValuePair<TKey, TValue> NewKeyValuePair<TKey, TValue>(TKey key, TValue value)
        {
            return new KeyValuePair<TKey, TValue>(key, value);
        }
    }
}
