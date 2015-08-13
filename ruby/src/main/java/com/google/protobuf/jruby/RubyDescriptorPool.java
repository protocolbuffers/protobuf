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
import org.jruby.runtime.*;
import org.jruby.runtime.builtin.IRubyObject;

import java.util.HashMap;
import java.util.Map;

@JRubyClass(name = "DescriptorPool")
public class RubyDescriptorPool extends RubyObject {
    public static void createRubyDescriptorPool(Ruby runtime) {
        RubyModule protobuf = runtime.getClassFromPath("Google::Protobuf");
        RubyClass cDescriptorPool = protobuf.defineClassUnder("DescriptorPool", runtime.getObject(), new ObjectAllocator() {
            @Override
            public IRubyObject allocate(Ruby runtime, RubyClass klazz) {
                return new RubyDescriptorPool(runtime, klazz);
            }
        });

        cDescriptorPool.defineAnnotatedMethods(RubyDescriptorPool.class);
        descriptorPool = (RubyDescriptorPool) cDescriptorPool.newInstance(runtime.getCurrentContext(), Block.NULL_BLOCK);
    }

    public RubyDescriptorPool(Ruby ruby, RubyClass klazz) {
        super(ruby, klazz);
    }

    @JRubyMethod
    public IRubyObject initialize(ThreadContext context) {
        this.symtab = new HashMap<IRubyObject, IRubyObject>();
        this.cBuilder = (RubyClass) context.runtime.getClassFromPath("Google::Protobuf::Builder");
        this.builder = DescriptorProtos.FileDescriptorProto.newBuilder();
        return this;
    }

    @JRubyMethod
    public IRubyObject build(ThreadContext context, Block block) {
        RubyBuilder ctx = (RubyBuilder) cBuilder.newInstance(context, Block.NULL_BLOCK);
        if (block.arity() == Arity.ONE_ARGUMENT) {
            block.yield(context, ctx);
        } else {
            Binding binding = block.getBinding();
            binding.setSelf(ctx);
            block.yieldSpecific(context);
        }
        ctx.finalizeToPool(context, this);
        buildFileDescriptor(context);
        return context.runtime.getNil();
    }

    @JRubyMethod
    public IRubyObject lookup(ThreadContext context, IRubyObject name) {
        IRubyObject descriptor = this.symtab.get(name);
        if (descriptor == null) {
            return context.runtime.getNil();
        }
        return descriptor;
    }

    /*
     * call-seq:
     *     DescriptorPool.generated_pool => descriptor_pool
     *
     * Class method that returns the global DescriptorPool. This is a singleton into
     * which generated-code message and enum types are registered. The user may also
     * register types in this pool for convenience so that they do not have to hold
     * a reference to a private pool instance.
     */
    @JRubyMethod(meta = true, name = "generated_pool")
    public static IRubyObject generatedPool(ThreadContext context, IRubyObject recv) {
        return descriptorPool;
    }

    protected void addToSymtab(ThreadContext context, RubyDescriptor def) {
        symtab.put(def.getName(context), def);
        this.builder.addMessageType(def.getBuilder());
    }

    protected void addToSymtab(ThreadContext context, RubyEnumDescriptor def) {
        symtab.put(def.getName(context), def);
        this.builder.addEnumType(def.getBuilder());
    }

    private void buildFileDescriptor(ThreadContext context) {
        Ruby runtime = context.runtime;
        try {
            this.builder.setSyntax("proto3");
            final Descriptors.FileDescriptor fileDescriptor = Descriptors.FileDescriptor.buildFrom(
                    this.builder.build(), new Descriptors.FileDescriptor[]{});

            for (Descriptors.EnumDescriptor enumDescriptor : fileDescriptor.getEnumTypes()) {
                String enumName = Utils.unescapeIdentifier(enumDescriptor.getName());
                if (enumDescriptor.findValueByNumber(0) == null) {
                    throw runtime.newTypeError("Enum definition " + enumName
                            + " does not contain a value for '0'");
                }
                ((RubyEnumDescriptor) symtab.get(runtime.newString(enumName)))
                        .setDescriptor(enumDescriptor);
            }
            for (Descriptors.Descriptor descriptor : fileDescriptor.getMessageTypes()) {
                RubyDescriptor rubyDescriptor = ((RubyDescriptor)
                        symtab.get(runtime.newString(Utils.unescapeIdentifier(descriptor.getName()))));
                for (Descriptors.FieldDescriptor fieldDescriptor : descriptor.getFields()) {
                    if (fieldDescriptor.isRequired()) {
                        throw runtime.newTypeError("Required fields are unsupported in proto3");
                    }
                    RubyFieldDescriptor rubyFieldDescriptor = rubyDescriptor.lookup(fieldDescriptor.getName());
                    rubyFieldDescriptor.setFieldDef(fieldDescriptor);
                    if (fieldDescriptor.getType() == Descriptors.FieldDescriptor.Type.MESSAGE) {
                        RubyDescriptor subType = (RubyDescriptor) lookup(context,
                                runtime.newString(Utils.unescapeIdentifier(fieldDescriptor.getMessageType().getName())));
                        rubyFieldDescriptor.setSubType(subType);
                    }
                    if (fieldDescriptor.getType() == Descriptors.FieldDescriptor.Type.ENUM) {
                        RubyEnumDescriptor subType = (RubyEnumDescriptor) lookup(context,
                                runtime.newString(Utils.unescapeIdentifier(fieldDescriptor.getEnumType().getName())));
                        rubyFieldDescriptor.setSubType(subType);
                    }
                }
                rubyDescriptor.setDescriptor(descriptor);
            }
        } catch (Descriptors.DescriptorValidationException e) {
            throw runtime.newRuntimeError(e.getMessage());
        }
    }

    private static RubyDescriptorPool descriptorPool;

    private RubyClass cBuilder;
    private Map<IRubyObject, IRubyObject> symtab;
    private DescriptorProtos.FileDescriptorProto.Builder builder;
}
