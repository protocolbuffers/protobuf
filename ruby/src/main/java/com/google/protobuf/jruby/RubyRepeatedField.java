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

import com.google.protobuf.Descriptors;
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
        RubyClass cRepeatedField = mProtobuf.defineClassUnder("RepeatedField", runtime.getObject(),
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

    public RubyRepeatedField(Ruby runtime, RubyClass klazz, Descriptors.FieldDescriptor.Type fieldType, IRubyObject typeClass) {
        this(runtime, klazz);
        this.fieldType = fieldType;
        this.storage = runtime.newArray();
        this.typeClass = typeClass;
    }

    @JRubyMethod(required = 1, optional = 2)
    public IRubyObject initialize(ThreadContext context, IRubyObject[] args) {
        Ruby runtime = context.runtime;
        this.storage = runtime.newArray();
        IRubyObject ary = null;
        if (!(args[0] instanceof RubySymbol)) {
            throw runtime.newArgumentError("Expected Symbol for type name");
        }
        this.fieldType = Utils.rubyToFieldType(args[0]);
        if (fieldType == Descriptors.FieldDescriptor.Type.MESSAGE
                || fieldType == Descriptors.FieldDescriptor.Type.ENUM) {
            if (args.length < 2)
                throw runtime.newArgumentError("Expected at least 2 arguments for message/enum");
            typeClass = args[1];
            if (args.length > 2)
                ary = args[2];
            Utils.validateTypeClass(context, fieldType, typeClass);
        } else {
            if (args.length > 2)
                throw runtime.newArgumentError("Too many arguments: expected 1 or 2");
            if (args.length > 1)
                ary = args[1];
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
        Utils.checkType(context, fieldType, value, (RubyModule) typeClass);
        this.storage.set(RubyNumeric.num2int(index), value);
        return context.runtime.getNil();
    }

    /*
     * call-seq:
     *     RepeatedField.[](index) => value
     *
     * Accesses the element at the given index. Throws an exception on out-of-bounds
     * errors.
     */
    @JRubyMethod(name = "[]")
    public IRubyObject index(ThreadContext context, IRubyObject index) {
        return this.storage.eltInternal(RubyNumeric.num2int(index));
    }

    /*
     * call-seq:
     *     RepeatedField.insert(*args)
     *
     * Pushes each arg in turn onto the end of the repeated field.
     */
    @JRubyMethod(rest = true)
    public IRubyObject insert(ThreadContext context, IRubyObject[] args) {
        for (int i = 0; i < args.length; i++) {
            Utils.checkType(context, fieldType, args[i], (RubyModule) typeClass);
            this.storage.add(args[i]);
        }
        return context.runtime.getNil();
    }

    /*
     * call-seq:
     *     RepeatedField.push(value)
     *
     * Adds a new element to the repeated field.
     */
    @JRubyMethod(name = {"push", "<<"})
    public IRubyObject push(ThreadContext context, IRubyObject value) {
        Utils.checkType(context, fieldType, value, (RubyModule) typeClass);
        this.storage.add(value);
        return this;
    }

    /*
     * call-seq:
     *     RepeatedField.pop => value
     *
     * Removes the last element and returns it. Throws an exception if the repeated
     * field is empty.
     */
    @JRubyMethod
    public IRubyObject pop(ThreadContext context) {
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
        RubyArray arr = (RubyArray) list;
        checkArrayElementType(context, arr);
        this.storage = arr;
        return context.runtime.getNil();
    }

    /*
     * call-seq:
     *     RepeatedField.clear
     *
     * Clears (removes all elements from) this repeated field.
     */
    @JRubyMethod
    public IRubyObject clear(ThreadContext context) {
        this.storage.clear();
        return context.runtime.getNil();
    }

    /*
     * call-seq:
     *     RepeatedField.length
     *
     * Returns the length of this repeated field.
     */
    @JRubyMethod(name = {"count", "length"})
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
    @JRubyMethod(name = "+")
    public IRubyObject plus(ThreadContext context, IRubyObject list) {
        RubyRepeatedField dup = (RubyRepeatedField) dup(context);
        if (list instanceof RubyArray) {
            checkArrayElementType(context, (RubyArray) list);
            dup.storage.addAll((RubyArray) list);
        } else {
            RubyRepeatedField repeatedField = (RubyRepeatedField) list;
            if (! fieldType.equals(repeatedField.fieldType) || (typeClass != null && !
                    typeClass.equals(repeatedField.typeClass)))
                throw context.runtime.newArgumentError("Attempt to append RepeatedField with different element type.");
            dup.storage.addAll((RubyArray) repeatedField.toArray(context));
        }
        return dup;
    }

    /*
     * call-seq:
     *     RepeatedField.hash => hash_value
     *
     * Returns a hash value computed from this repeated field's elements.
     */
    @JRubyMethod
    public IRubyObject hash(ThreadContext context) {
        int hashCode = System.identityHashCode(this.storage);
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
        return context.runtime.getNil();
    }

    @JRubyMethod(name = {"to_ary", "to_a"})
    public IRubyObject toArray(ThreadContext context) {
        for (int i = 0; i < this.storage.size(); i++) {
            IRubyObject defaultValue = defaultValue(context);
            if (storage.eltInternal(i).isNil()) {
                storage.set(i, defaultValue);
            }
        }
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
        for (int i = 0; i < this.storage.size(); i++) {
            dup.push(context, this.storage.eltInternal(i));
        }
        return dup;
    }

    /*
     * call-seq:
     *     RepeatedField.inspect => string
     *
     * Returns a string representing this repeated field's elements. It will be
     * formated as "[<element>, <element>, ...]", with each element's string
     * representation computed by its own #inspect method.
     */
    @JRubyMethod
    public IRubyObject inspect() {
        StringBuilder str = new StringBuilder("[");
        for (int i = 0; i < this.storage.size(); i++) {
            str.append(storage.eltInternal(i).inspect());
            str.append(", ");
        }

        if (str.length() > 1) {
            str.replace(str.length() - 2, str.length(), "]");
        } else {
            str.append("]");
        }

        return getRuntime().newString(str.toString());
    }

    // Java API
    protected IRubyObject get(int index) {
        return this.storage.eltInternal(index);
    }

    protected RubyRepeatedField deepCopy(ThreadContext context) {
        RubyRepeatedField copy = new RubyRepeatedField(context.runtime, metaClass, fieldType, typeClass);
        for (int i = 0; i < size(); i++) {
            IRubyObject value = storage.eltInternal(i);
            if (fieldType == Descriptors.FieldDescriptor.Type.MESSAGE) {
                copy.storage.add(((RubyMessage) value).deepCopy(context));
            } else {
                copy.storage.add(value);
            }
        }
        return copy;
    }

    protected int size() {
        return this.storage.size();
    }

    private IRubyObject defaultValue(ThreadContext context) {
        SentinelOuterClass.Sentinel sentinel = SentinelOuterClass.Sentinel.getDefaultInstance();
        Object value;
        switch (fieldType) {
            case INT32:
                value = sentinel.getDefaultInt32();
                break;
            case INT64:
                value = sentinel.getDefaultInt64();
                break;
            case UINT32:
                value = sentinel.getDefaultUnit32();
                break;
            case UINT64:
                value = sentinel.getDefaultUint64();
                break;
            case FLOAT:
                value = sentinel.getDefaultFloat();
                break;
            case DOUBLE:
                value = sentinel.getDefaultDouble();
                break;
            case BOOL:
                value = sentinel.getDefaultBool();
                break;
            case BYTES:
                value = sentinel.getDefaultBytes();
                break;
            case STRING:
                value = sentinel.getDefaultString();
                break;
            default:
                return context.runtime.getNil();
        }
        return Utils.wrapPrimaryValue(context, fieldType, value);
    }

    private void checkArrayElementType(ThreadContext context, RubyArray arr) {
        for (int i = 0; i < arr.getLength(); i++) {
            Utils.checkType(context, fieldType, arr.eltInternal(i), (RubyModule) typeClass);
        }
    }

    private RubyArray storage;
    private Descriptors.FieldDescriptor.Type fieldType;
    private IRubyObject typeClass;
}
