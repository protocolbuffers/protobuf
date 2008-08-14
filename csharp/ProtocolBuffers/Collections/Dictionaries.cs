using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers.Collections {

  /// <summary>
  /// Non-generic class with generic methods which proxy to the non-generic methods
  /// in the generic class.
  /// </summary>
  public static class Dictionaries {
    public static IDictionary<TKey, TValue> AsReadOnly<TKey, TValue> (IDictionary<TKey, TValue> dictionary) {
      return dictionary.IsReadOnly ? dictionary : new ReadOnlyDictionary<TKey, TValue>(dictionary);
    }
  }
}
