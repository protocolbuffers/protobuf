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
using System.IO;
using System.Collections.Generic;
using Google.Protobuf.TestProtos;
using NUnit.Framework;
using System.Collections;
using System.Linq;

namespace Google.Protobuf.Collections
{
    /// <summary>
    /// Tests for MapField which aren't reliant on the encoded format -
    /// tests for serialization/deserialization are part of GeneratedMessageTest.
    /// </summary>
    public class MapFieldTest
    {
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
        public void NullValuesProhibited()
        {
            TestNullValues<int?>(0);
            TestNullValues("");
            TestNullValues(new TestAllTypes());
        }

        private void TestNullValues<T>(T nonNullValue)
        {
            var map = new MapField<int, T>();
            var nullValue = (T) (object) null;
            Assert.Throws<ArgumentNullException>(() => map.Add(0, nullValue));
            Assert.Throws<ArgumentNullException>(() => map[0] = nullValue);
            map.Add(1, nonNullValue);
            map[1] = nonNullValue;
        }

        [Test]
        public void Add_ForbidsNullKeys()
        {
            var map = new MapField<string, ForeignMessage>();
            Assert.Throws<ArgumentNullException>(() => map.Add(null, new ForeignMessage()));
        }

        [Test]
        public void Indexer_ForbidsNullKeys()
        {
            var map = new MapField<string, ForeignMessage>();
            Assert.Throws<ArgumentNullException>(() => map[null] = new ForeignMessage());
        }

        [Test]
        public void AddPreservesInsertionOrder()
        {
            var map = new MapField<string, string>
            {
                { "a", "v1" },
                { "b", "v2" },
                { "c", "v3" }
            };
            map.Remove("b");
            map.Add("d", "v4");
            CollectionAssert.AreEqual(new[] { "a", "c", "d" }, map.Keys);
            CollectionAssert.AreEqual(new[] { "v1", "v3", "v4" }, map.Values);
        }

        [Test]
        public void EqualityIsOrderInsensitive()
        {
            var map1 = new MapField<string, string>
            {
                { "a", "v1" },
                { "b", "v2" }
            };

            var map2 = new MapField<string, string>
            {
                { "b", "v2" },
                { "a", "v1" }
            };

            EqualityTester.AssertEquality(map1, map2);
        }

        [Test]
        public void EqualityIsKeySensitive()
        {
            var map1 = new MapField<string, string>
            {
                { "first key", "v1" },
                { "second key", "v2" }
            };

            var map2 = new MapField<string, string>
            {
                { "third key", "v1" },
                { "fourth key", "v2" }
            };

            EqualityTester.AssertInequality(map1, map2);
        }

        [Test]
        public void Equality_Simple()
        {
            var map = new MapField<string, string>();
            EqualityTester.AssertEquality(map, map);
            EqualityTester.AssertInequality(map, null);
            Assert.IsFalse(map.Equals(new object()));
        }

        [Test]
        public void EqualityIsValueSensitive()
        {
            // Note: Without some care, it's a little easier than one might
            // hope to see hash collisions, but only in some environments...
            var map1 = new MapField<string, string>
            {
                { "a", "first value" },
                { "b", "second value" }
            };

            var map2 = new MapField<string, string>
            {
                { "a", "third value" },
                { "b", "fourth value" }
            };

            EqualityTester.AssertInequality(map1, map2);
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
            var map = new MapField<string, string> { { "foo", "bar" } };
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
            var map = new MapField<string, string> { { "foo", "bar" } };
            Assert.AreEqual(1, map.Count);
            Assert.IsFalse(map.Remove("missing"));
            Assert.AreEqual(1, map.Count);
            Assert.IsTrue(map.Remove("foo"));
            Assert.AreEqual(0, map.Count);
            Assert.Throws<ArgumentNullException>(() => map.Remove(null));
        }

