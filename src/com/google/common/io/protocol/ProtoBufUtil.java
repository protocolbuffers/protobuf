// Copyright 2008 Google Inc. All Rights Reserved.

package com.google.common.io.protocol;

import java.io.*;

/**
 * Utility functions for dealing with ProtoBuf objects consolidated from
 * previous spot implementations across the codebase.
 *
 */
public final class ProtoBufUtil {
  private ProtoBufUtil() {
  }

  /** Convenience method to return a string value from of a proto or "". */
  public static String getProtoValueOrEmpty(ProtoBuf proto, int tag) {
    try {
      return (proto != null && proto.has(tag)) ? proto.getString(tag) : "";
    } catch (ClassCastException e) {
      return "";
    }
  }

  /** Convenience method to return a string value from of a sub-proto or "". */
  public static String getSubProtoValueOrEmpty(
      ProtoBuf proto, int sub, int tag) {
    try {
      return getProtoValueOrEmpty(getSubProtoOrNull(proto, sub), tag);
    } catch (ClassCastException e) {
      return "";
    }
  }

  /** Convenience method to get a subproto if the proto has it. */
  public static ProtoBuf getSubProtoOrNull(ProtoBuf proto, int sub) {
    return (proto != null && proto.has(sub)) ? proto.getProtoBuf(sub) : null;
  }

  /**
   * Get an int with "tag" from the proto buffer. If the given field can't be
   * retrieved, return the provided default value.
   *
   * @param proto The proto buffer.
   * @param tag The tag value that identifies which protocol buffer field to
   *        retrieve.
   * @param defaultValue The value to return if the field can't be retrieved.
   * @return The result which should be an integer.
   */
  public static int getProtoValueOrDefault(ProtoBuf proto, int tag,
      int defaultValue) {
    try {
      return (proto != null && proto.has(tag))
          ? proto.getInt(tag) : defaultValue;
    } catch (IllegalArgumentException e) {
      return defaultValue;
    } catch (ClassCastException e) {
      return defaultValue;
    }
  }

  /**
   * Get an Int with "tag" from the proto buffer.
   * If the given field can't be retrieved, return 0.
   *
   * @param proto The proto buffer.
   * @param tag The tag value that identifies which protocol buffer field to
   * retrieve.
   * @return The result which should be an integer.
   */
  public static int getProtoValueOrZero(ProtoBuf proto, int tag) {
    return getProtoValueOrDefault(proto, tag, 0);
  }

  /**
   * Get an Long with "tag" from the proto buffer.
   * If the given field can't be retrieved, return 0.
   *
   * @param proto The proto buffer.
   * @param tag The tag value that identifies which protocol buffer field to
   * retrieve.
   * @return The result which should be an integer.
   */
  public static long getProtoLongValueOrZero(ProtoBuf proto, int tag) {
    try {
      return (proto != null && proto.has(tag)) ? proto.getLong(tag) : 0L;
    } catch (IllegalArgumentException e) {
      return 0L;
    } catch (ClassCastException e) {
      return 0L;
    }
  }

  /**
   * Get an Int with "tag" from the proto buffer.
   * If the given field can't be retrieved, return -1.
   *
   * @param proto The proto buffer.
   * @param tag The tag value that identifies which protocol buffer field to
   * retrieve.
   * @return The result which should be a long.
   */
  public static long getProtoValueOrNegativeOne(ProtoBuf proto, int tag) {
    try {
      return (proto != null && proto.has(tag)) ? proto.getLong(tag) : -1;
    } catch (IllegalArgumentException e) {
      return -1;
    } catch (ClassCastException e) {
      return -1;
    }
  }

  /**
   * Returns an input stream for reading protocol buffer
   * responses. This method reads a 32-bit signed integer from the
   * stream, which determines the data size and compression. If the
   * integer is negative, indicating a GZipped input stream which we
   * do not support, an exception is thrown. Otherwise, just a
   * BoundInputStream is returned. The input stream returned is always
   * limited to the data available.
   *
   * @param dataInput the data input to read from
   * @return an input stream, limited to the data size read from the stream
   * @throws IOException if the incoming stream is gzipped.
   */
  public static InputStream getInputStreamForProtoBufResponse(
      DataInput dataInput) throws IOException {

    int size = dataInput.readInt();
    InputStream is = new BoundInputStream((InputStream) dataInput,
        Math.abs(size));

    if (size < 0) {
        throw new IOException("Cannot read gzipped streams");
    }
    return is;
  }

