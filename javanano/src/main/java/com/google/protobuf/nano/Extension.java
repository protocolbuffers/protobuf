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
import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.List;

/**
 * Represents an extension.
 *
 * @author bduff@google.com (Brian Duff)
 * @author maxtroy@google.com (Max Cai)
 * @param <M> the type of the extendable message this extension is for.
 * @param <T> the Java type of the extension; see {@link #clazz}.
 */
public class Extension<M extends ExtendableMessageNano<M>, T> {

    /*
     * Because we typically only define message-typed extensions, the Extension class hierarchy is
     * designed as follows, to allow a big amount of code in this file to be removed by ProGuard:
     *
     *            Extension          // ready to use for message/group typed extensions
     *                Δ
     *                |
     *       PrimitiveExtension      // for primitive/enum typed extensions
     */

    public static final int TYPE_DOUBLE   = InternalNano.TYPE_DOUBLE;
    public static final int TYPE_FLOAT    = InternalNano.TYPE_FLOAT;
    public static final int TYPE_INT64    = InternalNano.TYPE_INT64;
    public static final int TYPE_UINT64   = InternalNano.TYPE_UINT64;
    public static final int TYPE_INT32    = InternalNano.TYPE_INT32;
    public static final int TYPE_FIXED64  = InternalNano.TYPE_FIXED64;
    public static final int TYPE_FIXED32  = InternalNano.TYPE_FIXED32;
    public static final int TYPE_BOOL     = InternalNano.TYPE_BOOL;
    public static final int TYPE_STRING   = InternalNano.TYPE_STRING;
    public static final int TYPE_GROUP    = InternalNano.TYPE_GROUP;
    public static final int TYPE_MESSAGE  = InternalNano.TYPE_MESSAGE;
    public static final int TYPE_BYTES    = InternalNano.TYPE_BYTES;
    public static final int TYPE_UINT32   = InternalNano.TYPE_UINT32;
    public static final int TYPE_ENUM     = InternalNano.TYPE_ENUM;
    public static final int TYPE_SFIXED32 = InternalNano.TYPE_SFIXED32;
    public static final int TYPE_SFIXED64 = InternalNano.TYPE_SFIXED64;
    public static final int TYPE_SINT32   = InternalNano.TYPE_SINT32;
    public static final int TYPE_SINT64   = InternalNano.TYPE_SINT64;

    /**
     * Creates an {@code Extension} of the given message type and tag number.
     * Should be used by the generated code only.
     *
     * @param type {@link #TYPE_MESSAGE} or {@link #TYPE_GROUP}
     * @deprecated use {@link #createMessageTyped(int, Class, long)} instead.
     */
    @Deprecated
    public static <M extends ExtendableMessageNano<M>, T extends MessageNano>
            Extension<M, T> createMessageTyped(int type, Class<T> clazz, int tag) {
        return new Extension<M, T>(type, clazz, tag, false);
    }

    // Note: these create...() methods take a long for the tag parameter,
    // because tags are represented as unsigned ints, and these values exist
    // in generated code as long values. However, they can fit in 32-bits, so
    // it's safe to cast them to int without loss of precision.

    /**
     * Creates an {@code Extension} of the given message type and tag number.
     * Should be used by the generated code only.
     *
     * @param type {@link #TYPE_MESSAGE} or {@link #TYPE_GROUP}
     */
    public static <M extends ExtendableMessageNano<M>, T extends MessageNano>
            Extension<M, T> createMessageTyped(int type, Class<T> clazz, long tag) {
        return new Extension<M, T>(type, clazz, (int) tag, false);
    }

    /**
     * Creates a repeated {@code Extension} of the given message type and tag number.
     * Should be used by the generated code only.
     *
     * @param type {@link #TYPE_MESSAGE} or {@link #TYPE_GROUP}
     */
    public static <M extends ExtendableMessageNano<M>, T extends MessageNano>
            Extension<M, T[]> createRepeatedMessageTyped(int type, Class<T[]> clazz, long tag) {
        return new Extension<M, T[]>(type, clazz, (int) tag, true);
    }

    /**
     * Creates an {@code Extension} of the given primitive type and tag number.
     * Should be used by the generated code only.
     *
     * @param type one of {@code TYPE_*}, except {@link #TYPE_MESSAGE} and {@link #TYPE_GROUP}
     * @param clazz the boxed Java type of this extension
     */
    public static <M extends ExtendableMessageNano<M>, T>
            Extension<M, T> createPrimitiveTyped(int type, Class<T> clazz, long tag) {
        return new PrimitiveExtension<M, T>(type, clazz, (int) tag, false, 0, 0);
    }