        [Test]
        public void Remove_Pair()
        {
            var map = new MapField<string, string> { { "foo", "bar" } };
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
            var map = new MapField<string, string> { { "foo", "bar" } };
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
            var map = new MapField<string, string> { ["x"] = "y" };
            Assert.AreEqual("y", map["x"]);
            map["x"] = "z"; // This won't throw, unlike Add.
            Assert.AreEqual("z", map["x"]);
        }

        [Test]
        public void GetEnumerator_NonGeneric()
        {
            IEnumerable map = new MapField<string, string> { { "x", "y" } };
            CollectionAssert.AreEqual(new[] { new KeyValuePair<string, string>("x", "y") },
                map.Cast<object>().ToList());
        }

        // Test for the explicitly-implemented non-generic IDictionary interface
        [Test]
        public void IDictionary_GetEnumerator()
        {
            IDictionary map = new MapField<string, string> { { "x", "y" } };
            var enumerator = map.GetEnumerator();

            // Commented assertions show an ideal situation - it looks like
            // the LinkedList enumerator doesn't throw when you ask for the current entry
            // at an inappropriate time; fixing this would be more work than it's worth.
            // Assert.Throws<InvalidOperationException>(() => enumerator.Current.GetHashCode());
            Assert.IsTrue(enumerator.MoveNext());
            Assert.AreEqual("x", enumerator.Key);
            Assert.AreEqual("y", enumerator.Value);
            Assert.AreEqual(new DictionaryEntry("x", "y"), enumerator.Current);
            Assert.AreEqual(new DictionaryEntry("x", "y"), enumerator.Entry);
            Assert.IsFalse(enumerator.MoveNext());
            // Assert.Throws<InvalidOperationException>(() => enumerator.Current.GetHashCode());
            enumerator.Reset();
            // Assert.Throws<InvalidOperationException>(() => enumerator.Current.GetHashCode());
            Assert.IsTrue(enumerator.MoveNext());
            Assert.AreEqual("x", enumerator.Key); // Assume the rest are okay
        }

        [Test]
        public void IDictionary_Add()
        {
            var map = new MapField<string, string> { { "x", "y" } };
            IDictionary dictionary = map;
            dictionary.Add("a", "b");
            Assert.AreEqual("b", map["a"]);
            Assert.Throws<ArgumentException>(() => dictionary.Add("a", "duplicate"));
            Assert.Throws<InvalidCastException>(() => dictionary.Add(new object(), "key is bad"));
            Assert.Throws<InvalidCastException>(() => dictionary.Add("value is bad", new object()));
        }

        [Test]
        public void IDictionary_Contains()
        {
            var map = new MapField<string, string> { { "x", "y" } };
            IDictionary dictionary = map;

            Assert.IsFalse(dictionary.Contains("a"));
            Assert.IsFalse(dictionary.Contains(5));
            // Surprising, but IDictionary.Contains is only about keys.
            Assert.IsFalse(dictionary.Contains(new DictionaryEntry("x", "y")));
            Assert.IsTrue(dictionary.Contains("x"));
        }

        [Test]
        public void IDictionary_Remove()
        {
            var map = new MapField<string, string> { { "x", "y" } };
            IDictionary dictionary = map;
            dictionary.Remove("a");
            Assert.AreEqual(1, dictionary.Count);
            dictionary.Remove(5);
            Assert.AreEqual(1, dictionary.Count);
            dictionary.Remove(new DictionaryEntry("x", "y"));
            Assert.AreEqual(1, dictionary.Count);
            dictionary.Remove("x");
            Assert.AreEqual(0, dictionary.Count);
            Assert.Throws<ArgumentNullException>(() => dictionary.Remove(null));
        }

