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

import com.google.protobuf.DescriptorProtos.EnumDescriptorProto;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import org.jruby.Ruby;
import org.jruby.RubyClass;
import org.jruby.RubyModule;
import org.jruby.RubyObject;
import org.jruby.RubyNumeric;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.Block;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;

@JRubyClass(name = "EnumDescriptor", include = "Enumerable")
public class RubyEnumDescriptor extends RubyObject {
    public static void createRubyEnumDescriptor(Ruby runtime) {
        RubyModule mProtobuf = runtime.getClassFromPath("Google::Protobuf");
        RubyClass cEnumDescriptor = mProtobuf.defineClassUnder("EnumDescriptor", runtime.getObject(), new ObjectAllocator() {
            @Override
            public IRubyObject allocate(Ruby runtime, RubyClass klazz) {
                return new RubyEnumDescriptor(runtime, klazz);
            }
        });
        cEnumDescriptor.includeModule(runtime.getEnumerable());
        cEnumDescriptor.defineAnnotatedMethods(RubyEnumDescriptor.class);
    }

    public RubyEnumDescriptor(Ruby runtime, RubyClass klazz) {
        super(runtime, klazz);
    }

    /*
     * call-seq:
     *     EnumDescriptor.name => name
     *
     * Returns the name of this enum type.
     */
    @JRubyMethod(name = "name")
    public IRubyObject getName(ThreadContext context) {
        return this.name;
    }

    /*
     * call-seq:
     *     EnumDescriptor.each(&block)
     *
     * Iterates over key => value mappings in this enum's definition, yielding to
     * the block with (key, value) arguments for each one.
     */
    @JRubyMethod
    public IRubyObject each(ThreadContext context, Block block) {
        Ruby runtime = context.runtime;
        for (EnumValueDescriptor enumValueDescriptor : descriptor.getValues()) {
            block.yield(context, runtime.newArray(runtime.newSymbol(enumValueDescriptor.getName()),
                    runtime.newFixnum(enumValueDescriptor.getNumber())));
        }
        return context.nil;
    }

    /*
     * call-seq:
     *     EnumDescriptor.enummodule => module
     *
     * Returns the Ruby module corresponding to this enum type. Cannot be called
     * until the enum descriptor has been added to a pool.
     */
    @JRubyMethod
    public IRubyObject enummodule(ThreadContext context) {
        return module;
    }

    /*
     * call-seq:
     *    EnumDescriptor.file_descriptor
     *
     * Returns the FileDescriptor object this enum belongs to.
     */
    @JRubyMethod(name = "file_descriptor")
    public IRubyObject getFileDescriptor(ThreadContext context) {
       return RubyFileDescriptor.getRubyFileDescriptor(context, descriptor);
    }

    public boolean isValidValue(ThreadContext context, IRubyObject value) {
        EnumValueDescriptor enumValue;

        if (Utils.isRubyNum(value)) {
            enumValue = descriptor.findValueByNumberCreatingIfUnknown(RubyNumeric.num2int(value));
        } else {
            enumValue = descriptor.findValueByName(value.asJavaString());
        }

        return enumValue != null;
    }

    protected IRubyObject nameToNumber(ThreadContext context, IRubyObject name)  {
        EnumValueDescriptor value = descriptor.findValueByName(name.asJavaString());
        return value == null ? context.nil : context.runtime.newFixnum(value.getNumber());
    }

    protected IRubyObject numberToName(ThreadContext context, IRubyObject number)  {
        EnumValueDescriptor value = descriptor.findValueByNumber(RubyNumeric.num2int(number));
        return value == null ? context.nil : context.runtime.newSymbol(value.getName());
    }

    protected void setDescriptor(ThreadContext context, EnumDescriptor descriptor) {
        this.descriptor = descriptor;
        this.module = buildModuleFromDescriptor(context);
    }

    protected void setName(IRubyObject name) {
        this.name = name;
    }

    private RubyModule buildModuleFromDescriptor(ThreadContext context) {
        Ruby runtime = context.runtime;

        RubyModule enumModule = RubyModule.newModule(runtime);
        boolean defaultValueRequiredButNotFound = descriptor.getFile().getSyntax() == FileDescriptor.Syntax.PROTO3;
        for (EnumValueDescriptor value : descriptor.getValues()) {
            String name = value.getName();
            // Make sure its a valid constant name before trying to create it
            if (Character.isUpperCase(name.codePointAt(0))) {
                enumModule.defineConstant(name, runtime.newFixnum(value.getNumber()));
            } else {
                runtime.getWarnings().warn("Enum value " + name + " does not start with an uppercase letter as is required for Ruby constants.");
            }
            if (value.getNumber() == 0) {
                defaultValueRequiredButNotFound = false;
            }
        }

        if (defaultValueRequiredButNotFound) {
            throw Utils.createTypeError(context, "Enum definition " + name + " does not contain a value for '0'");
        }
        enumModule.instance_variable_set(runtime.newString(Utils.DESCRIPTOR_INSTANCE_VAR), this);
        enumModule.defineAnnotatedMethods(RubyEnum.class);
        return enumModule;
    }

    private EnumDescriptor descriptor;
    private EnumDescriptorProto.Builder builder;
    private IRubyObject name;
    private RubyModule module;
}
