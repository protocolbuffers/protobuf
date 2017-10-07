// Protocol Buffers - Google's data interchange format
// Copyright 2013 Google Inc.  All rights reserved.
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

package com.google.protobuf.nano;

import java.io.IOException;

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
    protected FieldArray unknownFieldData;

    @Override
    protected int computeSerializedSize() {
        int size = 0;
        if (unknownFieldData != null) {
            for (int i = 0; i < unknownFieldData.size(); i++) {
                FieldData field = unknownFieldData.dataAt(i);
                size += field.computeSerializedSize();
            }
        }
        return size;
    }

    @Override
    public void writeTo(CodedOutputByteBufferNano output) throws IOException {
        if (unknownFieldData == null) {
            return;
        }
        for (int i = 0; i < unknownFieldData.size(); i++) {
            FieldData field = unknownFieldData.dataAt(i);
            field.writeTo(output);
        }
    }

    /**
     * Checks if there is a value stored for the specified extension in this
     * message.
     */
    public final boolean hasExtension(Extension<M, ?> extension) {
        if (unknownFieldData == null) {
            return false;
        }
        FieldData field = unknownFieldData.get(WireFormatNano.getTagFieldNumber(extension.tag));
        return field != null;
    }

    /**
     * Gets the value stored in the specified extension of this message.
     */
    public final <T> T getExtension(Extension<M, T> extension) {
        if (unknownFieldData == null) {
            return null;
        }
        FieldData field = unknownFieldData.get(WireFormatNano.getTagFieldNumber(extension.tag));
        return field == null ? null : field.getValue(extension);
    }

    /**
     * Sets the value of the specified extension of this message.
     */
    public final <T> M setExtension(Extension<M, T> extension, T value) {
        int fieldNumber = WireFormatNano.getTagFieldNumber(extension.tag);
        if (value == null) {
            if (unknownFieldData != null) {
                unknownFieldData.remove(fieldNumber);
                if (unknownFieldData.isEmpty()) {
                    unknownFieldData = null;
                }
            }
        } else {
            FieldData field = null;
            if (unknownFieldData == null) {
                unknownFieldData = new FieldArray();
            } else {
                field = unknownFieldData.get(fieldNumber);
            }
            if (field == null) {
                unknownFieldData.put(fieldNumber, new FieldData(extension, value));
            } else {
                field.setValue(extension, value);
            }
        }

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
        int fieldNumber = WireFormatNano.getTagFieldNumber(tag);
        int endPos = input.getPosition();
        byte[] bytes = input.getData(startPos, endPos - startPos);
        UnknownFieldData unknownField = new UnknownFieldData(tag, bytes);

        FieldData field = null;
        if (unknownFieldData == null) {
            unknownFieldData = new FieldArray();
        } else {
            field = unknownFieldData.get(fieldNumber);
        }
        if (field == null) {
            field = new FieldData();
            unknownFieldData.put(fieldNumber, field);
        }
        field.addUnknownField(unknownField);
        return true;
    }

    @Override
    public M clone() throws CloneNotSupportedException {
        M cloned = (M) super.clone();
        InternalNano.cloneUnknownFieldData(this, cloned);
        return cloned;
    }
}
