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
import org.jruby.runtime.Binding;
import org.jruby.runtime.Block;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;

@JRubyClass(name = "MessageBuilderContext")
public class RubyMessageBuilderContext extends RubyObject {
    public static void createRubyMessageBuilderContext(Ruby runtime) {
        RubyModule protobuf = runtime.getClassFromPath("Google::Protobuf");
        RubyClass cMessageBuilderContext = protobuf.defineClassUnder("MessageBuilderContext", runtime.getObject(), new ObjectAllocator() {
            @Override
            public IRubyObject allocate(Ruby runtime, RubyClass klazz) {
                return new RubyMessageBuilderContext(runtime, klazz);
            }
        });
        cMessageBuilderContext.defineAnnotatedMethods(RubyMessageBuilderContext.class);
    }

    public RubyMessageBuilderContext(Ruby ruby, RubyClass klazz) {
        super(ruby, klazz);
    }

    @JRubyMethod
    public IRubyObject initialize(ThreadContext context, IRubyObject descriptor, IRubyObject rubyBuilder) {
        this.cFieldDescriptor = (RubyClass) context.runtime.getClassFromPath("Google::Protobuf::FieldDescriptor");
        this.cDescriptor = (RubyClass) context.runtime.getClassFromPath("Google::Protobuf::Descriptor");
        this.cOneofDescriptor = (RubyClass) context.runtime.getClassFromPath("Google::Protobuf::OneofDescriptor");
        this.cOneofBuilderContext = (RubyClass) context.runtime.getClassFromPath("Google::Protobuf::Internal::OneofBuilderContext");
        this.descriptor = (RubyDescriptor) descriptor;
        this.builder = (RubyBuilder) rubyBuilder;
        return this;
    }

    /*
     * call-seq:
     *     MessageBuilderContext.optional(name, type, number, type_class = nil)
     *
     * Defines a new optional field on this message type with the given type, tag
     * number, and type class (for message and enum fields). The type must be a Ruby
     * symbol (as accepted by FieldDescriptor#type=) and the type_class must be a
     * string, if present (as accepted by FieldDescriptor#submsg_name=).
     */
    @JRubyMethod(required = 3, optional = 1)
    public IRubyObject optional(ThreadContext context, IRubyObject[] args) {
        Ruby runtime = context.runtime;
        IRubyObject typeClass = runtime.getNil();
        if (args.length > 3) typeClass = args[3];
        msgdefAddField(context, "optional", args[0], args[1], args[2], typeClass);
        return context.runtime.getNil();
    }

    /*
     * call-seq:
     *     MessageBuilderContext.required(name, type, number, type_class = nil)
     *
     * Defines a new required field on this message type with the given type, tag
     * number, and type class (for message and enum fields). The type must be a Ruby
     * symbol (as accepted by FieldDescriptor#type=) and the type_class must be a
     * string, if present (as accepted by FieldDescriptor#submsg_name=).
     *
     * Proto3 does not have required fields, but this method exists for
     * completeness. Any attempt to add a message type with required fields to a
     * pool will currently result in an error.
     */
    @JRubyMethod(required = 3, optional = 1)
    public IRubyObject required(ThreadContext context, IRubyObject[] args) {
        IRubyObject typeClass = context.runtime.getNil();
        if (args.length > 3) typeClass = args[3];
        msgdefAddField(context, "required", args[0], args[1], args[2], typeClass);
        return context.runtime.getNil();
    }

    /*
     * call-seq:
     *     MessageBuilderContext.repeated(name, type, number, type_class = nil)
     *
     * Defines a new repeated field on this message type with the given type, tag
     * number, and type class (for message and enum fields). The type must be a Ruby
     * symbol (as accepted by FieldDescriptor#type=) and the type_class must be a
     * string, if present (as accepted by FieldDescriptor#submsg_name=).
     */
    @JRubyMethod(required = 3, optional = 1)
    public IRubyObject repeated(ThreadContext context, IRubyObject[] args) {
        IRubyObject typeClass = context.runtime.getNil();
        if (args.length > 3) typeClass = args[3];
        msgdefAddField(context, "repeated", args[0], args[1], args[2], typeClass);
        return context.runtime.getNil();
    }

