// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;

import java.util.AbstractMap;
import java.util.AbstractSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.function.IntFunction;

final class IntTreeMap<T> extends AbstractMap<Integer, T> {

  private IntTreeNode<T> root = null;
  private transient Entries entries = null;
  private int size = 0;

  IntTreeMap() {}

  @Override
  public int size() {
    return size;
  }

  T computeIfAbsent(int key, IntFunction<? extends T> compute) {
    checkNotNull(compute);
    IntTreeNode<T> node = root;
    IntTreeNode<T> parent = null;
    int diff = 0;
    while (node != null) {
      parent = node;
      diff = Integer.compare(key, node.getKeyAsInt());
      if (diff < 0) {
        node = node.left;
      } else if (diff > 0) {
        node = node.right;
      } else {
        return node.getValue();
      }
    }
    T value = compute.apply(key);
    if (value == null) {
      return null;
    }
    insertNode(key, value, parent, diff);
    return value;
  }

  @Override
  public T get(Object key) {
    IntTreeNode<T> node = getNode(key);
    if (node == null) {
      return null;
    }
    return node.getValue();
  }

  T getFast(int key) {
    IntTreeNode<T> node = getNodeFast(key);
    if (node == null) {
      return null;
    }
    return node.getValue();
  }

  private IntTreeNode<T> getNode(Object key) {
    if (key instanceof Byte || key instanceof Short || key instanceof Integer) {
      return getNodeFast(((Number) key).intValue());
    }
    if (key instanceof Long) {
      final long keyAsLong = (long) key;
      if (keyAsLong < Integer.MIN_VALUE || keyAsLong > Integer.MAX_VALUE) {
        return null;
      }
      return getNodeFast((int) keyAsLong);
    }
    if (key instanceof Float) {
      final float keyAsFloat = (float) key;
      if ((float) (int) keyAsFloat != keyAsFloat) {
        return null;
      }
      return getNodeFast((int) keyAsFloat);
    }
    if (key instanceof Double) {
      final double keyAsDouble = (double) key;
      if ((double) (long) keyAsDouble != keyAsDouble) {
        return null;
      }
      final long keyAsLong = (long) keyAsDouble;
      if (keyAsLong < Integer.MIN_VALUE || keyAsLong > Integer.MAX_VALUE) {
        return null;
      }
      return getNodeFast((int) keyAsLong);
    }
    return getNodeSlow(key);
  }

  private IntTreeNode<T> getNodeFast(int key) {
    if (key >= 0) {
      IntTreeNode<T> node = root;
      while (node != null) {
        int diff = Integer.compare(key, node.getKeyAsInt());
        if (diff < 0) {
          node = node.left;
        } else if (diff > 0) {
          node = node.right;
        } else {
          return node;
        }
      }
    }
    return null;
  }

  private IntTreeNode<T> getNodeSlow(Object key) {
    @SuppressWarnings("unchecked")
    Comparable<? super Integer> comparable = (Comparable<? super Integer>) key;
    IntTreeNode<T> node = root;
    while (node != null) {
      int diff = comparable.compareTo(node.getKey());
      if (diff < 0) {
        node = node.left;
      } else if (diff > 0) {
        node = node.right;
      } else {
        return node;
      }
    }
    return null;
  }

  @Override
  public boolean containsKey(Object key) {
    if (key instanceof Byte || key instanceof Short || key instanceof Integer) {
      return containsKeyFast(((Number) key).intValue());
    }
    if (key instanceof Long) {
      final long keyAsLong = (long) key;
      if (keyAsLong < Integer.MIN_VALUE || keyAsLong > Integer.MAX_VALUE) {
        return false;
      }
      return containsKeyFast((int) keyAsLong);
    }
    if (key instanceof Float) {
      final float keyAsFloat = (float) key;
      if ((float) (int) keyAsFloat != keyAsFloat) {
        return false;
      }
      return containsKeyFast((int) keyAsFloat);
    }
    if (key instanceof Double) {
      final double keyAsDouble = (double) key;
      if ((double) (long) keyAsDouble != keyAsDouble) {
        return false;
      }
      final long keyAsLong = (long) keyAsDouble;
      if (keyAsLong < Integer.MIN_VALUE || keyAsLong > Integer.MAX_VALUE) {
        return false;
      }
      return containsKeyFast((int) keyAsLong);
    }
    return containsKeySlow(key);
  }

