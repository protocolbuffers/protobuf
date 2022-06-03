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