    /*
     * call-seq:
     *     MessageBuilderContext.map(name, key_type, value_type, number,
     *                               value_type_class = nil)
     *
     * Defines a new map field on this message type with the given key and value
     * types, tag number, and type class (for message and enum value types). The key
     * type must be :int32/:uint32/:int64/:uint64, :bool, or :string. The value type
     * type must be a Ruby symbol (as accepted by FieldDescriptor#type=) and the
     * type_class must be a string, if present (as accepted by
     * FieldDescriptor#submsg_name=).
     */
    @JRubyMethod(required = 4, optional = 1)
    public IRubyObject map(ThreadContext context, IRubyObject[] args) {
        Ruby runtime = context.runtime;
        IRubyObject name = args[0];
        IRubyObject keyType = args[1];
        IRubyObject valueType = args[2];
        IRubyObject number = args[3];
        IRubyObject typeClass = args.length > 4 ? args[4] : context.runtime.getNil();

        // Validate the key type. We can't accept enums, messages, or floats/doubles
        // as map keys. (We exclude these explicitly, and the field-descriptor setter
        // below then ensures that the type is one of the remaining valid options.)
        if (keyType.equals(RubySymbol.newSymbol(runtime, "float")) ||
                keyType.equals(RubySymbol.newSymbol(runtime, "double")) ||
                keyType.equals(RubySymbol.newSymbol(runtime, "enum")) ||
                keyType.equals(RubySymbol.newSymbol(runtime, "message")))
            throw runtime.newArgumentError("Cannot add a map field with a float, double, enum, or message type.");

        // Create a new message descriptor for the map entry message, and create a
        // repeated submessage field here with that type.
        RubyDescriptor mapentryDesc = (RubyDescriptor) cDescriptor.newInstance(context, Block.NULL_BLOCK);
        IRubyObject mapentryDescName = RubySymbol.newSymbol(runtime, name).id2name(context);
        mapentryDesc.setName(context, mapentryDescName);
        mapentryDesc.setMapEntry(true);

        //optional <type> key = 1;
        RubyFieldDescriptor keyField = (RubyFieldDescriptor) cFieldDescriptor.newInstance(context, Block.NULL_BLOCK);
        keyField.setName(context, runtime.newString("key"));
        keyField.setLabel(context, RubySymbol.newSymbol(runtime, "optional"));
        keyField.setNumber(context, runtime.newFixnum(1));
        keyField.setType(context, keyType);
        mapentryDesc.addField(context, keyField);

        //optional <type> value = 2;
        RubyFieldDescriptor valueField = (RubyFieldDescriptor) cFieldDescriptor.newInstance(context, Block.NULL_BLOCK);
        valueField.setName(context, runtime.newString("value"));
        valueField.setLabel(context, RubySymbol.newSymbol(runtime, "optional"));
        valueField.setNumber(context, runtime.newFixnum(2));
        valueField.setType(context, valueType);
        if (! typeClass.isNil()) valueField.setSubmsgName(context, typeClass);
        mapentryDesc.addField(context, valueField);

        // Add the map-entry message type to the current builder, and use the type to
        // create the map field itself.
        this.builder.pendingList.add(mapentryDesc);

        msgdefAddField(context, "repeated", name, runtime.newSymbol("message"), number, mapentryDescName);
        return runtime.getNil();
    }

    @JRubyMethod
    public IRubyObject oneof(ThreadContext context, IRubyObject name, Block block) {
        RubyOneofDescriptor oneofdef = (RubyOneofDescriptor)
                cOneofDescriptor.newInstance(context, Block.NULL_BLOCK);
        RubyOneofBuilderContext ctx = (RubyOneofBuilderContext)
                cOneofBuilderContext.newInstance(context, oneofdef, Block.NULL_BLOCK);
        oneofdef.setName(context, name);
        Binding binding = block.getBinding();
        binding.setSelf(ctx);
        block.yieldSpecific(context);
        descriptor.addOneof(context, oneofdef);
        return context.runtime.getNil();
    }

    private void msgdefAddField(ThreadContext context, String label, IRubyObject name,
                                IRubyObject type, IRubyObject number, IRubyObject typeClass) {
        descriptor.addField(context,
                Utils.msgdefCreateField(context, label, name, type, number, typeClass, cFieldDescriptor));
    }

    private RubyDescriptor descriptor;
    private RubyBuilder builder;
    private RubyClass cFieldDescriptor;
    private RubyClass cOneofDescriptor;
    private RubyClass cOneofBuilderContext;
    private RubyClass cDescriptor;
}