  boolean containsKeyFast(int key) {
    return getFast(key) != null;
  }

  private boolean containsKeySlow(Object key) {
    if (key != null) {
      IntTreeNode<T> node = min();
      while (node != null) {
        if (Objects.equals(node.getKey(), key)) {
          return true;
        }
        node = next(node);
      }
    }
    return false;
  }

  @Override
  public boolean containsValue(Object value) {
    IntTreeNode<T> node = min();
    while (node != null) {
      if (Objects.equals(node.getValue(), value)) {
        return true;
      }
      node = next(node);
    }
    return false;
  }

  @Override
  public void clear() {
    root = null;
    size = 0;
  }

  @CanIgnoreReturnValue
  @Override
  public T put(Integer key, T value) {
    return put((int) checkNotNull(key), value);
  }

  @CanIgnoreReturnValue
  T put(int key, T value) {
    checkNotNull(value);
    IntTreeNode<T> node = root;
    IntTreeNode<T> parent = null;
    int diff = 0;
    while (node != null) {
      parent = node;
      diff = Integer.compare(key, node.getKeyAsInt());
      if (diff < 0) {
        node = node.left;
      } else if (diff > 0) {
        node = node.right;
      } else {
        return node.setValue(value);
      }
    }
    insertNode(key, value, parent, diff);
    return value;
  }

  private void insertNode(int key, T value, IntTreeNode<T> parent, int diff) {
    IntTreeNode<T> node = new IntTreeNode<>(key, value, RedBlackTreeColor.RED);
    node.parent = parent;
    if (parent != null) {
      if (diff < 0) {
        parent.left = node;
      } else {
        parent.right = node;
      }
    } else {
      root = node;
    }
    insertColor(node);
    ++size;
  }

  @Override
  @SuppressWarnings("unchecked")
  public void putAll(Map<? extends Integer, ? extends T> m) {
    if (m instanceof IntTreeMap) {
      putAllFast((IntTreeMap<? extends T>) m);
      return;
    }
    putAllSlow(m);
  }

  void putAllFast(IntTreeMap<? extends T> m) {
    IntTreeNode<? extends T> node = m.min();
    while (node != null) {
      put(node.getKeyAsInt(), node.getValue());
      node = next(node);
    }
  }

  private void putAllSlow(Map<? extends Integer, ? extends T> m) {
    for (Map.Entry<? extends Integer, ? extends T> e : m.entrySet()) {
      put(e.getKey(), e.getValue());
    }
  }

  @CanIgnoreReturnValue
  @Override
  public T remove(Object key) {
    if (key instanceof Byte || key instanceof Short || key instanceof Integer) {
      return removeFast(((Number) key).intValue());
    }
    if (key instanceof Long) {
      final long keyAsLong = (long) key;
      if (keyAsLong < Integer.MIN_VALUE || keyAsLong > Integer.MAX_VALUE) {
        return null;
      }
      return removeFast((int) keyAsLong);
    }
    if (key instanceof Float) {
      final float keyAsFloat = (float) key;
      if ((float) (int) keyAsFloat != keyAsFloat) {
        return null;
      }
      return removeFast((int) keyAsFloat);
    }
    if (key instanceof Double) {
      final double keyAsDouble = (double) key;
      if ((double) (long) keyAsDouble != keyAsDouble) {
        return null;
      }
      final long keyAsLong = (long) keyAsDouble;
      if (keyAsLong < Integer.MIN_VALUE || keyAsLong > Integer.MAX_VALUE) {
        return null;
      }
      return removeFast((int) keyAsLong);
    }
    return removeSlow(key);
  }

  @CanIgnoreReturnValue
  T removeFast(int key) {
    if (key <= 0) {
      return null;
    }
    IntTreeNode<T> node = root;
    while (node != null) {
      int diff = Integer.compare(key, node.getKeyAsInt());
      if (diff < 0) {
        node = node.left;
      } else if (diff > 0) {
        node = node.right;
      } else {
        break;
      }
    }
    if (node == null) {
      return null;
    }
    removeNode(node);
    return node.getValue();
  }

