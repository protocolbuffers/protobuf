using System;
using System.Collections;
using System.Collections.Generic;

namespace Google.Protobuf.Collections
{
    /// <summary>
    /// The contents of a repeated field: essentially, a collection with some extra
    /// restrictions (no null values) and capabilities (deep cloning and freezing).
    /// </summary>
    /// <typeparam name="T">The element type of the repeated field.</typeparam>
    public sealed class RepeatedField<T> : IList<T>, IDeepCloneable<RepeatedField<T>>, IEquatable<RepeatedField<T>>, IFreezable
    {
        private static readonly T[] EmptyArray = new T[0];

        private bool frozen;
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

        public bool IsFrozen { get { return frozen; } }

        public void Freeze()
        {
            frozen = true;
            IFreezable[] freezableArray = array as IFreezable[];
            if (freezableArray != null)
            {
                for (int i = 0; i < count; i++)
                {
                    freezableArray[i].Freeze();
                }
            }
        }

        private void EnsureSize(int size)
        {
            size = Math.Max(size, MinArraySize);
            if (array.Length < size)
            {
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
            this.CheckMutable();
            EnsureSize(count + 1);
            array[count++] = item;
        }

        /// <summary>
        /// Hack to allow us to add enums easily... will only work with int-based types.
        /// </summary>
        /// <param name="readEnum"></param>
        internal void AddInt32(int item)
        {
            this.CheckMutable();
            EnsureSize(count + 1);
            int[] castArray = (int[]) (object) array;
            castArray[count++] = item;
        }

        public void Clear()
        {
            this.CheckMutable();
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
            this.CheckMutable();
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

        // TODO(jonskeet): If we implement freezing, make this reflect it.
        public bool IsReadOnly { get { return IsFrozen; } }

        public void Add(RepeatedField<T> values)
        {
            if (values == null)
            {
                throw new ArgumentNullException("values");
            }
            this.CheckMutable();
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
            this.CheckMutable();
            // TODO: Check for ICollection and get the Count?
            foreach (T item in values)
            {
                Add(item);
            }
        }

        public RepeatedField<T>.Enumerator GetEnumerator()
        {
            return new Enumerator(this);
        }

        IEnumerator<T> IEnumerable<T>.GetEnumerator()
        {
            return GetEnumerator();
        }

        public override bool Equals(object obj)
        {
            return Equals(obj as RepeatedField<T>);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        /// <summary>
        /// Returns an enumerator of the values in this list as integers.
        /// Used for enum types.
        /// </summary>
        internal Int32Enumerator GetInt32Enumerator()
        {
            return new Int32Enumerator((int[])(object)array, count);
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
            this.CheckMutable();
            EnsureSize(count + 1);
            Array.Copy(array, index, array, index + 1, count - index);
            count++;
        }

        public void RemoveAt(int index)
        {
            if (index < 0 || index >= count)
            {
                throw new ArgumentOutOfRangeException("index");
            }
            this.CheckMutable();
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
                this.CheckMutable();
                if (value == null)
                {
                    throw new ArgumentNullException("value");
                }
                array[index] = value;
            }
        }

        public struct Enumerator : IEnumerator<T>
        {
            private int index;
            private readonly RepeatedField<T> field;

            public Enumerator(RepeatedField<T> field)
            {
                this.field = field;
                this.index = -1;
            }

            public bool MoveNext()
            {
                if (index + 1 >= field.Count)
                {
                    return false;
                }
                index++;
                return true;
            }

            public void Reset()
            {
                index = -1;
            }

            public T Current
            {
                get
                {
                    if (index == -1 || index >= field.count)
                    {
                        throw new InvalidOperationException();
                    }
                    return field.array[index];
                }
            }

            object IEnumerator.Current
            {
                get { return Current; }
            }

            public void Dispose()
            {
            }
        }

        internal struct Int32Enumerator : IEnumerator<int>
        {
            private int index;
            private readonly int[] array;
            private readonly int count;

            public Int32Enumerator(int[] array, int count)
            {
                this.array = array;
                this.index = -1;
                this.count = count;
            }

            public bool MoveNext()
            {
                if (index + 1 >= count)
                {
                    return false;
                }
                index++;
                return true;
            }

            public void Reset()
            {
                index = -1;
            }

            // No guard here, as we're only going to use this internally...
            public int Current { get { return array[index]; } }

            object IEnumerator.Current
            {
                get { return Current; }
            }

            public void Dispose()
            {
            }
        }
    }
}