        [Test]
        public void IDictionary_CopyTo()
        {
            var map = new MapField<string, string> { { "x", "y" } };
            IDictionary dictionary = map;
            var array = new DictionaryEntry[3];
            dictionary.CopyTo(array, 1);
            CollectionAssert.AreEqual(new[] { default, new DictionaryEntry("x", "y"), default }, array);
            var objectArray = new object[3];
            dictionary.CopyTo(objectArray, 1);
            CollectionAssert.AreEqual(new object[] { null, new DictionaryEntry("x", "y"), null }, objectArray);
        }

        [Test]
        public void IDictionary_IsFixedSize()
        {
            var map = new MapField<string, string> { { "x", "y" } };
            IDictionary dictionary = map;
            Assert.IsFalse(dictionary.IsFixedSize);
        }

        [Test]
        public void IDictionary_Keys()
        {
            IDictionary dictionary = new MapField<string, string> { { "x", "y" } };
            CollectionAssert.AreEqual(new[] { "x" }, dictionary.Keys);
        }

        [Test]
        public void IDictionary_Values()
        {
            IDictionary dictionary = new MapField<string, string> { { "x", "y" } };
            CollectionAssert.AreEqual(new[] { "y" }, dictionary.Values);
        }

        [Test]
        public void IDictionary_IsSynchronized()
        {
            IDictionary dictionary = new MapField<string, string> { { "x", "y" } };
            Assert.IsFalse(dictionary.IsSynchronized);
        }

        [Test]
        public void IDictionary_SyncRoot()
        {
            IDictionary dictionary = new MapField<string, string> { { "x", "y" } };
            Assert.AreSame(dictionary, dictionary.SyncRoot);
        }

        [Test]
        public void IDictionary_Indexer_Get()
        {
            IDictionary dictionary = new MapField<string, string> { { "x", "y" } };
            Assert.AreEqual("y", dictionary["x"]);
            Assert.IsNull(dictionary["a"]);
            Assert.IsNull(dictionary[5]);
            Assert.Throws<ArgumentNullException>(() => dictionary[null].GetHashCode());
        }

        [Test]
        public void IDictionary_Indexer_Set()
        {
            var map = new MapField<string, string> { { "x", "y" } };
            IDictionary dictionary = map;
            map["a"] = "b";
            Assert.AreEqual("b", map["a"]);
            map["a"] = "c";
            Assert.AreEqual("c", map["a"]);
            Assert.Throws<InvalidCastException>(() => dictionary[5] = "x");
            Assert.Throws<InvalidCastException>(() => dictionary["x"] = 5);
            Assert.Throws<ArgumentNullException>(() => dictionary[null] = "z");
            Assert.Throws<ArgumentNullException>(() => dictionary["x"] = null);
        }

        [Test]
        public void KeysReturnsLiveView()
        {
            var map = new MapField<string, string>();
            var keys = map.Keys;
            CollectionAssert.AreEqual(new string[0], keys);
            map["foo"] = "bar";
            map["x"] = "y";
            CollectionAssert.AreEqual(new[] { "foo", "x" }, keys);
        }

        [Test]
        public void ValuesReturnsLiveView()
        {
            var map = new MapField<string, string>();
            var values = map.Values;
            CollectionAssert.AreEqual(new string[0], values);
            map["foo"] = "bar";
            map["x"] = "y";
            CollectionAssert.AreEqual(new[] { "bar", "y" }, values);
        }

        // Just test keys - we know the implementation is the same for values
        [Test]
        public void ViewsAreReadOnly()
        {
            var map = new MapField<string, string>();
            var keys = map.Keys;
            Assert.IsTrue(keys.IsReadOnly);
            Assert.Throws<NotSupportedException>(() => keys.Clear());
            Assert.Throws<NotSupportedException>(() => keys.Remove("a"));
            Assert.Throws<NotSupportedException>(() => keys.Add("a"));
        }

