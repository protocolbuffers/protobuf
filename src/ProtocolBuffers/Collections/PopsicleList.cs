using System;
using System.Collections.Generic;
using System.Text;
using System.Collections;

namespace Google.ProtocolBuffers.Collections {
  /// <summary>
  /// Proxies calls to a <see cref="List{T}" />, but allows the list
  /// to be made read-only (with the <see cref="MakeReadOnly" /> method), 
  /// after which any modifying methods throw <see cref="NotSupportedException" />.
  /// </summary>
  public sealed class PopsicleList<T> : IList<T> {

    private readonly List<T> items = new List<T>();
    private bool readOnly = false;

    /// <summary>
    /// Makes this list read-only ("freezes the popsicle"). From this
    /// point on, mutating methods (Clear, Add etc) will throw a
    /// NotSupportedException. There is no way of "defrosting" the list afterwards.
    /// </summary>
    public void MakeReadOnly() {
      readOnly = true;
    }

    public int IndexOf(T item) {
      return items.IndexOf(item);
    }

    public void Insert(int index, T item) {
      ValidateModification();
      items.Insert(index, item);
    }

    public void RemoveAt(int index) {
      ValidateModification();
      items.RemoveAt(index);
    }

    public T this[int index] {
      get {
        return items[index];
      }
      set {
        ValidateModification();
        items[index] = value;
      }
    }

    public void Add(T item) {
      ValidateModification();
      items.Add(item);
    }

    public void Clear() {
      ValidateModification();
      items.Clear();
    }

    public bool Contains(T item) {
      return items.Contains(item);
    }

    public void CopyTo(T[] array, int arrayIndex) {
      items.CopyTo(array, arrayIndex);
    }

    public int Count {
      get { return items.Count; }
    }

    public bool IsReadOnly {
      get { return readOnly; }
    }

    public bool Remove(T item) {
      ValidateModification();
      return items.Remove(item);
    }

    public IEnumerator<T> GetEnumerator() {
      return items.GetEnumerator();
    }

    IEnumerator IEnumerable.GetEnumerator() {
      return GetEnumerator();
    }

    private void ValidateModification() {
      if (readOnly) {
        throw new NotSupportedException("List is read-only");
      }
    }
  }
}
