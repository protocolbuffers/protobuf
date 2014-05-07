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

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * Base class of those Protocol Buffer messages that need to store unknown fields,
 * such as extensions.
 */
public abstract class ExtendableMessageNano<M extends ExtendableMessageNano<M>>
        extends MessageNano {
    /**
     * A container for fields unknown to the message, including extensions. Extension fields can
     * can be accessed through the {@link #getExtension} and {@link #setExtension} methods.
     */
    protected List<UnknownFieldData> unknownFieldData;

    @Override
    protected int computeSerializedSize() {
        int size = 0;
        int unknownFieldCount = unknownFieldData == null ? 0 : unknownFieldData.size();
        for (int i = 0; i < unknownFieldCount; i++) {
            UnknownFieldData unknownField = unknownFieldData.get(i);
            size += CodedOutputByteBufferNano.computeRawVarint32Size(unknownField.tag);
            size += unknownField.bytes.length;
        }
        return size;
    }

    @Override
    public void writeTo(CodedOutputByteBufferNano output) throws IOException {
        int unknownFieldCount = unknownFieldData == null ? 0 : unknownFieldData.size();
        for (int i = 0; i < unknownFieldCount; i++) {
            UnknownFieldData unknownField = unknownFieldData.get(i);
            output.writeRawVarint32(unknownField.tag);
            output.writeRawBytes(unknownField.bytes);
        }
    }

    /**
     * Gets the value stored in the specified extension of this message.
     */
    public final <T> T getExtension(Extension<M, T> extension) {
        return extension.getValueFrom(unknownFieldData);
    }

    /**
     * Sets the value of the specified extension of this message.
     */
    public final <T> M setExtension(Extension<M, T> extension, T value) {
        unknownFieldData = extension.setValueTo(value, unknownFieldData);

        @SuppressWarnings("unchecked") // Generated code should guarantee type safety
        M typedThis = (M) this;
        return typedThis;
    }

    /**
     * Stores the binary data of an unknown field.
     *
     * <p>Generated messages will call this for unknown fields if the store_unknown_fields
     * option is on.
     *
     * <p>Note that the tag might be a end-group tag (rather than the start of an unknown field) in
     * which case we do not want to add an unknown field entry.
     *
     * @param input the input buffer.
     * @param tag the tag of the field.

     * @return {@literal true} unless the tag is an end-group tag.
     */
    protected final boolean storeUnknownField(CodedInputByteBufferNano input, int tag)
            throws IOException {
        int startPos = input.getPosition();
        if (!input.skipField(tag)) {
            return false;  // This wasn't an unknown field, it's an end-group tag.
        }
        if (unknownFieldData == null) {
            unknownFieldData = new ArrayList<UnknownFieldData>();
        }
        int endPos = input.getPosition();
        byte[] bytes = input.getData(startPos, endPos - startPos);
        unknownFieldData.add(new UnknownFieldData(tag, bytes));
        return true;
    }
}
