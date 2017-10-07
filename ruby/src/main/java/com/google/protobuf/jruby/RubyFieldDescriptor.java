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

import com.google.protobuf.DescriptorProtos;
import com.google.protobuf.Descriptors;
import org.jruby.*;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;

@JRubyClass(name = "FieldDescriptor")
public class RubyFieldDescriptor extends RubyObject {
    public static void createRubyFileDescriptor(Ruby runtime) {
        RubyModule mProtobuf = runtime.getClassFromPath("Google::Protobuf");
        RubyClass cFieldDescriptor = mProtobuf.defineClassUnder("FieldDescriptor", runtime.getObject(), new ObjectAllocator() {
            @Override
            public IRubyObject allocate(Ruby runtime, RubyClass klazz) {
                return new RubyFieldDescriptor(runtime, klazz);
            }
        });
        cFieldDescriptor.defineAnnotatedMethods(RubyFieldDescriptor.class);
    }

    public RubyFieldDescriptor(Ruby runtime, RubyClass klazz) {
        super(runtime, klazz);
    }

    /*
     * call-seq:
     *     FieldDescriptor.new => field
     *
     * Returns a new field descriptor. Its name, type, etc. must be set before it is
     * added to a message type.
     */
    @JRubyMethod
    public IRubyObject initialize(ThreadContext context) {
        builder = DescriptorProtos.FieldDescriptorProto.newBuilder();
        return this;
    }

    /*
     * call-seq:
     *     FieldDescriptor.label
     *
     * Return the label of this field.
     */
    @JRubyMethod(name = "label")
    public IRubyObject getLabel(ThreadContext context) {
        return this.label;
    }

    /*
     * call-seq:
     *     FieldDescriptor.label = label
     *
     * Sets the label on this field. Cannot be called if field is part of a message
     * type already in a pool.
     */
    @JRubyMethod(name = "label=")
    public IRubyObject setLabel(ThreadContext context, IRubyObject value) {
        String labelName = value.asJavaString();
        this.label = context.runtime.newSymbol(labelName.toLowerCase());
        this.builder.setLabel(
                DescriptorProtos.FieldDescriptorProto.Label.valueOf("LABEL_" + labelName.toUpperCase()));
        return context.runtime.getNil();
    }

    /*
     * call-seq:
     *     FieldDescriptor.name => name
     *
     * Returns the name of this field as a Ruby String, or nil if it is not set.
     */
    @JRubyMethod(name = "name")
    public IRubyObject getName(ThreadContext context) {
        return this.name;
    }

    /*
     * call-seq:
     *     FieldDescriptor.name = name
     *
     * Sets the name of this field. Cannot be called once the containing message
     * type, if any, is added to a pool.
     */
    @JRubyMethod(name = "name=")
    public IRubyObject setName(ThreadContext context, IRubyObject value) {
        String nameStr = value.asJavaString();
        this.name = context.runtime.newString(nameStr);
        this.builder.setName(Utils.escapeIdentifier(nameStr));
        return context.runtime.getNil();
    }


    @JRubyMethod(name = "subtype")
    public IRubyObject getSubType(ThreadContext context) {
        return subType;
    }

    /*
     * call-seq:
     *     FieldDescriptor.type => type
     *
     * Returns this field's type, as a Ruby symbol, or nil if not yet set.
     *
     * Valid field types are:
     *     :int32, :int64, :uint32, :uint64, :float, :double, :bool, :string,
     *     :bytes, :message.
     */
    @JRubyMethod(name = "type")
    public IRubyObject getType(ThreadContext context) {
        return Utils.fieldTypeToRuby(context, this.builder.getType());
    }

    /*
     * call-seq:
     *     FieldDescriptor.type = type
     *
     * Sets this field's type. Cannot be called if field is part of a message type
     * already in a pool.
     */
    @JRubyMethod(name = "type=")
    public IRubyObject setType(ThreadContext context, IRubyObject value) {
        this.builder.setType(DescriptorProtos.FieldDescriptorProto.Type.valueOf("TYPE_" + value.asJavaString().toUpperCase()));
        return context.runtime.getNil();
    }