    /**
     * Creates a repeated {@code Extension} of the given primitive type and tag number.
     * Should be used by the generated code only.
     *
     * @param type one of {@code TYPE_*}, except {@link #TYPE_MESSAGE} and {@link #TYPE_GROUP}
     * @param clazz the Java array type of this extension, with an unboxed component type
     */
    public static <M extends ExtendableMessageNano<M>, T>
            Extension<M, T> createRepeatedPrimitiveTyped(
                    int type, Class<T> clazz, long tag, long nonPackedTag, long packedTag) {
        return new PrimitiveExtension<M, T>(type, clazz, (int) tag, true,
            (int) nonPackedTag, (int) packedTag);
    }

    /**
     * Protocol Buffer type of this extension; one of the {@code TYPE_} constants.
     */
    protected final int type;

    /**
     * Java type of this extension. For a singular extension, this is the boxed Java type for the
     * Protocol Buffer {@link #type}; for a repeated extension, this is an array type whose
     * component type is the unboxed Java type for {@link #type}. For example, for a singular
     * {@code int32}/{@link #TYPE_INT32} extension, this equals {@code Integer.class}; for a
     * repeated {@code int32} extension, this equals {@code int[].class}.
     */
    protected final Class<T> clazz;

    /**
     * Tag number of this extension. The data should be viewed as an unsigned 32-bit value.
     */
    public final int tag;

    /**
     * Whether this extension is repeated.
     */
    protected final boolean repeated;

    private Extension(int type, Class<T> clazz, int tag, boolean repeated) {
        this.type = type;
        this.clazz = clazz;
        this.tag = tag;
        this.repeated = repeated;
    }

    /**
     * Returns the value of this extension stored in the given list of unknown fields, or
     * {@code null} if no unknown fields matches this extension.
     *
     * @param unknownFields a list of {@link UnknownFieldData}. All of the elements must have a tag
     *                      that matches this Extension's tag.
     *
     */
    final T getValueFrom(List<UnknownFieldData> unknownFields) {
        if (unknownFields == null) {
            return null;
        }
        return repeated ? getRepeatedValueFrom(unknownFields) : getSingularValueFrom(unknownFields);
    }

    private T getRepeatedValueFrom(List<UnknownFieldData> unknownFields) {
        // For repeated extensions, read all matching unknown fields in their original order.
        List<Object> resultList = new ArrayList<Object>();
        for (int i = 0; i < unknownFields.size(); i++) {
            UnknownFieldData data = unknownFields.get(i);
            if (data.bytes.length != 0) {
                readDataInto(data, resultList);
            }
        }

        int resultSize = resultList.size();
        if (resultSize == 0) {
            return null;
        } else {
            T result = clazz.cast(Array.newInstance(clazz.getComponentType(), resultSize));
            for (int i = 0; i < resultSize; i++) {
                Array.set(result, i, resultList.get(i));
            }
            return result;
        }
    }

    private T getSingularValueFrom(List<UnknownFieldData> unknownFields) {
        // For singular extensions, get the last piece of data stored under this extension.
        if (unknownFields.isEmpty()) {
            return null;
        }
        UnknownFieldData lastData = unknownFields.get(unknownFields.size() - 1);
        return clazz.cast(readData(CodedInputByteBufferNano.newInstance(lastData.bytes)));
    }

    protected Object readData(CodedInputByteBufferNano input) {
        // This implementation is for message/group extensions.
        Class<?> messageType = repeated ? clazz.getComponentType() : clazz;
        try {
            switch (type) {
                case TYPE_GROUP:
                    MessageNano group = (MessageNano) messageType.newInstance();
                    input.readGroup(group, WireFormatNano.getTagFieldNumber(tag));
                    return group;
                case TYPE_MESSAGE:
                    MessageNano message = (MessageNano) messageType.newInstance();
                    input.readMessage(message);
                    return message;
                default:
                    throw new IllegalArgumentException("Unknown type " + type);
            }
        } catch (InstantiationException e) {
            throw new IllegalArgumentException(
                    "Error creating instance of class " + messageType, e);
        } catch (IllegalAccessException e) {
            throw new IllegalArgumentException(
                    "Error creating instance of class " + messageType, e);
        } catch (IOException e) {
            throw new IllegalArgumentException("Error reading extension field", e);
        }
    }

