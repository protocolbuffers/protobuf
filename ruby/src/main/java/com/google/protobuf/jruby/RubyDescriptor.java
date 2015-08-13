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
import org.jruby.runtime.Block;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;

import java.util.HashMap;
import java.util.Map;


@JRubyClass(name = "Descriptor", include = "Enumerable")
public class RubyDescriptor extends RubyObject {
    public static void createRubyDescriptor(Ruby runtime) {
        RubyModule protobuf = runtime.getClassFromPath("Google::Protobuf");
        RubyClass cDescriptor = protobuf.defineClassUnder("Descriptor", runtime.getObject(), new ObjectAllocator() {
            @Override
            public IRubyObject allocate(Ruby runtime, RubyClass klazz) {
                return new RubyDescriptor(runtime, klazz);
            }
        });
        cDescriptor.includeModule(runtime.getEnumerable());
        cDescriptor.defineAnnotatedMethods(RubyDescriptor.class);
    }

    public RubyDescriptor(Ruby runtime, RubyClass klazz) {
        super(runtime, klazz);
    }

    /*
     * call-seq:
     *     Descriptor.new => descriptor
     *
     * Creates a new, empty, message type descriptor. At a minimum, its name must be
     * set before it is added to a pool. It cannot be used to create messages until
     * it is added to a pool, after which it becomes immutable (as part of a
     * finalization process).
     */
    @JRubyMethod
    public IRubyObject initialize(ThreadContext context) {
        this.builder = DescriptorProtos.DescriptorProto.newBuilder();
        this.fieldDefMap = new HashMap<String, RubyFieldDescriptor>();
        this.oneofDefs = new HashMap<IRubyObject, RubyOneofDescriptor>();
        return this;
    }

    /*
     * call-seq:
     *     Descriptor.name => name
     *
     * Returns the name of this message type as a fully-qualfied string (e.g.,
     * My.Package.MessageType).
     */
    @JRubyMethod(name = "name")
    public IRubyObject getName(ThreadContext context) {
        return this.name;
    }

    /*
     * call-seq:
     *    Descriptor.name = name
     *
     * Assigns a name to this message type. The descriptor must not have been added
     * to a pool yet.
     */
    @JRubyMethod(name = "name=")
    public IRubyObject setName(ThreadContext context, IRubyObject name) {
        this.name = name;
        this.builder.setName(Utils.escapeIdentifier(this.name.asJavaString()));
        return context.runtime.getNil();
    }

    /*
     * call-seq:
     *     Descriptor.add_field(field) => nil
     *
     * Adds the given FieldDescriptor to this message type. The descriptor must not
     * have been added to a pool yet. Raises an exception if a field with the same
     * name or number already exists. Sub-type references (e.g. for fields of type
     * message) are not resolved at this point.
     */
    @JRubyMethod(name = "add_field")
    public IRubyObject addField(ThreadContext context, IRubyObject obj) {
        RubyFieldDescriptor fieldDef = (RubyFieldDescriptor) obj;
        this.fieldDefMap.put(fieldDef.getName(context).asJavaString(), fieldDef);
        this.builder.addField(fieldDef.build());
        return context.runtime.getNil();
    }

    /*
     * call-seq:
     *     Descriptor.lookup(name) => FieldDescriptor
     *
     * Returns the field descriptor for the field with the given name, if present,
     * or nil if none.
     */
    @JRubyMethod
    public IRubyObject lookup(ThreadContext context, IRubyObject fieldName) {
        return this.fieldDefMap.get(fieldName.asJavaString());
    }

    /*
     * call-seq:
     *     Descriptor.msgclass => message_klass
     *
     * Returns the Ruby class created for this message type. Valid only once the
     * message type has been added to a pool.
     */
    @JRubyMethod
    public IRubyObject msgclass(ThreadContext context) {
        if (this.klazz == null) {
            this.klazz = buildClassFromDescriptor(context);
        }
        return this.klazz;
    }

    /*
     * call-seq:
     *     Descriptor.each(&block)
     *
     * Iterates over fields in this message type, yielding to the block on each one.
     */
    @JRubyMethod
    public IRubyObject each(ThreadContext context, Block block) {
        for (Map.Entry<String, RubyFieldDescriptor> entry : fieldDefMap.entrySet()) {
            block.yield(context, entry.getValue());
        }
        return context.runtime.getNil();
    }