    /*
     * call-seq:
     *     FieldDescriptor.number => number
     *
     * Returns this field's number, as a Ruby Integer, or nil if not yet set.
     *
     */
    @JRubyMethod(name = "number")
    public IRubyObject getnumber(ThreadContext context) {
        return this.number;
    }

    /*
     * call-seq:
     *     FieldDescriptor.number = number
     *
     * Sets the tag number for this field. Cannot be called if field is part of a
     * message type already in a pool.
     */
    @JRubyMethod(name = "number=")
    public IRubyObject setNumber(ThreadContext context, IRubyObject value) {
        this.number = value;
        this.builder.setNumber(RubyNumeric.num2int(value));
        return context.runtime.getNil();
    }

    /*
     * call-seq:
     *     FieldDescriptor.submsg_name = submsg_name
     *
     * Sets the name of the message or enum type corresponding to this field, if it
     * is a message or enum field (respectively). This type name will be resolved
     * within the context of the pool to which the containing message type is added.
     * Cannot be called on field that are not of message or enum type, or on fields
     * that are part of a message type already added to a pool.
     */
    @JRubyMethod(name = "submsg_name=")
    public IRubyObject setSubmsgName(ThreadContext context, IRubyObject name) {
        this.builder.setTypeName("." + Utils.escapeIdentifier(name.asJavaString()));
        return context.runtime.getNil();
    }

    /*
     * call-seq:
     *     FieldDescriptor.get(message) => value
     *
     * Returns the value set for this field on the given message. Raises an
     * exception if message is of the wrong type.
     */
    @JRubyMethod(name = "get")
    public IRubyObject getValue(ThreadContext context, IRubyObject msgRb) {
        RubyMessage message = (RubyMessage) msgRb;
        if (message.getDescriptor() != fieldDef.getContainingType()) {
            throw context.runtime.newTypeError("set method called on wrong message type");
        }
        return message.getField(context, fieldDef);
    }

    /*
     * call-seq:
     *     FieldDescriptor.set(message, value)
     *
     * Sets the value corresponding to this field to the given value on the given
     * message. Raises an exception if message is of the wrong type. Performs the
     * ordinary type-checks for field setting.
     */
    @JRubyMethod(name = "set")
    public IRubyObject setValue(ThreadContext context, IRubyObject msgRb, IRubyObject value) {
        RubyMessage message = (RubyMessage) msgRb;
        if (message.getDescriptor() != fieldDef.getContainingType()) {
            throw context.runtime.newTypeError("set method called on wrong message type");
        }
        message.setField(context, fieldDef, value);
        return context.runtime.getNil();
    }

    protected void setSubType(IRubyObject rubyDescriptor) {
        this.subType = rubyDescriptor;
    }

    protected void setFieldDef(Descriptors.FieldDescriptor fieldDescriptor) {
        this.fieldDef = fieldDescriptor;
    }

    protected void setOneofName(IRubyObject name) {
        oneofName = name;
    }

    protected void setOneofIndex(int index) {
        hasOneofIndex = true;
        oneofIndex = index;
    }

    protected IRubyObject getOneofName() {
        return oneofName;
    }

    protected Descriptors.FieldDescriptor getFieldDef() {
        return fieldDef;
    }

    protected DescriptorProtos.FieldDescriptorProto build() {
        if (hasOneofIndex)
            builder.setOneofIndex(oneofIndex);
        return this.builder.build();
    }

    private DescriptorProtos.FieldDescriptorProto.Builder builder;
    private IRubyObject name;
    private IRubyObject label;
    private IRubyObject number;
    private IRubyObject subType;
    private IRubyObject oneofName;
    private Descriptors.FieldDescriptor fieldDef;
    private int oneofIndex;
    private boolean hasOneofIndex = false;
}
