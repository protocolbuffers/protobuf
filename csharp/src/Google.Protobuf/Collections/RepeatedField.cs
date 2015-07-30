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
using Google.Protobuf.Compatibility;

namespace Google.Protobuf.Collections
{
    /// <summary>
    /// The contents of a repeated field: essentially, a collection with some extra
    /// restrictions (no null values) and capabilities (deep cloning and freezing).
    /// </summary>
    /// <typeparam name="T">The element type of the repeated field.</typeparam>
    public sealed class RepeatedField<T> : IList<T>, IList, IDeepCloneable<RepeatedField<T>>, IEquatable<RepeatedField<T>>
    {
        private static readonly T[] EmptyArray = new T[0];
        private const int MinArraySize = 8;

        private T[] array = EmptyArray;
        private int count = 0;

        /// <summary>
        /// Creates a deep clone of this repeated field.
        /// </summary>
        /// <remarks>
        /// If the field type is
        /// a message type, each element is also cloned; otherwise, it is
        /// assumed that the field type is primitive (including string and
        /// bytes, both of which are immutable) and so a simple copy is
        /// equivalent to a deep clone.
        /// </remarks>
        /// <returns>A deep clone of this repeated field.</returns>
        public RepeatedField<T> Clone()
        {
            RepeatedField<T> clone = new RepeatedField<T>();
            // Clone is implicitly *not* frozen, even if this object is.
            if (array != EmptyArray)
            {
                clone.array = (T[])array.Clone();
                IDeepCloneable<T>[] cloneableArray = clone.array as IDeepCloneable<T>[];
                if (cloneableArray != null)
                {
                    for (int i = 0; i < count; i++)
                    {
                        clone.array[i] = cloneableArray[i].Clone();
                    }
                }
            }
            clone.count = count;
            return clone;
        }

        public void AddEntriesFrom(CodedInputStream input, FieldCodec<T> codec)
        {
            // TODO: Inline some of the Add code, so we can avoid checking the size on every
            // iteration and the mutability.
            uint tag = input.LastTag;
            var reader = codec.ValueReader;
            // Value types can be packed or not.
            if (typeof(T).IsValueType() && WireFormat.GetTagWireType(tag) == WireFormat.WireType.LengthDelimited)
            {
                int length = input.ReadLength();
                if (length > 0)
                {
                    int oldLimit = input.PushLimit(length);
                    while (!input.ReachedLimit)
                    {
                        Add(reader(input));
                    }
                    input.PopLimit(oldLimit);
                }
                // Empty packed field. Odd, but valid - just ignore.
            }
            else
            {
                // Not packed... (possibly not packable)
                do
                {
                    Add(reader(input));
                } while (input.MaybeConsumeTag(tag));
            }
        }

        public int CalculateSize(FieldCodec<T> codec)
        {
            if (count == 0)
            {
                return 0;
            }
            uint tag = codec.Tag;
            if (typeof(T).IsValueType() && WireFormat.GetTagWireType(tag) == WireFormat.WireType.LengthDelimited)
            {
                int dataSize = CalculatePackedDataSize(codec);
                return CodedOutputStream.ComputeRawVarint32Size(tag) +
                    CodedOutputStream.ComputeLengthSize(dataSize) +
                    dataSize;
            }
            else
            {
                var sizeCalculator = codec.ValueSizeCalculator;
                int size = count * CodedOutputStream.ComputeRawVarint32Size(tag);
                for (int i = 0; i < count; i++)
                {
                    size += sizeCalculator(array[i]);
                }
                return size;
            }
        }

        private int CalculatePackedDataSize(FieldCodec<T> codec)
        {
            int fixedSize = codec.FixedSize;
            if (fixedSize == 0)
            {
                var calculator = codec.ValueSizeCalculator;
                int tmp = 0;
                for (int i = 0; i < count; i++)
                {
                    tmp += calculator(array[i]);
                }
                return tmp;
            }
            else
            {
                return fixedSize * Count;
            }
        }

        public void WriteTo(CodedOutputStream output, FieldCodec<T> codec)
        {
            if (count == 0)
            {
                return;
            }
            var writer = codec.ValueWriter;
            var tag = codec.Tag;
            if (typeof(T).IsValueType() && WireFormat.GetTagWireType(tag) == WireFormat.WireType.LengthDelimited)
            {
                // Packed primitive type
                uint size = (uint)CalculatePackedDataSize(codec);
                output.WriteTag(tag);
                output.WriteRawVarint32(size);
                for (int i = 0; i < count; i++)
                {
                    writer(output, array[i]);
                }
            }
            else
            {
                // Not packed: a simple tag/value pair for each value.
                // Can't use codec.WriteTagAndValue, as that omits default values.
                for (int i = 0; i < count; i++)
                {
                    output.WriteTag(tag);
                    writer(output, array[i]);
                }
            }
        }