  @CanIgnoreReturnValue
  private T removeSlow(Object key) {
    if (key == null) {
      return null;
    }
    @SuppressWarnings("unchecked")
    Comparable<? super Integer> comparable = (Comparable<? super Integer>) key;
    IntTreeNode<T> node = root;
    while (node != null) {
      int diff = comparable.compareTo(node.getKey());
      if (diff < 0) {
        node = node.left;
      } else if (diff > 0) {
        node = node.right;
      } else {
        break;
      }
    }
    if (node == null) {
      return null;
    }
    removeNode(node);
    return node.getValue();
  }

  @SuppressWarnings("ReferenceEquality")
  private void removeNode(IntTreeNode<T> node) {
    final IntTreeNode<T> old = node;
    IntTreeNode<T> child;
    IntTreeNode<T> parent;
    boolean color;
    if (node.left == null) {
      child = node.right;
    } else if (node.right == null) {
      child = node.left;
    } else {
      IntTreeNode<T> left;
      node = node.right;
      while ((left = node.left) != null) {
        node = left;
      }
      child = node.right;
      parent = node.parent;
      color = node.getColor();
      if (child != null) {
        child.parent = parent;
      }
      if (parent != null) {
        if (parent.left == node) {
          parent.left = child;
        } else {
          parent.right = child;
        }
      } else {
        root = child;
      }
      if (node.parent == old) {
        parent = node;
      }
      node.parent = old.parent;
      node.left = old.left;
      node.right = old.right;
      node.setColor(old.getColor());
      if (old.parent != null) {
        if (old.parent.left == old) {
          old.parent.left = node;
        } else {
          old.parent.right = node;
        }
      } else {
        root = node;
      }
      if (old.right != null) {
        old.right.parent = node;
      }
      if (color == RedBlackTreeColor.BLACK) {
        removeColor(parent, child);
      }
      --size;
      old.parent = null;
      old.left = null;
      old.right = null;
      return;
    }
    parent = node.parent;
    color = node.getColor();
    if (child != null) {
      child.parent = parent;
    }
    if (parent != null) {
      if (parent.left == node) {
        parent.left = child;
      } else {
        parent.right = child;
      }
    } else {
      root = child;
    }
    if (color == RedBlackTreeColor.BLACK) {
      removeColor(parent, child);
    }
    --size;
    old.parent = null;
    old.left = null;
    old.right = null;
  }

  @SuppressWarnings("ReferenceEquality")
  private void removeColor(IntTreeNode<T> parent, IntTreeNode<T> node) {
    IntTreeNode<T> tmp;
    while ((node == null || node.getColor() == RedBlackTreeColor.BLACK) && node != root) {
      if (parent.left == node) {
        tmp = parent.right;
        if (tmp.getColor() == RedBlackTreeColor.RED) {
          tmp.setColor(RedBlackTreeColor.BLACK);
          parent.setColor(RedBlackTreeColor.RED);
          rotateLeft(parent);
          tmp = parent.right;
        }
        if ((tmp.left == null || tmp.left.getColor() == RedBlackTreeColor.BLACK)
            && (tmp.right == null || tmp.right.getColor() == RedBlackTreeColor.BLACK)) {
          tmp.setColor(RedBlackTreeColor.RED);
          node = parent;
          parent = node.parent;
        } else {
          if (tmp.right == null || tmp.right.getColor() == RedBlackTreeColor.BLACK) {
            IntTreeNode<T> left;
            if ((left = tmp.left) != null) {
              left.setColor(RedBlackTreeColor.BLACK);
            }
            tmp.setColor(RedBlackTreeColor.RED);
            rotateRight(tmp);
            tmp = parent.right;
          }
          tmp.setColor(parent.getColor());
          parent.setColor(RedBlackTreeColor.BLACK);
          if (tmp.right != null) {
            tmp.right.setColor(RedBlackTreeColor.BLACK);
          }
          rotateLeft(parent);
          node = root;
          break;
        }
      } else {
        tmp = parent.left;
        if (tmp.getColor() == RedBlackTreeColor.RED) {
          tmp.setColor(RedBlackTreeColor.BLACK);
          parent.setColor(RedBlackTreeColor.RED);
          rotateRight(parent);
          tmp = parent.left;
        }
        if ((tmp.left == null || tmp.left.getColor() == RedBlackTreeColor.BLACK)
            && (tmp.right == null || tmp.right.getColor() == RedBlackTreeColor.BLACK)) {
          tmp.setColor(RedBlackTreeColor.RED);
          node = parent;
          parent = node.parent;
        } else {
          if (tmp.left == null || tmp.left.getColor() == RedBlackTreeColor.BLACK) {
            IntTreeNode<T> right;
            if ((right = tmp.right) != null) {
              right.setColor(RedBlackTreeColor.BLACK);
            }
            tmp.setColor(RedBlackTreeColor.RED);
            rotateLeft(tmp);
            tmp = parent.left;
          }
          tmp.setColor(parent.getColor());
          parent.setColor(RedBlackTreeColor.BLACK);
          if (tmp.left != null) {
            tmp.setColor(RedBlackTreeColor.BLACK);
          }
          rotateRight(parent);
          node = root;
          break;
        }
      }
    }
    if (node != null) {
      node.setColor(RedBlackTreeColor.BLACK);
    }
  }

