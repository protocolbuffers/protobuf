#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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