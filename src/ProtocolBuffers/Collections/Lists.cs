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
using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace Google.ProtocolBuffers.Collections {

  /// <summary>
  /// Utility non-generic class for calling into Lists{T} using type inference.
  /// </summary>
  public static class Lists {

    /// <summary>
    /// Returns a read-only view of the specified list.
    /// </summary>
    public static IList<T> AsReadOnly<T>(IList<T> list) {
      return Lists<T>.AsReadOnly(list);
    }
  }

  /// <summary>
  /// Utility class for dealing with lists.
  /// </summary>
  public static class Lists<T> {

    static readonly ReadOnlyCollection<T> empty = new ReadOnlyCollection<T>(new T[0]);

    /// <summary>
    /// Returns an immutable empty list.
    /// </summary>
    public static ReadOnlyCollection<T> Empty {
      get { return empty; }
    }

    /// <summary>
    /// Returns either the original reference if it's already read-only,
    /// or a new ReadOnlyCollection wrapping the original list.
    /// </summary>
    public static IList<T> AsReadOnly(IList<T> list) {
      return list.IsReadOnly ? list : new ReadOnlyCollection<T>(list);
    }
  }
}