  @Override
  @SuppressWarnings("DoubleCheckedLocking")
  public Set<Map.Entry<Integer, T>> entrySet() {
    Entries entries = this.entries;
    if (entries == null) {
      this.entries = entries = new Entries();
    }
    return entries;
  }

  @Override
  public int hashCode() {
    int h = 0;
    IntTreeNode<T> node = min();
    while (node != null) {
      h += node.hashCode();
      node = next(node);
    }
    return h;
  }

  @Override
  public boolean equals(Object other) {
    if (other instanceof IntTreeMap) {
      return equalsFast((IntTreeMap<?>) other);
    }
    return equalsSlow(other);
  }

  @SuppressWarnings("ReferenceEquality")
  boolean equalsFast(IntTreeMap<?> that) {
    if (this == that) {
      return true;
    }
    if (size() != that.size()) {
      return false;
    }
    IntTreeNode<T> thisNode = min();
    IntTreeNode<?> thatNode = that.min();
    while (thisNode != null) {
      if (!thisNode.equalsFast(thatNode)) {
        return false;
      }
      thisNode = next(thisNode);
      thatNode = next(thatNode);
    }
    return true;
  }

  private boolean equalsSlow(Object that) {
    return super.equals(that);
  }

  @SuppressWarnings("ReferenceEquality")
  private void rotateLeft(IntTreeNode<T> node) {
    IntTreeNode<T> tmp = node.right;
    if ((node.right = tmp.left) != null) {
      tmp.left.parent = node;
    }
    if ((tmp.parent = node.parent) != null) {
      if (node == node.parent.left) {
        node.parent.left = tmp;
      } else {
        node.parent.right = tmp;
      }
    } else {
      root = tmp;
    }
    tmp.left = node;
    node.parent = tmp;
  }

  @SuppressWarnings("ReferenceEquality")
  private void rotateRight(IntTreeNode<T> node) {
    IntTreeNode<T> tmp = node.left;
    if ((node.left = tmp.right) != null) {
      tmp.right.parent = node;
    }
    if ((tmp.parent = node.parent) != null) {
      if (node == node.parent.left) {
        node.parent.left = tmp;
      } else {
        node.parent.right = tmp;
      }
    } else {
      root = tmp;
    }
    tmp.right = node;
    node.parent = tmp;
  }

