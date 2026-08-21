#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using Google.Protobuf.Compatibility;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Security;

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
    [DebuggerDisplay("Count = {Count}")]
    [DebuggerTypeProxy(typeof(MapField<,>.MapFieldDebugView))]
    public sealed class MapField<TKey, TValue> : IDeepCloneable<MapField<TKey, TValue>>, IDictionary<TKey, TValue>, IEquatable<MapField<TKey, TValue>>, IDictionary, IReadOnlyDictionary<TKey, TValue>
    {
        private static readonly EqualityComparer<TValue> ValueEqualityComparer = ProtobufEqualityComparers.GetEqualityComparer<TValue>();
        private static readonly EqualityComparer<TKey> KeyEqualityComparer = ProtobufEqualityComparers.GetEqualityComparer<TKey>();

        // TODO: Don't create the map/list until we have an entry. (Assume many maps will be empty.)
        private readonly Dictionary<TKey, LinkedListNode<KeyValuePair<TKey, TValue>>> map = new(KeyEqualityComparer);
        private readonly LinkedList<KeyValuePair<TKey, TValue>> list = new();

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
            if (map.TryGetValue(key, out LinkedListNode<KeyValuePair<TKey, TValue>> node))
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
            if (map.TryGetValue(key, out LinkedListNode<KeyValuePair<TKey, TValue>> node))
            {
                value = node.Value.Value;
                return true;
            }
            else
            {
                value = default;
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
                if (TryGetValue(key, out TValue value))
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
                var pair = new KeyValuePair<TKey, TValue>(key, value);
                if (map.TryGetValue(key, out LinkedListNode<KeyValuePair<TKey, TValue>> node))
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
        public ICollection<TKey> Keys => new MapView<TKey>(this, pair => pair.Key, ContainsKey);

        /// <summary>
        /// Gets a collection containing the values in the map.
        /// </summary>
        public ICollection<TValue> Values => new MapView<TValue>(this, pair => pair.Value, ContainsValue);

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
        /// Adds the specified entries to the map, replacing any existing entries with the same keys.
        /// The keys and values are not automatically cloned.
        /// </summary>
        /// <remarks>This method primarily exists to be called from MergeFrom methods in generated classes for messages.</remarks>
        /// <param name="entries">The entries to add to the map.</param>
        public void MergeFrom(IDictionary<TKey, TValue> entries)
        {
            ProtoPreconditions.CheckNotNull(entries, nameof(entries));
            foreach (var pair in entries)
            {
                this[pair.Key] = pair.Value;
            }
        }

        /// <summary>
        /// Returns an enumerator that iterates through the collection.
        /// </summary>
        /// <returns>
        /// An enumerator that can be used to iterate through the collection.
        /// </returns>
        public IEnumerator<KeyValuePair<TKey, TValue>> GetEnumerator() => list.GetEnumerator();

        /// <summary>
        /// Returns an enumerator that iterates through a collection.
        /// </summary>
        /// <returns>
        /// An <see cref="T:System.Collections.IEnumerator" /> object that can be used to iterate through the collection.
        /// </returns>
        IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();

        /// <summary>
        /// Adds the specified item to the map.
        /// </summary>
        /// <param name="item">The item to add to the map.</param>
        void ICollection<KeyValuePair<TKey, TValue>>.Add(KeyValuePair<TKey, TValue> item) => Add(item.Key, item.Value);

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
        bool ICollection<KeyValuePair<TKey, TValue>>.Contains(KeyValuePair<TKey, TValue> item) =>
            TryGetValue(item.Key, out TValue value) && ValueEqualityComparer.Equals(item.Value, value);

        /// <summary>
        /// Copies the key/value pairs in this map to an array.
        /// </summary>
        /// <param name="array">The array to copy the entries into.</param>
        /// <param name="arrayIndex">The index of the array at which to start copying values.</param>
        void ICollection<KeyValuePair<TKey, TValue>>.CopyTo(KeyValuePair<TKey, TValue>[] array, int arrayIndex) =>
            list.CopyTo(array, arrayIndex);

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
            if (map.TryGetValue(item.Key, out LinkedListNode<KeyValuePair<TKey, TValue>> node) &&
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
        public int Count => list.Count;

        /// <summary>
        /// Gets a value indicating whether the map is read-only.
        /// </summary>
        public bool IsReadOnly => false;

        /// <summary>
        /// Determines whether the specified <see cref="System.Object" />, is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="System.Object" /> to compare with this instance.</param>
        /// <returns>
        ///   <c>true</c> if the specified <see cref="System.Object" /> is equal to this instance; otherwise, <c>false</c>.
        /// </returns>
        public override bool Equals(object other) => Equals(other as MapField<TKey, TValue>);

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
                if (!other.TryGetValue(pair.Key, out TValue value))
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
            ParseContext.Initialize(input, out ParseContext ctx);
            try
            {
                AddEntriesFrom(ref ctx, codec);
            }
            finally
            {
                ctx.CopyStateTo(input);
            }
        }

        /// <summary>
        /// Adds entries to the map from the given parse context.
        /// </summary>
        /// <remarks>
        /// It is assumed that the input is initially positioned after the tag specified by the codec.
        /// This method will continue reading entries from the input until the end is reached, or
        /// a different tag is encountered.
        /// </remarks>
        /// <param name="ctx">Input to read from</param>
        /// <param name="codec">Codec describing how the key/value pairs are encoded</param>
        [SecuritySafeCritical]
        public void AddEntriesFrom(ref ParseContext ctx, Codec codec)
        {
            do
            {
                KeyValuePair<TKey, TValue> entry = ParsingPrimitivesMessages.ReadMapEntry(ref ctx, codec);
                this[entry.Key] = entry.Value;
            } while (ParsingPrimitives.MaybeConsumeTag(ref ctx.buffer, ref ctx.state, codec.MapTag));
        }

        /// <summary>
        /// Writes the contents of this map to the given coded output stream, using the specified codec
        /// to encode each entry.
        /// </summary>
        /// <param name="output">The output stream to write to.</param>
        /// <param name="codec">The codec to use for each entry.</param>
        public void WriteTo(CodedOutputStream output, Codec codec)
        {
            WriteContext.Initialize(output, out WriteContext ctx);
            try
            {
                IEnumerable<KeyValuePair<TKey, TValue>> listToWrite = list;

                if (output.Deterministic)
                {
                    listToWrite = GetSortedListCopy(list);
                }
                WriteTo(ref ctx, codec, listToWrite);
            }
            finally
            {
                ctx.CopyStateTo(output);
            }
        }

        internal IEnumerable<KeyValuePair<TKey, TValue>> GetSortedListCopy(IEnumerable<KeyValuePair<TKey, TValue>> listToSort)
        {
            // We can't sort the list in place, as that would invalidate the linked list.
            // Instead, we create a new list, sort that, and then write it out.
            var listToWrite = new List<KeyValuePair<TKey, TValue>>(listToSort);
            listToWrite.Sort((pair1, pair2) =>
            {
                if (typeof(TKey) == typeof(string))
                {
                    // Use Ordinal, otherwise Comparer<string>.Default uses StringComparer.CurrentCulture
                    return StringComparer.Ordinal.Compare(pair1.Key.ToString(), pair2.Key.ToString());
                }
                return Comparer<TKey>.Default.Compare(pair1.Key, pair2.Key);
            });
            return listToWrite;
        }

        /// <summary>
        /// Writes the contents of this map to the given write context, using the specified codec
        /// to encode each entry.
        /// </summary>
        /// <param name="ctx">The write context to write to.</param>
        /// <param name="codec">The codec to use for each entry.</param>
        [SecuritySafeCritical]
        public void WriteTo(ref WriteContext ctx, Codec codec)
        {
            IEnumerable<KeyValuePair<TKey, TValue>> listToWrite = list;
            if (ctx.state.CodedOutputStream?.Deterministic ?? false)
            {
                listToWrite = GetSortedListCopy(list);
            }
            WriteTo(ref ctx, codec, listToWrite);
        }

        [SecuritySafeCritical]
        private void WriteTo(ref WriteContext ctx, Codec codec, IEnumerable<KeyValuePair<TKey, TValue>> listKvp)
        {
            foreach (var entry in listKvp)
            {
                ctx.WriteTag(codec.MapTag);

                WritingPrimitives.WriteLength(ref ctx.buffer, ref ctx.state, CalculateEntrySize(codec, entry));
                codec.KeyCodec.WriteTagAndValue(ref ctx, entry.Key);
                codec.ValueCodec.WriteTagAndValue(ref ctx, entry.Value);
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
            int size = 0;
            foreach (var entry in list)
            {
                int entrySize = CalculateEntrySize(codec, entry);

                size += CodedOutputStream.ComputeRawVarint32Size(codec.MapTag);
                size += CodedOutputStream.ComputeLengthSize(entrySize) + entrySize;
            }
            return size;
        }

        private static int CalculateEntrySize(Codec codec, KeyValuePair<TKey, TValue> entry)
        {
            return codec.KeyCodec.CalculateSizeWithTag(entry.Key) + codec.ValueCodec.CalculateSizeWithTag(entry.Value);
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

        void IDictionary.Add(object key, object value) => Add((TKey)key, (TValue)value);

        bool IDictionary.Contains(object key) => key is TKey k && ContainsKey(k);

        IDictionaryEnumerator IDictionary.GetEnumerator() => new DictionaryEnumerator(GetEnumerator());

        void IDictionary.Remove(object key)
        {
            ProtoPreconditions.CheckNotNull(key, nameof(key));
            if (key is TKey k)
            {
                Remove(k);
            }
        }

        void ICollection.CopyTo(Array array, int index)
        {
            // This is ugly and slow as heck, but with any luck it will never be used anyway.
            ICollection temp = this.Select(pair => new DictionaryEntry(pair.Key, pair.Value)).ToList();
            temp.CopyTo(array, index);
        }

        bool IDictionary.IsFixedSize => false;

        ICollection IDictionary.Keys => (ICollection)Keys;

        ICollection IDictionary.Values => (ICollection)Values;

        bool ICollection.IsSynchronized => false;

        object ICollection.SyncRoot => this;

        object IDictionary.this[object key]
        {
            get
            {
                ProtoPreconditions.CheckNotNull(key, nameof(key));
                if (key is TKey k)
                {
                    TryGetValue(k, out TValue value);
                    return value;
                }
                return null;
            }

            set
            {
                this[(TKey)key] = (TValue)value;
            }
        }
        #endregion

        #region IReadOnlyDictionary explicit interface implementation
        IEnumerable<TKey> IReadOnlyDictionary<TKey, TValue>.Keys => Keys;
        IEnumerable<TValue> IReadOnlyDictionary<TKey, TValue>.Values => Values;
        #endregion

        private sealed class DictionaryEnumerator : IDictionaryEnumerator
        {
            private readonly IEnumerator<KeyValuePair<TKey, TValue>> enumerator;

            internal DictionaryEnumerator(IEnumerator<KeyValuePair<TKey, TValue>> enumerator)
            {
                this.enumerator = enumerator;
            }

            public bool MoveNext() => enumerator.MoveNext();

            public void Reset() => enumerator.Reset();

            public object Current => Entry;
            public DictionaryEntry Entry => new DictionaryEntry(Key, Value);
            public object Key => enumerator.Current.Key;
            public object Value => enumerator.Current.Value;
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
            /// The key codec.
            /// </summary>
            internal FieldCodec<TKey> KeyCodec => keyCodec;

            /// <summary>
            /// The value codec.
            /// </summary>
            internal FieldCodec<TValue> ValueCodec => valueCodec;

            /// <summary>
            /// The tag used in the enclosing message to indicate map entries.
            /// </summary>
            internal uint MapTag => mapTag;
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

            public int Count => parent.Count;

            public bool IsReadOnly => true;

            public bool IsSynchronized => false;

            public object SyncRoot => parent;

            public void Add(T item) => throw new NotSupportedException();

            public void Clear() => throw new NotSupportedException();

            public bool Contains(T item) => containsCheck(item);

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

            public bool Remove(T item) => throw new NotSupportedException();

            IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();

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

        private sealed class MapFieldDebugView
        {
            private readonly MapField<TKey, TValue> map;

            public MapFieldDebugView(MapField<TKey, TValue> map)
            {
                this.map = map;
            }

            [DebuggerBrowsable(DebuggerBrowsableState.RootHidden)]
            public KeyValuePair<TKey, TValue>[] Items => map.list.ToArray();
        }
    }
}
