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

import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.DescriptorProtos.OneofDescriptorProto;
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
        RubyModule internal = runtime.getClassFromPath("Google::Protobuf::Internal");
        RubyClass cMessageBuilderContext = internal.defineClassUnder("MessageBuilderContext", runtime.getObject(), new ObjectAllocator() {
            @Override
            public IRubyObject allocate(Ruby runtime, RubyClass klazz) {
                return new RubyMessageBuilderContext(runtime, klazz);
            }
        });
        cMessageBuilderContext.defineAnnotatedMethods(RubyMessageBuilderContext.class);

        cFieldDescriptor = (RubyClass) runtime.getClassFromPath("Google::Protobuf::FieldDescriptor");
        cOneofBuilderContext = (RubyClass) runtime.getClassFromPath("Google::Protobuf::Internal::OneofBuilderContext");
    }

    public RubyMessageBuilderContext(Ruby runtime, RubyClass klazz) {
        super(runtime, klazz);
    }

    @JRubyMethod
    public IRubyObject initialize(ThreadContext context, IRubyObject fileBuilderContext, IRubyObject name) {
        this.fileBuilderContext = (RubyFileBuilderContext) fileBuilderContext;
        this.builder = this.fileBuilderContext.getNewMessageBuilder();
        this.builder.setName(name.asJavaString());

        return this;
    }

    /*
     * call-seq:
     *     MessageBuilderContext.optional(name, type, number, type_class = nil,
     *                                    options = nil)
     *
     * Defines a new optional field on this message type with the given type, tag
     * number, and type class (for message and enum fields). The type must be a Ruby
     * symbol (as accepted by FieldDescriptor#type=) and the type_class must be a
     * string, if present (as accepted by FieldDescriptor#submsg_name=).
     */
    @JRubyMethod(required = 3, optional = 2)
    public IRubyObject optional(ThreadContext context, IRubyObject[] args) {
        addField(context, OPTIONAL, args, false);
        return context.nil;
    }

    @JRubyMethod(required = 3, optional = 2)
    public IRubyObject proto3_optional(ThreadContext context, IRubyObject[] args) {
      addField(context, OPTIONAL, args, true);
      return context.nil;
    }

    /*
     * call-seq:
     *     MessageBuilderContext.required(name, type, number, type_class = nil,
     *                                    options = nil)
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
    @JRubyMethod(required = 3, optional = 2)
    public IRubyObject required(ThreadContext context, IRubyObject[] args) {
        if (fileBuilderContext.isProto3()) throw Utils.createTypeError(context, "Required fields are unsupported in proto3");
        addField(context, "required", args, false);
        return context.nil;
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
        addField(context, "repeated", args, false);
        return context.nil;
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
        if (!fileBuilderContext.isProto3()) throw runtime.newArgumentError("Cannot add a native map field using proto2 syntax.");

        RubySymbol messageSym = runtime.newSymbol("message");

        IRubyObject name = args[0];
        IRubyObject keyType = args[1];
        IRubyObject valueType = args[2];
        IRubyObject number = args[3];
        IRubyObject typeClass = args.length > 4 ? args[4] : context.nil;

        // Validate the key type. We can't accept enums, messages, or floats/doubles
        // as map keys. (We exclude these explicitly, and the field-descriptor setter
        // below then ensures that the type is one of the remaining valid options.)
        if (keyType.equals(runtime.newSymbol("float")) ||
                keyType.equals(runtime.newSymbol("double")) ||
                keyType.equals(runtime.newSymbol("enum")) ||
                keyType.equals(messageSym))
            throw runtime.newArgumentError("Cannot add a map field with a float, double, enum, or message type.");

        DescriptorProto.Builder mapEntryBuilder = fileBuilderContext.getNewMessageBuilder();
        mapEntryBuilder.setName(builder.getName() + "_MapEntry_" + name.asJavaString());
        mapEntryBuilder.getOptionsBuilder().setMapEntry(true);

        mapEntryBuilder.addField(
            Utils.createFieldBuilder(
                context,
                OPTIONAL,
                new IRubyObject[] {
                    runtime.newString("key"),
                    keyType,
                    runtime.newFixnum(1)
                }
            )
        );

        mapEntryBuilder.addField(
            Utils.createFieldBuilder(
                context,
                OPTIONAL,
                new IRubyObject[] {
                    runtime.newString("value"),
                    valueType,
                    runtime.newFixnum(2),
                    typeClass
                }
            )
        );

        IRubyObject[] addFieldArgs = {
            name, messageSym, number, runtime.newString(mapEntryBuilder.getName())
        };

        repeated(context, addFieldArgs);

        return context.nil;
    }

    /*
     * call-seq:
     *     MessageBuilderContext.oneof(name, &block) => nil
     *
     * Creates a new OneofDescriptor with the given name, creates a
     * OneofBuilderContext attached to that OneofDescriptor, evaluates the given
     * block in the context of that OneofBuilderContext with #instance_eval, and
     * then adds the oneof to the message.
     *
     * This is the recommended, idiomatic way to build oneof definitions.
     */
    @JRubyMethod
    public IRubyObject oneof(ThreadContext context, IRubyObject name, Block block) {
        RubyOneofBuilderContext ctx = (RubyOneofBuilderContext)
                cOneofBuilderContext.newInstance(
                        context,
                        context.runtime.newFixnum(builder.getOneofDeclCount()),
                        this,
                        Block.NULL_BLOCK
                );

        builder.addOneofDeclBuilder().setName(name.asJavaString());
        ctx.instance_eval(context, block);

        return context.nil;
    }

    protected void addFieldBuilder(FieldDescriptorProto.Builder fieldBuilder) {
        builder.addField(fieldBuilder);
    }

    private FieldDescriptorProto.Builder addField(ThreadContext context, String label, IRubyObject[] args, boolean proto3Optional) {
        FieldDescriptorProto.Builder fieldBuilder =
                Utils.createFieldBuilder(context, label, args);

        fieldBuilder.setProto3Optional(proto3Optional);
        builder.addField(fieldBuilder);

        return fieldBuilder;
    }

    private static RubyClass cFieldDescriptor;
    private static RubyClass cOneofBuilderContext;

    private static final String OPTIONAL = "optional";

    private DescriptorProto.Builder builder;
    private RubyClass cDescriptor;
    private RubyFileBuilderContext fileBuilderContext;
}
