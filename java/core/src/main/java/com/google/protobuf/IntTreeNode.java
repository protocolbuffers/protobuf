// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;

import java.util.Map.Entry;
import java.util.Objects;

final class IntTreeNode<V> implements Entry<Integer, V> {

  private static final int COLOR_BIT = 1 << 31;
  private static final int COLOR_MASK = ~COLOR_BIT;

  IntTreeNode<V> parent = null;
  IntTreeNode<V> left = null;
  IntTreeNode<V> right = null;
  private V value;
  private int keyAndColor;

  IntTreeNode(int key, V value, boolean color) {
    if (key <= 0) {
      throw new IllegalArgumentException("Key cannot be zero or negative: " + key);
    }
    this.value = checkNotNull(value);
    this.keyAndColor = key | (color == RedBlackTreeColor.RED ? COLOR_BIT : 0);
  }

  void setColor(boolean color) {
    if (color) {
      // RED
      keyAndColor |= COLOR_BIT;
    } else {
      // BLACK
      keyAndColor &= COLOR_MASK;
    }
  }

  boolean getColor() {
    return (keyAndColor & COLOR_BIT) != 0 ? RedBlackTreeColor.RED : RedBlackTreeColor.BLACK;
  }

  @SuppressWarnings("ReferenceEquality")
  IntTreeNode<V> prev() {
    IntTreeNode<V> node = this;
    if (node.left != null) {
      node = node.left;
      while (node.right != null) {
        node = node.right;
      }
    } else {
      if (node.parent != null && node == node.parent.right) {
        node = node.parent;
      } else {
        while (node.parent != null && node == node.parent.left) {
          node = node.parent;
        }
        node = node.parent;
      }
    }
    return node;
  }

  @SuppressWarnings("ReferenceEquality")
  IntTreeNode<V> next() {
    IntTreeNode<V> node = this;
    if (node.right != null) {
      node = node.right;
      while (node.left != null) {
        node = node.left;
      }
    } else {
      if (node.parent != null && node == node.parent.left) {
        node = node.parent;
      } else {
        while (node.parent != null && node == node.parent.right) {
          node = node.parent;
        }
        node = node.parent;
      }
    }
    return node;
  }

  @Override
  public Integer getKey() {
    return getKeyAsInt();
  }

  int getKeyAsInt() {
    return keyAndColor & COLOR_MASK;
  }

  @Override
  public V getValue() {
    return value;
  }

  @Override
  public V setValue(V value) {
    V old = this.value;
    this.value = checkNotNull(value);
    return old;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(getKeyAsInt()) ^ Objects.hashCode(getValue());
  }

  @Override
  public boolean equals(Object other) {
    if (other instanceof IntTreeNode) {
      return equalsFast((IntTreeNode<?>) other);
    }
    if (other instanceof Entry) {
      return equalsSlow((Entry<?, ?>) other);
    }
    return false;
  }

  @SuppressWarnings("ReferenceEquality")
  boolean equalsFast(IntTreeNode<?> that) {
    return this == that
        || (getKeyAsInt() == that.getKeyAsInt() && Objects.equals(getValue(), that.getValue()));
  }

  private boolean equalsSlow(Entry<?, ?> that) {
    return Objects.equals(getKey(), that.getKey()) && Objects.equals(getValue(), that.getValue());
  }
}
