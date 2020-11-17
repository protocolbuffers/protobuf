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

import com.google.common.base.Splitter;
import com.google.errorprone.annotations.CanIgnoreReturnValue;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.FieldMask;
import com.google.protobuf.Message;
import java.util.ArrayList;
import java.util.List;
import java.util.Map.Entry;
import java.util.SortedMap;
import java.util.TreeMap;
import java.util.logging.Logger;

/**
 * A tree representation of a FieldMask. Each leaf node in this tree represent
 * a field path in the FieldMask.
 *
 * <p>For example, FieldMask "foo.bar,foo.baz,bar.baz" as a tree will be:
 * <pre>
 *   [root] -+- foo -+- bar
 *           |       |
 *           |       +- baz
 *           |
 *           +- bar --- baz
 * </pre>
 *
 * <p>By representing FieldMasks with this tree structure we can easily convert
 * a FieldMask to a canonical form, merge two FieldMasks, calculate the
 * intersection to two FieldMasks and traverse all fields specified by the
 * FieldMask in a message tree.
 */
final class FieldMaskTree {
  private static final Logger logger = Logger.getLogger(FieldMaskTree.class.getName());

  private static final String FIELD_PATH_SEPARATOR_REGEX = "\\.";

  private static final class Node {
    final SortedMap<String, Node> children = new TreeMap<>();
  }

  private final Node root = new Node();

  /**
   * Creates an empty FieldMaskTree.
   */
  FieldMaskTree() {}

  /**
   * Creates a FieldMaskTree for a given FieldMask.
   */
  FieldMaskTree(FieldMask mask) {
    mergeFromFieldMask(mask);
  }

  @Override
  public String toString() {
    return FieldMaskUtil.toString(toFieldMask());
  }

  /**
   * Adds a field path to the tree. In a FieldMask, every field path matches the specified field as
   * well as all its sub-fields. For example, a field path "foo.bar" matches field "foo.bar" and
   * also "foo.bar.baz", etc. When adding a field path to the tree, redundant sub-paths will be
   * removed. That is, after adding "foo.bar" to the tree, "foo.bar.baz" will be removed if it
   * exists, which will turn the tree node for "foo.bar" to a leaf node. Likewise, if the field path
   * to add is a sub-path of an existing leaf node, nothing will be changed in the tree.
   */
  @CanIgnoreReturnValue
  @SuppressWarnings("StringSplitter")
  FieldMaskTree addFieldPath(String path) {
    String[] parts = path.split(FIELD_PATH_SEPARATOR_REGEX);
    if (parts.length == 0) {
      return this;
    }
    Node node = root;
    boolean createNewBranch = false;
    // Find the matching node in the tree.
    for (String part : parts) {
      // Check whether the path matches an existing leaf node.
      if (!createNewBranch && node != root && node.children.isEmpty()) {
        // The path to add is a sub-path of an existing leaf node.
        return this;
      }
      if (node.children.containsKey(part)) {
        node = node.children.get(part);
      } else {
        createNewBranch = true;
        Node tmp = new Node();
        node.children.put(part, tmp);
        node = tmp;
      }
    }
    // Turn the matching node into a leaf node (i.e., remove sub-paths).
    node.children.clear();
    return this;
  }

  /** Merges all field paths in a FieldMask into this tree. */
  @CanIgnoreReturnValue
  FieldMaskTree mergeFromFieldMask(FieldMask mask) {
    for (String path : mask.getPathsList()) {
      addFieldPath(path);
    }
    return this;
  }

  /**
   * Removes {@code path} from the tree.
   *
   * <ul>
   *   When removing a field path from the tree:
   *   <li>All sub-paths will be removed. That is, after removing "foo.bar" from the tree,
   *       "foo.bar.baz" will be removed.
   *   <li>If all children of a node has been removed, the node itself will be removed as well.
   *       That is, if "foo" only has one child "bar" and "foo.bar" only has one child "baz",
   *       removing "foo.bar.barz" would remove both "foo" and "foo.bar".
   *       If "foo" has both "bar" and "qux" as children, removing "foo.bar" would leave the path
   *       "foo.qux" intact.
   *   <li>If the field path to remove is a non-exist sub-path, nothing will be changed.
   * </ul>
   */
  @CanIgnoreReturnValue
  FieldMaskTree removeFieldPath(String path) {
    List<String> parts = Splitter.onPattern(FIELD_PATH_SEPARATOR_REGEX).splitToList(path);
    if (parts.isEmpty()) {
      return this;
    }
    removeFieldPath(root, parts, 0);
    return this;
  }

  /**
   * Removes {@code parts} from {@code node} recursively.
   *
   * @return a boolean value indicating whether current {@code node} should be removed.
   */
  @CanIgnoreReturnValue
  private static boolean removeFieldPath(Node node, List<String> parts, int index) {
    String key = parts.get(index);

    // Base case 1: path not match.
    if (!node.children.containsKey(key)) {
      return false;
    }
    // Base case 2: last element in parts.
    if (index == parts.size() - 1) {
      node.children.remove(key);
      return node.children.isEmpty();
    }
    // Recursive remove sub-path.
    if (removeFieldPath(node.children.get(key), parts, index + 1)) {
      node.children.remove(key);
    }
    return node.children.isEmpty();
  }

