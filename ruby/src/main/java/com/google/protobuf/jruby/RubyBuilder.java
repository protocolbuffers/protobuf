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

import org.jruby.*;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.*;
import org.jruby.runtime.builtin.IRubyObject;

@JRubyClass(name = "Builder")
public class RubyBuilder extends RubyObject {
    public static void createRubyBuilder(Ruby runtime) {
        RubyModule protobuf = runtime.getClassFromPath("Google::Protobuf");
        RubyClass cBuilder = protobuf.defineClassUnder("Builder", runtime.getObject(), new ObjectAllocator() {
            @Override
            public IRubyObject allocate(Ruby runtime, RubyClass klazz) {
                return new RubyBuilder(runtime, klazz);
            }
        });
        cBuilder.defineAnnotatedMethods(RubyBuilder.class);
    }

    public RubyBuilder(Ruby runtime, RubyClass metaClass) {
        super(runtime, metaClass);
        this.cDescriptor = (RubyClass) runtime.getClassFromPath("Google::Protobuf::Descriptor");
        this.cEnumDescriptor = (RubyClass) runtime.getClassFromPath("Google::Protobuf::EnumDescriptor");
        this.cMessageBuilderContext = (RubyClass) runtime.getClassFromPath("Google::Protobuf::MessageBuilderContext");
        this.cEnumBuilderContext = (RubyClass) runtime.getClassFromPath("Google::Protobuf::EnumBuilderContext");
    }

    /*
     * call-seq:
     *     Builder.new => builder
     *
     * Creates a new Builder. A Builder can accumulate a set of new message and enum
     * descriptors and atomically register them into a pool in a way that allows for
     * (co)recursive type references.
     */
    @JRubyMethod
    public IRubyObject initialize(ThreadContext context) {
        Ruby runtime = context.runtime;
        this.pendingList = runtime.newArray();
        return this;
    }

    /*
     * call-seq:
     *     Builder.add_message(name, &block)
     *
     * Creates a new, empty descriptor with the given name, and invokes the block in
     * the context of a MessageBuilderContext on that descriptor. The block can then
     * call, e.g., MessageBuilderContext#optional and MessageBuilderContext#repeated
     * methods to define the message fields.
     *
     * This is the recommended, idiomatic way to build message definitions.
     */
    @JRubyMethod(name = "add_message")
    public IRubyObject addMessage(ThreadContext context, IRubyObject name, Block block) {
        RubyDescriptor msgdef = (RubyDescriptor) cDescriptor.newInstance(context, Block.NULL_BLOCK);
        IRubyObject ctx = cMessageBuilderContext.newInstance(context, msgdef, this, Block.NULL_BLOCK);
        msgdef.setName(context, name);
        if (block.isGiven()) {
            if (block.arity() == Arity.ONE_ARGUMENT) {
                block.yield(context, ctx);
            } else {
                Binding binding = block.getBinding();
                binding.setSelf(ctx);
                block.yieldSpecific(context);
            }
        }
        this.pendingList.add(msgdef);
        return context.runtime.getNil();
    }

    /*
     * call-seq:
     *     Builder.add_enum(name, &block)
     *
     * Creates a new, empty enum descriptor with the given name, and invokes the block in
     * the context of an EnumBuilderContext on that descriptor. The block can then
     * call EnumBuilderContext#add_value to define the enum values.
     *
     * This is the recommended, idiomatic way to build enum definitions.
     */
    @JRubyMethod(name = "add_enum")
    public IRubyObject addEnum(ThreadContext context, IRubyObject name, Block block) {
        RubyEnumDescriptor enumDef = (RubyEnumDescriptor) cEnumDescriptor.newInstance(context, Block.NULL_BLOCK);
        IRubyObject ctx = cEnumBuilderContext.newInstance(context, enumDef, Block.NULL_BLOCK);
        enumDef.setName(context, name);

        if (block.isGiven()) {
            if (block.arity() == Arity.ONE_ARGUMENT) {
                block.yield(context, ctx);
            } else {
                Binding binding = block.getBinding();
                binding.setSelf(ctx);
                block.yieldSpecific(context);
            }
        }

        this.pendingList.add(enumDef);
        return context.runtime.getNil();
    }

    /*
     * call-seq:
     *     Builder.finalize_to_pool(pool)
     *
     * Adds all accumulated message and enum descriptors created in this builder
     * context to the given pool. The operation occurs atomically, and all
     * descriptors can refer to each other (including in cycles). This is the only
     * way to build (co)recursive message definitions.
     *
     * This method is usually called automatically by DescriptorPool#build after it
     * invokes the given user block in the context of the builder. The user should
     * not normally need to call this manually because a Builder is not normally
     * created manually.
     */
    @JRubyMethod(name = "finalize_to_pool")
    public IRubyObject finalizeToPool(ThreadContext context, IRubyObject rbPool) {
        RubyDescriptorPool pool = (RubyDescriptorPool) rbPool;
        for (int i = 0; i < this.pendingList.size(); i++) {
            IRubyObject defRb = this.pendingList.entry(i);
            if (defRb instanceof RubyDescriptor) {
                pool.addToSymtab(context, (RubyDescriptor) defRb);
            } else {
                pool.addToSymtab(context, (RubyEnumDescriptor) defRb);
            }
        }
        this.pendingList = context.runtime.newArray();
        return context.runtime.getNil();
    }

    protected RubyArray pendingList;
    private RubyClass cDescriptor, cEnumDescriptor, cMessageBuilderContext, cEnumBuilderContext;
}
