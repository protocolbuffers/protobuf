// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

package com.google.protobuf.util;

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.FieldMask;
import com.google.protobuf.Internal;
import com.google.protobuf.Message;

import java.util.Arrays;
import java.util.List;

/**
 * Utility helper functions to work with {@link com.google.protobuf.FieldMask}.
 */
public class FieldMaskUtil {
  private static final String FIELD_PATH_SEPARATOR = ",";
  private static final String FIELD_PATH_SEPARATOR_REGEX = ",";
  private static final String FIELD_SEPARATOR_REGEX = "\\.";
  
  private FieldMaskUtil() {}

  /**
   * Converts a FieldMask to a string.
   */
  public static String toString(FieldMask fieldMask) {
    StringBuilder result = new StringBuilder();
    boolean first = true;
    for (String value : fieldMask.getPathsList()) {
      if (value.isEmpty()) {
        // Ignore empty paths.
        continue;
      }
      if (first) {
        first = false;
      } else {
        result.append(FIELD_PATH_SEPARATOR);
      }
      result.append(value);
    }
    return result.toString();
  }

  /**
   * Parses from a string to a FieldMask.
   */
  public static FieldMask fromString(String value) {
    return fromStringList(
        null, Arrays.asList(value.split(FIELD_PATH_SEPARATOR_REGEX)));
  }

  /**
   * Parses from a string to a FieldMask and validates all field paths.
   * 
   * @throws IllegalArgumentException if any of the field path is invalid.
   */
  public static FieldMask fromString(Class<? extends Message> type, String value)
      throws IllegalArgumentException {
    return fromStringList(
        type, Arrays.asList(value.split(FIELD_PATH_SEPARATOR_REGEX)));
  }

  /**
   * Constructs a FieldMask for a list of field paths in a certain type.
   *
   * @throws IllegalArgumentException if any of the field path is not valid.
   */
  public static FieldMask fromStringList(
      Class<? extends Message> type, List<String> paths)
      throws IllegalArgumentException {
    FieldMask.Builder builder = FieldMask.newBuilder();
    for (String path : paths) {
      if (path.isEmpty()) {
        // Ignore empty field paths.
        continue;
      }
      if (type != null && !isValid(type, path)) {
        throw new IllegalArgumentException(
            path + " is not a valid path for " + type);
      }
      builder.addPaths(path);
    }
    return builder.build();
  }

  /**
   * Checks whether a given field path is valid.
   */
  public static boolean isValid(Class<? extends Message> type, String path) {
    String[] parts = path.split(FIELD_SEPARATOR_REGEX);
    if (parts.length == 0) {
      return false;
    }
    Descriptor descriptor =
        Internal.getDefaultInstance(type).getDescriptorForType();
    for (String name : parts) {
      if (descriptor == null) {
        return false;
      }
      FieldDescriptor field = descriptor.findFieldByName(name);
      if (field == null) {
        return false;
      }
      if (!field.isRepeated()
          && field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
        descriptor = field.getMessageType();
      } else {
        descriptor = null;
      }
    }
    return true;
  }
  
  /**
   * Converts a FieldMask to its canonical form. In the canonical form of a
   * FieldMask, all field paths are sorted alphabetically and redundant field
   * paths are moved.
   */
  public static FieldMask normalize(FieldMask mask) {
    return new FieldMaskTree(mask).toFieldMask();
  }
  
  /**
   * Creates an union of two FieldMasks.
   */
  public static FieldMask union(FieldMask mask1, FieldMask mask2) {
    return new FieldMaskTree(mask1).mergeFromFieldMask(mask2).toFieldMask();
  }
  
  /**
   * Calculates the intersection of two FieldMasks.
   */
  public static FieldMask intersection(FieldMask mask1, FieldMask mask2) {
    FieldMaskTree tree = new FieldMaskTree(mask1);
    FieldMaskTree result = new FieldMaskTree();
    for (String path : mask2.getPathsList()) {
      tree.intersectFieldPath(path, result);
    }
    return result.toFieldMask();
  }

  /**
   * Options to customize merging behavior.
   */
  public static class MergeOptions {
    private boolean replaceMessageFields = false;
    private boolean replaceRepeatedFields = false;

    /**
     * Whether to replace message fields (i.e., discard existing content in
     * destination message fields) when merging.
     * Default behavior is to merge the source message field into the
     * destination message field.
     */ 
    public boolean replaceMessageFields() {
      return replaceMessageFields;
    }

    /**
     * Whether to replace repeated fields (i.e., discard existing content in
     * destination repeated fields) when merging.
     * Default behavior is to append elements from source repeated field to the
     * destination repeated field.
     */
    public boolean replaceRepeatedFields() {
      return replaceRepeatedFields;
    }
    
    public void setReplaceMessageFields(boolean value) {
      replaceMessageFields = value;
    }

    public void setReplaceRepeatedFields(boolean value) {
      replaceRepeatedFields = value;
    }
  }
  
  /**
   * Merges fields specified by a FieldMask from one message to another.
   */
  public static void merge(FieldMask mask, Message source,
      Message.Builder destination, MergeOptions options) {
    new FieldMaskTree(mask).merge(source, destination, options);
  }

  /**
   * Merges fields specified by a FieldMask from one message to another.
   */
  public static void merge(FieldMask mask, Message source,
      Message.Builder destination) {
    merge(mask, source, destination, new MergeOptions());
  }
}
