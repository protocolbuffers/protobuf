// Copyright 2007 The Android Open Source Project
// All Rights Reserved.

package com.google.common.io.protocol;

import java.io.*;
import java.util.*;

/**
 * Protocol buffer message object. Currently, it is assumed that tags ids are 
 * not large. This could be improved by storing a start offset, reducing the 
 * assumption to a dense number space.
 * <p>
 * ProtoBuf instances may or may not reference a ProtoBufType instance,
 * representing information from a corresponding .proto file, which defines tag 
 * data types. The type can only be set in the constructor, it cannot be 
 * changed later. 
 * <p>
 * If the type is null, the ProtoBuffer should be used only for reading or 
 * as a local persistent storage buffer. An untyped Protocol Buffer must never 
 * be sent to a server. 
 * <p>
 * If a ProtoBufType is set, unknown values are read from the stream and 
 * preserved, but it is not possible to add values for undefined tags using
 * this API. Attempts to set undefined tags will result in an exception.
 * <p>
 * This class provides two different sets of access methods for simple and
 * repeated tags. Simple access methods are has(tag), getXXX(tag), 
 * and setXXX(tag, value). Access methods for repeated tags are getCount(tag),
 * getXXX(tag, index), remove(tag, index), insert(tag, index, value) and 
 * addXXX(tag, value). Note that both sets of methods can be used in both cases,
 * but only the simple methods take default values into account. The reason for
 * this behavior is that default values cannot be removed -- they would reappear
 * after a serialization cycle. If a tag has repeated values, setXXX(tag, value)
 * will overwrite all of them and getXXX(tag) will throw an exception.
 *
 */

public class ProtoBuf {

  public static final Boolean FALSE = new Boolean(false);
  public static final Boolean TRUE = new Boolean(true);

  private static final String MSG_EOF = "Unexp.EOF";
  private static final String MSG_MISMATCH = "Type mismatch";
  private static final String MSG_UNSUPPORTED = "Unsupp.Type";

  // names copied from //net/proto2/internal/wire_format.cc
  static final int WIRETYPE_END_GROUP = 4;
  static final int WIRETYPE_FIXED32 = 5;
  static final int WIRETYPE_FIXED64 = 1;
  static final int WIRETYPE_LENGTH_DELIMITED = 2;
  static final int WIRETYPE_START_GROUP = 3;
  static final int WIRETYPE_VARINT = 0;

  /** Maximum number of bytes for VARINT wire format (64 bit, 7 bit/byte) */
  private static final int VARINT_MAX_BYTES = 10;
  
  private static Long[] SMALL_NUMBERS = {
      new Long(0), new Long(1), new Long(2), new Long(3), new Long(4),
      new Long(5), new Long(6), new Long(7), new Long(8), new Long(9),
      new Long(10), new Long(11), new Long(12), new Long(13), new Long(14),
      new Long(15)};

  private ProtoBufType msgType;
  private final Vector values = new Vector();
  
  /** 
   * Wire types picked up on the wire or implied by setters (if no other
   * type information is available.
   */
  private final StringBuffer wireTypes = new StringBuffer();

  /**
   * Creates a protocol message according to the given description. The
   * description is required if it is necessary to write the protocol buffer for
   * data exchange with other systems relying on the .proto file. 
   */
  public ProtoBuf(ProtoBufType type) {
    this.msgType = type;
  }

  /** 
   * Clears all data stored in this ProtoBuf.
   */
  public void clear() {
    values.setSize(0);
    wireTypes.setLength(0);
  }
  
  /**
   * Creates a new instance of the group with the given tag.
   */
  public ProtoBuf createGroup(int tag) {
    return new ProtoBuf((ProtoBufType) getType().getData(tag));
  }

  /**
   * Appends the given (repeated) tag with the given boolean value. 
   */
  public void addBool(int tag, boolean value){
    insertBool(tag, getCount(tag), value);
  }

  /**
   * Appends the given (repeated) tag with the given byte[] value.
   */
  public void addBytes(int tag, byte[] value){
    insertBytes(tag, getCount(tag), value);
  }

  /**
   * Appends the given (repeated) tag with the given int value.
   */
  public void addInt(int tag, int value){
    insertInt(tag, getCount(tag), value);
  }
  
  /**
   * Appends the given (repeated) tag with the given long value.
   */
  public void addLong(int tag, long value){
    insertLong(tag, getCount(tag), value);
  }

  /**
   * Appends the given (repeated) tag with the given float value.
   */
  public void addFloat(int tag, float value) {
    insertFloat(tag, getCount(tag), value);
  }

  /**
   * Appends the given (repeated) tag with the given double value.
   */
  public void addDouble(int tag, double value) {
    insertDouble(tag, getCount(tag), value);
  }

  /**
   * Appends the given (repeated) tag with the given group or message value.
   */
  public void addProtoBuf(int tag, ProtoBuf value){
    insertProtoBuf(tag, getCount(tag), value);
  }

  /**
   * Adds a new protobuf for the specified tag, setting the child protobuf's
   * type correctly for the tag.
   * @param tag the tag for which to create a new protobuf
   * @return the newly created protobuf
   */
  public ProtoBuf addNewProtoBuf(int tag) {
    ProtoBuf child = newProtoBufForTag(tag);
    addProtoBuf(tag, child);
    return child;
  }

