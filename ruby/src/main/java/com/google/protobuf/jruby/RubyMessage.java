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

import com.google.protobuf.*;
import org.jruby.*;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.Block;
import org.jruby.runtime.Helpers;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.util.ByteList;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.HashMap;
import java.util.Map;

public class RubyMessage extends RubyObject {
    public RubyMessage(Ruby ruby, RubyClass klazz, Descriptors.Descriptor descriptor) {
        super(ruby, klazz);
        this.descriptor = descriptor;
    }

    /*
     * call-seq:
     *     Message.new(kwargs) => new_message
     *
     * Creates a new instance of the given message class. Keyword arguments may be
     * provided with keywords corresponding to field names.
     *
     * Note that no literal Message class exists. Only concrete classes per message
     * type exist, as provided by the #msgclass method on Descriptors after they
     * have been added to a pool. The method definitions described here on the
     * Message class are provided on each concrete message class.
     */
    @JRubyMethod(optional = 1)
    public IRubyObject initialize(final ThreadContext context, IRubyObject[] args) {
        final Ruby runtime = context.runtime;
        this.cRepeatedField = (RubyClass) runtime.getClassFromPath("Google::Protobuf::RepeatedField");
        this.cMap = (RubyClass) runtime.getClassFromPath("Google::Protobuf::Map");
        this.builder = DynamicMessage.newBuilder(this.descriptor);
        this.repeatedFields = new HashMap<Descriptors.FieldDescriptor, RubyRepeatedField>();
        this.maps = new HashMap<Descriptors.FieldDescriptor, RubyMap>();
        this.fields = new HashMap<Descriptors.FieldDescriptor, IRubyObject>();
        this.oneofCases = new HashMap<Descriptors.OneofDescriptor, Descriptors.FieldDescriptor>();
        if (args.length == 1) {
            if (!(args[0] instanceof RubyHash)) {
                throw runtime.newArgumentError("expected Hash arguments.");
            }
            RubyHash hash = args[0].convertToHash();
            hash.visitAll(new RubyHash.Visitor() {
                @Override
                public void visit(IRubyObject key, IRubyObject value) {
                    if (!(key instanceof RubySymbol) && !(key instanceof RubyString))
                        throw runtime.newTypeError("Expected string or symbols as hash keys in initialization map.");
                    final Descriptors.FieldDescriptor fieldDescriptor = findField(context, key);

                    if (Utils.isMapEntry(fieldDescriptor)) {
                        if (!(value instanceof RubyHash))
                            throw runtime.newArgumentError("Expected Hash object as initializer value for map field '" +  key.asJavaString() + "'.");

                        final RubyMap map = newMapForField(context, fieldDescriptor);
                        map.mergeIntoSelf(context, value);
                        maps.put(fieldDescriptor, map);
                    } else if (fieldDescriptor.isRepeated()) {
                        if (!(value instanceof RubyArray))
                            throw runtime.newArgumentError("Expected array as initializer value for repeated field '" +  key.asJavaString() + "'.");
                        RubyRepeatedField repeatedField = rubyToRepeatedField(context, fieldDescriptor, value);
                        addRepeatedField(fieldDescriptor, repeatedField);
                    } else {
                        Descriptors.OneofDescriptor oneof = fieldDescriptor.getContainingOneof();
                        if (oneof != null) {
                            oneofCases.put(oneof, fieldDescriptor);
                        }

                        if (value instanceof RubyHash && fieldDescriptor.getType() == Descriptors.FieldDescriptor.Type.MESSAGE) {
                            RubyDescriptor descriptor = (RubyDescriptor) getDescriptorForField(context, fieldDescriptor);
                            RubyClass typeClass = (RubyClass) descriptor.msgclass(context);
                            value = (IRubyObject) typeClass.newInstance(context, value, Block.NULL_BLOCK);
                        }

                        fields.put(fieldDescriptor, value);
                    }
                }
            });
        }
        return this;
    }

    /*
     * call-seq:
     *     Message.[]=(index, value)
     *
     * Sets a field's value by field name. The provided field name should be a
     * string.
     */
    @JRubyMethod(name = "[]=")
    public IRubyObject indexSet(ThreadContext context, IRubyObject fieldName, IRubyObject value) {
        Descriptors.FieldDescriptor fieldDescriptor = findField(context, fieldName);
        return setField(context, fieldDescriptor, value);
    }