    protected void readDataInto(UnknownFieldData data, List<Object> resultList) {
        // This implementation is for message/group extensions.
        resultList.add(readData(CodedInputByteBufferNano.newInstance(data.bytes)));
    }

    void writeTo(Object value, CodedOutputByteBufferNano output) throws IOException {
        if (repeated) {
            writeRepeatedData(value, output);
        } else {
            writeSingularData(value, output);
        }
    }

    protected void writeSingularData(Object value, CodedOutputByteBufferNano out) {
        // This implementation is for message/group extensions.
        try {
            out.writeRawVarint32(tag);
            switch (type) {
                case TYPE_GROUP:
                    MessageNano groupValue = (MessageNano) value;
                    int fieldNumber = WireFormatNano.getTagFieldNumber(tag);
                    out.writeGroupNoTag(groupValue);
                    // The endgroup tag must be included in the data payload.
                    out.writeTag(fieldNumber, WireFormatNano.WIRETYPE_END_GROUP);
                    break;
                case TYPE_MESSAGE:
                    MessageNano messageValue = (MessageNano) value;
                    out.writeMessageNoTag(messageValue);
                    break;
                default:
                    throw new IllegalArgumentException("Unknown type " + type);
            }
        } catch (IOException e) {
            // Should not happen
            throw new IllegalStateException(e);
        }
    }

    protected void writeRepeatedData(Object array, CodedOutputByteBufferNano output) {
        // This implementation is for non-packed extensions.
        int arrayLength = Array.getLength(array);
        for (int i = 0; i < arrayLength; i++) {
            Object element = Array.get(array, i);
            if (element != null) {
                writeSingularData(element, output);
            }
        }
    }

    int computeSerializedSize(Object value) {
        if (repeated) {
            return computeRepeatedSerializedSize(value);
        } else {
            return computeSingularSerializedSize(value);
        }
    }

    protected int computeRepeatedSerializedSize(Object array) {
        // This implementation is for non-packed extensions.
        int size = 0;
        int arrayLength = Array.getLength(array);
        for (int i = 0; i < arrayLength; i++) {
            Object element = Array.get(array, i);
            if (element != null) {
                size += computeSingularSerializedSize(Array.get(array, i));
            }
        }
        return size;
    }

    protected int computeSingularSerializedSize(Object value) {
        // This implementation is for message/group extensions.
        int fieldNumber = WireFormatNano.getTagFieldNumber(tag);
        switch (type) {
            case TYPE_GROUP:
                MessageNano groupValue = (MessageNano) value;
                return CodedOutputByteBufferNano.computeGroupSize(fieldNumber, groupValue);
            case TYPE_MESSAGE:
                MessageNano messageValue = (MessageNano) value;
                return CodedOutputByteBufferNano.computeMessageSize(fieldNumber, messageValue);
            default:
                throw new IllegalArgumentException("Unknown type " + type);
        }
    }

