// Protocol Buffers - Google's data interchange format
// Copyright 2013 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

package com.google.protobuf.nano;

import java.util.ArrayList;
import java.util.List;

/**
 * Base class of those Protocol Buffer messages that need to store unknown fields,
 * such as extensions.
 */
public abstract class ExtendableMessageNano extends MessageNano {
    /**
     * A container for fields unknown to the message, including extensions. Extension fields can
     * can be accessed through the {@link getExtension()} and {@link setExtension()} methods.
     */
    protected List<UnknownFieldData> unknownFieldData;

    @Override
    public int getSerializedSize() {
        int size = WireFormatNano.computeWireSize(unknownFieldData);
        cachedSize = size;
        return size;
    }

    /**
     * Gets the value stored in the specified extension of this message.
     */
    public <T> T getExtension(Extension<T> extension) {
        return WireFormatNano.getExtension(extension, unknownFieldData);
    }

    /**
     * Sets the value of the specified extension of this message.
     */
    public <T> void setExtension(Extension<T> extension, T value) {
        if (unknownFieldData == null) {
            unknownFieldData = new ArrayList<UnknownFieldData>();
        }
        WireFormatNano.setExtension(extension, value, unknownFieldData);
    }
}