  /**
   * Reads a single protocol buffer from the given input stream. This method is
   * provided where the client needs incremental access to the contents of a
   * protocol buffer which contains a sequence of protocol buffers.
   * <p />
   * Please use {@link #getInputStreamForProtoBufResponse} to obtain an input
   * stream suitable for this method.
   *
   * @param umbrellaType the type of the "outer" protocol buffer containing
   *                    the message to read
   * @param is the stream to read the protocol buffer from
   * @param result the result protocol buffer (must be empty, will be filled
   *               with the data read and the type will be set)
   * @return the tag id of the message, -1 at the end of the stream
   */
  public static int readNextProtoBuf(ProtoBufType umbrellaType,
      InputStream is, ProtoBuf result) throws IOException {
    long tagAndType = ProtoBuf.readVarInt(is, true /* permits EOF */);
    if (tagAndType == -1) {
      return -1;
    }

    if ((tagAndType & 7) != ProtoBuf.WIRETYPE_LENGTH_DELIMITED) {
      throw new IOException("Message expected");
    }
    int tag = (int) (tagAndType >>> 3);

    result.setType((ProtoBufType) umbrellaType.getData(tag));
    int length = (int) ProtoBuf.readVarInt(is, false);
    result.parse(is, length);
    return tag;
  }

  /**
   * Reads a size int and a protocol buffer from a DataInput. If the size
   * is negative, this is interpreted as an indicator that the protocol buffer
   * is packed with GZIP. In this case, -size bytes are read, and the data is
   * unpacked with GZIP before constructing the protocol buffer.
   *
   * @param protoBufType the protocol buffer type to read
   * @param dataInput the data input to read from
   * @return a protocol buffer of the given type
   * @throws IOException
   */
  public static ProtoBuf readProtoBufResponse(ProtoBufType protoBufType,
      DataInput dataInput) throws IOException {
    ProtoBuf response = new ProtoBuf(protoBufType);
    InputStream is = getInputStreamForProtoBufResponse(dataInput);
    response.parse(is);
    if (is.read() != -1) {
      throw new IOException();
    }
    return response;
  }

  /**
   * A wrapper for <code> getProtoValueOrNegativeOne </code> that drills into
   * a sub message returning the long value if it exists, returning -1 if it
   * does not.
   *
   * @param proto The proto buffer.
   * @param tag The tag value that identifies which protocol buffer field to
   * retrieve.
   * @param sub The sub tag value that identifies which protocol buffer
   * sub-field to retrieve.n
   * @return The result which should be a long.
   */
  public static long getSubProtoValueOrNegativeOne(
      ProtoBuf proto, int sub, int tag) {
    try {
      return getProtoValueOrNegativeOne(getSubProtoOrNull(proto, sub), tag);
    } catch (IllegalArgumentException e) {
      return -1;
    } catch (ClassCastException e) {
      return -1;
    }
  }

  /**
   * A wrapper for {@link #getProtoValueOrDefault(ProtoBuf, int, int)} that
   * drills into a sub message returning the int value if it exists, returning
   * the given default if it does not.
   *
   * @param proto The proto buffer.
   * @param tag The tag value that identifies which protocol buffer field to
   * retrieve.
   * @param sub The sub tag value that identifies which protocol buffer
   * sub-field to retrieve.
   * @param defaultValue The value to return if the field is not present.
   * @return The result which should be a long.
   */
  public static int getSubProtoValueOrDefault(ProtoBuf proto, int sub, int tag,
      int defaultValue) {
    try {
      return getProtoValueOrDefault(getSubProtoOrNull(proto, sub), tag,
          defaultValue);
    } catch (IllegalArgumentException e) {
      return defaultValue;
    } catch (ClassCastException e) {
      return defaultValue;
    }
  }

  /**
   * Creates a sub ProtoBuf of the given Protobuf and sets it.
   *
   * @param proto The proto buffer.
   * @param tag The tag value that identifies which protocol buffer field to
   * create.
   * @return the sub ProtoBuf generated.
   */
  public static ProtoBuf createProtoBuf(ProtoBuf proto, int tag) {
    ProtoBuf child = proto.createGroup(tag);
    proto.setProtoBuf(tag, child);
    return child;
  }

  /**
   * Creates a sub ProtoBuf of the given Protobuf and adds it.
   *
   * @param proto The proto buffer.
   * @param tag The tag value that identifies which protocol buffer field to
   * add.
   * @return the sub ProtoBuf generated.
   */
  public static ProtoBuf addProtoBuf(ProtoBuf proto, int tag) {
    ProtoBuf child = proto.createGroup(tag);
    proto.addProtoBuf(tag, child);
    return child;
  }

  /**
   * Writes the ProtoBuf to the given DataOutput.  This is useful for unit
   * tests.
   *
   * @param output The data output to write to.
   * @param protoBuf The proto buffer.
   */
  public static void writeProtoBufToOutput(DataOutput output, ProtoBuf protoBuf)
      throws IOException {
    ByteArrayOutputStream baos = new ByteArrayOutputStream();
    protoBuf.outputTo(baos);
    byte[] bytes = baos.toByteArray();
    output.writeInt(bytes.length);
    output.write(bytes);
  }
}
