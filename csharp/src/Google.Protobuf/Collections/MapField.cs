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

using Google.Protobuf.Compatibility;
using Google.Protobuf.Reflection;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace Google.Protobuf.Collections
{
    /// <summary>
    /// Representation of a map field in a Protocol Buffer message.
    /// </summary>
    /// <typeparam name="TKey">Key type in the map. Must be a type supported by Protocol Buffer map keys.</typeparam>
    /// <typeparam name="TValue">Value type in the map. Must be a type supported by Protocol Buffers.</typeparam>
    /// <remarks>
    /// <para>
    /// For string keys, the equality comparison is provided by <see cref="StringComparer.Ordinal" />.
    /// </para>
    /// <para>
    /// Null values are not permitted in the map, either for wrapper types or regular messages.
    /// If a map is deserialized from a data stream and the value is missing from an entry, a default value
    /// is created instead. For primitive types, that is the regular default value (0, the empty string and so
    /// on); for message types, an empty instance of the message is created, as if the map entry contained a 0-length
    /// encoded value for the field.
    /// </para>
    /// <para>
    /// This implementation does not generally prohibit the use of key/value types which are not
    /// supported by Protocol Buffers (e.g. using a key type of <code>byte</code>) but nor does it guarantee
    /// that all operations will work in such cases.
    /// </para>
    /// <para>
    /// The order in which entries are returned when iterating over this object is undefined, and may change
    /// in future versions.
    /// </para>
    /// </remarks>
    public sealed class MapField<TKey, TValue> : IDeepCloneable<MapField<TKey, TValue>>, IDictionary<TKey, TValue>, IEquatable<MapField<TKey, TValue>>, IDictionary
#if !NET35
        , IReadOnlyDictionary<TKey, TValue>
#endif
    {
        private static readonly EqualityComparer<TValue> ValueEqualityComparer = ProtobufEqualityComparers.GetEqualityComparer<TValue>();
        private static readonly EqualityComparer<TKey> KeyEqualityComparer = ProtobufEqualityComparers.GetEqualityComparer<TKey>();

        // TODO: Don't create the map/list until we have an entry. (Assume many maps will be empty.)
        private readonly Dictionary<TKey, LinkedListNode<KeyValuePair<TKey, TValue>>> map =
            new Dictionary<TKey, LinkedListNode<KeyValuePair<TKey, TValue>>>(KeyEqualityComparer);
        private readonly LinkedList<KeyValuePair<TKey, TValue>> list = new LinkedList<KeyValuePair<TKey, TValue>>();

        /// <summary>
        /// Creates a deep clone of this object.
        /// </summary>
        /// <returns>
        /// A deep clone of this object.
        /// </returns>
        public MapField<TKey, TValue> Clone()
        {
            var clone = new MapField<TKey, TValue>();
            // Keys are never cloneable. Values might be.
            if (typeof(IDeepCloneable<TValue>).IsAssignableFrom(typeof(TValue)))
            {
                foreach (var pair in list)
                {
                    clone.Add(pair.Key, ((IDeepCloneable<TValue>)pair.Value).Clone());
                }
            }
            else
            {
                // Nothing is cloneable, so we don't need to worry.
                clone.Add(this);
            }
            return clone;
        }

        /// <summary>
        /// Adds the specified key/value pair to the map.
        /// </summary>
        /// <remarks>
        /// This operation fails if the key already exists in the map. To replace an existing entry, use the indexer.
        /// </remarks>
        /// <param name="key">The key to add</param>
        /// <param name="value">The value to add.</param>
        /// <exception cref="System.ArgumentException">The given key already exists in map.</exception>
        public void Add(TKey key, TValue value)
        {
            // Validation of arguments happens in ContainsKey and the indexer
            if (ContainsKey(key))
            {
                throw new ArgumentException("Key already exists in map", nameof(key));
            }
            this[key] = value;
        }

        /// <summary>
        /// Determines whether the specified key is present in the map.
        /// </summary>
        /// <param name="key">The key to check.</param>
        /// <returns><c>true</c> if the map contains the given key; <c>false</c> otherwise.</returns>
        public bool ContainsKey(TKey key)
        {
            ProtoPreconditions.CheckNotNullUnconstrained(key, nameof(key));
            return map.ContainsKey(key);
        }

        private bool ContainsValue(TValue value) =>
            list.Any(pair => ValueEqualityComparer.Equals(pair.Value, value));

        /// <summary>
        /// Removes the entry identified by the given key from the map.
        /// </summary>
        /// <param name="key">The key indicating the entry to remove from the map.</param>
        /// <returns><c>true</c> if the map contained the given key before the entry was removed; <c>false</c> otherwise.</returns>
        public bool Remove(TKey key)
        {
            ProtoPreconditions.CheckNotNullUnconstrained(key, nameof(key));
            LinkedListNode<KeyValuePair<TKey, TValue>> node;
            if (map.TryGetValue(key, out node))
            {
                map.Remove(key);
                node.List.Remove(node);
                return true;
            }
            else
            {
                return false;
            }
        }

        /// <summary>
        /// Gets the value associated with the specified key.
        /// </summary>
        /// <param name="key">The key whose value to get.</param>
        /// <param name="value">When this method returns, the value associated with the specified key, if the key is found;
        /// otherwise, the default value for the type of the <paramref name="value"/> parameter.
        /// This parameter is passed uninitialized.</param>
        /// <returns><c>true</c> if the map contains an element with the specified key; otherwise, <c>false</c>.</returns>
        public bool TryGetValue(TKey key, out TValue value)
        {
            LinkedListNode<KeyValuePair<TKey, TValue>> node;
            if (map.TryGetValue(key, out node))
            {
                value = node.Value.Value;
                return true;
            }
            else
            {
                value = default(TValue);
                return false;
            }
        }

        /// <summary>
        /// Gets or sets the value associated with the specified key.
        /// </summary>
        /// <param name="key">The key of the value to get or set.</param>
        /// <exception cref="KeyNotFoundException">The property is retrieved and key does not exist in the collection.</exception>
        /// <returns>The value associated with the specified key. If the specified key is not found,
        /// a get operation throws a <see cref="KeyNotFoundException"/>, and a set operation creates a new element with the specified key.</returns>
        public TValue this[TKey key]
        {
            get
            {
                ProtoPreconditions.CheckNotNullUnconstrained(key, nameof(key));
                TValue value;
                if (TryGetValue(key, out value))
                {
                    return value;
                }
                throw new KeyNotFoundException();
            }
            set
            {
                ProtoPreconditions.CheckNotNullUnconstrained(key, nameof(key));
                // value == null check here is redundant, but avoids boxing.
                if (value == null)
                {
                    ProtoPreconditions.CheckNotNullUnconstrained(value, nameof(value));
                }
                LinkedListNode<KeyValuePair<TKey, TValue>> node;
                var pair = new KeyValuePair<TKey, TValue>(key, value);
                if (map.TryGetValue(key, out node))
                {
                    node.Value = pair;
                }
                else
                {
                    node = list.AddLast(pair);
                    map[key] = node;
                }
            }
        }

        /// <summary>
        /// Gets a collection containing the keys in the map.
        /// </summary>
        public ICollection<TKey> Keys { get { return new MapView<TKey>(this, pair => pair.Key, ContainsKey); } }

        /// <summary>
        /// Gets a collection containing the values in the map.
        /// </summary>
        public ICollection<TValue> Values { get { return new MapView<TValue>(this, pair => pair.Value, ContainsValue); } }

        /// <summary>
        /// Adds the specified entries to the map. The keys and values are not automatically cloned.
        /// </summary>
        /// <param name="entries">The entries to add to the map.</param>
        public void Add(IDictionary<TKey, TValue> entries)
        {
            ProtoPreconditions.CheckNotNull(entries, nameof(entries));
            foreach (var pair in entries)
            {
                Add(pair.Key, pair.Value);
            }
        }

        /// <summary>
        /// Returns an enumerator that iterates through the collection.
        /// </summary>
        /// <returns>
        /// An enumerator that can be used to iterate through the collection.
        /// </returns>
        public IEnumerator<KeyValuePair<TKey, TValue>> GetEnumerator()
        {
            return list.GetEnumerator();
        }

        /// <summary>
        /// Returns an enumerator that iterates through a collection.
        /// </summary>
        /// <returns>
        /// An <see cref="T:System.Collections.IEnumerator" /> object that can be used to iterate through the collection.
        /// </returns>
        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        /// <summary>
        /// Adds the specified item to the map.
        /// </summary>
        /// <param name="item">The item to add to the map.</param>
        void ICollection<KeyValuePair<TKey, TValue>>.Add(KeyValuePair<TKey, TValue> item)
        {
            Add(item.Key, item.Value);
        }

        /// <summary>
        /// Removes all items from the map.
        /// </summary>
        public void Clear()
        {
            list.Clear();
            map.Clear();
        }

        /// <summary>
        /// Determines whether map contains an entry equivalent to the given key/value pair.
        /// </summary>
        /// <param name="item">The key/value pair to find.</param>
        /// <returns></returns>
        bool ICollection<KeyValuePair<TKey, TValue>>.Contains(KeyValuePair<TKey, TValue> item)
        {
            TValue value;
            return TryGetValue(item.Key, out value) && ValueEqualityComparer.Equals(item.Value, value);
        }

        /// <summary>
        /// Copies the key/value pairs in this map to an array.
        /// </summary>
        /// <param name="array">The array to copy the entries into.</param>
        /// <param name="arrayIndex">The index of the array at which to start copying values.</param>
        void ICollection<KeyValuePair<TKey, TValue>>.CopyTo(KeyValuePair<TKey, TValue>[] array, int arrayIndex)
        {
            list.CopyTo(array, arrayIndex);
        }

        /// <summary>
        /// Removes the specified key/value pair from the map.
        /// </summary>
        /// <remarks>Both the key and the value must be found for the entry to be removed.</remarks>
        /// <param name="item">The key/value pair to remove.</param>
        /// <returns><c>true</c> if the key/value pair was found and removed; <c>false</c> otherwise.</returns>
        bool ICollection<KeyValuePair<TKey, TValue>>.Remove(KeyValuePair<TKey, TValue> item)
        {
            if (item.Key == null)
            {
                throw new ArgumentException("Key is null", nameof(item));
            }
            LinkedListNode<KeyValuePair<TKey, TValue>> node;
            if (map.TryGetValue(item.Key, out node) &&
                EqualityComparer<TValue>.Default.Equals(item.Value, node.Value.Value))
            {
                map.Remove(item.Key);
                node.List.Remove(node);
                return true;
            }
            else
            {
                return false;
            }
        }

        /// <summary>
        /// Gets the number of elements contained in the map.
        /// </summary>
        public int Count { get { return list.Count; } }

        /// <summary>
        /// Gets a value indicating whether the map is read-only.
        /// </summary>
        public bool IsReadOnly { get { return false; } }

        /// <summary>
        /// Determines whether the specified <see cref="System.Object" />, is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="System.Object" /> to compare with this instance.</param>
        /// <returns>
        ///   <c>true</c> if the specified <see cref="System.Object" /> is equal to this instance; otherwise, <c>false</c>.
        /// </returns>
        public override bool Equals(object other)
        {
            return Equals(other as MapField<TKey, TValue>);
        }

        /// <summary>
        /// Returns a hash code for this instance.
        /// </summary>
        /// <returns>
        /// A hash code for this instance, suitable for use in hashing algorithms and data structures like a hash table. 
        /// </returns>
        public override int GetHashCode()
        {
            var keyComparer = KeyEqualityComparer;
            var valueComparer = ValueEqualityComparer;
            int hash = 0;
            foreach (var pair in list)
            {
                hash ^= keyComparer.GetHashCode(pair.Key) * 31 + valueComparer.GetHashCode(pair.Value);
            }
            return hash;
        }

        /// <summary>
        /// Compares this map with another for equality.
        /// </summary>
        /// <remarks>
        /// The order of the key/value pairs in the maps is not deemed significant in this comparison.
        /// </remarks>
        /// <param name="other">The map to compare this with.</param>
        /// <returns><c>true</c> if <paramref name="other"/> refers to an equal map; <c>false</c> otherwise.</returns>
        public bool Equals(MapField<TKey, TValue> other)
        {
            if (other == null)
            {
                return false;
            }
            if (other == this)
            {
                return true;
            }
            if (other.Count != this.Count)
            {
                return false;
            }
            var valueComparer = ValueEqualityComparer;
            foreach (var pair in this)
            {
                TValue value;
                if (!other.TryGetValue(pair.Key, out value))
                {
                    return false;
                }
                if (!valueComparer.Equals(value, pair.Value))
                {
                    return false;
                }
            }
            return true;
        }

        /// <summary>
        /// Adds entries to the map from the given stream.
        /// </summary>
        /// <remarks>
        /// It is assumed that the stream is initially positioned after the tag specified by the codec.
        /// This method will continue reading entries from the stream until the end is reached, or
        /// a different tag is encountered.
        /// </remarks>
        /// <param name="input">Stream to read from</param>
        /// <param name="codec">Codec describing how the key/value pairs are encoded</param>
        public void AddEntriesFrom(CodedInputStream input, Codec codec)
        {
            var adapter = new Codec.MessageAdapter(codec);
            do
            {
                adapter.Reset();
                input.ReadMessage(adapter);
                this[adapter.Key] = adapter.Value;
            } while (input.MaybeConsumeTag(codec.MapTag));
        }

        /// <summary>
        /// Writes the contents of this map to the given coded output stream, using the specified codec
        /// to encode each entry.
        /// </summary>
        /// <param name="output">The output stream to write to.</param>
        /// <param name="codec">The codec to use for each entry.</param>
        public void WriteTo(CodedOutputStream output, Codec codec)
        {
            var message = new Codec.MessageAdapter(codec);
            foreach (var entry in list)
            {
                message.Key = entry.Key;
                message.Value = entry.Value;
                output.WriteTag(codec.MapTag);
                output.WriteMessage(message);
            }
        }

        /// <summary>
        /// Calculates the size of this map based on the given entry codec.
        /// </summary>
        /// <param name="codec">The codec to use to encode each entry.</param>
        /// <returns></returns>
        public int CalculateSize(Codec codec)
        {
            if (Count == 0)
            {
                return 0;
            }
            var message = new Codec.MessageAdapter(codec);
            int size = 0;
            foreach (var entry in list)
            {
                message.Key = entry.Key;
                message.Value = entry.Value;
                size += CodedOutputStream.ComputeRawVarint32Size(codec.MapTag);
                size += CodedOutputStream.ComputeMessageSize(message);
            }
            return size;
        }

        /// <summary>
        /// Returns a string representation of this repeated field, in the same
        /// way as it would be represented by the default JSON formatter.
        /// </summary>
        public override string ToString()
        {
            var writer = new StringWriter();
            JsonFormatter.Default.WriteDictionary(writer, this);
            return writer.ToString();
        }

        #region IDictionary explicit interface implementation
        void IDictionary.Add(object key, object value)
        {
            Add((TKey)key, (TValue)value);
        }

        bool IDictionary.Contains(object key)
        {
            if (!(key is TKey))
            {
                return false;
            }
            return ContainsKey((TKey)key);
        }

        IDictionaryEnumerator IDictionary.GetEnumerator()
        {
            return new DictionaryEnumerator(GetEnumerator());
        }

        void IDictionary.Remove(object key)
        {
            ProtoPreconditions.CheckNotNull(key, nameof(key));
            if (!(key is TKey))
            {
                return;
            }
            Remove((TKey)key);
        }

        void ICollection.CopyTo(Array array, int index)
        {
            // This is ugly and slow as heck, but with any luck it will never be used anyway.
            ICollection temp = this.Select(pair => new DictionaryEntry(pair.Key, pair.Value)).ToList();
            temp.CopyTo(array, index);
        }

        bool IDictionary.IsFixedSize { get { return false; } }

        ICollection IDictionary.Keys { get { return (ICollection)Keys; } }

        ICollection IDictionary.Values { get { return (ICollection)Values; } }

        bool ICollection.IsSynchronized { get { return false; } }

        object ICollection.SyncRoot { get { return this; } }

        object IDictionary.this[object key]
        {
            get
            {
                ProtoPreconditions.CheckNotNull(key, nameof(key));
                if (!(key is TKey))
                {
                    return null;
                }
                TValue value;
                TryGetValue((TKey)key, out value);
                return value;
            }

            set
            {
                this[(TKey)key] = (TValue)value;
            }
        }
        #endregion

        #region IReadOnlyDictionary explicit interface implementation
#if !NET35
        IEnumerable<TKey> IReadOnlyDictionary<TKey, TValue>.Keys => Keys;

        IEnumerable<TValue> IReadOnlyDictionary<TKey, TValue>.Values => Values;
#endif
        #endregion

        private class DictionaryEnumerator : IDictionaryEnumerator
        {
            private readonly IEnumerator<KeyValuePair<TKey, TValue>> enumerator;

            internal DictionaryEnumerator(IEnumerator<KeyValuePair<TKey, TValue>> enumerator)
            {
                this.enumerator = enumerator;
            }

            public bool MoveNext()
            {
                return enumerator.MoveNext();
            }

            public void Reset()
            {
                enumerator.Reset();
            }

            public object Current { get { return Entry; } }
            public DictionaryEntry Entry { get { return new DictionaryEntry(Key, Value); } }
            public object Key { get { return enumerator.Current.Key; } }
            public object Value { get { return enumerator.Current.Value; } }
        }

        /// <summary>
        /// A codec for a specific map field. This contains all the information required to encode and
        /// decode the nested messages.
        /// </summary>
        public sealed class Codec
        {
            private readonly FieldCodec<TKey> keyCodec;
            private readonly FieldCodec<TValue> valueCodec;
            private readonly uint mapTag;

            /// <summary>
            /// Creates a new entry codec based on a separate key codec and value codec,
            /// and the tag to use for each map entry.
            /// </summary>
            /// <param name="keyCodec">The key codec.</param>
            /// <param name="valueCodec">The value codec.</param>
            /// <param name="mapTag">The map tag to use to introduce each map entry.</param>
            public Codec(FieldCodec<TKey> keyCodec, FieldCodec<TValue> valueCodec, uint mapTag)
            {
                this.keyCodec = keyCodec;
                this.valueCodec = valueCodec;
                this.mapTag = mapTag;
            }

            /// <summary>
            /// The tag used in the enclosing message to indicate map entries.
            /// </summary>
            internal uint MapTag { get { return mapTag; } }

            /// <summary>
            /// A mutable message class, used for parsing and serializing. This
            /// delegates the work to a codec, but implements the <see cref="IMessage"/> interface
            /// for interop with <see cref="CodedInputStream"/> and <see cref="CodedOutputStream"/>.
            /// This is nested inside Codec as it's tightly coupled to the associated codec,
            /// and it's simpler if it has direct access to all its fields.
            /// </summary>
            internal class MessageAdapter : IMessage
            {
                private static readonly byte[] ZeroLengthMessageStreamData = new byte[] { 0 };

                private readonly Codec codec;
                internal TKey Key { get; set; }
                internal TValue Value { get; set; }

                internal MessageAdapter(Codec codec)
                {
                    this.codec = codec;
                }

                internal void Reset()
                {
                    Key = codec.keyCodec.DefaultValue;
                    Value = codec.valueCodec.DefaultValue;
                }

                public void MergeFrom(CodedInputStream input)
                {
                    uint tag;
                    while ((tag = input.ReadTag()) != 0)
                    {
                        if (tag == codec.keyCodec.Tag)
                        {
                            Key = codec.keyCodec.Read(input);
                        }
                        else if (tag == codec.valueCodec.Tag)
                        {
                            Value = codec.valueCodec.Read(input);
                        }
                        else 
                        {
                            input.SkipLastField();
                        }
                    }

                    // Corner case: a map entry with a key but no value, where the value type is a message.
                    // Read it as if we'd seen an input stream with no data (i.e. create a "default" message).
                    if (Value == null)
                    {
                        Value = codec.valueCodec.Read(new CodedInputStream(ZeroLengthMessageStreamData));
                    }
                }

                public void WriteTo(CodedOutputStream output)
                {
                    codec.keyCodec.WriteTagAndValue(output, Key);
                    codec.valueCodec.WriteTagAndValue(output, Value);
                }

                public int CalculateSize()
                {
                    return codec.keyCodec.CalculateSizeWithTag(Key) + codec.valueCodec.CalculateSizeWithTag(Value);
                }

                MessageDescriptor IMessage.Descriptor { get { return null; } }
            }
        }

        private class MapView<T> : ICollection<T>, ICollection
        {
            private readonly MapField<TKey, TValue> parent;
            private readonly Func<KeyValuePair<TKey, TValue>, T> projection;
            private readonly Func<T, bool> containsCheck;

            internal MapView(
                MapField<TKey, TValue> parent,
                Func<KeyValuePair<TKey, TValue>, T> projection,
                Func<T, bool> containsCheck)
            {
                this.parent = parent;
                this.projection = projection;
                this.containsCheck = containsCheck;
            }

            public int Count { get { return parent.Count; } }

            public bool IsReadOnly { get { return true; } }

            public bool IsSynchronized { get { return false; } }

            public object SyncRoot { get { return parent; } }

            public void Add(T item)
            {
                throw new NotSupportedException();
            }

            public void Clear()
            {
                throw new NotSupportedException();
            }

            public bool Contains(T item)
            {
                return containsCheck(item);
            }

            public void CopyTo(T[] array, int arrayIndex)
            {
                if (arrayIndex < 0)
                {
                    throw new ArgumentOutOfRangeException(nameof(arrayIndex));
                }
                if (arrayIndex + Count > array.Length)
                {
                    throw new ArgumentException("Not enough space in the array", nameof(array));
                }
                foreach (var item in this)
                {
                    array[arrayIndex++] = item;
                }
            }

            public IEnumerator<T> GetEnumerator()
            {
                return parent.list.Select(projection).GetEnumerator();
            }

            public bool Remove(T item)
            {
                throw new NotSupportedException();
            }

            IEnumerator IEnumerable.GetEnumerator()
            {
                return GetEnumerator();
            }

            public void CopyTo(Array array, int index)
            {
                if (index < 0)
                {
                    throw new ArgumentOutOfRangeException(nameof(index));
                }
                if (index + Count > array.Length)
                {
                    throw new ArgumentException("Not enough space in the array", nameof(array));
                }
                foreach (var item in this)
                {
                    array.SetValue(item, index++);
                }
            }
        }
    }
}
