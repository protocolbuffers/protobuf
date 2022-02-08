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

import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.DescriptorValidationException;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.InvalidProtocolBufferException;
import org.jruby.*;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.exceptions.RaiseException;
import org.jruby.runtime.*;
import org.jruby.runtime.builtin.IRubyObject;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
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
        cDescriptor = (RubyClass) runtime.getClassFromPath("Google::Protobuf::Descriptor");
        cEnumDescriptor = (RubyClass) runtime.getClassFromPath("Google::Protobuf::EnumDescriptor");
    }

    public RubyDescriptorPool(Ruby runtime, RubyClass klazz) {
        super(runtime, klazz);
        this.fileDescriptors = new ArrayList<>();
        this.symtab = new HashMap<IRubyObject, IRubyObject>();
    }

    @JRubyMethod
    public IRubyObject build(ThreadContext context, Block block) {
        RubyClass cBuilder = (RubyClass) context.runtime.getClassFromPath("Google::Protobuf::Internal::Builder");
        RubyBasicObject ctx = (RubyBasicObject) cBuilder.newInstance(context, this, Block.NULL_BLOCK);
        ctx.instance_eval(context, block);
        ctx.callMethod(context, "build"); // Needs to be called to support the deprecated syntax
        return context.nil;
    }

    /*
     * call-seq:
     *     DescriptorPool.lookup(name) => descriptor
     *
     * Finds a Descriptor or EnumDescriptor by name and returns it, or nil if none
     * exists with the given name.
     *
     * This currently lazy loads the ruby descriptor objects as they are requested.
     * This allows us to leave the heavy lifting to the java library
     */
    @JRubyMethod
    public IRubyObject lookup(ThreadContext context, IRubyObject name) {
        return Helpers.nullToNil(symtab.get(name), context.nil);
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

    @JRubyMethod(required = 1)
    public IRubyObject add_serialized_file (ThreadContext context, IRubyObject data ) {
        byte[] bin = data.convertToString().getBytes();
        try {
            FileDescriptorProto.Builder builder = FileDescriptorProto.newBuilder().mergeFrom(bin);
            registerFileDescriptor(context, builder);
        } catch (InvalidProtocolBufferException e) {
            throw RaiseException.from(context.runtime, (RubyClass) context.runtime.getClassFromPath("Google::Protobuf::ParseError"), e.getMessage());
        }
        return context.nil;
    }

    protected void registerFileDescriptor(ThreadContext context, FileDescriptorProto.Builder builder) {
        final FileDescriptor fd;
        try {
            fd = FileDescriptor.buildFrom(builder.build(), existingFileDescriptors());
        } catch (DescriptorValidationException e) {
            throw context.runtime.newRuntimeError(e.getMessage());
        }

        String packageName = fd.getPackage();
        if (!packageName.isEmpty()) {
            packageName = packageName + ".";
        }

        // Need to make sure enums are registered first in case anything references them
        for (EnumDescriptor ed : fd.getEnumTypes()) registerEnumDescriptor(context, ed, packageName);
        for (Descriptor message : fd.getMessageTypes()) registerDescriptor(context, message, packageName);

        // Mark this as a loaded file
        fileDescriptors.add(fd);
    }

    private void registerDescriptor(ThreadContext context, Descriptor descriptor, String parentPath) {
        String fullName = parentPath + descriptor.getName();
        String fullPath = fullName + ".";
        RubyString name = context.runtime.newString(fullName);

        RubyDescriptor des = (RubyDescriptor) cDescriptor.newInstance(context, Block.NULL_BLOCK);
        des.setName(name);
        des.setDescriptor(context, descriptor, this);
        symtab.put(name, des);

        // Need to make sure enums are registered first in case anything references them
        for (EnumDescriptor ed : descriptor.getEnumTypes()) registerEnumDescriptor(context, ed, fullPath);
        for (Descriptor message : descriptor.getNestedTypes()) registerDescriptor(context, message, fullPath);
    }

    private void registerEnumDescriptor(ThreadContext context, EnumDescriptor descriptor, String parentPath) {
        RubyString name = context.runtime.newString(parentPath + descriptor.getName());
        RubyEnumDescriptor des = (RubyEnumDescriptor) cEnumDescriptor.newInstance(context, Block.NULL_BLOCK);
        des.setName(name);
        des.setDescriptor(context, descriptor);
        symtab.put(name, des);
    }

    private FileDescriptor[] existingFileDescriptors() {
        return fileDescriptors.toArray(new FileDescriptor[fileDescriptors.size()]);
    }

    private static RubyClass cDescriptor;
    private static RubyClass cEnumDescriptor;
    private static RubyDescriptorPool descriptorPool;

    private List<FileDescriptor> fileDescriptors;
    private Map<IRubyObject, IRubyObject> symtab;
}
