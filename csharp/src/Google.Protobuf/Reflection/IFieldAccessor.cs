#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Collections;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Allows fields to be reflectively accessed.
    /// </summary>
    public interface IFieldAccessor
    {
        /// <summary>
        /// Returns the descriptor associated with this field.
        /// </summary>
        FieldDescriptor Descriptor { get; }

        /// <summary>
        /// Clears the field in the specified message. (For repeated fields,
        /// this clears the list.)
        /// </summary>
        void Clear(IMessage message);

        /// <summary>
        /// Fetches the field value. For repeated values, this will be an
        /// <see cref="IList"/> implementation. For map values, this will be an
        /// <see cref="IDictionary"/> implementation.
        /// </summary>
        object GetValue(IMessage message);

        /// <summary>
        /// Indicates whether the field in the specified message is set.
        /// For proto3 fields that aren't explicitly optional, this throws an <see cref="InvalidOperationException"/>
        /// </summary>
        bool HasValue(IMessage message);

        /// <summary>
        /// Mutator for single "simple" fields only.
        /// </summary>
        /// <remarks>
        /// Repeated fields are mutated by fetching the value and manipulating it as a list.
        /// Map fields are mutated by fetching the value and manipulating it as a dictionary.
        /// </remarks>
        /// <exception cref="InvalidOperationException">The field is not a "simple" field.</exception>
        void SetValue(IMessage message, object value);
    }
}