    /*
     * call-seq:
     *     Descriptor.add_oneof(oneof) => nil
     *
     * Adds the given OneofDescriptor to this message type. This descriptor must not
     * have been added to a pool yet. Raises an exception if a oneof with the same
     * name already exists, or if any of the oneof's fields' names or numbers
     * conflict with an existing field in this message type. All fields in the oneof
     * are added to the message descriptor. Sub-type references (e.g. for fields of
     * type message) are not resolved at this point.
     */
    @JRubyMethod(name = "add_oneof")
    public IRubyObject addOneof(ThreadContext context, IRubyObject obj) {
        RubyOneofDescriptor def = (RubyOneofDescriptor) obj;
        builder.addOneofDecl(def.build(builder.getOneofDeclCount()));
        for (RubyFieldDescriptor fieldDescriptor : def.getFields()) {
            addField(context, fieldDescriptor);
        }
        oneofDefs.put(def.getName(context), def);
        return context.runtime.getNil();
    }

    /*
     * call-seq:
     *     Descriptor.each_oneof(&block) => nil
     *
     * Invokes the given block for each oneof in this message type, passing the
     * corresponding OneofDescriptor.
     */
    @JRubyMethod(name = "each_oneof")
    public IRubyObject eachOneof(ThreadContext context, Block block) {
        for (RubyOneofDescriptor oneofDescriptor : oneofDefs.values()) {
            block.yieldSpecific(context, oneofDescriptor);
        }
        return context.runtime.getNil();
    }

    /*
     * call-seq:
     *     Descriptor.lookup_oneof(name) => OneofDescriptor
     *
     * Returns the oneof descriptor for the oneof with the given name, if present,
     * or nil if none.
     */
    @JRubyMethod(name = "lookup_oneof")
    public IRubyObject lookupOneof(ThreadContext context, IRubyObject name) {
        if (name instanceof RubySymbol) {
            name = ((RubySymbol) name).id2name();
        }
        return oneofDefs.containsKey(name) ? oneofDefs.get(name) : context.runtime.getNil();
    }

    public void setDescriptor(Descriptors.Descriptor descriptor) {
        this.descriptor = descriptor;
    }

    public Descriptors.Descriptor getDescriptor() {
        return this.descriptor;
    }

    public DescriptorProtos.DescriptorProto.Builder getBuilder() {
        return builder;
    }

    public void setMapEntry(boolean isMapEntry) {
        this.builder.setOptions(DescriptorProtos.MessageOptions.newBuilder().setMapEntry(isMapEntry));
    }

    private RubyModule buildClassFromDescriptor(ThreadContext context) {
        Ruby runtime = context.runtime;

        ObjectAllocator allocator = new ObjectAllocator() {
            @Override
            public IRubyObject allocate(Ruby runtime, RubyClass klazz) {
                return new RubyMessage(runtime, klazz, descriptor);
            }
        };

        // rb_define_class_id
        RubyClass klass = RubyClass.newClass(runtime, runtime.getObject());
        klass.setAllocator(allocator);
        klass.makeMetaClass(runtime.getObject().getMetaClass());
        klass.inherit(runtime.getObject());
        RubyModule messageExts = runtime.getClassFromPath("Google::Protobuf::MessageExts");
        klass.include(new IRubyObject[] {messageExts});
        klass.instance_variable_set(runtime.newString(Utils.DESCRIPTOR_INSTANCE_VAR), this);
        klass.defineAnnotatedMethods(RubyMessage.class);
        return klass;
    }

    protected RubyFieldDescriptor lookup(String fieldName) {
        return fieldDefMap.get(Utils.unescapeIdentifier(fieldName));
    }

    private IRubyObject name;
    private RubyModule klazz;

    private DescriptorProtos.DescriptorProto.Builder builder;
    private Descriptors.Descriptor descriptor;
    private Map<String, RubyFieldDescriptor> fieldDefMap;
    private Map<IRubyObject, RubyOneofDescriptor> oneofDefs;
}
