// Copyright 2008 The Android Open Source Project

package com.google.common.io.protocol;

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
      ProtoBuf subProto =
          (proto != null && proto.has(sub)) ? proto.getProtoBuf(sub) : null;
      return getProtoValueOrEmpty(subProto, tag);
    } catch (ClassCastException e) {
      return "";
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
    try {
      return (proto != null && proto.has(tag)) ? proto.getInt(tag) : 0;
    } catch (IllegalArgumentException e) {
      return 0;
    } catch (ClassCastException e) {
      return 0;
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
      ProtoBuf subProto =
          (proto != null && proto.has(sub)) ? proto.getProtoBuf(sub) : null;
      return getProtoValueOrNegativeOne(subProto, tag);
    } catch (IllegalArgumentException e) {
      return -1;
    } catch (ClassCastException e) {
      return -1; 
    }
  }
}