  /**
   * Creates and returns a new protobuf for the specified tag, setting the new
   * protobuf's type correctly for the tag.
   * @param tag the tag for which to create a new protobuf
   * @return the newly created protobuf
   */
  public ProtoBuf newProtoBufForTag(int tag) {
      return new ProtoBuf((ProtoBufType) msgType.getData(tag));
  }

  /**
   * Appends the given (repeated) tag with the given String value.
   */
  public void addString(int tag, String value){
    insertString(tag, getCount(tag), value);
  }
  
  /** 
   * Returns the boolean value for the given tag.
   */
  public boolean getBool(int tag) {
    return ((Boolean) getObject(tag, ProtoBufType.TYPE_BOOL))
        .booleanValue();
  }

  /** 
   * Returns the boolean value for the given repeated tag at the given index. 
   */
  public boolean getBool(int tag, int index) {
    return ((Boolean) getObject(tag, index, ProtoBufType.TYPE_BOOL))
        .booleanValue();
  }

  /**
   * Returns the given string tag as byte array.
   */
  public byte[] getBytes(int tag) {
    return (byte[]) getObject(tag, ProtoBufType.TYPE_DATA);
  }

  /**
   * Returns the given repeated string tag at the given index as byte array.
   */
  public byte[] getBytes(int tag, int index) {
    return (byte[]) getObject(tag, index, ProtoBufType.TYPE_DATA);
  }

  /**
   * Returns the integer value for the given tag. 
   */
  public int getInt(int tag) {
    return (int) ((Long) getObject(tag, ProtoBufType.TYPE_INT32)).longValue();
  }

  /** 
   * Returns the integer value for the given repeated tag at the given index. 
   */
  public int getInt(int tag, int index) {
    return (int) ((Long) getObject(tag, index, 
        ProtoBufType.TYPE_INT32)).longValue();
  }

  /**
   * Returns the long value for the given tag. 
   */
  public long getLong(int tag) {
    return ((Long) getObject(tag, ProtoBufType.TYPE_INT64)).longValue();
  }

  /** 
   * Returns the long value for the given repeated tag at the given index. 
   */
  public long getLong(int tag, int index) {
    return ((Long) getObject(tag, index, ProtoBufType.TYPE_INT64)).longValue();
  }

  /**
   * Returns the float value for the given tag.
   */
  public float getFloat(int tag) {
    return Float.intBitsToFloat(getInt(tag));
  }

  /**
   * Returns the float value for the given repeated tag at the given index. 
   */
  public float getFloat(int tag, int index) {
    return Float.intBitsToFloat(getInt(tag, index));
  }

  /**
   * Returns the double value for the given tag.
   */
  public double getDouble(int tag) {
    return Double.longBitsToDouble(getLong(tag));
  }

  /**
   * Returns the double value for the given repeated tag at the given index. 
   */
  public double getDouble(int tag, int index) {
    return Double.longBitsToDouble(getLong(tag, index));
  }

  /**
   * Returns the group or nested message for the given tag.
   */
  public ProtoBuf getProtoBuf(int tag) {
    return (ProtoBuf) getObject(tag, ProtoBufType.TYPE_GROUP);
  }

  /**
   * Returns the group or nested message for the given repeated tag at the given
   * index.
   */
  public ProtoBuf getProtoBuf(int tag, int index) {
    return (ProtoBuf) getObject(tag, index, ProtoBufType.TYPE_GROUP);
  }

  /**
   * Returns the string value for a given tag converted to a Java String
   * assuming UTF-8 encoding.
   */
  public String getString(int tag) {
    return (String) getObject(tag, ProtoBufType.TYPE_TEXT);
  }

  /**
   * Returns the string value for a given repeated tag at the given index
   * converted to a Java String assuming UTF-8 encoding.
   */
  public String getString(int tag, int index) {
    return (String) getObject(tag, index, ProtoBufType.TYPE_TEXT);
  }

  /**
   * Returns the type definition of this protocol buffer or group -- if set. 
   */
  public ProtoBufType getType() {
    return msgType;
  }

  /**
   * Sets the type definition of this protocol buffer. Used internally in
   * ProtoBufUtil for incremental reading.
   * 
   * @param type the new type
   */
  void setType(ProtoBufType type) {
    if (values.size() != 0 || 
        (msgType != null && type != null && type != msgType)) {
      throw new IllegalArgumentException();
    }
    this.msgType = type;
  }
  
  /**
   * Convenience method for determining whether a tag has a value. Note: in 
   * contrast to getCount(tag) &gt; 0, this method takes the default value
   * into account.
   */
  public boolean has(int tag){
    return getCount(tag) > 0 || getDefault(tag) != null;
  }
  
  /**
   * Reads the contents of this ProtocolMessage from the given byte array.
   * Currently, this is a shortcut for parse(new ByteArrayInputStream(data)).
   * However, this may change in future versions for efficiency reasons.
   * 
   * @param data the byte array the ProtocolMessage is read from
   * @throws     IOException if an unexpected "End of file" is encountered in 
   *             the byte array
   */
  public ProtoBuf parse(byte[] data) throws IOException {
    parse(new ByteArrayInputStream(data), data.length);
    return this;
  }

