// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
using System;
using System.Collections;
using System.Collections.Generic;

namespace Google.ProtocolBuffers.Collections {

  /// <summary>
  /// Non-generic class with generic methods which proxy to the non-generic methods
  /// in the generic class.
  /// </summary>
  public static class Dictionaries {

    /// <summary>
    /// Compares two dictionaries for equality. Each value is compared with equality using Equals
    /// for non-IEnumerable implementations, and using EnumerableEquals otherwise.
    /// TODO(jonskeet): This is clearly pretty slow, and involves lots of boxing/unboxing...
    /// </summary>
    public static bool Equals<TKey, TValue>(IDictionary<TKey, TValue> left, IDictionary<TKey, TValue> right) {
      if (left.Count != right.Count) {
        return false;
      }
      foreach (KeyValuePair<TKey,TValue> leftEntry in left)
      {
        TValue rightValue;
        if (!right.TryGetValue(leftEntry.Key, out rightValue)) {
          return false;
        }

        IEnumerable leftEnumerable = leftEntry.Value as IEnumerable;
        IEnumerable rightEnumerable = rightValue as IEnumerable;
        if (leftEnumerable == null || rightEnumerable == null) {
          if (!object.Equals(leftEntry.Value, rightValue)) {
            return false;
          }
        } else {
          IEnumerator leftEnumerator = leftEnumerable.GetEnumerator();
          try {
            foreach (object rightObject in rightEnumerable) {
              if (!leftEnumerator.MoveNext()) {
                return false;
              }
              if (!object.Equals(leftEnumerator.Current, rightObject)) {
                return false;
              }
            }
            if (leftEnumerator.MoveNext()) {
              return false;
            }
          } finally {
            if (leftEnumerator is IDisposable) {
              ((IDisposable)leftEnumerator).Dispose();
            }
          }
        }
      }
      return true;
    }

    public static IDictionary<TKey, TValue> AsReadOnly<TKey, TValue> (IDictionary<TKey, TValue> dictionary) {
      return dictionary.IsReadOnly ? dictionary : new ReadOnlyDictionary<TKey, TValue>(dictionary);
    }

    /// <summary>
    /// Creates a hashcode for a dictionary by XORing the hashcodes of all the fields
    /// and values. (By XORing, we avoid ordering issues.)
    /// TODO(jonskeet): Currently XORs other stuff too, and assumes non-null values.
    /// </summary>
    public static int GetHashCode<TKey, TValue>(IDictionary<TKey, TValue> dictionary) {
      int ret = 31;
      foreach (KeyValuePair<TKey, TValue> entry in dictionary) {
        int hash = entry.Key.GetHashCode() ^ GetDeepHashCode(entry.Value);
        ret ^= hash;
      }
      return ret;
    }

    /// <summary>
    /// Determines the hash of a value by either taking it directly or hashing all the elements
    /// for IEnumerable implementations.
    /// </summary>
    private static int GetDeepHashCode(object value) {
      IEnumerable iterable = value as IEnumerable;
      if (iterable == null) {
        return value.GetHashCode();
      }
      int hash = 29;
      foreach (object element in iterable) {
        hash = hash * 37 + element.GetHashCode();
      }
      return hash;
    }
  }
}