    /**
     * Represents an extension of a primitive (including enum) type. If there is no primitive
     * extensions, this subclass will be removable by ProGuard.
     */
    private static class PrimitiveExtension<M extends ExtendableMessageNano<M>, T>
            extends Extension<M, T> {

        /**
         * Tag of a piece of non-packed data from the wire compatible with this extension.
         */
        private final int nonPackedTag;

        /**
         * Tag of a piece of packed data from the wire compatible with this extension.
         * 0 if the type of this extension is not packable.
         */
        private final int packedTag;

        public PrimitiveExtension(int type, Class<T> clazz, int tag, boolean repeated,
                int nonPackedTag, int packedTag) {
            super(type, clazz, tag, repeated);
            this.nonPackedTag = nonPackedTag;
            this.packedTag = packedTag;
        }

        @Override
        protected Object readData(CodedInputByteBufferNano input) {
            try {
              return input.readPrimitiveField(type);
            } catch (IOException e) {
                throw new IllegalArgumentException("Error reading extension field", e);
            }
        }

        @Override
        protected void readDataInto(UnknownFieldData data, List<Object> resultList) {
            // This implementation is for primitive typed extensions,
            // which can read both packed and non-packed data.
            if (data.tag == nonPackedTag) {
                resultList.add(readData(CodedInputByteBufferNano.newInstance(data.bytes)));
            } else {
                CodedInputByteBufferNano buffer =
                        CodedInputByteBufferNano.newInstance(data.bytes);
                try {
                    buffer.pushLimit(buffer.readRawVarint32()); // length limit
                } catch (IOException e) {
                    throw new IllegalArgumentException("Error reading extension field", e);
                }
                while (!buffer.isAtEnd()) {
                    resultList.add(readData(buffer));
                }
            }
        }

        @Override
        protected final void writeSingularData(Object value, CodedOutputByteBufferNano output) {
            try {
                output.writeRawVarint32(tag);
                switch (type) {
                    case TYPE_DOUBLE:
                        Double doubleValue = (Double) value;
                        output.writeDoubleNoTag(doubleValue);
                        break;
                    case TYPE_FLOAT:
                        Float floatValue = (Float) value;
                        output.writeFloatNoTag(floatValue);
                        break;
                    case TYPE_INT64:
                        Long int64Value = (Long) value;
                        output.writeInt64NoTag(int64Value);
                        break;
                    case TYPE_UINT64:
                        Long uint64Value = (Long) value;
                        output.writeUInt64NoTag(uint64Value);
                        break;
                    case TYPE_INT32:
                        Integer int32Value = (Integer) value;
                        output.writeInt32NoTag(int32Value);
                        break;
                    case TYPE_FIXED64:
                        Long fixed64Value = (Long) value;
                        output.writeFixed64NoTag(fixed64Value);
                        break;
                    case TYPE_FIXED32:
                        Integer fixed32Value = (Integer) value;
                        output.writeFixed32NoTag(fixed32Value);
                        break;
                    case TYPE_BOOL:
                        Boolean boolValue = (Boolean) value;
                        output.writeBoolNoTag(boolValue);
                        break;
                    case TYPE_STRING:
                        String stringValue = (String) value;
                        output.writeStringNoTag(stringValue);
                        break;
                    case TYPE_BYTES:
                        byte[] bytesValue = (byte[]) value;
                        output.writeBytesNoTag(bytesValue);
                        break;
                    case TYPE_UINT32:
                        Integer uint32Value = (Integer) value;
                        output.writeUInt32NoTag(uint32Value);
                        break;
                    case TYPE_ENUM:
                        Integer enumValue = (Integer) value;
                        output.writeEnumNoTag(enumValue);
                        break;
                    case TYPE_SFIXED32:
                        Integer sfixed32Value = (Integer) value;
                        output.writeSFixed32NoTag(sfixed32Value);
                        break;
                    case TYPE_SFIXED64:
                        Long sfixed64Value = (Long) value;
                        output.writeSFixed64NoTag(sfixed64Value);
                        break;
                    case TYPE_SINT32:
                        Integer sint32Value = (Integer) value;
                        output.writeSInt32NoTag(sint32Value);
                        break;
                    case TYPE_SINT64:
                        Long sint64Value = (Long) value;
                        output.writeSInt64NoTag(sint64Value);
                        break;
                    default:
                        throw new IllegalArgumentException("Unknown type " + type);
                }
            } catch (IOException e) {
                // Should not happen
                throw new IllegalStateException(e);
            }
        }

        @Override
        protected void writeRepeatedData(Object array, CodedOutputByteBufferNano output) {
            if (tag == nonPackedTag) {
                // Use base implementation for non-packed data
                super.writeRepeatedData(array, output);
            } else if (tag == packedTag) {
                // Packed. Note that the array element type is guaranteed to be primitive, so there
                // won't be any null elements, so no null check in this block.
                int arrayLength = Array.getLength(array);
                int dataSize = computePackedDataSize(array);

                try {
                    output.writeRawVarint32(tag);
                    output.writeRawVarint32(dataSize);
                    switch (type) {
                        case TYPE_BOOL:
                            for (int i = 0; i < arrayLength; i++) {
                                output.writeBoolNoTag(Array.getBoolean(array, i));
                            }
                            break;
                        case TYPE_FIXED32:
                            for (int i = 0; i < arrayLength; i++) {
                                output.writeFixed32NoTag(Array.getInt(array, i));
                            }
                            break;
                        case TYPE_SFIXED32:
                            for (int i = 0; i < arrayLength; i++) {
                                output.writeSFixed32NoTag(Array.getInt(array, i));
                            }
                            break;
                        case TYPE_FLOAT:
                            for (int i = 0; i < arrayLength; i++) {
                                output.writeFloatNoTag(Array.getFloat(array, i));
                            }
                            break;
                        case TYPE_FIXED64:
                            for (int i = 0; i < arrayLength; i++) {
                                output.writeFixed64NoTag(Array.getLong(array, i));
                            }
                            break;
                        case TYPE_SFIXED64:
                            for (int i = 0; i < arrayLength; i++) {
                                output.writeSFixed64NoTag(Array.getLong(array, i));
                            }
                            break;
                        case TYPE_DOUBLE:
                            for (int i = 0; i < arrayLength; i++) {
                                output.writeDoubleNoTag(Array.getDouble(array, i));
                            }
                            break;
                        case TYPE_INT32:
                            for (int i = 0; i < arrayLength; i++) {
                                output.writeInt32NoTag(Array.getInt(array, i));
                            }
                            break;
                        case TYPE_SINT32:
                            for (int i = 0; i < arrayLength; i++) {
                                output.writeSInt32NoTag(Array.getInt(array, i));
                            }
                            break;
                        case TYPE_UINT32:
                            for (int i = 0; i < arrayLength; i++) {
                                output.writeUInt32NoTag(Array.getInt(array, i));
                            }
                            break;
                        case TYPE_INT64:
                            for (int i = 0; i < arrayLength; i++) {
                                output.writeInt64NoTag(Array.getLong(array, i));
                            }
                            break;
                        case TYPE_SINT64:
                            for (int i = 0; i < arrayLength; i++) {
                                output.writeSInt64NoTag(Array.getLong(array, i));
                            }
                            break;
                        case TYPE_UINT64:
                            for (int i = 0; i < arrayLength; i++) {
                                output.writeUInt64NoTag(Array.getLong(array, i));
                            }
                            break;
                        case TYPE_ENUM:
                            for (int i = 0; i < arrayLength; i++) {
                                output.writeEnumNoTag(Array.getInt(array, i));
                            }
                            break;
                        default:
                            throw new IllegalArgumentException("Unpackable type " + type);
                    }
                } catch (IOException e) {
                    // Should not happen.
                    throw new IllegalStateException(e);
                }
            } else {
                throw new IllegalArgumentException("Unexpected repeated extension tag " + tag
                        + ", unequal to both non-packed variant " + nonPackedTag
                        + " and packed variant " + packedTag);
            }
        }

        private int computePackedDataSize(Object array) {
            int dataSize = 0;
            int arrayLength = Array.getLength(array);
            switch (type) {
                case TYPE_BOOL:
                    // Bools are stored as int32 but just as 0 or 1, so 1 byte each.
                    dataSize = arrayLength;
                    break;
                case TYPE_FIXED32:
                case TYPE_SFIXED32:
                case TYPE_FLOAT:
                    dataSize = arrayLength * CodedOutputByteBufferNano.LITTLE_ENDIAN_32_SIZE;
                    break;
                case TYPE_FIXED64:
                case TYPE_SFIXED64:
                case TYPE_DOUBLE:
                    dataSize = arrayLength * CodedOutputByteBufferNano.LITTLE_ENDIAN_64_SIZE;
                    break;
                case TYPE_INT32:
                    for (int i = 0; i < arrayLength; i++) {
                        dataSize += CodedOutputByteBufferNano.computeInt32SizeNoTag(
                                Array.getInt(array, i));
                    }
                    break;
                case TYPE_SINT32:
                    for (int i = 0; i < arrayLength; i++) {
                        dataSize += CodedOutputByteBufferNano.computeSInt32SizeNoTag(
                                Array.getInt(array, i));
                    }
                    break;
                case TYPE_UINT32:
                    for (int i = 0; i < arrayLength; i++) {
                        dataSize += CodedOutputByteBufferNano.computeUInt32SizeNoTag(
                                Array.getInt(array, i));
                    }
                    break;
                case TYPE_INT64:
                    for (int i = 0; i < arrayLength; i++) {
                        dataSize += CodedOutputByteBufferNano.computeInt64SizeNoTag(
                                Array.getLong(array, i));
                    }
                    break;
                case TYPE_SINT64:
                    for (int i = 0; i < arrayLength; i++) {
                        dataSize += CodedOutputByteBufferNano.computeSInt64SizeNoTag(
                                Array.getLong(array, i));
                    }
                    break;
                case TYPE_UINT64:
                    for (int i = 0; i < arrayLength; i++) {
                        dataSize += CodedOutputByteBufferNano.computeUInt64SizeNoTag(
                                Array.getLong(array, i));
                    }
                    break;
                case TYPE_ENUM:
                    for (int i = 0; i < arrayLength; i++) {
                        dataSize += CodedOutputByteBufferNano.computeEnumSizeNoTag(
                                Array.getInt(array, i));
                    }
                    break;
                default:
                    throw new IllegalArgumentException("Unexpected non-packable type " + type);
            }
            return dataSize;
        }

        @Override
        protected int computeRepeatedSerializedSize(Object array) {
            if (tag == nonPackedTag) {
                // Use base implementation for non-packed data
                return super.computeRepeatedSerializedSize(array);
            } else if (tag == packedTag) {
                // Packed.
                int dataSize = computePackedDataSize(array);
                int payloadSize =
                        dataSize + CodedOutputByteBufferNano.computeRawVarint32Size(dataSize);
                return payloadSize + CodedOutputByteBufferNano.computeRawVarint32Size(tag);
            } else {
                throw new IllegalArgumentException("Unexpected repeated extension tag " + tag
                        + ", unequal to both non-packed variant " + nonPackedTag
                        + " and packed variant " + packedTag);
            }
        }

        @Override
        protected final int computeSingularSerializedSize(Object value) {
            int fieldNumber = WireFormatNano.getTagFieldNumber(tag);
            switch (type) {
                case TYPE_DOUBLE:
                    Double doubleValue = (Double) value;
                    return CodedOutputByteBufferNano.computeDoubleSize(fieldNumber, doubleValue);
                case TYPE_FLOAT:
                    Float floatValue = (Float) value;
                    return CodedOutputByteBufferNano.computeFloatSize(fieldNumber, floatValue);
                case TYPE_INT64:
                    Long int64Value = (Long) value;
                    return CodedOutputByteBufferNano.computeInt64Size(fieldNumber, int64Value);
                case TYPE_UINT64:
                    Long uint64Value = (Long) value;
                    return CodedOutputByteBufferNano.computeUInt64Size(fieldNumber, uint64Value);
                case TYPE_INT32:
                    Integer int32Value = (Integer) value;
                    return CodedOutputByteBufferNano.computeInt32Size(fieldNumber, int32Value);
                case TYPE_FIXED64:
                    Long fixed64Value = (Long) value;
                    return CodedOutputByteBufferNano.computeFixed64Size(fieldNumber, fixed64Value);
                case TYPE_FIXED32:
                    Integer fixed32Value = (Integer) value;
                    return CodedOutputByteBufferNano.computeFixed32Size(fieldNumber, fixed32Value);
                case TYPE_BOOL:
                    Boolean boolValue = (Boolean) value;
                    return CodedOutputByteBufferNano.computeBoolSize(fieldNumber, boolValue);
                case TYPE_STRING:
                    String stringValue = (String) value;
                    return CodedOutputByteBufferNano.computeStringSize(fieldNumber, stringValue);
                case TYPE_BYTES:
                    byte[] bytesValue = (byte[]) value;
                    return CodedOutputByteBufferNano.computeBytesSize(fieldNumber, bytesValue);
                case TYPE_UINT32:
                    Integer uint32Value = (Integer) value;
                    return CodedOutputByteBufferNano.computeUInt32Size(fieldNumber, uint32Value);
                case TYPE_ENUM:
                    Integer enumValue = (Integer) value;
                    return CodedOutputByteBufferNano.computeEnumSize(fieldNumber, enumValue);
                case TYPE_SFIXED32:
                    Integer sfixed32Value = (Integer) value;
                    return CodedOutputByteBufferNano.computeSFixed32Size(fieldNumber,
                            sfixed32Value);
                case TYPE_SFIXED64:
                    Long sfixed64Value = (Long) value;
                    return CodedOutputByteBufferNano.computeSFixed64Size(fieldNumber,
                            sfixed64Value);
                case TYPE_SINT32:
                    Integer sint32Value = (Integer) value;
                    return CodedOutputByteBufferNano.computeSInt32Size(fieldNumber, sint32Value);
                case TYPE_SINT64:
                    Long sint64Value = (Long) value;
                    return CodedOutputByteBufferNano.computeSInt64Size(fieldNumber, sint64Value);
                default:
                    throw new IllegalArgumentException("Unknown type " + type);
            }
        }
    }
}
