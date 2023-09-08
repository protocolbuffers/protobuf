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
    // warning: this is a mutable struct, so it needs to be only passed as a ref!
    internal struct ParserInternalState
    {
        // NOTE: the Span representing the current buffer is kept separate so that this doesn't have to be a ref struct and so it can
        // be included in CodedInputStream's internal state

        /// <summary>
        /// The position within the current buffer (i.e. the next byte to read)
        /// </summary>
        internal int bufferPos;

        /// <summary>
        /// Size of the current buffer
        /// </summary>
        internal int bufferSize;

        /// <summary>
        /// If we are currently inside a length-delimited block, this is the number of
        /// bytes in the buffer that are still available once we leave the delimited block.
        /// </summary>
        internal int bufferSizeAfterLimit;

        /// <summary>
        /// The absolute position of the end of the current length-delimited block (including totalBytesRetired)
        /// </summary>
        internal int currentLimit;

        /// <summary>
        /// The total number of consumed before the start of the current buffer. The
        /// total bytes read up to the current position can be computed as
        /// totalBytesRetired + bufferPos.
        /// </summary>
        internal int totalBytesRetired;

        internal int recursionDepth;  // current recursion depth

        internal SegmentedBufferHelper segmentedBufferHelper;

        /// <summary>
        /// The last tag we read. 0 indicates we've read to the end of the stream
        /// (or haven't read anything yet).
        /// </summary>
        internal uint lastTag;

        /// <summary>
        /// The next tag, used to store the value read by PeekTag.
        /// </summary>
        internal uint nextTag;
        internal bool hasNextTag;

        // these fields are configuration, they should be readonly
        internal int sizeLimit;
        internal int recursionLimit;

        // If non-null, the top level parse method was started with given coded input stream as an argument
        // which also means we can potentially fallback to calling MergeFrom(CodedInputStream cis) if needed.
        internal CodedInputStream CodedInputStream => segmentedBufferHelper.CodedInputStream;

        /// <summary>
        /// Internal-only property; when set to true, unknown fields will be discarded while parsing.
        /// </summary>
        internal bool DiscardUnknownFields { get; set; }

        /// <summary>
        /// Internal-only property; provides extension identifiers to compatible messages while parsing.
        /// </summary>
        internal ExtensionRegistry ExtensionRegistry { get; set; }
    }
}