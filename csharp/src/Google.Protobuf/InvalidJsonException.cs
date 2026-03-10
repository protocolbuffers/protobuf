#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System.IO;

namespace Google.Protobuf
{
    /// <summary>
    /// Thrown when an attempt is made to parse invalid JSON, e.g. using
    /// a non-string property key, or including a redundant comma. Parsing a protocol buffer
    /// message represented in JSON using <see cref="JsonParser"/> can throw both this
    /// exception and <see cref="InvalidProtocolBufferException"/> depending on the situation. This
    /// exception is only thrown for "pure JSON" errors, whereas <c>InvalidProtocolBufferException</c>
    /// is thrown when the JSON may be valid in and of itself, but cannot be parsed as a protocol buffer
    /// message.
    /// </summary>
    public sealed class InvalidJsonException : IOException
    {
        internal InvalidJsonException(string message)
            : base(message)
        {
        }
    }
}