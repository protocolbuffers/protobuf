#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Internal class containing utility methods when working with descriptors.
    /// </summary>
    internal static class DescriptorUtil
    {
        /// <summary>
        /// Equivalent to Func[TInput, int, TOutput] but usable in .NET 2.0. Only used to convert
        /// arrays.
        /// </summary>
        internal delegate TOutput IndexedConverter<TInput, TOutput>(TInput element, int index);

        /// <summary>
        /// Converts the given array into a read-only list, applying the specified conversion to
        /// each input element.
        /// </summary>
        internal static IList<TOutput> ConvertAndMakeReadOnly<TInput, TOutput>
            (IList<TInput> input, IndexedConverter<TInput, TOutput> converter)
        {
            TOutput[] array = new TOutput[input.Count];
            for (int i = 0; i < array.Length; i++)
            {
                array[i] = converter(input[i], i);
            }
            return new ReadOnlyCollection<TOutput>(array);
        }
    }
}