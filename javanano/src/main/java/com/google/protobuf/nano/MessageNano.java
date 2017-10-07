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
import java.util.Arrays;

/**
 * Abstract interface implemented by Protocol Message objects.
 *
 * @author wink@google.com Wink Saville
 */
public abstract class MessageNano {
    protected volatile int cachedSize = -1;

    /**
     * Get the number of bytes required to encode this message.
     * Returns the cached size or calls getSerializedSize which
     * sets the cached size. This is used internally when serializing
     * so the size is only computed once. If a member is modified
     * then this could be stale call getSerializedSize if in doubt.
     */
    public int getCachedSize() {
        if (cachedSize < 0) {
            // getSerializedSize sets cachedSize
            getSerializedSize();
        }
        return cachedSize;
    }

    /**
     * Computes the number of bytes required to encode this message.
     * The size is cached and the cached result can be retrieved
     * using getCachedSize().
     */
    public int getSerializedSize() {
        int size = computeSerializedSize();
        cachedSize = size;
        return size;
    }

    /**
     * Computes the number of bytes required to encode this message. This does not update the
     * cached size.
     */
    protected int computeSerializedSize() {
      // This is overridden if the generated message has serialized fields.
      return 0;
    }

    /**
     * Serializes the message and writes it to {@code output}.
     *
     * @param output the output to receive the serialized form.
     * @throws IOException if an error occurred writing to {@code output}.
     */
    public void writeTo(CodedOutputByteBufferNano output) throws IOException {
        // Does nothing by default. Overridden by subclasses which have data to write.
    }

    /**
     * Parse {@code input} as a message of this type and merge it with the
     * message being built.
     */
    public abstract MessageNano mergeFrom(CodedInputByteBufferNano input) throws IOException;

    /**
     * Serialize to a byte array.
     * @return byte array with the serialized data.
     */
    public static final byte[] toByteArray(MessageNano msg) {
        final byte[] result = new byte[msg.getSerializedSize()];
        toByteArray(msg, result, 0, result.length);
        return result;
    }

    /**
     * Serialize to a byte array starting at offset through length. The
     * method getSerializedSize must have been called prior to calling
     * this method so the proper length is know.  If an attempt to
     * write more than length bytes OutOfSpaceException will be thrown
     * and if length bytes are not written then IllegalStateException
     * is thrown.
     */
    public static final void toByteArray(MessageNano msg, byte[] data, int offset, int length) {
        try {
            final CodedOutputByteBufferNano output =
                CodedOutputByteBufferNano.newInstance(data, offset, length);
            msg.writeTo(output);
            output.checkNoSpaceLeft();
        } catch (IOException e) {
            throw new RuntimeException("Serializing to a byte array threw an IOException "
                    + "(should never happen).", e);
        }
    }

    /**
     * Parse {@code data} as a message of this type and merge it with the
     * message being built.
     */
    public static final <T extends MessageNano> T mergeFrom(T msg, final byte[] data)
        throws InvalidProtocolBufferNanoException {
        return mergeFrom(msg, data, 0, data.length);
    }

    /**
     * Parse {@code data} as a message of this type and merge it with the
     * message being built.
     */
    public static final <T extends MessageNano> T mergeFrom(T msg, final byte[] data,
            final int off, final int len) throws InvalidProtocolBufferNanoException {
        try {
            final CodedInputByteBufferNano input =
                CodedInputByteBufferNano.newInstance(data, off, len);
            msg.mergeFrom(input);
            input.checkLastTagWas(0);
            return msg;
        } catch (InvalidProtocolBufferNanoException e) {
            throw e;
        } catch (IOException e) {
            throw new RuntimeException("Reading from a byte array threw an IOException (should "
                    + "never happen).");
        }
    }

    /**
     * Compares two {@code MessageNano}s and returns true if the message's are the same class and
     * have serialized form equality (i.e. all of the field values are the same).
     */
    public static final boolean messageNanoEquals(MessageNano a, MessageNano b) {
        if (a == b) {
            return true;
        }
        if (a == null || b == null) {
            return false;
        }
        if (a.getClass() != b.getClass()) {
          return false;
        }
        final int serializedSize = a.getSerializedSize();
        if (b.getSerializedSize() != serializedSize) {
            return false;
        }
        final byte[] aByteArray = new byte[serializedSize];
        final byte[] bByteArray = new byte[serializedSize];
        toByteArray(a, aByteArray, 0, serializedSize);
        toByteArray(b, bByteArray, 0, serializedSize);
        return Arrays.equals(aByteArray, bByteArray);
    }

    /**
     * Returns a string that is (mostly) compatible with ProtoBuffer's TextFormat. Note that groups
     * (which are deprecated) are not serialized with the correct field name.
     *
     * <p>This is implemented using reflection, so it is not especially fast nor is it guaranteed
     * to find all fields if you have method removal turned on for proguard.
     */
    @Override
    public String toString() {
        return MessageNanoPrinter.print(this);
    }

    /**
     * Provides support for cloning. This only works if you specify the generate_clone method.
     */
    @Override
    public MessageNano clone() throws CloneNotSupportedException {
        return (MessageNano) super.clone();
    }
}