  /**
   * Reads the contents of this ProtocolMessage from the given stream.
   * 
   * @param is the input stream providing the contents
   * @return   this
   * @throws   IOException raised if an IO exception occurs in the underlying
   *           stream or the end of the stream is reached at an unexpected
   *           position
   */
  
  public ProtoBuf parse(InputStream is) throws IOException {
    parse(is, Integer.MAX_VALUE);
    return this;
  }
  
  /**
   * Reads the contents of this ProtocolMessage from the given stream, consuming
   * at most the given number of bytes.
   * 
   * @param is        the input stream providing the contents
   * @param available maximum number of bytes to read
   * @return          this
   * @throws          IOException raised if an IO exception occurs in the 
   *                  underlying stream or the end of the stream is reached at 
   *                  an unexpected position
   */
  public int parse(InputStream is, int available) throws IOException {

    clear();
    while (available > 0) {
      long tagAndType = readVarInt(is, true /* permits EOF */);

      if (tagAndType == -1){
        break;
      }
      available -= getVarIntSize(tagAndType);
      int wireType = ((int) tagAndType) & 0x07;
      if (wireType == WIRETYPE_END_GROUP) {
        break;
      }
      int tag = (int) (tagAndType >>> 3);
      while (wireTypes.length() <= tag){
        wireTypes.append((char) ProtoBufType.TYPE_UNDEFINED);
      }
      wireTypes.setCharAt(tag, (char) wireType);

      // first step: decode tag value
      Object value;
      switch (wireType) {
        case WIRETYPE_VARINT:
          long v = readVarInt(is, false);
          available -= getVarIntSize(v);
          if (isZigZagEncodedType(tag)) {
            v = zigZagDecode(v);
          }
          value = (v >= 0 && v < SMALL_NUMBERS.length) ? 
              SMALL_NUMBERS[(int) v] : new Long(v);
          break;

        // also used for fixed values
        case WIRETYPE_FIXED32:
        case WIRETYPE_FIXED64:
          v = 0;
          int shift = 0;
          int count = (wireType == WIRETYPE_FIXED32) ? 4 : 8;
          available -= count;
          
          while (count-- > 0) {
            long l = is.read();
            v |= l << shift;
            shift += 8;
          }

          value = (v >= 0 && v < SMALL_NUMBERS.length) 
              ? SMALL_NUMBERS[(int) v]
              : new Long(v);
          break;

        case WIRETYPE_LENGTH_DELIMITED:
          int total = (int) readVarInt(is, false);
          available -= getVarIntSize(total);
          available -= total;
          
          if (getType(tag) == ProtoBufType.TYPE_MESSAGE) {
            ProtoBuf msg = new ProtoBuf((ProtoBufType) msgType.getData(tag));
            msg.parse(is, total);
            value = msg;
          } else {
            byte[] data = new byte[total];
            int pos = 0;
            while (pos < total) {
              count = is.read(data, pos, total - pos);
              if (count <= 0) {
                throw new IOException(MSG_EOF);
              }
              pos += count;
            }
            value = data;
          }
          break;

        case WIRETYPE_START_GROUP:
          ProtoBuf group = new ProtoBuf(msgType == null 
              ? null 
              : ((ProtoBufType) msgType.getData(tag)));
          available = group.parse(is, available);
          value = group;
          break;

        default:
          throw new RuntimeException(MSG_UNSUPPORTED + wireType);
      }
      insertObject(tag, getCount(tag), value);
    }
    
    if (available < 0){
      throw new IOException();
    }
    
    return available;
  }

  /**
   * Removes the tag value at the given index.
   */
  public void remove(int tag, int index){
    int count = getCount(tag);
    if (index >= count){
      throw new ArrayIndexOutOfBoundsException();
    }
    if (count == 1){
      values.setElementAt(null, tag);
    } else {
      Vector v = (Vector) values.elementAt(tag);
      v.removeElementAt(index);
    }
  }
  
  /**
   * Returns the number of repeated and optional (0..1) values for a given tag.
   * Note: Default values are not counted (and in general not considered in 
   * access methods for repeated tags), but considered for has(tag). 
   */
  public int getCount(int tag) {
    if (tag >= values.size()){
      return 0;
    }
    Object o = values.elementAt(tag);
    if (o == null){
      return 0;
    }
    return (o instanceof Vector) ? ((Vector) o).size() : 1;
  }

  /**
   * Returns the tag type of the given tag (one of the ProtoBufType.TYPE_XXX 
   * constants). If no ProtoBufType is set, the wire type is returned. If no
   * wire type is available, the wire type is determined by looking at the
   * tag value (making sure the wire type is consistent for all values). If
   * no value is set, TYPE_UNDEFINED is returned.
   */
  public int getType(int tag){
    int tagType = ProtoBufType.TYPE_UNDEFINED;
    if (msgType != null){
      tagType = msgType.getType(tag);
    }

    if (tagType == ProtoBufType.TYPE_UNDEFINED && tag < wireTypes.length()) {
      tagType = wireTypes.charAt(tag);
    }
    
    if (tagType == ProtoBufType.TYPE_UNDEFINED && getCount(tag) > 0) {
      Object o = getObject(tag, 0, ProtoBufType.TYPE_UNDEFINED);
      
      tagType = (o instanceof Long) || (o instanceof Boolean) 
        ? WIRETYPE_VARINT : WIRETYPE_LENGTH_DELIMITED;
    }
    
    return tagType;
  }
  
