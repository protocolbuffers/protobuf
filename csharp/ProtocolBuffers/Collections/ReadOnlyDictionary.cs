using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using IEnumerable=System.Collections.IEnumerable;

namespace Google.ProtocolBuffers.Collections {
  /// <summary>
  /// Read-only wrapper around another dictionary.
  /// </summary>
  public class ReadOnlyDictionary<TKey, TValue> : IDictionary<TKey, TValue> {
    readonly IDictionary<TKey, TValue> wrapped;

    public ReadOnlyDictionary(IDictionary<TKey, TValue> wrapped) {
      this.wrapped = wrapped;
    }

    public void Add(TKey key, TValue value) {
      throw new InvalidOperationException();
    }

    public bool ContainsKey(TKey key) {
      return wrapped.ContainsKey(key);
    }

    public ICollection<TKey> Keys {
      get { return wrapped.Keys; }
    }

    public bool Remove(TKey key) {
      throw new InvalidOperationException();
    }

    public bool TryGetValue(TKey key, out TValue value) {
      return wrapped.TryGetValue(key, out value);
    }

    public ICollection<TValue> Values {
      get { return wrapped.Values; }
    }

    public TValue this[TKey key] {
      get {
        return wrapped[key];
      }
      set {
        throw new InvalidOperationException();
      }
    }

    public void Add(KeyValuePair<TKey, TValue> item) {
      throw new InvalidOperationException();
    }

    public void Clear() {
      throw new InvalidOperationException();
    }

    public bool Contains(KeyValuePair<TKey, TValue> item) {
      return wrapped.Contains(item);
    }

    public void CopyTo(KeyValuePair<TKey, TValue>[] array, int arrayIndex) {
      wrapped.CopyTo(array, arrayIndex);
    }

    public int Count {
      get { return wrapped.Count; }
    }

    public bool IsReadOnly {
      get { return true; }
    }

    public bool Remove(KeyValuePair<TKey, TValue> item) {
      throw new InvalidOperationException();
    }

    public IEnumerator<KeyValuePair<TKey, TValue>> GetEnumerator() {
      return wrapped.GetEnumerator();
    }

    IEnumerator IEnumerable.GetEnumerator() {
      return ((IEnumerable) wrapped).GetEnumerator();
    }

    public override bool Equals(object obj) {
      return wrapped.Equals(obj);
    }

    public override int GetHashCode() {
      return wrapped.GetHashCode();
    }

    public override string ToString() {
      return wrapped.ToString();
    }
  }
}
