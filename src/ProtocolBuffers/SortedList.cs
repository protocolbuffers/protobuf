#region Copyright notice and license

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
// http://code.google.com/p/protobuf/
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

#if SILVERLIGHT
using System.Collections;
using System.Collections.Generic;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Dictionary implementation which always yields keys in sorted order.
    /// This is not particularly efficient: it wraps a normal dictionary
    /// for most operations, but sorts by key when either iterating or
    /// fetching the Keys/Values properties.
    /// This is only used for Silverlight, which doesn't have the normal
    /// sorted collections.
    /// </summary>
    internal sealed class SortedList<TKey, TValue> : IDictionary<TKey, TValue>
    {
        private readonly IDictionary<TKey, TValue> wrapped = new Dictionary<TKey, TValue>();

        public SortedList()
        {
        }

        public SortedList(IDictionary<TKey, TValue> dictionary)
        {
            foreach (KeyValuePair<TKey, TValue> entry in dictionary)
            {
                Add(entry.Key, entry.Value);
            }
        }

        public void Add(TKey key, TValue value)
        {
            wrapped.Add(key, value);
        }

        public bool ContainsKey(TKey key)
        {
            return wrapped.ContainsKey(key);
        }

        public ICollection<TKey> Keys
        {
            get
            {
                List<TKey> keys = new List<TKey>(wrapped.Count);
                foreach (var pair in this)
                {
                    keys.Add(pair.Key);
                }
                return keys;
            }
        }

        public bool Remove(TKey key)
        {
            return wrapped.Remove(key);
        }

        public bool TryGetValue(TKey key, out TValue value)
        {
            return wrapped.TryGetValue(key, out value);
        }

        public ICollection<TValue> Values
        {
            get
            {
                List<TValue> values = new List<TValue>(wrapped.Count);
                foreach (var pair in this)
                {
                    values.Add(pair.Value);
                }
                return values;
            }
        }

        public TValue this[TKey key]
        {
            get { return wrapped[key]; }
            set { wrapped[key] = value; }
        }

        public void Add(KeyValuePair<TKey, TValue> item)
        {
            wrapped.Add(item);
        }

        public void Clear()
        {
            wrapped.Clear();
        }

        public bool Contains(KeyValuePair<TKey, TValue> item)
        {
            return wrapped.Contains(item);
        }

        public void CopyTo(KeyValuePair<TKey, TValue>[] array, int arrayIndex)
        {
            wrapped.CopyTo(array, arrayIndex);
        }

        public int Count
        {
            get { return wrapped.Count; }
        }

        public bool IsReadOnly
        {
            get { return wrapped.IsReadOnly; }
        }

        public bool Remove(KeyValuePair<TKey, TValue> item)
        {
            return wrapped.Remove(item);
        }

        public IEnumerator<KeyValuePair<TKey, TValue>> GetEnumerator()
        {
            IComparer<TKey> comparer = Comparer<TKey>.Default;
            var list = new List<KeyValuePair<TKey, TValue>>(wrapped);
            list.Sort((x, y) => comparer.Compare(x.Key, y.Key));
            return list.GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
    }
}

#endif