  /** 
   * Returns the number of bytes needed to store this protocol buffer 
   */
  public int getDataSize() {
    int size = 0;
    for (int tag = 0; tag <= maxTag(); tag++) {
      for (int i = 0; i < getCount(tag); i++) {
        size += getDataSize(tag, i);
      }
    }      
    return size;
  }
  
 
  /** 
   * Returns the size of the given value 
   */
  private int getDataSize(int tag, int i) {
    int tagSize = getVarIntSize(tag << 3);
    
    switch(getWireType(tag)){
      case WIRETYPE_FIXED32:
        return tagSize + 4;
      case WIRETYPE_FIXED64:
        return tagSize + 8;
      case WIRETYPE_VARINT:
        long value = getLong(tag, i);
        if (isZigZagEncodedType(tag)) {
          value = zigZagEncode(value);
        }
        return tagSize + getVarIntSize(value);
      case WIRETYPE_START_GROUP:
        // take end group into account....
        return tagSize + getProtoBuf(tag, i).getDataSize() + tagSize;
    }
    
    // take the object as stored
    Object o = getObject(tag, i, ProtoBufType.TYPE_UNDEFINED);
    
    int contentSize;
    
    if (o instanceof byte[]){
      contentSize = ((byte[]) o).length;
    } else if (o instanceof String) {
      contentSize = encodeUtf8((String) o, null, 0);
    } else {
      contentSize = ((ProtoBuf) o).getDataSize();
    }
    
    return tagSize + getVarIntSize(contentSize) + contentSize;
  }

  /**
   * Returns the number of bytes needed to encode the given value using 
   * WIRETYPE_VARINT
   */
  private static int getVarIntSize(long i) {
    if (i < 0) {
      return 10;
    }
    int size = 1;
    while (i >= 128) {
      size++;
      i >>= 7;
    }
    return size;
  }

  /** 
   * Writes this and nested protocol buffers to the given output stream.
   * 
   * @param os target output stream
   * @throws   IOException thrown if there is an IOException
   */
  public void outputTo(OutputStream os) throws IOException {
    for (int tag = 0; tag <= maxTag(); tag++) {
      int size = getCount(tag);
      int wireType = getWireType(tag);
      
      // ignore default values
      for (int i = 0; i < size; i++) {
        writeVarInt(os, (tag << 3) | wireType);

        switch (wireType) {
          case WIRETYPE_FIXED32:
          case WIRETYPE_FIXED64:
            long v = ((Long) getObject(tag, i, ProtoBufType.TYPE_INT64))
                .longValue();
            int cnt = (wireType == WIRETYPE_FIXED32) ? 4 : 8;
            for (int b = 0; b < cnt; b++) {
              os.write((int) (v & 0x0ff));
              v >>= 8;
            }
            break;

          case WIRETYPE_VARINT:
            v = ((Long) getObject(tag, i, ProtoBufType.TYPE_INT64)).longValue();
            if (isZigZagEncodedType(tag)) {
              v = zigZagEncode(v);
            }
            writeVarInt(os, v);
            break;

          case WIRETYPE_LENGTH_DELIMITED:
            Object o = getObject(tag, i, 
                getType(tag) == ProtoBufType.TYPE_MESSAGE 
                ? ProtoBufType.TYPE_UNDEFINED
                : ProtoBufType.TYPE_DATA);
            
            if (o instanceof byte[]){
              byte[] data = (byte[]) o;
              writeVarInt(os, data.length);
              os.write(data);
            } else {
              ProtoBuf msg = (ProtoBuf) o;
              writeVarInt(os, msg.getDataSize());
              msg.outputTo(os);
            }
            break;

          case WIRETYPE_START_GROUP:
            ((ProtoBuf) getObject(tag, i, ProtoBufType.TYPE_GROUP))
                .outputTo(os);
            writeVarInt(os, (tag << 3) | WIRETYPE_END_GROUP);
            break;
            
          default:
            throw new IllegalArgumentException();
        }
      }
    }
  }

  /**
   * Returns true if the given tag has a signed type that should be ZigZag-
   * encoded on the wire.
   *
   * ZigZag encoding turns a signed number into
   * a non-negative number by mapping negative input numbers to positive odd
   * numbers in the output space, and positive input numbers to positive even
   * numbers in the output space.  This is useful because the wire format
   * for protocol buffers requires a large number of bytes to encode
   * negative integers, while positive integers take up a smaller number
   * of bytes proportional to their magnitude.
   */
  private boolean isZigZagEncodedType(int tag) {
    int declaredType = getType(tag);
    return declaredType == ProtoBufType.TYPE_SINT32 ||
        declaredType == ProtoBufType.TYPE_SINT64;
  }