  /** Removes all field paths in {@code mask} from this tree. */
  @CanIgnoreReturnValue
  FieldMaskTree removeFromFieldMask(FieldMask mask) {
    for (String path : mask.getPathsList()) {
      removeFieldPath(path);
    }
    return this;
  }

  /**
   * Converts this tree to a FieldMask.
   */
  FieldMask toFieldMask() {
    if (root.children.isEmpty()) {
      return FieldMask.getDefaultInstance();
    }
    List<String> paths = new ArrayList<>();
    getFieldPaths(root, "", paths);
    return FieldMask.newBuilder().addAllPaths(paths).build();
  }

  /** Gathers all field paths in a sub-tree. */
  private static void getFieldPaths(Node node, String path, List<String> paths) {
    if (node.children.isEmpty()) {
      paths.add(path);
      return;
    }
    for (Entry<String, Node> entry : node.children.entrySet()) {
      String childPath = path.isEmpty() ? entry.getKey() : path + "." + entry.getKey();
      getFieldPaths(entry.getValue(), childPath, paths);
    }
  }

  /**
   * Adds the intersection of this tree with the given {@code path} to {@code output}.
   */
  void intersectFieldPath(String path, FieldMaskTree output) {
    if (root.children.isEmpty()) {
      return;
    }
    String[] parts = path.split(FIELD_PATH_SEPARATOR_REGEX);
    if (parts.length == 0) {
      return;
    }
    Node node = root;
    for (String part : parts) {
      if (node != root && node.children.isEmpty()) {
        // The given path is a sub-path of an existing leaf node in the tree.
        output.addFieldPath(path);
        return;
      }
      if (node.children.containsKey(part)) {
        node = node.children.get(part);
      } else {
        return;
      }
    }
    // We found a matching node for the path. All leaf children of this matching
    // node is in the intersection.
    List<String> paths = new ArrayList<>();
    getFieldPaths(node, path, paths);
    for (String value : paths) {
      output.addFieldPath(value);
    }
  }

  /**
   * Merges all fields specified by this FieldMaskTree from {@code source} to {@code destination}.
   */
  void merge(Message source, Message.Builder destination, FieldMaskUtil.MergeOptions options) {
    if (source.getDescriptorForType() != destination.getDescriptorForType()) {
      throw new IllegalArgumentException("Cannot merge messages of different types.");
    }
    if (root.children.isEmpty()) {
      return;
    }
    merge(root, "", source, destination, options);
  }

  /** Merges all fields specified by a sub-tree from {@code source} to {@code destination}. */
  private static void merge(
      Node node,
      String path,
      Message source,
      Message.Builder destination,
      FieldMaskUtil.MergeOptions options) {
    if (source.getDescriptorForType() != destination.getDescriptorForType()) {
      throw new IllegalArgumentException(
          String.format(
              "source (%s) and destination (%s) descriptor must be equal",
              source.getDescriptorForType(), destination.getDescriptorForType()));
    }

    Descriptor descriptor = source.getDescriptorForType();
    for (Entry<String, Node> entry : node.children.entrySet()) {
      FieldDescriptor field = descriptor.findFieldByName(entry.getKey());
      if (field == null) {
        logger.warning(
            "Cannot find field \""
                + entry.getKey()
                + "\" in message type "
                + descriptor.getFullName());
        continue;
      }
      if (!entry.getValue().children.isEmpty()) {
        if (field.isRepeated() || field.getJavaType() != FieldDescriptor.JavaType.MESSAGE) {
          logger.warning(
              "Field \""
                  + field.getFullName()
                  + "\" is not a "
                  + "singular message field and cannot have sub-fields.");
          continue;
        }
        if (!source.hasField(field) && !destination.hasField(field)) {
          // If the message field is not present in both source and destination, skip recursing
          // so we don't create unnecessary empty messages.
          continue;
        }
        String childPath = path.isEmpty() ? entry.getKey() : path + "." + entry.getKey();
        Message.Builder childBuilder = ((Message) destination.getField(field)).toBuilder();
        merge(entry.getValue(), childPath, (Message) source.getField(field), childBuilder, options);
        destination.setField(field, childBuilder.buildPartial());
        continue;
      }
      if (field.isRepeated()) {
        if (options.replaceRepeatedFields()) {
          destination.setField(field, source.getField(field));
        } else {
          for (Object element : (List) source.getField(field)) {
            destination.addRepeatedField(field, element);
          }
        }
      } else {
        if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
          if (options.replaceMessageFields()) {
            if (!source.hasField(field)) {
              destination.clearField(field);
            } else {
              destination.setField(field, source.getField(field));
            }
          } else {
            if (source.hasField(field)) {
              destination.setField(
                  field,
                  ((Message) destination.getField(field))
                      .toBuilder()
                      .mergeFrom((Message) source.getField(field))
                      .build());
            }
          }
        } else {
          if (source.hasField(field) || !options.replacePrimitiveFields()) {
            destination.setField(field, source.getField(field));
          } else {
            destination.clearField(field);
          }
        }
      }
    }
  }
}
