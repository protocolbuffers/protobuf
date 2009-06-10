// Copyright 2007 Google Inc.
// All Rights Reserved.

package com.google.common.io.protocol;

import java.util.*;

/**
 * This class can be used to create a memory model of a .proto file. Currently, 
 * it is assumed that tags ids are not large. This could be improved by storing 
 * a start offset, relaxing the assumption to a dense number space.
 */
public class ProtoBufType {
  // Note: Values 0..15 are reserved for wire types!
  public static final int TYPE_UNDEFINED = 16;
  public static final int TYPE_DOUBLE = 17;
  public static final int TYPE_FLOAT = 18;
  public static final int TYPE_INT64 = 19;
  public static final int TYPE_UINT64 = 20;
  public static final int TYPE_INT32 = 21;
  public static final int TYPE_FIXED64 = 22;
  public static final int TYPE_FIXED32 = 23;
  public static final int TYPE_BOOL = 24;
  public static final int TYPE_DATA = 25;
  public static final int TYPE_GROUP = 26;
  public static final int TYPE_MESSAGE = 27;
  public static final int TYPE_TEXT = 28;
  public static final int TYPE_UINT32 = 29;
  public static final int TYPE_ENUM = 30;
  public static final int TYPE_SFIXED32 = 31;
  public static final int TYPE_SFIXED64 = 32;
  
  // new protobuf 2 types
  public static final int TYPE_SINT32 = 33;
  public static final int TYPE_SINT64 = 34;
  public static final int TYPE_BYTES = 35;
  public static final int TYPE_STRING = 36;

  public static final int MASK_TYPE = 0x0ff;
  public static final int MASK_MODIFIER = 0x0ff00;

  public static final int REQUIRED = 0x100;
  public static final int OPTIONAL = 0x200;
  public static final int REPEATED = 0x400;
  
  private final StringBuffer types = new StringBuffer();
  private final Vector data = new Vector();
  private final String typeName;
  
  /**
   * Empty constructor.
   */
  public ProtoBufType() {
    typeName = null;
  }
  
  /**
   * Constructor including a type name for debugging purposes.
   */
  public ProtoBufType(String typeName) {
    this.typeName = typeName;
  }
  
  /**
   * Adds a tag description. The data parameter contains the group definition 
   * for group elements and the default value for regular elements.
   * 
   * @param optionsAndType any legal combination (bitwise or) of REQUIRED 
   *                       or OPTIONAL and REPEATED and one of the TYPE_ 
   *                       constants
   * @param tag            the tag id
   * @param data           the type for group elements (or the default value for
   *                       regular elements in future versions)
   * @return               this is returned to permit cascading
   */
  public ProtoBufType addElement(int optionsAndType, int tag, Object data) {
    while (types.length() <= tag) {
      types.append((char) TYPE_UNDEFINED);
      this.data.addElement(null);
    }
    types.setCharAt(tag, (char) optionsAndType);
    this.data.setElementAt(data, tag);

    return this;
  }
  
  /** 
   * Returns the type for the given tag id (without modifiers such as OPTIONAL,
   * REPEATED). For undefined tags, TYPE_UNDEFINED is returned.
   */  
  public int getType(int tag) {
    return (tag < 0 || tag >= types.length()) 
        ? TYPE_UNDEFINED
        : (types.charAt(tag) & MASK_TYPE);
  }
  
  /** 
   * Returns a bit combination of the modifiers for the given tag id 
   * (OPTIONAL, REPEATED, REQUIRED). For undefined tags, OPTIONAL|REPEATED
   * is returned.
   */  
  public int getModifiers(int tag) {
    return (tag < 0 || tag >= types.length()) 
        ? (OPTIONAL | REPEATED)
        : (types.charAt(tag) & MASK_MODIFIER);
  }
  
  /**
   * Returns the data associated to a given tag (either the default value for 
   * regular elements or a ProtoBufType for groups and messages). For undefined
   * tags, null is returned.
   */
  public Object getData(int tag) {
    return (tag < 0 || tag >= data.size()) ? null : data.elementAt(tag);
  }
  
  /**
   * Returns the type name set in the constructor for debugging purposes.
   */
  public String toString() {
    return typeName;
  }
  
  /**
   * {@inheritDoc}
   * <p>Two ProtoBufTypes are equals if the fields types are the same.
   */
  public boolean equals(Object object) {
    if (null == object) {
      // trivial check
      return false;
    } else if (this == object) {
      // trivial check
      return true;
    } else if (this.getClass() != object.getClass()) {
      // different class
      return false;
    }
    ProtoBufType other = (ProtoBufType) object;

    return stringEquals(types, other.types);
  }
   
  /**
   * {@inheritDoc}
   */
  public int hashCode() {
    if (types != null) {
      return types.hashCode();
    } else {
      return super.hashCode();
    }
  }

  public static boolean stringEquals(CharSequence a, CharSequence b) {
    if (a == b) return true;
    int length;
    if (a != null && b != null && (length = a.length()) == b.length()) {
      if (a instanceof String && b instanceof String) {
        return a.equals(b);
      } else {
        for (int i = 0; i < length; i++) {
          if (a.charAt(i) != b.charAt(i)) return false;
        }
        return true;
      }
    }
    return false;
  }
}