        private void EnsureSize(int size)
        {
            if (array.Length < size)
            {
                size = Math.Max(size, MinArraySize);
                int newSize = Math.Max(array.Length * 2, size);
                var tmp = new T[newSize];
                Array.Copy(array, 0, tmp, 0, array.Length);
                array = tmp;
            }
        }

        public void Add(T item)
        {
            if (item == null)
            {
                throw new ArgumentNullException("item");
            }
            EnsureSize(count + 1);
            array[count++] = item;
        }

        public void Clear()
        {
            array = EmptyArray;
            count = 0;
        }

        public bool Contains(T item)
        {
            return IndexOf(item) != -1;
        }

        public void CopyTo(T[] array, int arrayIndex)
        {
            Array.Copy(this.array, 0, array, arrayIndex, count);
        }

        public bool Remove(T item)
        {
            int index = IndexOf(item);
            if (index == -1)
            {
                return false;
            }            
            Array.Copy(array, index + 1, array, index, count - index - 1);
            count--;
            array[count] = default(T);
            return true;
        }

        public int Count { get { return count; } }

        public bool IsReadOnly { get { return false; } }

        public void Add(RepeatedField<T> values)
        {
            if (values == null)
            {
                throw new ArgumentNullException("values");
            }
            EnsureSize(count + values.count);
            // We know that all the values will be valid, because it's a RepeatedField.
            Array.Copy(values.array, 0, array, count, values.count);
            count += values.count;
        }

        public void Add(IEnumerable<T> values)
        {
            if (values == null)
            {
                throw new ArgumentNullException("values");
            }
            // TODO: Check for ICollection and get the Count?
            foreach (T item in values)
            {
                Add(item);
            }
        }

        public IEnumerator<T> GetEnumerator()
        {
            for (int i = 0; i < count; i++)
            {
                yield return array[i];
            }
        }

        public override bool Equals(object obj)
        {
            return Equals(obj as RepeatedField<T>);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }        

        public override int GetHashCode()
        {
            int hash = 0;
            for (int i = 0; i < count; i++)
            {
                hash = hash * 31 + array[i].GetHashCode();
            }
            return hash;
        }

        public bool Equals(RepeatedField<T> other)
        {
            if (ReferenceEquals(other, null))
            {
                return false;
            }
            if (ReferenceEquals(other, this))
            {
                return true;
            }
            if (other.Count != this.Count)
            {
                return false;
            }
            // TODO(jonskeet): Does this box for enums?
            EqualityComparer<T> comparer = EqualityComparer<T>.Default;
            for (int i = 0; i < count; i++)
            {
                if (!comparer.Equals(array[i], other.array[i]))
                {
                    return false;
                }
            }
            return true;
        }

        public int IndexOf(T item)
        {
            if (item == null)
            {
                throw new ArgumentNullException("item");
            }
            // TODO(jonskeet): Does this box for enums?
            EqualityComparer<T> comparer = EqualityComparer<T>.Default;
            for (int i = 0; i < count; i++)
            {
                if (comparer.Equals(array[i], item))
                {
                    return i;
                }
            }
            return -1;
        }

        public void Insert(int index, T item)
        {
            if (item == null)
            {
                throw new ArgumentNullException("item");
            }
            if (index < 0 || index > count)
            {
                throw new ArgumentOutOfRangeException("index");
            }
            EnsureSize(count + 1);
            Array.Copy(array, index, array, index + 1, count - index);
            array[index] = item;
            count++;
        }

        public void RemoveAt(int index)
        {
            if (index < 0 || index >= count)
            {
                throw new ArgumentOutOfRangeException("index");
            }
            Array.Copy(array, index + 1, array, index, count - index - 1);
            count--;
            array[count] = default(T);
        }

        public T this[int index]
        {
            get
            {
                if (index < 0 || index >= count)
                {
                    throw new ArgumentOutOfRangeException("index");
                }
                return array[index];
            }
            set
            {
                if (index < 0 || index >= count)
                {
                    throw new ArgumentOutOfRangeException("index");
                }
                if (value == null)
                {
                    throw new ArgumentNullException("value");
                }
                array[index] = value;
            }
        }

        #region Explicit interface implementation for IList and ICollection.
        bool IList.IsFixedSize { get { return false; } }

        void ICollection.CopyTo(Array array, int index)
        {
            Array.Copy(this.array, 0, array, index, count);
        }

        bool ICollection.IsSynchronized { get { return false; } }

        object ICollection.SyncRoot { get { return this; } }

        object IList.this[int index]
        {
            get { return this[index]; }
            set { this[index] = (T)value; }
        }

        int IList.Add(object value)
        {
            Add((T) value);
            return count - 1;
        }

        bool IList.Contains(object value)
        {
            return (value is T && Contains((T)value));
        }

        int IList.IndexOf(object value)
        {
            if (!(value is T))
            {
                return -1;
            }
            return IndexOf((T)value);
        }

        void IList.Insert(int index, object value)
        {
            Insert(index, (T) value);
        }

        void IList.Remove(object value)
        {
            if (!(value is T))
            {
                return;
            }
            Remove((T)value);
        }
        #endregion        
    }
}
