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
    internal struct WriterInternalState
    {
        // NOTE: the Span representing the current buffer is kept separate so that this doesn't have to be a ref struct and so it can
        // be included in CodedOutputStream's internal state

        internal int limit;  // the size of the current buffer
        internal int position;  // position in the current buffer

        internal WriteBufferHelper writeBufferHelper;

        // If non-null, the top level parse method was started with given coded output stream as an argument
        // which also means we can potentially fallback to calling WriteTo(CodedOutputStream cos) if needed.
        internal CodedOutputStream CodedOutputStream => writeBufferHelper.CodedOutputStream;
    }
}