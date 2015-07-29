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

using Google.Protobuf.Reflection;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using Google.Protobuf.Compatibility;

namespace Google.Protobuf.Collections
{
    /// <summary>
    /// Representation of a map field in a Protocol Buffer message.
    /// </summary>
    /// <remarks>
    /// This implementation preserves insertion order for simplicity of testing
    /// code using maps fields. Overwriting an existing entry does not change the
    /// position of that entry within the map. Equality is not order-sensitive.
    /// For string keys, the equality comparison is provided by <see cref="StringComparer.Ordinal"/>.
    /// </remarks>
    /// <typeparam name="TKey">Key type in the map. Must be a type supported by Protocol Buffer map keys.</typeparam>
    /// <typeparam name="TValue">Value type in the map. Must be a type supported by Protocol Buffers.</typeparam>
    public sealed class MapField<TKey, TValue> : IDeepCloneable<MapField<TKey, TValue>>, IDictionary<TKey, TValue>, IEquatable<MapField<TKey, TValue>>, IDictionary
    {
        // TODO: Don't create the map/list until we have an entry. (Assume many maps will be empty.)
        private readonly bool allowNullValues;
        private bool frozen;
        private readonly Dictionary<TKey, LinkedListNode<KeyValuePair<TKey, TValue>>> map =
            new Dictionary<TKey, LinkedListNode<KeyValuePair<TKey, TValue>>>();
        private readonly LinkedList<KeyValuePair<TKey, TValue>> list = new LinkedList<KeyValuePair<TKey, TValue>>();

        /// <summary>
        /// Constructs a new map field, defaulting the value nullability to only allow null values for message types
        /// and non-nullable value types.
        /// </summary>
        public MapField() : this(typeof(IMessage).IsAssignableFrom(typeof(TValue)) || Nullable.GetUnderlyingType(typeof(TValue)) != null)
        {
        }

        /// <summary>
        /// Constructs a new map field, overriding the choice of whether null values are permitted in the map.
        /// This is used by wrapper types, where maps with string and bytes wrappers as the value types
        /// support null values.
        /// </summary>
        /// <param name="allowNullValues">Whether null values are permitted in the map or not.</param>
        public MapField(bool allowNullValues)
        {
            if (allowNullValues && typeof(TValue).IsValueType() && Nullable.GetUnderlyingType(typeof(TValue)) == null)
            {
                throw new ArgumentException("allowNullValues", "Non-nullable value types do not support null values");
            }
            this.allowNullValues = allowNullValues;
        }

        public MapField<TKey, TValue> Clone()
        {
            var clone = new MapField<TKey, TValue>(allowNullValues);
            // Keys are never cloneable. Values might be.
            if (typeof(IDeepCloneable<TValue>).IsAssignableFrom(typeof(TValue)))
            {
                foreach (var pair in list)
                {
                    clone.Add(pair.Key, pair.Value == null ? pair.Value : ((IDeepCloneable<TValue>)pair.Value).Clone());
                }
            }
            else
            {
                // Nothing is cloneable, so we don't need to worry.
                clone.Add(this);
            }
            return clone;
        }

        public void Add(TKey key, TValue value)
        {
            // Validation of arguments happens in ContainsKey and the indexer
            if (ContainsKey(key))
            {
                throw new ArgumentException("Key already exists in map", "key");
            }
            this[key] = value;
        }

        public bool ContainsKey(TKey key)
        {
            ThrowHelper.ThrowIfNull(key, "key");
            return map.ContainsKey(key);
        }

        public bool Remove(TKey key)
        {
            ThrowHelper.ThrowIfNull(key, "key");
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

        public TValue this[TKey key]
        {
            get
            {
                ThrowHelper.ThrowIfNull(key, "key");
                TValue value;
                if (TryGetValue(key, out value))
                {
                    return value;
                }
                throw new KeyNotFoundException();
            }
            set
            {
                ThrowHelper.ThrowIfNull(key, "key");
                // value == null check here is redundant, but avoids boxing.
                if (value == null && !allowNullValues)
                {
                    ThrowHelper.ThrowIfNull(value, "value");
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

        // TODO: Make these views?
        public ICollection<TKey> Keys { get { return list.Select(t => t.Key).ToList(); } }
        public ICollection<TValue> Values { get { return list.Select(t => t.Value).ToList(); } }

        public void Add(IDictionary<TKey, TValue> entries)
        {
            ThrowHelper.ThrowIfNull(entries, "entries");
            foreach (var pair in entries)
            {
                Add(pair.Key, pair.Value);
            }
        }

        public IEnumerator<KeyValuePair<TKey, TValue>> GetEnumerator()
        {
            return list.GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        void ICollection<KeyValuePair<TKey, TValue>>.Add(KeyValuePair<TKey, TValue> item)
        {
            Add(item.Key, item.Value);
        }

        public void Clear()
        {
            list.Clear();
            map.Clear();
        }

        bool ICollection<KeyValuePair<TKey, TValue>>.Contains(KeyValuePair<TKey, TValue> item)
        {
            TValue value;
            return TryGetValue(item.Key, out value)
                && EqualityComparer<TValue>.Default.Equals(item.Value, value);
        }

        void ICollection<KeyValuePair<TKey, TValue>>.CopyTo(KeyValuePair<TKey, TValue>[] array, int arrayIndex)
        {
            list.CopyTo(array, arrayIndex);
        }

        bool ICollection<KeyValuePair<TKey, TValue>>.Remove(KeyValuePair<TKey, TValue> item)
        {
            if (item.Key == null)
            {
                throw new ArgumentException("Key is null", "item");
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
        /// Returns whether or not this map allows values to be null.
        /// </summary>
        public bool AllowsNullValues { get { return allowNullValues; } }

        public int Count { get { return list.Count; } }
        public bool IsReadOnly { get { return frozen; } }

        public override bool Equals(object other)
        {
            return Equals(other as MapField<TKey, TValue>);
        }

        public override int GetHashCode()
        {
            var valueComparer = EqualityComparer<TValue>.Default;
            int hash = 0;
            foreach (var pair in list)
            {
                hash ^= pair.Key.GetHashCode() * 31 + valueComparer.GetHashCode(pair.Value);
            }
            return hash;
        }

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
            var valueComparer = EqualityComparer<TValue>.Default;
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
            ThrowHelper.ThrowIfNull(key, "key");
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
                ThrowHelper.ThrowIfNull(key, "key");
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
                if (frozen)
                {
                    throw new NotSupportedException("Dictionary is frozen");
                }
                this[(TKey)key] = (TValue)value;
            }
        }
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
        /// A codec for a specific map field. This contains all the information required to encoded and
        /// decode the nested messages.
        /// </summary>
        public sealed class Codec
        {
            private readonly FieldCodec<TKey> keyCodec;
            private readonly FieldCodec<TValue> valueCodec;
            private readonly uint mapTag;

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
                    while (input.ReadTag(out tag))
                    {
                        if (tag == 0)
                        {
                            throw InvalidProtocolBufferException.InvalidTag();
                        }
                        if (tag == codec.keyCodec.Tag)
                        {
                            Key = codec.keyCodec.Read(input);
                        }
                        else if (tag == codec.valueCodec.Tag)
                        {
                            Value = codec.valueCodec.Read(input);
                        }
                        else if (WireFormat.IsEndGroupTag(tag))
                        {
                            // TODO(jonskeet): Do we need this? (Given that we don't support groups...)
                            return;
                        }
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
    }
}