        // Just test keys - we know the implementation is the same for values
        [Test]
        public void ViewCopyTo()
        {
            var map = new MapField<string, string> { { "foo", "bar" }, { "x", "y" } };
            var keys = map.Keys;
            var array = new string[4];
            Assert.Throws<ArgumentException>(() => keys.CopyTo(array, 3));
            Assert.Throws<ArgumentOutOfRangeException>(() => keys.CopyTo(array, -1));
            keys.CopyTo(array, 1);
            CollectionAssert.AreEqual(new[] { null, "foo", "x", null }, array);
        }

        // Just test keys - we know the implementation is the same for values
        [Test]
        public void NonGenericViewCopyTo()
        {
            IDictionary map = new MapField<string, string> { { "foo", "bar" }, { "x", "y" } };
            ICollection keys = map.Keys;
            // Note the use of the Array type here rather than string[]
            Array array = new string[4];
            Assert.Throws<ArgumentException>(() => keys.CopyTo(array, 3));
            Assert.Throws<ArgumentOutOfRangeException>(() => keys.CopyTo(array, -1));
            keys.CopyTo(array, 1);
            CollectionAssert.AreEqual(new[] { null, "foo", "x", null }, array);
        }

        [Test]
        public void KeysContains()
        {
            var map = new MapField<string, string> { { "foo", "bar" }, { "x", "y" } };
            var keys = map.Keys;
            Assert.IsTrue(keys.Contains("foo"));
            Assert.IsFalse(keys.Contains("bar")); // It's a value!
            Assert.IsFalse(keys.Contains("1"));
            // Keys can't be null, so we should prevent contains check
            Assert.Throws<ArgumentNullException>(() => keys.Contains(null));
        }

        [Test]
        public void KeysCopyTo()
        {
            var map = new MapField<string, string> { { "foo", "bar" }, { "x", "y" } };
            var keys = map.Keys.ToArray(); // Uses CopyTo internally
            CollectionAssert.AreEquivalent(new[] { "foo", "x" }, keys);
        }

        [Test]
        public void ValuesContains()
        {
            var map = new MapField<string, string> { { "foo", "bar" }, { "x", "y" } };
            var values = map.Values;
            Assert.IsTrue(values.Contains("bar"));
            Assert.IsFalse(values.Contains("foo")); // It's a key!
            Assert.IsFalse(values.Contains("1"));
            // Values can be null, so this makes sense
            Assert.IsFalse(values.Contains(null));
        }

        [Test]
        public void ValuesCopyTo()
        {
            var map = new MapField<string, string> { { "foo", "bar" }, { "x", "y" } };
            var values = map.Values.ToArray(); // Uses CopyTo internally
            CollectionAssert.AreEquivalent(new[] { "bar", "y" }, values);
        }

        [Test]
        public void ToString_StringToString()
        {
            var map = new MapField<string, string> { { "foo", "bar" }, { "x", "y" } };
            Assert.AreEqual("{ \"foo\": \"bar\", \"x\": \"y\" }", map.ToString());
        }

        [Test]
        public void ToString_UnsupportedKeyType()
        {
            var map = new MapField<byte, string> { { 10, "foo" } };
            Assert.Throws<ArgumentException>(() => map.ToString());
        }

        [Test]
        public void NaNValuesComparedBitwise()
        {
            var map1 = new MapField<string, double>
            {
                { "x", SampleNaNs.Regular },
                { "y", SampleNaNs.SignallingFlipped }
            };

            var map2 = new MapField<string, double>
            {
                { "x", SampleNaNs.Regular },
                { "y", SampleNaNs.PayloadFlipped }
            };

            var map3 = new MapField<string, double>
            {
                { "x", SampleNaNs.Regular },
                { "y", SampleNaNs.SignallingFlipped }
            };

            EqualityTester.AssertInequality(map1, map2);
            EqualityTester.AssertEquality(map1, map3);
            Assert.True(map1.Values.Contains(SampleNaNs.SignallingFlipped));
            Assert.False(map2.Values.Contains(SampleNaNs.SignallingFlipped));
        }

