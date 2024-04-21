/*
 * Protocol Buffers - Google's data interchange format
 * Copyright 2014 Google Inc.  All rights reserved.
 * https://developers.google.com/protocol-buffers/
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package com.google.protobuf.jruby;

import com.google.protobuf.Descriptors.FieldDescriptor;
import org.jruby.*;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.Block;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;

@JRubyClass(name = "RepeatedClass", include = "Enumerable")
public class RubyRepeatedField extends RubyObject {
  public static void createRubyRepeatedField(Ruby runtime) {
    RubyModule mProtobuf = runtime.getClassFromPath("Google::Protobuf");
    RubyClass cRepeatedField =
        mProtobuf.defineClassUnder(
            "RepeatedField",
            runtime.getObject(),
            new ObjectAllocator() {
              @Override
              public IRubyObject allocate(Ruby runtime, RubyClass klazz) {
                return new RubyRepeatedField(runtime, klazz);
              }
            });
    cRepeatedField.defineAnnotatedMethods(RubyRepeatedField.class);
    cRepeatedField.includeModule(runtime.getEnumerable());
  }

  public RubyRepeatedField(Ruby runtime, RubyClass klazz) {
    super(runtime, klazz);
  }

  public RubyRepeatedField(
      Ruby runtime, RubyClass klazz, FieldDescriptor.Type fieldType, IRubyObject typeClass) {
    this(runtime, klazz);
    this.fieldType = fieldType;
    this.storage = runtime.newArray();
    this.typeClass = typeClass;
  }

  @JRubyMethod(required = 1, optional = 2)
  public IRubyObject initialize(ThreadContext context, IRubyObject[] args) {
    Ruby runtime = context.runtime;
    // Workaround for https://github.com/jruby/jruby/issues/7851. Can be removed when JRuby 9.4.3.0
    // is no longer supported.
    if (args.length < 1) throw runtime.newArgumentError("Expected at least 1 argument");
    this.storage = runtime.newArray();
    IRubyObject ary = null;
    if (!(args[0] instanceof RubySymbol)) {
      throw runtime.newArgumentError("Expected Symbol for type name");
    }
    this.fieldType = Utils.rubyToFieldType(args[0]);
    if (fieldType == FieldDescriptor.Type.MESSAGE || fieldType == FieldDescriptor.Type.ENUM) {
      if (args.length < 2)
        throw runtime.newArgumentError("Expected at least 2 arguments for message/enum");
      typeClass = args[1];
      if (args.length > 2) ary = args[2];
      Utils.validateTypeClass(context, fieldType, typeClass);
    } else {
      if (args.length > 2) throw runtime.newArgumentError("Too many arguments: expected 1 or 2");
      if (args.length > 1) ary = args[1];
    }
    if (ary != null) {
      RubyArray arr = ary.convertToArray();
      for (int i = 0; i < arr.size(); i++) {
        this.storage.add(arr.eltInternal(i));
      }
    }
    return this;
  }

  /*
   * call-seq:
   *     RepeatedField.[]=(index, value)
   *
   * Sets the element at the given index. On out-of-bounds assignments, extends
   * the array and fills the hole (if any) with default values.
   */
  @JRubyMethod(name = "[]=")
  public IRubyObject indexSet(ThreadContext context, IRubyObject index, IRubyObject value) {
    testFrozen("Can't set index in frozen repeated field");
    int arrIndex = normalizeArrayIndex(index);
    value = Utils.checkType(context, fieldType, name, value, (RubyModule) typeClass);
    IRubyObject defaultValue = defaultValue(context);
    for (int i = this.storage.size(); i < arrIndex; i++) {
      this.storage.set(i, defaultValue);
    }
    this.storage.set(arrIndex, value);
    return context.runtime.getNil();
  }

  /*
   * call-seq:
   *     RepeatedField.[](index) => value
   *
   * Accesses the element at the given index. Returns nil on out-of-bounds
   */
  @JRubyMethod(
      required = 1,
      optional = 1,
      name = {"at", "[]"})
  public IRubyObject index(ThreadContext context, IRubyObject[] args) {
    if (args.length == 1) {
      IRubyObject arg = args[0];
      if (Utils.isRubyNum(arg)) {
        /* standard case */
        int arrIndex = normalizeArrayIndex(arg);
        if (arrIndex < 0 || arrIndex >= this.storage.size()) {
          return context.runtime.getNil();
        }
        return this.storage.eltInternal(arrIndex);
      } else if (arg instanceof RubyRange) {
        RubyRange range = ((RubyRange) arg);

        boolean beginless = range.begin(context).isNil();
        int first =
            normalizeArrayIndex(
                beginless ? RubyNumeric.int2fix(context.runtime, 0) : range.begin(context));
        boolean endless = range.end(context).isNil();
        int last =
            normalizeArrayIndex(
                endless ? RubyNumeric.int2fix(context.runtime, -1) : range.end(context));

        if (last - first < 0) {
          return context.runtime.newEmptyArray();
        }
        boolean excludeEnd = range.isExcludeEnd() && !endless;
        return this.storage.subseq(first, last - first + (excludeEnd ? 0 : 1));
      }
    }
    /* assume 2 arguments */
    int beg = RubyNumeric.num2int(args[0]);
    int len = RubyNumeric.num2int(args[1]);
    if (beg < 0) {
      beg += this.storage.size();
    }
    if (beg >= this.storage.size()) {
      return context.runtime.getNil();
    }
    return this.storage.subseq(beg, len);
  }

  /*
   * call-seq:
   *     RepeatedField.push(value)
   *
   * Adds a new element to the repeated field.
   */
  @JRubyMethod(
      name = {"push", "<<"},
      required = 1,
      rest = true)
  public IRubyObject push(ThreadContext context, IRubyObject[] args) {
    testFrozen("Can't push frozen repeated field");
    for (int i = 0; i < args.length; i++) {
      IRubyObject val = args[i];
      if (fieldType != FieldDescriptor.Type.MESSAGE || !val.isNil()) {
        val = Utils.checkType(context, fieldType, name, val, (RubyModule) typeClass);
      }
      storage.add(val);
    }

    return this;
  }

  /*
   * private Ruby method used by RepeatedField.pop
   */
  @JRubyMethod(visibility = org.jruby.runtime.Visibility.PRIVATE)
  public IRubyObject pop_one(ThreadContext context) {
    testFrozen("Can't pop frozen repeated field");
    IRubyObject ret = this.storage.last();
    this.storage.remove(ret);
    return ret;
  }

  /*
   * call-seq:
   *     RepeatedField.replace(list)
   *
   * Replaces the contents of the repeated field with the given list of elements.
   */
  @JRubyMethod
  public IRubyObject replace(ThreadContext context, IRubyObject list) {
    testFrozen("Can't replace frozen repeated field");
    RubyArray arr = (RubyArray) list;
    checkArrayElementType(context, arr);
    this.storage = arr;
    return this;
  }

  /*
   * call-seq:
   *     RepeatedField.clear
   *
   * Clears (removes all elements from) this repeated field.
   */
  @JRubyMethod
  public IRubyObject clear(ThreadContext context) {
    testFrozen("Can't clear frozen repeated field");
    this.storage.clear();
    return this;
  }

  /*
   * call-seq:
   *     RepeatedField.length
   *
   * Returns the length of this repeated field.
   */
  @JRubyMethod(name = {"length", "size"})
  public IRubyObject length(ThreadContext context) {
    return context.runtime.newFixnum(this.storage.size());
  }

  /*
   * call-seq:
   *     RepeatedField.+(other) => repeated field
   *
   * Returns a new repeated field that contains the concatenated list of this
   * repeated field's elements and other's elements. The other (second) list may
   * be either another repeated field or a Ruby array.
   */
  @JRubyMethod(name = {"+"})
  public IRubyObject plus(ThreadContext context, IRubyObject list) {
    RubyRepeatedField dup = (RubyRepeatedField) dup(context);
    if (list instanceof RubyArray) {
      checkArrayElementType(context, (RubyArray) list);
      dup.storage.addAll((RubyArray) list);
    } else {
      RubyRepeatedField repeatedField = (RubyRepeatedField) list;
      if (!fieldType.equals(repeatedField.fieldType)
          || (typeClass != null && !typeClass.equals(repeatedField.typeClass)))
        throw context.runtime.newArgumentError(
            "Attempt to append RepeatedField with different element type.");
      dup.storage.addAll((RubyArray) repeatedField.toArray(context));
    }
    return dup;
  }

  /*
   * call-seq:
   *     RepeatedField.concat(other) => self
   *
   * concats the passed in array to self.  Returns a Ruby array.
   */
  @JRubyMethod
  public IRubyObject concat(ThreadContext context, IRubyObject list) {
    testFrozen("Can't concat frozen repeated field");
    if (list instanceof RubyArray) {
      checkArrayElementType(context, (RubyArray) list);
      this.storage.addAll((RubyArray) list);
    } else {
      RubyRepeatedField repeatedField = (RubyRepeatedField) list;
      if (!fieldType.equals(repeatedField.fieldType)
          || (typeClass != null && !typeClass.equals(repeatedField.typeClass)))
        throw context.runtime.newArgumentError(
            "Attempt to append RepeatedField with different element type.");
      this.storage.addAll((RubyArray) repeatedField.toArray(context));
    }
    return this;
  }

  /*
   * call-seq:
   *     RepeatedField.hash => hash_value
   *
   * Returns a hash value computed from this repeated field's elements.
   */
  @JRubyMethod
  public IRubyObject hash(ThreadContext context) {
    int hashCode = this.storage.hashCode();
    return context.runtime.newFixnum(hashCode);
  }

  /*
   * call-seq:
   *     RepeatedField.==(other) => boolean
   *
   * Compares this repeated field to another. Repeated fields are equal if their
   * element types are equal, their lengths are equal, and each element is equal.
   * Elements are compared as per normal Ruby semantics, by calling their :==
   * methods (or performing a more efficient comparison for primitive types).
   */
  @JRubyMethod(name = "==")
  public IRubyObject eq(ThreadContext context, IRubyObject other) {
    return this.toArray(context).op_equal(context, other);
  }

  /*
   * call-seq:
   *     RepeatedField.each(&block)
   *
   * Invokes the block once for each element of the repeated field. RepeatedField
   * also includes Enumerable; combined with this method, the repeated field thus
   * acts like an ordinary Ruby sequence.
   */
  @JRubyMethod
  public IRubyObject each(ThreadContext context, Block block) {
    this.storage.each(context, block);
    return this;
  }

  @JRubyMethod(name = {"to_ary", "to_a"})
  public IRubyObject toArray(ThreadContext context) {
    return this.storage;
  }

  /*
   * call-seq:
   *     RepeatedField.dup => repeated_field
   *
   * Duplicates this repeated field with a shallow copy. References to all
   * non-primitive element objects (e.g., submessages) are shared.
   */
  @JRubyMethod
  public IRubyObject dup(ThreadContext context) {
    RubyRepeatedField dup = new RubyRepeatedField(context.runtime, metaClass, fieldType, typeClass);
    dup.push(context, storage.toJavaArray());
    return dup;
  }

  @JRubyMethod
  public IRubyObject inspect() {
    return storage.inspect();
  }

  @JRubyMethod
  public IRubyObject freeze(ThreadContext context) {
    if (isFrozen()) {
      return this;
    }
    setFrozen(true);
    if (fieldType == FieldDescriptor.Type.MESSAGE) {
      for (int i = 0; i < size(); i++) {
        ((RubyMessage) storage.eltInternal(i)).freeze(context);
      }
    }
    return this;
  }

  // Java API
  protected IRubyObject get(int index) {
    return this.storage.eltInternal(index);
  }

  protected RubyRepeatedField deepCopy(ThreadContext context) {
    RubyRepeatedField copy =
        new RubyRepeatedField(context.runtime, metaClass, fieldType, typeClass);
    for (int i = 0; i < size(); i++) {
      IRubyObject value = storage.eltInternal(i);
      if (fieldType == FieldDescriptor.Type.MESSAGE) {
        copy.storage.add(((RubyMessage) value).deepCopy(context));
      } else {
        copy.storage.add(value);
      }
    }
    return copy;
  }

  protected void setName(String name) {
    this.name = name;
  }

  protected int size() {
    return this.storage.size();
  }

  private IRubyObject defaultValue(ThreadContext context) {
    Object value;
    switch (fieldType) {
      case INT32:
      case UINT32:
        value = 0;
        break;
      case INT64:
      case UINT64:
        value = 0L;
        break;
      case FLOAT:
        value = 0F;
        break;
      case DOUBLE:
        value = 0D;
        break;
      case BOOL:
        value = false;
        break;
      case BYTES:
        value = com.google.protobuf.ByteString.EMPTY;
        break;
      case STRING:
        value = "";
        break;
      case ENUM:
        IRubyObject defaultEnumLoc = context.runtime.newFixnum(0);
        return RubyEnum.lookup(context, typeClass, defaultEnumLoc);
      default:
        return context.runtime.getNil();
    }
    return Utils.wrapPrimaryValue(context, fieldType, value);
  }

  private void checkArrayElementType(ThreadContext context, RubyArray arr) {
    for (int i = 0; i < arr.getLength(); i++) {
      Utils.checkType(context, fieldType, name, arr.eltInternal(i), (RubyModule) typeClass);
    }
  }

  private int normalizeArrayIndex(IRubyObject index) {
    int arrIndex = RubyNumeric.num2int(index);
    int arrSize = this.storage.size();
    if (arrIndex < 0 && arrSize > 0) {
      arrIndex = arrSize + arrIndex;
    }
    return arrIndex;
  }

  private FieldDescriptor.Type fieldType;
  private IRubyObject typeClass;
  private RubyArray storage;
  private String name;
}
