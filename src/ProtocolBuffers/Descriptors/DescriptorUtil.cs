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
using Google.ProtocolBuffers.Collections;

namespace Google.ProtocolBuffers.Descriptors {
  /// <summary>
  /// Internal class containing utility methods when working with descriptors.
  /// </summary>
  internal static class DescriptorUtil {
    /// <summary>
    /// Equivalent to Func[TInput, int, TOutput] but usable in .NET 2.0. Only used to convert
    /// arrays.
    /// </summary>
    internal delegate TOutput IndexedConverter<TInput, TOutput>(TInput element, int index);

    /// <summary>
    /// Converts the given array into a read-only list, applying the specified conversion to
    /// each input element.
    /// </summary>
    internal static IList<TOutput> ConvertAndMakeReadOnly<TInput, TOutput>(IList<TInput> input,
        IndexedConverter<TInput, TOutput> converter) {
      TOutput[] array = new TOutput[input.Count];
      for (int i = 0; i < array.Length; i++) {
        array[i] = converter(input[i], i);
      }
      return Lists<TOutput>.AsReadOnly(array);
    }
  }
}