        // This wouldn't usually happen, as protos can't use doubles as map keys,
        // but let's be consistent.
        [Test]
        public void NaNKeysComparedBitwise()
        {
            var map = new MapField<double, string>
            {
                { SampleNaNs.Regular, "x" },
                { SampleNaNs.SignallingFlipped, "y" }
            };
            Assert.AreEqual("x", map[SampleNaNs.Regular]);
            Assert.AreEqual("y", map[SampleNaNs.SignallingFlipped]);
            Assert.False(map.TryGetValue(SampleNaNs.PayloadFlipped, out _));
        }

        [Test]
        public void AddEntriesFrom_CodedInputStream()
        {
            // map will have string key and string value
            var keyTag = WireFormat.MakeTag(1, WireFormat.WireType.LengthDelimited);
            var valueTag = WireFormat.MakeTag(2, WireFormat.WireType.LengthDelimited);

            var memoryStream = new MemoryStream();
            var output = new CodedOutputStream(memoryStream);
            output.WriteLength(20);  // total of keyTag + key + valueTag + value
            output.WriteTag(keyTag);
            output.WriteString("the_key");
            output.WriteTag(valueTag);
            output.WriteString("the_value");
            output.Flush();

            var field = new MapField<string,string>();
            var mapCodec = new MapField<string,string>.Codec(FieldCodec.ForString(keyTag, ""), FieldCodec.ForString(valueTag, ""), 10);
            var input = new CodedInputStream(memoryStream.ToArray());

            // test the legacy overload of AddEntriesFrom that takes a CodedInputStream
            field.AddEntriesFrom(input, mapCodec);
            CollectionAssert.AreEquivalent(new[] { "the_key" }, field.Keys);
            CollectionAssert.AreEquivalent(new[] { "the_value" }, field.Values);
            Assert.IsTrue(input.IsAtEnd);
        }

        [Test]
        public void AddEntriesFrom_CodedInputStream_MissingKey()
        {
            // map will have string key and string value
            var keyTag = WireFormat.MakeTag(1, WireFormat.WireType.LengthDelimited);
            var valueTag = WireFormat.MakeTag(2, WireFormat.WireType.LengthDelimited);

            var memoryStream = new MemoryStream();
            var output = new CodedOutputStream(memoryStream);
            output.WriteLength(11);  // total of valueTag + value
            output.WriteTag(valueTag);
            output.WriteString("the_value");
            output.Flush();

            var field = new MapField<string, string>();
            var mapCodec = new MapField<string, string>.Codec(FieldCodec.ForString(keyTag, ""), FieldCodec.ForString(valueTag, ""), 10);
            var input = new CodedInputStream(memoryStream.ToArray());

            field.AddEntriesFrom(input, mapCodec);
            CollectionAssert.AreEquivalent(new[] { "" }, field.Keys);
            CollectionAssert.AreEquivalent(new[] { "the_value" }, field.Values);
            Assert.IsTrue(input.IsAtEnd);
        }

        [Test]
        public void IDictionaryKeys_Equals_IReadOnlyDictionaryKeys()
        {
            var map = new MapField<string, string> { { "foo", "bar" }, { "x", "y" } };
            CollectionAssert.AreEquivalent(((IDictionary<string, string>)map).Keys, ((IReadOnlyDictionary<string, string>)map).Keys);
        }

        [Test]
        public void IDictionaryValues_Equals_IReadOnlyDictionaryValues()
        {
            var map = new MapField<string, string> { { "foo", "bar" }, { "x", "y" } };
            CollectionAssert.AreEquivalent(((IDictionary<string, string>)map).Values, ((IReadOnlyDictionary<string, string>)map).Values);
        }

        private static KeyValuePair<TKey, TValue> NewKeyValuePair<TKey, TValue>(TKey key, TValue value)
        {
            return new KeyValuePair<TKey, TValue>(key, value);
        }
    }
}
