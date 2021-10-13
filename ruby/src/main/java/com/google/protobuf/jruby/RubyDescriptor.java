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

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.OneofDescriptor;
import org.jruby.*;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.Block;
import org.jruby.runtime.Helpers;
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
        cFieldDescriptor = (RubyClass) runtime.getClassFromPath("Google::Protobuf::FieldDescriptor");
        cOneofDescriptor = (RubyClass) runtime.getClassFromPath("Google::Protobuf::OneofDescriptor");
    }

    public RubyDescriptor(Ruby runtime, RubyClass klazz) {
        super(runtime, klazz);
    }

    /*
     * call-seq:
     *     Descriptor.name => name
     *
     * Returns the name of this message type as a fully-qualified string (e.g.,
     * My.Package.MessageType).
     */
    @JRubyMethod(name = "name")
    public IRubyObject getName(ThreadContext context) {
        return name;
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
        return Helpers.nullToNil(fieldDescriptors.get(fieldName), context.nil);
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
        return klazz;
    }

    /*
     * call-seq:
     *     Descriptor.each(&block)
     *
     * Iterates over fields in this message type, yielding to the block on each one.
     */
    @JRubyMethod
    public IRubyObject each(ThreadContext context, Block block) {
        for (Map.Entry<IRubyObject, RubyFieldDescriptor> entry : fieldDescriptors.entrySet()) {
            block.yield(context, entry.getValue());
        }
        return context.nil;
    }

    /*
     * call-seq:
     *    Descriptor.file_descriptor
     *
     * Returns the FileDescriptor object this message belongs to.
     */
     @JRubyMethod(name = "file_descriptor")
     public IRubyObject getFileDescriptor(ThreadContext context) {
        return RubyFileDescriptor.getRubyFileDescriptor(context, descriptor);
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
        for (RubyOneofDescriptor oneofDescriptor : oneofDescriptors.values()) {
            block.yieldSpecific(context, oneofDescriptor);
        }
        return context.nil;
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
        return Helpers.nullToNil(oneofDescriptors.get(Utils.symToString(name)), context.nil);
    }

    protected FieldDescriptor getField(String name) {
        return descriptor.findFieldByName(name);
    }

    protected void setDescriptor(ThreadContext context, Descriptor descriptor, RubyDescriptorPool pool) {
        Ruby runtime = context.runtime;
        Map<FieldDescriptor, RubyFieldDescriptor> cache = new HashMap();
        this.descriptor = descriptor;

        // Populate the field caches
        fieldDescriptors = new HashMap<IRubyObject, RubyFieldDescriptor>();
        oneofDescriptors = new HashMap<IRubyObject, RubyOneofDescriptor>();

        for (FieldDescriptor fieldDescriptor : descriptor.getFields()) {
            RubyFieldDescriptor fd = (RubyFieldDescriptor) cFieldDescriptor.newInstance(context, Block.NULL_BLOCK);
            fd.setDescriptor(context, fieldDescriptor, pool);
            fieldDescriptors.put(runtime.newString(fieldDescriptor.getName()), fd);
            cache.put(fieldDescriptor, fd);
        }

        for (OneofDescriptor oneofDescriptor : descriptor.getRealOneofs()) {
            RubyOneofDescriptor ood = (RubyOneofDescriptor) cOneofDescriptor.newInstance(context, Block.NULL_BLOCK);
            ood.setDescriptor(context, oneofDescriptor, cache);
            oneofDescriptors.put(runtime.newString(oneofDescriptor.getName()), ood);
        }

        // Make sure our class is built
        this.klazz = buildClassFromDescriptor(context);
    }

    protected void setName(IRubyObject name) {
        this.name = name;
    }

    private RubyClass buildClassFromDescriptor(ThreadContext context) {
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

    private static RubyClass cFieldDescriptor;
    private static RubyClass cOneofDescriptor;

    private Descriptor descriptor;
    private IRubyObject name;
    private Map<IRubyObject, RubyFieldDescriptor> fieldDescriptors;
    private Map<IRubyObject, RubyOneofDescriptor> oneofDescriptors;
    private RubyClass klazz;
}
