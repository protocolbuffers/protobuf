using System;
using System.Collections;
using System.Collections.Generic;

namespace Google.Protobuf.Collections
{
    public sealed class RepeatedField<T> : IList<T>, IEquatable<RepeatedField<T>>
    {
        private readonly List<T> list = new List<T>();

        public void Add(T item)
        {
            if (item == null)
            {
                throw new ArgumentNullException("item");
            }
            list.Add(item);
        }

        public void Clear()
        {
            list.Clear();
        }

        public bool Contains(T item)
        {
            if (item == null)
            {
                throw new ArgumentNullException("item");
            }
            return list.Contains(item);
        }

        public void CopyTo(T[] array, int arrayIndex)
        {
            list.CopyTo(array);
        }

        public bool Remove(T item)
        {
            if (item == null)
            {
                throw new ArgumentNullException("item");
            }
            return list.Remove(item);
        }

        public int Count { get { return list.Count; } }

        // TODO(jonskeet): If we implement freezing, make this reflect it.
        public bool IsReadOnly { get { return false; } }

        public void Add(RepeatedField<T> values)
        {
            if (values == null)
            {
                throw new ArgumentNullException("values");
            }
            // We know that all the values will be valid, because it's a RepeatedField.
            list.AddRange(values);
        }

        public void Add(IEnumerable<T> values)
        {
            if (values == null)
            {
                throw new ArgumentNullException("values");
            }
            foreach (T item in values)
            {
                Add(item);
            }
        }

        // TODO(jonskeet): Create our own mutable struct for this, rather than relying on List<T>.
        public List<T>.Enumerator GetEnumerator()
        {
            return list.GetEnumerator();
        }

        IEnumerator<T> IEnumerable<T>.GetEnumerator()
        {
            return list.GetEnumerator();
        }

        public override bool Equals(object obj)
        {
            return Equals(obj as RepeatedField<T>);
        }

        public override int GetHashCode()
        {
            int hash = 23;
            foreach (T item in this)
            {
                hash = hash * 31 + item.GetHashCode();
            }
            return hash;
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
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
            for (int i = 0; i < Count; i++)
            {
                if (!comparer.Equals(this[i], other[i]))
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
            return list.IndexOf(item);
        }

        public void Insert(int index, T item)
        {
            if (item == null)
            {
                throw new ArgumentNullException("item");
            }
            list.Insert(index, item);
        }

        public void RemoveAt(int index)
        {
            list.RemoveAt(index);
        }

        public T this[int index]
        {
            get { return list[index]; }
            set
            {
                if (value == null)
                {
                    throw new ArgumentNullException("value");
                }
                list[index] = value;
            }
        }
    }
}