  /**
   * Converts a signed number into a non-negative ZigZag-encoded number.
   */
  private static long zigZagEncode(long v) {
    v = ((v << 1) ^ -(v >>> 63));
    return v;
  }

  /**
   * Converts a non-negative ZigZag-encoded number back into a signed number.
   */
  private static long zigZagDecode(long v) {
    v = (v >>> 1) ^ -(v & 1);
    return v;
  }

  /**
   * Writes this and nested protocol buffers to a byte array.
   *
   * @throws IOException thrown if there is problem writing the byte array
   */
  public byte[] toByteArray() throws IOException {
    ByteArrayOutputStream baos = new ByteArrayOutputStream();
    outputTo(baos);
    return baos.toByteArray();
  }
  
  /**
   * Returns the largest tag id used in this message (to simplify testing). 
   */
  public int maxTag() {
    return values.size() - 1;
  }

  /** 
   * Sets the given tag to the given boolean value. 
   */
  public void setBool(int tag, boolean value) {
    setObject(tag, value ? TRUE : FALSE);
  }

  /** 
   * Sets the given tag to the given data bytes. 
   */
  public void setBytes(int tag, byte[] value) {
    setObject(tag, value);
  }

  /** 
   * Sets the given tag to the given integer value. 
   */
  public void setInt(int tag, int value) {
    setLong(tag, value);
  }

  /** 
   * Sets the given tag to the given long value.
   */
  public void setLong(int tag, long value) {
    setObject(tag, value >= 0 && value < SMALL_NUMBERS.length
        ? SMALL_NUMBERS[(int) value] : new Long(value));
  }

  /**
   * Sets the given tag to the given double value.
   */
  public void setDouble(int tag, double value) {
    setLong(tag, Double.doubleToLongBits(value));
  }

  /**
   * Sets the given tag to the given float value.
   */
  public void setFloat(int tag, float value) {
    setInt(tag, Float.floatToIntBits(value));
  }

  /** 
   * Sets the given tag to the given Group or nested Message. 
   */
  public void setProtoBuf(int tag, ProtoBuf pb) {
    setObject(tag, pb);
  }

  /**
   * Sets a new protobuf for the specified tag, setting the child protobuf's
   * type correctly for the tag.
   * @param tag the tag for which to create a new protobuf
   * @return the newly created protobuf
   */
  public ProtoBuf setNewProtoBuf(int tag) {
    ProtoBuf child = newProtoBufForTag(tag);
    setProtoBuf(tag, child);
    return child;
  }

  /** 
   * Sets the given tag to the given String value. 
   */
  public void setString(int tag, String value) {
    setObject(tag, value);
  }

  /** 
   * Inserts the given boolean value for the given tag at the given index. 
   */
  public void insertBool(int tag, int index, boolean value) {
    insertObject(tag, index, value ? TRUE : FALSE);
  }

  /** 
   * Inserts the given byte array value for the given tag at the given index. 
   */
  public void insertBytes(int tag, int index, byte[] value) {
    insertObject(tag, index, value);
  }

  /** 
   * Inserts the given int value for the given tag at the given index. 
   */
  public void insertInt(int tag, int index, int value) {
    insertLong(tag, index, value);
  }

  /** 
   * Inserts the given long value for the given tag at the given index. 
   */
  public void insertLong(int tag, int index, long value) {
    insertObject(tag, index, value >= 0 && value < SMALL_NUMBERS.length
        ? SMALL_NUMBERS[(int) value] : new Long(value));
  }

  /**
   * Inserts the given float value for the given tag at the given index.
   */
  public void insertFloat(int tag, int index, float value) {
    insertInt(tag, index, Float.floatToIntBits(value));
  }

  /**
   * Inserts the given double value for the given tag at the given index.
   */
  public void insertDouble(int tag, int index, double value) {
    insertLong(tag, index, Double.doubleToLongBits(value));
  }

  /** 
   * Inserts the given group or message for the given tag at the given index. 
   */
  public void insertProtoBuf(int tag, int index, ProtoBuf pb) {
    insertObject(tag, index, pb);
  }

  /** 
   * Inserts the given string value for the given tag at the given index. 
   */
  public void insertString(int tag, int index, String value) {
    insertObject(tag, index, value);
  }

  // ----------------- private stuff below this line ------------------------