    /*
     * call-seq:
     *     Message.[](index) => value
     *
     * Accesses a field's value by field name. The provided field name should be a
     * string.
     */
    @JRubyMethod(name = "[]")
    public IRubyObject index(ThreadContext context, IRubyObject fieldName) {
        Descriptors.FieldDescriptor fieldDescriptor = findField(context, fieldName);
        return getField(context, fieldDescriptor);
    }

    /*
     * call-seq:
     *     Message.inspect => string
     *
     * Returns a human-readable string representing this message. It will be
     * formatted as "<MessageType: field1: value1, field2: value2, ...>". Each
     * field's value is represented according to its own #inspect method.
     */
    @JRubyMethod
    public IRubyObject inspect() {
        String cname = metaClass.getName();
        StringBuilder sb = new StringBuilder("<");
        sb.append(cname);
        sb.append(": ");
        sb.append(this.layoutInspect());
        sb.append(">");

        return getRuntime().newString(sb.toString());
    }

    /*
     * call-seq:
     *     Message.hash => hash_value
     *
     * Returns a hash value that represents this message's field values.
     */
    @JRubyMethod
    public IRubyObject hash(ThreadContext context) {
        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            for (RubyMap map : maps.values()) {
                digest.update((byte) map.hashCode());
            }
            for (RubyRepeatedField repeatedField : repeatedFields.values()) {
                digest.update((byte) repeatedFields.hashCode());
            }
            for (IRubyObject field : fields.values()) {
                digest.update((byte) field.hashCode());
            }
            return context.runtime.newString(new ByteList(digest.digest()));
        } catch (NoSuchAlgorithmException ignore) {
            return context.runtime.newFixnum(System.identityHashCode(this));
        }
    }

    /*
     * call-seq:
     *     Message.==(other) => boolean
     *
     * Performs a deep comparison of this message with another. Messages are equal
     * if they have the same type and if each field is equal according to the :==
     * method's semantics (a more efficient comparison may actually be done if the
     * field is of a primitive type).
     */
    @JRubyMethod(name = "==")
    public IRubyObject eq(ThreadContext context, IRubyObject other) {
        Ruby runtime = context.runtime;
        if (!(other instanceof RubyMessage))
            return runtime.getFalse();
        RubyMessage message = (RubyMessage) other;
        if (descriptor != message.descriptor) {
            return runtime.getFalse();
        }

        for (Descriptors.FieldDescriptor fdef : descriptor.getFields()) {
            IRubyObject thisVal = getField(context, fdef);
            IRubyObject thatVal = message.getField(context, fdef);
            IRubyObject ret = thisVal.callMethod(context, "==", thatVal);
            if (!ret.isTrue()) {
                return runtime.getFalse();
            }
        }
        return runtime.getTrue();
    }

    /*
     * call-seq:
     *     Message.method_missing(*args)
     *
     * Provides accessors and setters for message fields according to their field
     * names. For any field whose name does not conflict with a built-in method, an
     * accessor is provided with the same name as the field, and a setter is
     * provided with the name of the field plus the '=' suffix. Thus, given a
     * message instance 'msg' with field 'foo', the following code is valid:
     *
     *     msg.foo = 42
     *     puts msg.foo
     */
    @JRubyMethod(name = "method_missing", rest = true)
    public IRubyObject methodMissing(ThreadContext context, IRubyObject[] args) {
        if (args.length == 1) {
            RubyDescriptor rubyDescriptor = (RubyDescriptor) getDescriptor(context, metaClass);
            IRubyObject oneofDescriptor = rubyDescriptor.lookupOneof(context, args[0]);
            if (oneofDescriptor.isNil()) {
                if (!hasField(args[0])) {
                    return Helpers.invokeSuper(context, this, metaClass, "method_missing", args, Block.NULL_BLOCK);
                }
                return index(context, args[0]);
            }
            RubyOneofDescriptor rubyOneofDescriptor = (RubyOneofDescriptor) oneofDescriptor;
            Descriptors.FieldDescriptor fieldDescriptor =
                    oneofCases.get(rubyOneofDescriptor.getOneofDescriptor());
            if (fieldDescriptor == null)
                return context.runtime.getNil();

            return context.runtime.newSymbol(fieldDescriptor.getName());
        } else {
            // fieldName is RubySymbol
            RubyString field = args[0].asString();
            RubyString equalSign = context.runtime.newString(Utils.EQUAL_SIGN);
            if (field.end_with_p(context, equalSign).isTrue()) {
                field.chomp_bang(context, equalSign);
            }

            if (!hasField(field)) {
                return Helpers.invokeSuper(context, this, metaClass, "method_missing", args, Block.NULL_BLOCK);
            }
            return indexSet(context, field, args[1]);
        }
    }

    /**
     * call-seq:
     * Message.dup => new_message
     * Performs a shallow copy of this message and returns the new copy.
     */
    @JRubyMethod
    public IRubyObject dup(ThreadContext context) {
        RubyMessage dup = (RubyMessage) metaClass.newInstance(context, Block.NULL_BLOCK);
        IRubyObject value;
        for (Descriptors.FieldDescriptor fieldDescriptor : this.descriptor.getFields()) {
            if (fieldDescriptor.isRepeated()) {
                dup.addRepeatedField(fieldDescriptor, this.getRepeatedField(context, fieldDescriptor));
            } else if (fields.containsKey(fieldDescriptor)) {
                dup.fields.put(fieldDescriptor, fields.get(fieldDescriptor));
            } else if (this.builder.hasField(fieldDescriptor)) {
                dup.fields.put(fieldDescriptor, wrapField(context, fieldDescriptor, this.builder.getField(fieldDescriptor)));
            }
        }
        for (Descriptors.FieldDescriptor fieldDescriptor : maps.keySet()) {
            dup.maps.put(fieldDescriptor, maps.get(fieldDescriptor));
        }
        return dup;
    }

    /*
     * call-seq:
     *     Message.descriptor => descriptor
     *
     * Class method that returns the Descriptor instance corresponding to this
     * message class's type.
     */
    @JRubyMethod(name = "descriptor", meta = true)
    public static IRubyObject getDescriptor(ThreadContext context, IRubyObject recv) {
        return ((RubyClass) recv).getInstanceVariable(Utils.DESCRIPTOR_INSTANCE_VAR);
    }

    /*
     * call-seq:
     *     MessageClass.encode(msg) => bytes
     *
     * Encodes the given message object to its serialized form in protocol buffers
     * wire format.
     */
    @JRubyMethod(meta = true)
    public static IRubyObject encode(ThreadContext context, IRubyObject recv, IRubyObject value) {
        RubyMessage message = (RubyMessage) value;
        return context.runtime.newString(new ByteList(message.build(context).toByteArray()));
    }

    /*
     * call-seq:
     *     MessageClass.decode(data) => message
     *
     * Decodes the given data (as a string containing bytes in protocol buffers wire
     * format) under the interpretration given by this message class's definition
     * and returns a message object with the corresponding field values.
     */
    @JRubyMethod(meta = true)
    public static IRubyObject decode(ThreadContext context, IRubyObject recv, IRubyObject data) {
        byte[] bin = data.convertToString().getBytes();
        RubyMessage ret = (RubyMessage) ((RubyClass) recv).newInstance(context, Block.NULL_BLOCK);
        try {
            ret.builder.mergeFrom(bin);
        } catch (InvalidProtocolBufferException e) {
            throw context.runtime.newRuntimeError(e.getMessage());
        }
        return ret;
    }

    /*
     * call-seq:
     *     MessageClass.encode_json(msg) => json_string
     *
     * Encodes the given message object into its serialized JSON representation.
     */
    @JRubyMethod(name = "encode_json", meta = true)
    public static IRubyObject encodeJson(ThreadContext context, IRubyObject recv, IRubyObject msgRb) {
        RubyMessage message = (RubyMessage) msgRb;
        return Helpers.invoke(context, message.toHash(context), "to_json");
    }

    /*
     * call-seq:
     *     MessageClass.decode_json(data) => message
     *
     * Decodes the given data (as a string containing bytes in protocol buffers wire
     * format) under the interpretration given by this message class's definition
     * and returns a message object with the corresponding field values.
     */
    @JRubyMethod(name = "decode_json", meta = true)
    public static IRubyObject decodeJson(ThreadContext context, IRubyObject recv, IRubyObject json) {
        Ruby runtime = context.runtime;
        RubyMessage ret = (RubyMessage) ((RubyClass) recv).newInstance(context, Block.NULL_BLOCK);
        RubyModule jsonModule = runtime.getClassFromPath("JSON");
        RubyHash opts = RubyHash.newHash(runtime);
        opts.fastASet(runtime.newSymbol("symbolize_names"), runtime.getTrue());
        IRubyObject[] args = new IRubyObject[] { Helpers.invoke(context, jsonModule, "parse", json, opts) };
        ret.initialize(context, args);
        return ret;
    }

    @JRubyMethod(name = {"to_h", "to_hash"})
    public IRubyObject toHash(ThreadContext context) {
        Ruby runtime = context.runtime;
        RubyHash ret = RubyHash.newHash(runtime);
        for (Descriptors.FieldDescriptor fdef : this.descriptor.getFields()) {
            IRubyObject value = getField(context, fdef);
            if (!value.isNil()) {
                if (fdef.isRepeated() && !fdef.isMapField()) {
                    if (fdef.getType() != Descriptors.FieldDescriptor.Type.MESSAGE) {
                        value = Helpers.invoke(context, value, "to_a");
                    } else {
                        RubyArray ary = value.convertToArray();
                        for (int i = 0; i < ary.size(); i++) {
                            IRubyObject submsg = Helpers.invoke(context, ary.eltInternal(i), "to_h");
                            ary.eltInternalSet(i, submsg);
                        }

                        value = ary.to_ary();
                    }
                } else if (value.respondsTo("to_h")) {
                    value = Helpers.invoke(context, value, "to_h");
                } else if (value.respondsTo("to_a")) {
                    value = Helpers.invoke(context, value, "to_a");
                }
            }
            ret.fastASet(runtime.newSymbol(fdef.getName()), value);
        }
        return ret;
    }

    protected DynamicMessage build(ThreadContext context) {
        return build(context, 0);
    }

    protected DynamicMessage build(ThreadContext context, int depth) {
        if (depth > SINK_MAXIMUM_NESTING) {
            throw context.runtime.newRuntimeError("Maximum recursion depth exceeded during encoding.");
        }
        for (Descriptors.FieldDescriptor fieldDescriptor : maps.keySet()) {
            this.builder.clearField(fieldDescriptor);
            RubyDescriptor mapDescriptor = (RubyDescriptor) getDescriptorForField(context, fieldDescriptor);
            for (DynamicMessage kv : maps.get(fieldDescriptor).build(context, mapDescriptor)) {
                this.builder.addRepeatedField(fieldDescriptor, kv);
            }
        }
        for (Descriptors.FieldDescriptor fieldDescriptor : repeatedFields.keySet()) {
            RubyRepeatedField repeatedField = repeatedFields.get(fieldDescriptor);
            this.builder.clearField(fieldDescriptor);
            for (int i = 0; i < repeatedField.size(); i++) {
                Object item = convert(context, fieldDescriptor, repeatedField.get(i), depth);
                this.builder.addRepeatedField(fieldDescriptor, item);
            }
        }
        for (Descriptors.FieldDescriptor fieldDescriptor : fields.keySet()) {
            IRubyObject value = fields.get(fieldDescriptor);
            this.builder.setField(fieldDescriptor, convert(context, fieldDescriptor, value, depth));
        }
        return this.builder.build();
    }

    protected Descriptors.Descriptor getDescriptor() {
        return this.descriptor;
    }

    // Internal use only, called by Google::Protobuf.deep_copy
    protected IRubyObject deepCopy(ThreadContext context) {
        RubyMessage copy = (RubyMessage) metaClass.newInstance(context, Block.NULL_BLOCK);
        for (Descriptors.FieldDescriptor fdef : this.descriptor.getFields()) {
            if (fdef.isRepeated()) {
                copy.addRepeatedField(fdef, this.getRepeatedField(context, fdef).deepCopy(context));
            } else if (fields.containsKey(fdef)) {
                copy.fields.put(fdef, fields.get(fdef));
            } else if (this.builder.hasField(fdef)) {
                copy.fields.put(fdef, wrapField(context, fdef, this.builder.getField(fdef)));
            }
        }
        return copy;
    }

    private RubyRepeatedField getRepeatedField(ThreadContext context, Descriptors.FieldDescriptor fieldDescriptor) {
        if (this.repeatedFields.containsKey(fieldDescriptor)) {
            return this.repeatedFields.get(fieldDescriptor);
        }
        int count = this.builder.getRepeatedFieldCount(fieldDescriptor);
        RubyRepeatedField ret = repeatedFieldForFieldDescriptor(context, fieldDescriptor);
        for (int i = 0; i < count; i++) {
            ret.push(context, wrapField(context, fieldDescriptor, this.builder.getRepeatedField(fieldDescriptor, i)));
        }
        addRepeatedField(fieldDescriptor, ret);
        return ret;
    }

    private void addRepeatedField(Descriptors.FieldDescriptor fieldDescriptor, RubyRepeatedField repeatedField) {
        this.repeatedFields.put(fieldDescriptor, repeatedField);
    }

    private IRubyObject buildFrom(ThreadContext context, DynamicMessage dynamicMessage) {
        this.builder.mergeFrom(dynamicMessage);
        return this;
    }

    private Descriptors.FieldDescriptor findField(ThreadContext context, IRubyObject fieldName) {
        String nameStr = fieldName.asJavaString();
        Descriptors.FieldDescriptor ret = this.descriptor.findFieldByName(Utils.escapeIdentifier(nameStr));
        if (ret == null)
            throw context.runtime.newArgumentError("field " + fieldName.asJavaString() + " is not found");
        return ret;
    }

    private boolean hasField(IRubyObject fieldName) {
        String nameStr = fieldName.asJavaString();
        return this.descriptor.findFieldByName(Utils.escapeIdentifier(nameStr)) != null;
    }

    private void checkRepeatedFieldType(ThreadContext context, IRubyObject value,
                                        Descriptors.FieldDescriptor fieldDescriptor) {
        Ruby runtime = context.runtime;
        if (!(value instanceof RubyRepeatedField)) {
            throw runtime.newTypeError("Expected repeated field array");
        }
    }

    // convert a ruby object to protobuf type, with type check
    private Object convert(ThreadContext context,
                           Descriptors.FieldDescriptor fieldDescriptor,
                           IRubyObject value, int depth) {
        Ruby runtime = context.runtime;
        Object val = null;
        switch (fieldDescriptor.getType()) {
            case INT32:
            case INT64:
            case UINT32:
            case UINT64:
                if (!Utils.isRubyNum(value)) {
                    throw runtime.newTypeError("Expected number type for integral field.");
                }
                Utils.checkIntTypePrecision(context, fieldDescriptor.getType(), value);
                switch (fieldDescriptor.getType()) {
                    case INT32:
                        val = RubyNumeric.num2int(value);
                        break;
                    case INT64:
                        val = RubyNumeric.num2long(value);
                        break;
                    case UINT32:
                        val = Utils.num2uint(value);
                        break;
                    case UINT64:
                        val = Utils.num2ulong(context.runtime, value);
                        break;
                    default:
                        break;
                }
                break;
            case FLOAT:
                if (!Utils.isRubyNum(value))
                    throw runtime.newTypeError("Expected number type for float field.");
                val = (float) RubyNumeric.num2dbl(value);
                break;
            case DOUBLE:
                if (!Utils.isRubyNum(value))
                    throw runtime.newTypeError("Expected number type for double field.");
                val = RubyNumeric.num2dbl(value);
                break;
            case BOOL:
                if (!(value instanceof RubyBoolean))
                    throw runtime.newTypeError("Invalid argument for boolean field.");
                val = value.isTrue();
                break;
            case BYTES:
                Utils.validateStringEncoding(context, fieldDescriptor.getType(), value);
                val = ByteString.copyFrom(((RubyString) value).getBytes());
                break;
            case STRING:
                Utils.validateStringEncoding(context, fieldDescriptor.getType(), value);
                val = ((RubyString) value).asJavaString();
                break;
            case MESSAGE:
                RubyClass typeClass = (RubyClass) ((RubyDescriptor) getDescriptorForField(context, fieldDescriptor)).msgclass(context);
                if (!value.getMetaClass().equals(typeClass))
                    throw runtime.newTypeError(value, "Invalid type to assign to submessage field.");
                val = ((RubyMessage) value).build(context, depth + 1);
                break;
            case ENUM:
                Descriptors.EnumDescriptor enumDescriptor = fieldDescriptor.getEnumType();

                if (Utils.isRubyNum(value)) {
                    val = enumDescriptor.findValueByNumberCreatingIfUnknown(RubyNumeric.num2int(value));
                } else if (value instanceof RubySymbol || value instanceof RubyString) {
                    val = enumDescriptor.findValueByName(value.asJavaString());
                } else {
                    throw runtime.newTypeError("Expected number or symbol type for enum field.");
                }
                if (val == null) {
                    throw runtime.newRangeError("Enum value " + value + " is not found.");
                }
                break;
            default:
                break;
        }
        return val;
    }

    private IRubyObject wrapField(ThreadContext context, Descriptors.FieldDescriptor fieldDescriptor, Object value) {
        if (value == null) {
            return context.runtime.getNil();
        }
        Ruby runtime = context.runtime;
        switch (fieldDescriptor.getType()) {
            case INT32:
            case INT64:
            case UINT32:
            case UINT64:
            case FLOAT:
            case DOUBLE:
            case BOOL:
            case BYTES:
            case STRING:
                return Utils.wrapPrimaryValue(context, fieldDescriptor.getType(), value);
            case MESSAGE:
                RubyClass typeClass = (RubyClass) ((RubyDescriptor) getDescriptorForField(context, fieldDescriptor)).msgclass(context);
                RubyMessage msg = (RubyMessage) typeClass.newInstance(context, Block.NULL_BLOCK);
                return msg.buildFrom(context, (DynamicMessage) value);
            case ENUM:
                Descriptors.EnumValueDescriptor enumValueDescriptor = (Descriptors.EnumValueDescriptor) value;
                if (enumValueDescriptor.getIndex() == -1) { // UNKNOWN ENUM VALUE
                    return runtime.newFixnum(enumValueDescriptor.getNumber());
                }
                return runtime.newSymbol(enumValueDescriptor.getName());
            default:
                return runtime.newString(value.toString());
        }
    }

    private RubyRepeatedField repeatedFieldForFieldDescriptor(ThreadContext context,
                                                              Descriptors.FieldDescriptor fieldDescriptor) {
        IRubyObject typeClass = context.runtime.getNilClass();

        IRubyObject descriptor = getDescriptorForField(context, fieldDescriptor);
        Descriptors.FieldDescriptor.Type type = fieldDescriptor.getType();
        if (type == Descriptors.FieldDescriptor.Type.MESSAGE) {
            typeClass = ((RubyDescriptor) descriptor).msgclass(context);

        } else if (type == Descriptors.FieldDescriptor.Type.ENUM) {
            typeClass = ((RubyEnumDescriptor) descriptor).enummodule(context);
        }
        return new RubyRepeatedField(context.runtime, cRepeatedField, type, typeClass);
    }

    protected IRubyObject getField(ThreadContext context, Descriptors.FieldDescriptor fieldDescriptor) {
        Descriptors.OneofDescriptor oneofDescriptor = fieldDescriptor.getContainingOneof();
        if (oneofDescriptor != null) {
            if (oneofCases.get(oneofDescriptor) == fieldDescriptor) {
                return fields.get(fieldDescriptor);
            } else {
                Descriptors.FieldDescriptor oneofCase = builder.getOneofFieldDescriptor(oneofDescriptor);
                if (oneofCase != fieldDescriptor) {
                  if (fieldDescriptor.getType() == Descriptors.FieldDescriptor.Type.MESSAGE) {
                    return context.runtime.getNil();
                  } else {
                    return wrapField(context, fieldDescriptor, fieldDescriptor.getDefaultValue());
                  }
                }
                IRubyObject value = wrapField(context, oneofCase, builder.getField(oneofCase));
                fields.put(fieldDescriptor, value);
                return value;
            }
        }

        if (Utils.isMapEntry(fieldDescriptor)) {
            RubyMap map = maps.get(fieldDescriptor);
            if (map == null) {
                map = newMapForField(context, fieldDescriptor);
                int mapSize = this.builder.getRepeatedFieldCount(fieldDescriptor);
                Descriptors.FieldDescriptor keyField = fieldDescriptor.getMessageType().findFieldByNumber(1);
                Descriptors.FieldDescriptor valueField = fieldDescriptor.getMessageType().findFieldByNumber(2);
                RubyDescriptor kvDescriptor = (RubyDescriptor) getDescriptorForField(context, fieldDescriptor);
                RubyClass kvClass = (RubyClass) kvDescriptor.msgclass(context);
                for (int i = 0; i < mapSize; i++) {
                    RubyMessage kvMessage = (RubyMessage) kvClass.newInstance(context, Block.NULL_BLOCK);
                    DynamicMessage message = (DynamicMessage) this.builder.getRepeatedField(fieldDescriptor, i);
                    kvMessage.buildFrom(context, message);
                    map.indexSet(context, kvMessage.getField(context, keyField), kvMessage.getField(context, valueField));
                }
                maps.put(fieldDescriptor, map);
            }
            return map;
        }
        if (fieldDescriptor.isRepeated()) {
            return getRepeatedField(context, fieldDescriptor);
        }
        if (fieldDescriptor.getType() != Descriptors.FieldDescriptor.Type.MESSAGE ||
                this.builder.hasField(fieldDescriptor) || fields.containsKey(fieldDescriptor)) {
            if (fields.containsKey(fieldDescriptor)) {
                return fields.get(fieldDescriptor);
            } else {
                IRubyObject value = wrapField(context, fieldDescriptor, this.builder.getField(fieldDescriptor));
                if (this.builder.hasField(fieldDescriptor)) {
                    fields.put(fieldDescriptor, value);
                }
                return value;
            }
        }
        return context.runtime.getNil();
    }

    protected IRubyObject setField(ThreadContext context, Descriptors.FieldDescriptor fieldDescriptor, IRubyObject value) {
        if (Utils.isMapEntry(fieldDescriptor)) {
            if (!(value instanceof RubyMap)) {
                throw context.runtime.newTypeError("Expected Map instance");
            }
            RubyMap thisMap = (RubyMap) getField(context, fieldDescriptor);
            thisMap.mergeIntoSelf(context, value);
        } else if (fieldDescriptor.isRepeated()) {
            checkRepeatedFieldType(context, value, fieldDescriptor);
            if (value instanceof RubyRepeatedField) {
                addRepeatedField(fieldDescriptor, (RubyRepeatedField) value);
            } else {
                RubyArray ary = value.convertToArray();
                RubyRepeatedField repeatedField = rubyToRepeatedField(context, fieldDescriptor, ary);
                addRepeatedField(fieldDescriptor, repeatedField);
            }
        } else {
            Descriptors.OneofDescriptor oneofDescriptor = fieldDescriptor.getContainingOneof();
            if (oneofDescriptor != null) {
                Descriptors.FieldDescriptor oneofCase = oneofCases.get(oneofDescriptor);
                if (oneofCase != null && oneofCase != fieldDescriptor) {
                    fields.remove(oneofCase);
                }
                if (value.isNil()) {
                    oneofCases.remove(oneofDescriptor);
                    fields.remove(fieldDescriptor);
                } else {
                    oneofCases.put(oneofDescriptor, fieldDescriptor);
                    fields.put(fieldDescriptor, value);
                }
            } else {
                Descriptors.FieldDescriptor.Type fieldType = fieldDescriptor.getType();
                IRubyObject typeClass = context.runtime.getObject();
                boolean addValue = true;
                if (fieldType == Descriptors.FieldDescriptor.Type.MESSAGE) {
                    typeClass = ((RubyDescriptor) getDescriptorForField(context, fieldDescriptor)).msgclass(context);
                    if (value.isNil()){
                        addValue = false;
                    }
                } else if (fieldType == Descriptors.FieldDescriptor.Type.ENUM) {
                    typeClass = ((RubyEnumDescriptor) getDescriptorForField(context, fieldDescriptor)).enummodule(context);
                    Descriptors.EnumDescriptor enumDescriptor = fieldDescriptor.getEnumType();
                    if (Utils.isRubyNum(value)) {
                        Descriptors.EnumValueDescriptor val =
                                enumDescriptor.findValueByNumberCreatingIfUnknown(RubyNumeric.num2int(value));
                        if (val.getIndex() != -1) value = context.runtime.newSymbol(val.getName());
                    }
                }
                if (addValue) {
                    value = Utils.checkType(context, fieldType, value, (RubyModule) typeClass);
                    this.fields.put(fieldDescriptor, value);
                } else {
                    this.fields.remove(fieldDescriptor);
                }
            }
        }
        return context.runtime.getNil();
    }

    private String layoutInspect() {
        ThreadContext context = getRuntime().getCurrentContext();
        StringBuilder sb = new StringBuilder();
        for (Descriptors.FieldDescriptor fdef : descriptor.getFields()) {
            sb.append(Utils.unescapeIdentifier(fdef.getName()));
            sb.append(": ");
            sb.append(getField(context, fdef).inspect());
            sb.append(", ");
        }
        return sb.substring(0, sb.length() - 2);
    }

    private IRubyObject getDescriptorForField(ThreadContext context, Descriptors.FieldDescriptor fieldDescriptor) {
        RubyDescriptor thisRbDescriptor = (RubyDescriptor) getDescriptor(context, metaClass);
        return thisRbDescriptor.lookup(fieldDescriptor.getName()).getSubType(context);
    }

    private RubyRepeatedField rubyToRepeatedField(ThreadContext context,
                                                  Descriptors.FieldDescriptor fieldDescriptor, IRubyObject value) {
        RubyArray arr = value.convertToArray();
        RubyRepeatedField repeatedField = repeatedFieldForFieldDescriptor(context, fieldDescriptor);

        RubyClass typeClass = null;
        if (fieldDescriptor.getType() == Descriptors.FieldDescriptor.Type.MESSAGE) {
            RubyDescriptor descriptor = (RubyDescriptor) getDescriptorForField(context, fieldDescriptor);
            typeClass = (RubyClass) descriptor.msgclass(context);
        }

        for (int i = 0; i < arr.size(); i++) {
            IRubyObject row = arr.eltInternal(i);
            if (row instanceof RubyHash && typeClass != null) {
                row = (IRubyObject) typeClass.newInstance(context, row, Block.NULL_BLOCK);
            }

            repeatedField.push(context, row);
        }
        return repeatedField;
    }

    private RubyMap newMapForField(ThreadContext context, Descriptors.FieldDescriptor fieldDescriptor) {
        RubyDescriptor mapDescriptor = (RubyDescriptor) getDescriptorForField(context, fieldDescriptor);
        Descriptors.FieldDescriptor keyField = fieldDescriptor.getMessageType().findFieldByNumber(1);
        Descriptors.FieldDescriptor valueField = fieldDescriptor.getMessageType().findFieldByNumber(2);
        IRubyObject keyType = RubySymbol.newSymbol(context.runtime, keyField.getType().name());
        IRubyObject valueType = RubySymbol.newSymbol(context.runtime, valueField.getType().name());
        if (valueField.getType() == Descriptors.FieldDescriptor.Type.MESSAGE) {
            RubyFieldDescriptor rubyFieldDescriptor = (RubyFieldDescriptor) mapDescriptor.lookup(context,
                    context.runtime.newString("value"));
            RubyDescriptor rubyDescriptor = (RubyDescriptor) rubyFieldDescriptor.getSubType(context);
            return (RubyMap) cMap.newInstance(context, keyType, valueType,
                    rubyDescriptor.msgclass(context), Block.NULL_BLOCK);
        } else {
            return (RubyMap) cMap.newInstance(context, keyType, valueType, Block.NULL_BLOCK);
        }
    }

    private Descriptors.FieldDescriptor getOneofCase(Descriptors.OneofDescriptor oneof) {
        if (oneofCases.containsKey(oneof)) {
            return oneofCases.get(oneof);
        }
        return builder.getOneofFieldDescriptor(oneof);
    }

    private Descriptors.Descriptor descriptor;
    private DynamicMessage.Builder builder;
    private RubyClass cRepeatedField;
    private RubyClass cMap;
    private Map<Descriptors.FieldDescriptor, RubyRepeatedField> repeatedFields;
    private Map<Descriptors.FieldDescriptor, RubyMap> maps;
    private Map<Descriptors.FieldDescriptor, IRubyObject> fields;
    private Map<Descriptors.OneofDescriptor, Descriptors.FieldDescriptor> oneofCases;

    private static final int SINK_MAXIMUM_NESTING = 64;
}
