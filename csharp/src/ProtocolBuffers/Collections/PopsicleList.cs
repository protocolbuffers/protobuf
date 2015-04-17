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
using System;
using System.Collections;
using System.Collections.Generic;

namespace Google.ProtocolBuffers.Collections
{
    /// <summary>
    /// Proxies calls to a <see cref="List{T}" />, but allows the list
    /// to be made read-only (with the <see cref="MakeReadOnly" /> method), 
    /// after which any modifying methods throw <see cref="NotSupportedException" />.
    /// </summary>
    public sealed class PopsicleList<T> : IPopsicleList<T>, ICastArray
    {
        private static readonly bool CheckForNull = default(T) == null;
        private static readonly T[] EmptySet = new T[0];

        private List<T> items;
        private bool readOnly;

        /// <summary>
        /// Makes this list read-only ("freezes the popsicle"). From this
        /// point on, mutating methods (Clear, Add etc) will throw a
        /// NotSupportedException. There is no way of "defrosting" the list afterwards.
        /// </summary>
        public void MakeReadOnly()
        {
            readOnly = true;
        }

        public int IndexOf(T item)
        {
            return items == null ? -1 : items.IndexOf(item);
        }

        public void Insert(int index, T item)
        {
            ValidateModification();
            if (CheckForNull)
            {
                ThrowHelper.ThrowIfNull(item);
            }
            items.Insert(index, item);
        }

        public void RemoveAt(int index)
        {
            ValidateModification();
            items.RemoveAt(index);
        }

        public T this[int index]
        {
            get
            {
                if (items == null)
                {
                    throw new ArgumentOutOfRangeException();
                }
                return items[index];
            }
            set
            {
                ValidateModification();
                if (CheckForNull)
                {
                    ThrowHelper.ThrowIfNull(value);
                }
                items[index] = value;
            }
        }

        public void Add(T item)
        {
            ValidateModification();
            if (CheckForNull)
            {
                ThrowHelper.ThrowIfNull(item);
            }
            items.Add(item);
        }

        public void Clear()
        {
            ValidateModification();
            items.Clear();
        }

        public bool Contains(T item)
        {
            return items == null ? false : items.Contains(item);
        }

        public void CopyTo(T[] array, int arrayIndex)
        {
            if (items != null)
            {
                items.CopyTo(array, arrayIndex);
            }
        }

        public int Count
        {
            get { return items == null ? 0 : items.Count; }
        }

        public bool IsReadOnly
        {
            get { return readOnly; }
        }

        public bool Remove(T item)
        {
            ValidateModification();
            return items.Remove(item);
        }

        public IEnumerator<T> GetEnumerator()
        {
            IEnumerable<T> tenum = (IEnumerable<T>)items ?? EmptySet;
            return tenum.GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        public void Add(IEnumerable<T> collection)
        {
            ValidateModification();
            ThrowHelper.ThrowIfNull(collection);

            if (!CheckForNull || collection is PopsicleList<T>)
            {
                items.AddRange(collection);
            }
            else
            {
                // Assumption, it's ok to enumerate collections more than once.
                if (collection is ICollection<T>)
                {
                    ThrowHelper.ThrowIfAnyNull(collection);
                    items.AddRange(collection);
                }
                else
                {
                    foreach (T item in collection)
                    {
                        ThrowHelper.ThrowIfNull(item);
                        items.Add(item);
                    }
                }
            }
        }

        private void ValidateModification()
        {
            if (readOnly)
            {
                throw new NotSupportedException("List is read-only");
            }
            if (items == null)
            {
                items = new List<T>();
            }
        }

        IEnumerable<TItemType> ICastArray.CastArray<TItemType>()
        {
            if (items == null)
            {
                return PopsicleList<TItemType>.EmptySet;
            }
            return (TItemType[]) (object) items.ToArray();
        }
    }
}