  private void assertTypeMatch(int tag, Object object){
    int tagType = getType(tag);
    if (tagType == ProtoBufType.TYPE_UNDEFINED && msgType == null) {
      return;
    }
    
    if (object instanceof Boolean) {
      if (tagType == ProtoBufType.TYPE_BOOL 
          || tagType == WIRETYPE_VARINT) {
        return;
      }
    } else if (object instanceof Long) {
      switch(tagType){
        case WIRETYPE_FIXED32:
        case WIRETYPE_FIXED64:
        case WIRETYPE_VARINT:
        case ProtoBufType.TYPE_BOOL:
        case ProtoBufType.TYPE_ENUM:
        case ProtoBufType.TYPE_FIXED32:
        case ProtoBufType.TYPE_FIXED64:
        case ProtoBufType.TYPE_INT32:
        case ProtoBufType.TYPE_INT64:
        case ProtoBufType.TYPE_SFIXED32:
        case ProtoBufType.TYPE_SFIXED64:
        case ProtoBufType.TYPE_UINT32:
        case ProtoBufType.TYPE_UINT64:
        case ProtoBufType.TYPE_SINT32:
        case ProtoBufType.TYPE_SINT64:
        case ProtoBufType.TYPE_FLOAT:
        case ProtoBufType.TYPE_DOUBLE:
          return;
      }
    } else if (object instanceof byte[]){
      switch (tagType){
        case WIRETYPE_LENGTH_DELIMITED:
        case ProtoBufType.TYPE_DATA:
        case ProtoBufType.TYPE_MESSAGE:
        case ProtoBufType.TYPE_TEXT:
        case ProtoBufType.TYPE_BYTES:
        case ProtoBufType.TYPE_STRING:
          return;
      }
    } else if (object instanceof ProtoBuf) {
      switch (tagType){
        case WIRETYPE_LENGTH_DELIMITED:
        case WIRETYPE_START_GROUP:
        case ProtoBufType.TYPE_DATA:
        case ProtoBufType.TYPE_GROUP:
        case ProtoBufType.TYPE_MESSAGE:
          if (msgType == null || msgType.getData(tag) == null ||
              ((ProtoBuf) object).msgType == null || 
              ((ProtoBuf) object).msgType == msgType.getData(tag)) {
            return;
          }
      }
    } else if (object instanceof String){
      switch (tagType){
        case WIRETYPE_LENGTH_DELIMITED:
        case ProtoBufType.TYPE_DATA:
        case ProtoBufType.TYPE_TEXT:
        case ProtoBufType.TYPE_STRING:
          return;
      }
    }
    throw new IllegalArgumentException(MSG_MISMATCH + " type:" + msgType + 
        " tag:" + tag);
  }

  /**
   * Returns the default value for the given tag.
   */
  private Object getDefault(int tag){

    switch(getType(tag)){
      case ProtoBufType.TYPE_UNDEFINED:
      case ProtoBufType.TYPE_GROUP:
      case ProtoBufType.TYPE_MESSAGE:
        return null;
      default:
        return msgType.getData(tag);
    }
  }
  
  /** 
   * Returns the indicated value converted to the given type. 
   * 
   * @throws ArrayIndexOutOfBoundsException for invalid tags and indices
   * @throws IllegalArgumentException if count is greater than one.
   */
  private Object getObject(int tag, int desiredType) {

    int count = getCount(tag);
    
    if (count == 0){
      return getDefault(tag);
    }
    
    if (count > 1){
      throw new IllegalArgumentException();
    }

    return getObject(tag, 0, desiredType);
  }

  /** 
   * Returns the indicated value converted to the given type. 
   * 
   * @throws ArrayIndexOutOfBoundsException for invalid tags and indices
   */
  private Object getObject(int tag, int index, int desiredType) {

    if (index >= getCount(tag)) {
      throw new ArrayIndexOutOfBoundsException();
    }

    Object o = values.elementAt(tag);
    
    Vector v = null;
    if (o instanceof Vector) {
      v = (Vector) o;
      o = v.elementAt(index);
    } 
    
    Object o2 = convert(o, desiredType);

    if (o2 != o && o != null) {
      if (v == null){
        setObject(tag, o2);
      } else {
        v.setElementAt(o2, index);
      }
    }
    
    return o2;
  }

  /** 
   * Returns the wire type for the given tag. Calls getType() internally,
   * so a wire type should be found for all non-empty tags, even if no
   * message type is set and the tag was not previously read.
   */
  private final int getWireType(int tag) {

    int tagType = getType(tag);
    
    switch (tagType) {
      case WIRETYPE_VARINT:
      case WIRETYPE_FIXED32:
      case WIRETYPE_FIXED64:
      case WIRETYPE_LENGTH_DELIMITED:
      case WIRETYPE_START_GROUP:
      case ProtoBufType.TYPE_UNDEFINED:
        return tagType;
      
      case ProtoBufType.TYPE_BOOL:
      case ProtoBufType.TYPE_INT32:
      case ProtoBufType.TYPE_INT64:
      case ProtoBufType.TYPE_UINT32:
      case ProtoBufType.TYPE_UINT64:
      case ProtoBufType.TYPE_SINT32:
      case ProtoBufType.TYPE_SINT64:
      case ProtoBufType.TYPE_ENUM:
        return WIRETYPE_VARINT;
      case ProtoBufType.TYPE_DATA:
      case ProtoBufType.TYPE_MESSAGE:
      case ProtoBufType.TYPE_TEXT:
      case ProtoBufType.TYPE_BYTES:
      case ProtoBufType.TYPE_STRING:
        return WIRETYPE_LENGTH_DELIMITED;
      case ProtoBufType.TYPE_DOUBLE:
      case ProtoBufType.TYPE_FIXED64:
      case ProtoBufType.TYPE_SFIXED64:
        return WIRETYPE_FIXED64;
      case ProtoBufType.TYPE_FLOAT:
      case ProtoBufType.TYPE_FIXED32:
      case ProtoBufType.TYPE_SFIXED32:
        return WIRETYPE_FIXED32;
      case ProtoBufType.TYPE_GROUP:
        return WIRETYPE_START_GROUP;
      default:
        throw new RuntimeException(MSG_UNSUPPORTED + ':' + msgType + '/' + 
            tag + '/' + tagType);
    }
  }
  