  @SuppressWarnings("ReferenceEquality")
  private void insertColor(IntTreeNode<T> node) {
    IntTreeNode<T> parent;
    IntTreeNode<T> grandparent;
    IntTreeNode<T> tmp;
    while ((parent = node.parent) != null && parent.getColor() == RedBlackTreeColor.RED) {
      grandparent = parent.parent;
      if (parent == grandparent.left) {
        tmp = grandparent.right;
        if (tmp != null && tmp.getColor() == RedBlackTreeColor.RED) {
          tmp.setColor(RedBlackTreeColor.BLACK);
          parent.setColor(RedBlackTreeColor.BLACK);
          grandparent.setColor(RedBlackTreeColor.RED);
          node = grandparent;
          continue;
        }
        if (parent.right == node) {
          rotateLeft(parent);
          tmp = parent;
          parent = node;
          node = tmp;
        }
        parent.setColor(RedBlackTreeColor.BLACK);
        grandparent.setColor(RedBlackTreeColor.RED);
        rotateRight(grandparent);
      } else {
        tmp = grandparent.left;
        if (tmp != null && tmp.getColor() == RedBlackTreeColor.RED) {
          tmp.setColor(RedBlackTreeColor.BLACK);
          parent.setColor(RedBlackTreeColor.BLACK);
          grandparent.setColor(RedBlackTreeColor.RED);
          node = grandparent;
          continue;
        }
        if (parent.left == node) {
          rotateRight(parent);
          tmp = parent;
          parent = node;
          node = tmp;
        }
        parent.setColor(RedBlackTreeColor.BLACK);
        grandparent.setColor(RedBlackTreeColor.RED);
        rotateLeft(grandparent);
      }
    }
    root.setColor(RedBlackTreeColor.BLACK);
  }

  IntTreeNode<T> min() {
    IntTreeNode<T> node = root;
    IntTreeNode<T> parent = null;
    while (node != null) {
      parent = node;
      node = node.left;
    }
    return parent;
  }

  IntTreeNode<T> max() {
    IntTreeNode<T> node = root;
    IntTreeNode<T> parent = null;
    while (node != null) {
      parent = node;
      node = node.right;
    }
    return parent;
  }

  static <T> IntTreeNode<T> next(IntTreeNode<T> node) {
    return node.next();
  }

  static <T> IntTreeNode<T> prev(IntTreeNode<T> node) {
    return node.prev();
  }

  private final class Entries extends AbstractSet<Map.Entry<Integer, T>> {
    @Override
    public void clear() {
      IntTreeMap.this.clear();
    }

    @Override
    public boolean contains(Object object) {
      if (object instanceof IntTreeNode) {
        return containsFast((IntTreeNode<?>) object);
      }
      if (object instanceof Map.Entry) {
        return containsSlow((Map.Entry<?, ?>) object);
      }
      return false;
    }

    private boolean containsFast(IntTreeNode<?> node) {
      T value = IntTreeMap.this.getFast(node.getKeyAsInt());
      return value != null && Objects.equals(node.getValue(), value);
    }

    private boolean containsSlow(Map.Entry<?, ?> entry) {
      T value = IntTreeMap.this.get(entry.getKey());
      return value != null && Objects.equals(entry.getValue(), value);
    }

    @Override
    public Iterator<Map.Entry<Integer, T>> iterator() {
      return new EntryIterator();
    }

    @Override
    public boolean remove(Object object) {
      if (object instanceof IntTreeNode) {
        return removeFast((IntTreeNode<?>) object);
      }
      if (object instanceof Map.Entry) {
        return removeSlow((Map.Entry<?, ?>) object);
      }
      return false;
    }

    private boolean removeFast(IntTreeNode<?> node) {
      IntTreeNode<T> toRemove = IntTreeMap.this.getNodeFast(node.getKeyAsInt());
      if (Objects.equals(node.getValue(), toRemove.getValue())) {
        IntTreeMap.this.removeNode(toRemove);
        return true;
      }
      return false;
    }

    private boolean removeSlow(Map.Entry<?, ?> entry) {
      IntTreeNode<T> toRemove = IntTreeMap.this.getNode(entry.getKey());
      if (Objects.equals(entry.getValue(), toRemove.getValue())) {
        IntTreeMap.this.removeNode(toRemove);
        return true;
      }
      return false;
    }

    @Override
    public int size() {
      return IntTreeMap.this.size();
    }
  }

  private final class EntryIterator implements Iterator<Map.Entry<Integer, T>> {

    private IntTreeNode<T> prev;
    private IntTreeNode<T> node;

    private EntryIterator() {
      prev = null;
      node = min();
    }

    @Override
    public boolean hasNext() {
      return node != null;
    }

    @Override
    public IntTreeNode<T> next() {
      IntTreeNode<T> curr = node;
      prev = curr;
      node = IntTreeMap.next(node);
      return curr;
    }

    @Override
    public void remove() {
      IntTreeMap.this.removeNode(prev);
    }
  }
}
