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
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Security;
using System.Threading;

namespace Google.Protobuf.Collections
{
    /// <summary>
    /// The contents of a repeated field: essentially, a collection with some extra
    /// restrictions (no null values) and capabilities (deep cloning).
    /// </summary>
    /// <remarks>
    /// This implementation does not generally prohibit the use of types which are not
    /// supported by Protocol Buffers but nor does it guarantee that all operations will work in such cases.
    /// </remarks>
    /// <typeparam name="T">The element type of the repeated field.</typeparam>
    public sealed class RepeatedField<T> : IList<T>, IList, IDeepCloneable<RepeatedField<T>>, IEquatable<RepeatedField<T>>
#if !NET35
        , IReadOnlyList<T>
#endif
    {
        private static readonly EqualityComparer<T> EqualityComparer = ProtobufEqualityComparers.GetEqualityComparer<T>();
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

        /// <summary>
        /// Adds the entries from the given input stream, decoding them with the specified codec.
        /// </summary>
        /// <param name="input">The input stream to read from.</param>
        /// <param name="codec">The codec to use in order to read each entry.</param>
        public void AddEntriesFrom(CodedInputStream input, FieldCodec<T> codec)
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
        /// Adds the entries from the given parse context, decoding them with the specified codec.
        /// </summary>
        /// <param name="ctx">The input to read from.</param>
        /// <param name="codec">The codec to use in order to read each entry.</param>
        [SecuritySafeCritical]
        public void AddEntriesFrom(ref ParseContext ctx, FieldCodec<T> codec)
        {
            // TODO: Inline some of the Add code, so we can avoid checking the size on every
            // iteration.
            uint tag = ctx.state.lastTag;
            var reader = codec.ValueReader;
            // Non-nullable value types can be packed or not.
            if (FieldCodec<T>.IsPackedRepeatedField(tag))
            {
                int length = ctx.ReadLength();
                if (length > 0)
                {
                    int oldLimit = SegmentedBufferHelper.PushLimit(ref ctx.state, length);

                    // If the content is fixed size then we can calculate the length
                    // of the repeated field and pre-initialize the underlying collection.
                    //
                    // Check that the supplied length doesn't exceed the underlying buffer.
                    // That prevents a malicious length from initializing a very large collection.
                    if (codec.FixedSize > 0 && length % codec.FixedSize == 0 && ParsingPrimitives.IsDataAvailable(ref ctx.state, length))
                    {
                        EnsureSize(count + (length / codec.FixedSize));

                        while (!SegmentedBufferHelper.IsReachedLimit(ref ctx.state))
                        {
                            // Only FieldCodecs with a fixed size can reach here, and they are all known
                            // types that don't allow the user to specify a custom reader action.
                            // reader action will never return null.
                            array[count++] = reader(ref ctx);
                        }
                    }
                    else
                    {
                        // Content is variable size so add until we reach the limit.
                        while (!SegmentedBufferHelper.IsReachedLimit(ref ctx.state))
                        {
                            Add(reader(ref ctx));
                        }
                    }
                    SegmentedBufferHelper.PopLimit(ref ctx.state, oldLimit);
                }
                // Empty packed field. Odd, but valid - just ignore.
            }
            else
            {
                // Not packed... (possibly not packable)
                do
                {
                    Add(reader(ref ctx));
                } while (ParsingPrimitives.MaybeConsumeTag(ref ctx.buffer, ref ctx.state, tag));
            }
        }

        /// <summary>
        /// Calculates the size of this collection based on the given codec.
        /// </summary>
        /// <param name="codec">The codec to use when encoding each field.</param>
        /// <returns>The number of bytes that would be written to an output by one of the <c>WriteTo</c> methods,
        /// using the same codec.</returns>
        public int CalculateSize(FieldCodec<T> codec)
        {
            if (count == 0)
            {
                return 0;
            }
            uint tag = codec.Tag;
            if (codec.PackedRepeatedField)
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
                if (codec.EndTag != 0)
                {
                    size += count * CodedOutputStream.ComputeRawVarint32Size(codec.EndTag);
                }
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

        /// <summary>
        /// Writes the contents of this collection to the given <see cref="CodedOutputStream"/>,
        /// encoding each value using the specified codec.
        /// </summary>
        /// <param name="output">The output stream to write to.</param>
        /// <param name="codec">The codec to use when encoding each value.</param>
        public void WriteTo(CodedOutputStream output, FieldCodec<T> codec)
        {
            WriteContext.Initialize(output, out WriteContext ctx);
            try
            {
                WriteTo(ref ctx, codec);
            }
            finally
            {
                ctx.CopyStateTo(output);
            }
        }

        /// <summary>
        /// Writes the contents of this collection to the given write context,
        /// encoding each value using the specified codec.
        /// </summary>
        /// <param name="ctx">The write context to write to.</param>
        /// <param name="codec">The codec to use when encoding each value.</param>
        [SecuritySafeCritical]
        public void WriteTo(ref WriteContext ctx, FieldCodec<T> codec)
        {
            if (count == 0)
            {
                return;
            }
            var writer = codec.ValueWriter;
            var tag = codec.Tag;
            if (codec.PackedRepeatedField)
            {
                // Packed primitive type
                int size = CalculatePackedDataSize(codec);
                ctx.WriteTag(tag);
                ctx.WriteLength(size);
                for (int i = 0; i < count; i++)
                {
                    writer(ref ctx, array[i]);
                }
            }
            else
            {
                // Not packed: a simple tag/value pair for each value.
                // Can't use codec.WriteTagAndValue, as that omits default values.
                for (int i = 0; i < count; i++)
                {
                    ctx.WriteTag(tag);
                    writer(ref ctx, array[i]);
                    if (codec.EndTag != 0)
                    {
                        ctx.WriteTag(codec.EndTag);
                    }
                }
            }
        }

        /// <summary>
        /// Gets and sets the capacity of the RepeatedField's internal array.  WHen set, the internal array is reallocated to the given capacity.
        /// <exception cref="ArgumentOutOfRangeException">The new value is less than Count -or- when Count is less than 0.</exception>
        /// </summary>
        public int Capacity
        {
            get { return array.Length; }
            set
            {
                if (value < count)
                {
                    throw new ArgumentOutOfRangeException("Capacity", value,
                        $"Cannot set Capacity to a value smaller than the current item count, {count}");
                }

                if (value >= 0 && value != array.Length)
                {
                    SetSize(value);
                }
            }
        }

        // May increase the size of the internal array, but will never shrink it.
        private void EnsureSize(int size)
        {
            if (array.Length < size)
            {
                size = Math.Max(size, MinArraySize);
                int newSize = Math.Max(array.Length * 2, size);
                SetSize(newSize);
            }
        }

        // Sets the internal array to an exact size.
        private void SetSize(int size)
        {
            if (size != array.Length)
            {
                var tmp = new T[size];
                Array.Copy(array, 0, tmp, 0, count);
                array = tmp;
            }
        }

        /// <summary>
        /// Adds the specified item to the collection.
        /// </summary>
        /// <param name="item">The item to add.</param>
        public void Add(T item)
        {
            ProtoPreconditions.CheckNotNullUnconstrained(item, nameof(item));
            EnsureSize(count + 1);
            array[count++] = item;
        }

        /// <summary>
        /// Removes all items from the collection.
        /// </summary>
        public void Clear()
        {
            array = EmptyArray;
            count = 0;
        }

        /// <summary>
        /// Determines whether this collection contains the given item.
        /// </summary>
        /// <param name="item">The item to find.</param>
        /// <returns><c>true</c> if this collection contains the given item; <c>false</c> otherwise.</returns>
        public bool Contains(T item)
        {
            return IndexOf(item) != -1;
        }

        /// <summary>
        /// Copies this collection to the given array.
        /// </summary>
        /// <param name="array">The array to copy to.</param>
        /// <param name="arrayIndex">The first index of the array to copy to.</param>
        public void CopyTo(T[] array, int arrayIndex)
        {
            Array.Copy(this.array, 0, array, arrayIndex, count);
        }

        /// <summary>
        /// Removes the specified item from the collection
        /// </summary>
        /// <param name="item">The item to remove.</param>
        /// <returns><c>true</c> if the item was found and removed; <c>false</c> otherwise.</returns>
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

        /// <summary>
        /// Gets the number of elements contained in the collection.
        /// </summary>
        public int Count => count;

        /// <summary>
        /// Gets a value indicating whether the collection is read-only.
        /// </summary>
        public bool IsReadOnly => false;

        /// <summary>
        /// Adds all of the specified values into this collection.
        /// </summary>
        /// <param name="values">The values to add to this collection.</param>
        public void AddRange(IEnumerable<T> values)
        {
            ProtoPreconditions.CheckNotNull(values, nameof(values));

            // Optimization 1: If the collection we're adding is already a RepeatedField<T>,
            // we know the values are valid.
            var otherRepeatedField = values as RepeatedField<T>;
            if (otherRepeatedField != null)
            {
                EnsureSize(count + otherRepeatedField.count);
                Array.Copy(otherRepeatedField.array, 0, array, count, otherRepeatedField.count);
                count += otherRepeatedField.count;
                return;
            }

            // Optimization 2: The collection is an ICollection, so we can expand
            // just once and ask the collection to copy itself into the array.
            var collection = values as ICollection;
            if (collection != null)
            {
                var extraCount = collection.Count;
                // For reference types and nullable value types, we need to check that there are no nulls
                // present. (This isn't a thread-safe approach, but we don't advertise this is thread-safe.)
                // We expect the JITter to optimize this test to true/false, so it's effectively conditional
                // specialization.
                if (default(T) == null)
                {
                    // TODO: Measure whether iterating once to check and then letting the collection copy
                    // itself is faster or slower than iterating and adding as we go. For large
                    // collections this will not be great in terms of cache usage... but the optimized
                    // copy may be significantly faster than doing it one at a time.
                    foreach (var item in collection)
                    {
                        if (item == null)
                        {
                            throw new ArgumentException("Sequence contained null element", nameof(values));
                        }
                    }
                }
                EnsureSize(count + extraCount);
                collection.CopyTo(array, count);
                count += extraCount;
                return;
            }

            // We *could* check for ICollection<T> as well, but very very few collections implement
            // ICollection<T> but not ICollection. (HashSet<T> does, for one...)

            // Fall back to a slower path of adding items one at a time.
            foreach (T item in values)
            {
                Add(item);
            }
        }

        /// <summary>
        /// Adds all of the specified values into this collection. This method is present to
        /// allow repeated fields to be constructed from queries within collection initializers.
        /// Within non-collection-initializer code, consider using the equivalent <see cref="AddRange"/>
        /// method instead for clarity.
        /// </summary>
        /// <param name="values">The values to add to this collection.</param>
        public void Add(IEnumerable<T> values)
        {
            AddRange(values);
        }

        /// <summary>
        /// Returns an enumerator that iterates through the collection.
        /// </summary>
        /// <returns>
        /// An enumerator that can be used to iterate through the collection.
        /// </returns>
        public IEnumerator<T> GetEnumerator()
        {
            for (int i = 0; i < count; i++)
            {
                yield return array[i];
            }
        }

        /// <summary>
        /// Determines whether the specified <see cref="System.Object" />, is equal to this instance.
        /// </summary>
        /// <param name="obj">The <see cref="System.Object" /> to compare with this instance.</param>
        /// <returns>
        ///   <c>true</c> if the specified <see cref="System.Object" /> is equal to this instance; otherwise, <c>false</c>.
        /// </returns>
        public override bool Equals(object obj)
        {
            return Equals(obj as RepeatedField<T>);
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
        /// Returns a hash code for this instance.
        /// </summary>
        /// <returns>
        /// A hash code for this instance, suitable for use in hashing algorithms and data structures like a hash table. 
        /// </returns>
        public override int GetHashCode()
        {
            int hash = 0;
            for (int i = 0; i < count; i++)
            {
                hash = hash * 31 + array[i].GetHashCode();
            }
            return hash;
        }

        /// <summary>
        /// Compares this repeated field with another for equality.
        /// </summary>
        /// <param name="other">The repeated field to compare this with.</param>
        /// <returns><c>true</c> if <paramref name="other"/> refers to an equal repeated field; <c>false</c> otherwise.</returns>
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
            EqualityComparer<T> comparer = EqualityComparer;
            for (int i = 0; i < count; i++)
            {
                if (!comparer.Equals(array[i], other.array[i]))
                {
                    return false;
                }
            }
            return true;
        }

        /// <summary>
        /// Returns the index of the given item within the collection, or -1 if the item is not
        /// present.
        /// </summary>
        /// <param name="item">The item to find in the collection.</param>
        /// <returns>The zero-based index of the item, or -1 if it is not found.</returns>
        public int IndexOf(T item)
        {
            ProtoPreconditions.CheckNotNullUnconstrained(item, nameof(item));
            EqualityComparer<T> comparer = EqualityComparer;
            for (int i = 0; i < count; i++)
            {
                if (comparer.Equals(array[i], item))
                {
                    return i;
                }
            }
            return -1;
        }

        /// <summary>
        /// Inserts the given item at the specified index.
        /// </summary>
        /// <param name="index">The index at which to insert the item.</param>
        /// <param name="item">The item to insert.</param>
        public void Insert(int index, T item)
        {
            ProtoPreconditions.CheckNotNullUnconstrained(item, nameof(item));
            if (index < 0 || index > count)
            {
                throw new ArgumentOutOfRangeException(nameof(index));
            }
            EnsureSize(count + 1);
            Array.Copy(array, index, array, index + 1, count - index);
            array[index] = item;
            count++;
        }

        /// <summary>
        /// Removes the item at the given index.
        /// </summary>
        /// <param name="index">The zero-based index of the item to remove.</param>
        public void RemoveAt(int index)
        {
            if (index < 0 || index >= count)
            {
                throw new ArgumentOutOfRangeException(nameof(index));
            }
            Array.Copy(array, index + 1, array, index, count - index - 1);
            count--;
            array[count] = default(T);
        }

        /// <summary>
        /// Returns a string representation of this repeated field, in the same
        /// way as it would be represented by the default JSON formatter.
        /// </summary>
        public override string ToString()
        {
            var writer = new StringWriter();
            JsonFormatter.Default.WriteList(writer, this);
            return writer.ToString();
        }

        /// <summary>
        /// Gets or sets the item at the specified index.
        /// </summary>
        /// <value>
        /// The element at the specified index.
        /// </value>
        /// <param name="index">The zero-based index of the element to get or set.</param>
        /// <returns>The item at the specified index.</returns>
        public T this[int index]
        {
            get
            {
                if (index < 0 || index >= count)
                {
                    throw new ArgumentOutOfRangeException(nameof(index));
                }
                return array[index];
            }
            set
            {
                if (index < 0 || index >= count)
                {
                    throw new ArgumentOutOfRangeException(nameof(index));
                }
                ProtoPreconditions.CheckNotNullUnconstrained(value, nameof(value));
                array[index] = value;
            }
        }

        #region Explicit interface implementation for IList and ICollection.
        bool IList.IsFixedSize => false;

        void ICollection.CopyTo(Array array, int index)
        {
            Array.Copy(this.array, 0, array, index, count);
        }

        bool ICollection.IsSynchronized => false;

        object ICollection.SyncRoot => this;

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