  /** 
   * Inserts a value.
   */
  private void insertObject(int tag, int index, Object o) {
    assertTypeMatch(tag, o);
    
    int count = getCount(tag);

    if (count == 0) {
      setObject(tag, o);
    } else {
      Object curr = values.elementAt(tag);
      Vector v;
      if (curr instanceof Vector) {
        v = (Vector) curr;
      } else {
        v = new Vector();
        v.addElement(curr);
        values.setElementAt(v, tag);
      }
      v.insertElementAt(o, index);
    }
  }

  /**
   * Converts the object if a better suited class exists for the given .proto 
   * type. If the formats are not compatible, an exception is thrown.
   */
  private Object convert(Object obj, int tagType) {
    switch (tagType) {
      case ProtoBufType.TYPE_UNDEFINED:
        return obj;

      case ProtoBufType.TYPE_BOOL:
        if (obj instanceof Boolean) {
          return obj;
        }
        switch ((int) ((Long) obj).longValue()) {
          case 0:
            return FALSE;
          case 1:
            return TRUE;
          default:
            throw new IllegalArgumentException(MSG_MISMATCH);
        }
      case ProtoBufType.TYPE_FIXED32:
      case ProtoBufType.TYPE_FIXED64:
      case ProtoBufType.TYPE_INT32:
      case ProtoBufType.TYPE_INT64:
      case ProtoBufType.TYPE_SFIXED32:
      case ProtoBufType.TYPE_SFIXED64:
      case ProtoBufType.TYPE_SINT32:
      case ProtoBufType.TYPE_SINT64:
        if (obj instanceof Boolean) {
          return SMALL_NUMBERS[((Boolean) obj).booleanValue() ? 1 : 0];
        }
        return obj;
      case ProtoBufType.TYPE_DATA:
      case ProtoBufType.TYPE_BYTES:
        if (obj instanceof String) {
          return encodeUtf8((String) obj);
        } else if (obj instanceof ProtoBuf) {
          ByteArrayOutputStream buf = new ByteArrayOutputStream();
          try {
            ((ProtoBuf) obj).outputTo(buf);
            return buf.toByteArray();
          } catch (IOException e) {
            throw new RuntimeException(e.toString());
          }
        }
        return obj;
      case ProtoBufType.TYPE_TEXT:
      case ProtoBufType.TYPE_STRING:
        if (obj instanceof byte[]) {
          byte[] data = (byte[]) obj;
          return decodeUtf8(data, 0, data.length, true);
        }
        return obj;
      case ProtoBufType.TYPE_GROUP:
      case ProtoBufType.TYPE_MESSAGE:
        if (obj instanceof byte[]) {
          try {
            return new ProtoBuf(null).parse((byte[]) obj);
          } catch (IOException e) {
            throw new RuntimeException(e.toString());
          }
        }
        return obj;
      default:
        // default includes FLOAT and DOUBLE
        throw new RuntimeException(MSG_UNSUPPORTED);
    }
  }

  /** 
   * Reads a variable-size integer (up to 10 bytes for 64 bit) from the 
   * given input stream.
   * 
   * @param is        the stream to read from
   * @param permitEOF if true, -1 is returned when EOF is reached instead of
   *                  throwing an IOException
   * @return          the integer value read from the stream, or -1 if EOF is
   *                  reached and permitEOF is true
   * @throws          IOException thrown for underlying IO issues and if EOF
   *                  is reached and permitEOF is false
   */
  static long readVarInt(InputStream is, boolean permitEOF) throws IOException {

    long result = 0;
    int shift = 0;

    // max 10 byte wire format for 64 bit integer (7 bit data per byte)

    for (int i = 0; i < VARINT_MAX_BYTES; i++) {
      int in = is.read();

      if (in == -1) {
        if (i == 0 && permitEOF) {
          return -1;
        } else {
          throw new IOException("EOF");
        }
      }
      result |= ((long) (in & 0x07f)) << shift;

      if ((in & 0x80) == 0){
        break; // get out early
      }
      
      shift += 7;
    }
    return result;
  }

  /**
   * Internal helper method to set a (single) value. Overwrites all existing 
   * values.
   */
  private void setObject(int tag, Object o) {
    if (values.size() <= tag) {
      values.setSize(tag + 1);
    }
    if (o != null) {
      assertTypeMatch(tag, o);
    }
    values.setElementAt(o, tag);
  }

  /** 
   * Write a variable-size integer to the given output stream.
   */
  static void writeVarInt(OutputStream os, long value) throws IOException {
    for (int i = 0; i < VARINT_MAX_BYTES; i++) {

      int toWrite = (int) (value & 0x7f);

      value >>>= 7;

      if (value == 0) {
        os.write(toWrite);
        break;
      } else {
        os.write(toWrite | 0x080);
      }
    }
  }

