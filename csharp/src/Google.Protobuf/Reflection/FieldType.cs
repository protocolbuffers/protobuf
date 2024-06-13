#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Enumeration of all the possible field types.
    /// </summary>
    public enum FieldType
    {
        /// <summary>
        /// The <c>double</c> field type.
        /// </summary>
        Double,
        /// <summary>
        /// The <c>float</c> field type.
        /// </summary>
        Float,
        /// <summary>
        /// The <c>int64</c> field type.
        /// </summary>
        Int64,
        /// <summary>
        /// The <c>uint64</c> field type.
        /// </summary>
        UInt64,
        /// <summary>
        /// The <c>int32</c> field type.
        /// </summary>
        Int32,
        /// <summary>
        /// The <c>fixed64</c> field type.
        /// </summary>
        Fixed64,
        /// <summary>
        /// The <c>fixed32</c> field type.
        /// </summary>
        Fixed32,
        /// <summary>
        /// The <c>bool</c> field type.
        /// </summary>
        Bool,
        /// <summary>
        /// The <c>string</c> field type.
        /// </summary>
        String,
        /// <summary>
        /// The field type used for groups.
        /// </summary>
        Group,
        /// <summary>
        /// The field type used for message fields.
        /// </summary>
        Message,
        /// <summary>
        /// The <c>bytes</c> field type.
        /// </summary>
        Bytes,
        /// <summary>
        /// The <c>uint32</c> field type.
        /// </summary>
        UInt32,
        /// <summary>
        /// The <c>sfixed32</c> field type.
        /// </summary>
        SFixed32,
        /// <summary>
        /// The <c>sfixed64</c> field type.
        /// </summary>
        SFixed64,
        /// <summary>
        /// The <c>sint32</c> field type.
        /// </summary>
        SInt32,
        /// <summary>
        /// The <c>sint64</c> field type.
        /// </summary>
        SInt64,
        /// <summary>
        /// The field type used for enum fields.
        /// </summary>
        Enum
    }
}