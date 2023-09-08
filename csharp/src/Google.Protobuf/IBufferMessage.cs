#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

namespace Google.Protobuf
{
    /// <summary>
    /// Interface for a Protocol Buffers message, supporting
    /// parsing from <see cref="ParseContext"/> and writing to <see cref="WriteContext"/>.
    /// </summary>
    public interface IBufferMessage : IMessage
    {
        /// <summary>
        /// Internal implementation of merging data from given parse context into this message.
        /// Users should never invoke this method directly.
        /// </summary>        
        void InternalMergeFrom(ref ParseContext ctx);

        /// <summary>
        /// Internal implementation of writing this message to a given write context.
        /// Users should never invoke this method directly.
        /// </summary>        
        void InternalWriteTo(ref WriteContext ctx);
    }
}