    /**
   * Returns a byte array containing the given string, encoded as UTF-8. The
   * returned byte array contains at least s.length() bytes and at most
   * 4 * s.length() bytes. UTF-16 surrogates are transcoded to UTF-8.
   *
   * @param s input string to be encoded
   * @return UTF-8 encoded input string
   */
  static byte[] encodeUtf8(String s) {
    int len = encodeUtf8(s, null, 0);
    byte[] result = new byte[len];
    encodeUtf8(s, result, 0);
    return result;
  }

  /**
   * Encodes the given string to UTF-8 in the given buffer or calculates
   * the space needed if the buffer is null.
   *
   * @param s the string to be UTF-8 encoded
   * @param buf byte array to write to
   * @return new buffer position after writing (which equals the required size
   *    if pos is 0)
   */
  static int encodeUtf8(String s, byte[] buf, int pos){
    int len = s.length();
    for (int i = 0; i < len; i++){
      int code = s.charAt(i);

      // surrogate 0xd800 .. 0xdfff?
      if (code >= 0x0d800 && code <= 0x0dfff && i + 1 < len){
        int codeLo = s.charAt(i + 1);

        // 0xfc00 is the surrogate id mask (first six bit of 16 set)
        // 0x03ff is the surrogate data mask (remaining 10 bit)
        // check if actually a surrogate pair (d800 ^ dc00 == 0400)
        if (((codeLo & 0xfc00) ^ (code & 0x0fc00)) == 0x0400){

          i += 1;

          int codeHi;
          if ((codeLo & 0xfc00) == 0x0d800){
            codeHi = codeLo;
            codeLo = code;
          } else {
            codeHi = code;
          }
          code = (((codeHi & 0x3ff) << 10) | (codeLo & 0x3ff)) + 0x10000;
        }
      }
      if (code <= 0x007f) {
        if (buf != null){
          buf[pos] = (byte) code;
        }
        pos += 1;
      } else if (code <= 0x07FF) {
        // non-ASCII <= 0x7FF
        if (buf != null){
          buf[pos] = (byte) (0xc0 | (code >> 6));
          buf[pos + 1] = (byte) (0x80 | (code & 0x3F));
        }
        pos += 2;
      } else if (code <= 0xFFFF){
        // 0x7FF < code <= 0xFFFF
        if (buf != null){
          buf[pos] = (byte) ((0xe0 | (code >> 12)));
          buf[pos + 1] = (byte) ((0x80 | ((code >> 6) & 0x3F)));
          buf[pos + 2] = (byte) ((0x80 | (code & 0x3F)));
        }
        pos += 3;
      } else {
        if (buf != null){
          buf[pos] = (byte) ((0xf0 | (code >> 18)));
          buf[pos + 1] = (byte) ((0x80 | ((code >> 12) & 0x3F)));
          buf[pos + 2] = (byte) ((0x80 | ((code >> 6) & 0x3F)));
          buf[pos + 3] = (byte) ((0x80 | (code & 0x3F)));
        }
        pos += 4;
      }
    }

    return pos;
  }

  /**
   * Decodes an array of UTF-8 bytes to a Java string (UTF-16). The tolerant
   * flag determines what to do in case of illegal or unsupported sequences.
   *
   * @param data input byte array containing UTF-8 data
   * @param start decoding start position in byte array
   * @param end decoding end position in byte array
   * @param tolerant if true, an IllegalArgumentException is thrown for illegal
   *    UTF-8 codes
   * @return the string containing the UTF-8 decoding result
   */
  static String decodeUtf8(byte[] data, int start, int end,
      boolean tolerant){

    StringBuffer sb = new StringBuffer(end - start);
    int pos = start;

    while (pos < end){
      int b = data[pos++] & 0x0ff;
      if (b <= 0x7f){
        sb.append((char) b);
      } else if (b >= 0xf5){ // byte sequence too long
        if (!tolerant){
          throw new IllegalArgumentException("Invalid UTF8");
        }
        sb.append((char) b);
      } else {
        int border = 0xe0;
        int count = 1;
        int minCode = 128;
        int mask = 0x01f;
        while (b >= border){
          border = (border >> 1) | 0x80;
          minCode = minCode << (count == 1 ? 4 : 5);
          count++;
          mask = mask >> 1;
        }
        int code = b & mask;

        for (int i = 0; i < count; i++){
          code = code << 6;
          if (pos >= end){
            if (!tolerant){
              throw new IllegalArgumentException("Invalid UTF8");
            }
            // otherwise, assume zeroes
          } else {
            if (!tolerant && (data[pos] & 0xc0) != 0x80){
              throw new IllegalArgumentException("Invalid UTF8");
            }
            code |= (data[pos++] & 0x3f); // six bit
          }
        }

        // illegal code or surrogate code
        if (!tolerant && code < minCode || (code >= 0xd800 && code <= 0xdfff)){
          throw new IllegalArgumentException("Invalid UTF8");
        }

        if (code <= 0x0ffff){
          sb.append((char) code);
        } else { // surrogate UTF16
          code -= 0x10000;
          sb.append((char) (0xd800 | (code >> 10))); // high 10 bit
          sb.append((char) (0xdc00 | (code & 0x3ff))); // low 10 bit
        }
      }
    }
    return sb.toString();
  }

}
