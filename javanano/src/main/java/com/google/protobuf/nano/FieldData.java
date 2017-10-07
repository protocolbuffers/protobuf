// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Stores unknown fields. These might be extensions or fields that the generated API doesn't
 * know about yet.
 */
class FieldData implements Cloneable {
    private Extension<?, ?> cachedExtension;
    private Object value;
    /** The serialised values for this object. Will be cleared if getValue is called */
    private List<UnknownFieldData> unknownFieldData;

    <T> FieldData(Extension<?, T> extension, T newValue) {
        cachedExtension = extension;
        value = newValue;
    }

    FieldData() {
        unknownFieldData = new ArrayList<UnknownFieldData>();
    }

    void addUnknownField(UnknownFieldData unknownField) {
        unknownFieldData.add(unknownField);
    }

    UnknownFieldData getUnknownField(int index) {
        if (unknownFieldData == null) {
            return null;
        }
        if (index < unknownFieldData.size()) {
            return unknownFieldData.get(index);
        }
        return null;
    }

    int getUnknownFieldSize() {
        if (unknownFieldData == null) {
            return 0;
        }
        return unknownFieldData.size();
    }

    <T> T getValue(Extension<?, T> extension) {
        if (value != null){
            if (cachedExtension != extension) {  // Extension objects are singletons.
                throw new IllegalStateException(
                        "Tried to getExtension with a differernt Extension.");
            }
        } else {
            cachedExtension = extension;
            value = extension.getValueFrom(unknownFieldData);
            unknownFieldData = null;
        }
        return (T) value;
    }

    <T> void setValue(Extension<?, T> extension, T newValue) {
        cachedExtension = extension;
        value = newValue;
        unknownFieldData = null;
    }

    int computeSerializedSize() {
        int size = 0;
        if (value != null) {
            size = cachedExtension.computeSerializedSize(value);
        } else {
            for (UnknownFieldData unknownField : unknownFieldData) {
                size += unknownField.computeSerializedSize();
            }
        }
        return size;
    }

    void writeTo(CodedOutputByteBufferNano output) throws IOException {
        if (value != null) {
            cachedExtension.writeTo(value, output);
        } else {
            for (UnknownFieldData unknownField : unknownFieldData) {
                unknownField.writeTo(output);
            }
        }
    }

    @Override
    public boolean equals(Object o) {
        if (o == this) {
            return true;
        }
        if (!(o instanceof FieldData)) {
            return false;
        }

        FieldData other = (FieldData) o;
        if (value != null && other.value != null) {
            // If both objects have deserialized values, compare those.
            // Since unknown fields are only compared if messages have generated equals methods
            // we know this will be a meaningful comparison (not identity) for all values.
            if (cachedExtension != other.cachedExtension) {  // Extension objects are singletons.
                return false;
            }
            if (!cachedExtension.clazz.isArray()) {
                // Can't test (!cachedExtension.repeated) due to 'bytes' -> 'byte[]'
                return value.equals(other.value);
            }
            if (value instanceof byte[]) {
                return Arrays.equals((byte[]) value, (byte[]) other.value);
            } else if (value instanceof int[]) {
                return Arrays.equals((int[]) value, (int[]) other.value);
            } else if (value instanceof long[]) {
                return Arrays.equals((long[]) value, (long[]) other.value);
            } else if (value instanceof float[]) {
                return Arrays.equals((float[]) value, (float[]) other.value);
            } else if (value instanceof double[]) {
                return Arrays.equals((double[]) value, (double[]) other.value);
            } else if (value instanceof boolean[]) {
                return Arrays.equals((boolean[]) value, (boolean[]) other.value);
            } else {
                return Arrays.deepEquals((Object[]) value, (Object[]) other.value);
            }
        }
        if (unknownFieldData != null && other.unknownFieldData != null) {
            // If both objects have byte arrays compare those directly.
            return unknownFieldData.equals(other.unknownFieldData);
        }
        try {
            // As a last resort, serialize and compare the resulting byte arrays.
            return Arrays.equals(toByteArray(), other.toByteArray());
        } catch (IOException e) {
            // Should not happen.
            throw new IllegalStateException(e);
        }
    }

    @Override
    public int hashCode() {
        int result = 17;
        try {
            // The only way to generate a consistent hash is to use the serialized form.
            result = 31 * result + Arrays.hashCode(toByteArray());
        } catch (IOException e) {
            // Should not happen.
            throw new IllegalStateException(e);
        }
        return result;
    }

    private byte[] toByteArray() throws IOException {
        byte[] result = new byte[computeSerializedSize()];
        CodedOutputByteBufferNano output = CodedOutputByteBufferNano.newInstance(result);
        writeTo(output);
        return result;
    }

    @Override
    public final FieldData clone() {
        FieldData clone = new FieldData();
        try {
            clone.cachedExtension = cachedExtension;
            if (unknownFieldData == null) {
                clone.unknownFieldData = null;
            } else {
                clone.unknownFieldData.addAll(unknownFieldData);
            }

            // Whether we need to deep clone value depends on its type. Primitive reference types
            // (e.g. Integer, Long etc.) are ok, since they're immutable. We need to clone arrays
            // and messages.
            if (value == null) {
                // No cloning required.
            } else if (value instanceof MessageNano) {
                clone.value = ((MessageNano) value).clone();
            } else if (value instanceof byte[]) {
                clone.value = ((byte[]) value).clone();
            } else if (value instanceof byte[][]) {
                byte[][] valueArray = (byte[][]) value;
                byte[][] cloneArray = new byte[valueArray.length][];
                clone.value = cloneArray;
                for (int i = 0; i < valueArray.length; i++) {
                    cloneArray[i] = valueArray[i].clone();
                }
            } else if (value instanceof boolean[]) {
                clone.value = ((boolean[]) value).clone();
            } else if (value instanceof int[]) {
                clone.value = ((int[]) value).clone();
            } else if (value instanceof long[]) {
                clone.value = ((long[]) value).clone();
            } else if (value instanceof float[]) {
                clone.value = ((float[]) value).clone();
            } else if (value instanceof double[]) {
                clone.value = ((double[]) value).clone();
            } else if (value instanceof MessageNano[]) {
                MessageNano[] valueArray = (MessageNano[]) value;
                MessageNano[] cloneArray = new MessageNano[valueArray.length];
                clone.value = cloneArray;
                for (int i = 0; i < valueArray.length; i++) {
                    cloneArray[i] = valueArray[i].clone();
                }
            }
            return clone;
        } catch (CloneNotSupportedException e) {
            throw new AssertionError(e);
        }
    }
}
