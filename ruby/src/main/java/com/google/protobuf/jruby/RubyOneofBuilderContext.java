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

import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import org.jruby.Ruby;
import org.jruby.RubyClass;
import org.jruby.RubyHash;
import org.jruby.RubyModule;
import org.jruby.RubyNumeric;
import org.jruby.RubyObject;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;

@JRubyClass(name = "OneofBuilderContext")
public class RubyOneofBuilderContext extends RubyObject {
    public static void createRubyOneofBuilderContext(Ruby runtime) {
        RubyModule internal = runtime.getClassFromPath("Google::Protobuf::Internal");
        RubyClass cRubyOneofBuidlerContext = internal.defineClassUnder("OneofBuilderContext", runtime.getObject(), new ObjectAllocator() {
            @Override
            public IRubyObject allocate(Ruby ruby, RubyClass rubyClass) {
                return new RubyOneofBuilderContext(ruby, rubyClass);
            }
        });
        cRubyOneofBuidlerContext.defineAnnotatedMethods(RubyOneofBuilderContext.class);
    }

    public RubyOneofBuilderContext(Ruby ruby, RubyClass rubyClass) {
        super(ruby, rubyClass);
    }

    /*
     * call-seq:
     *     OneofBuilderContext.new(oneof_index, message_builder) => context
     *
     * Create a new oneof builder context around the given oneof descriptor and
     * builder context. This class is intended to serve as a DSL context to be used
     * with #instance_eval.
     */
    @JRubyMethod
    public IRubyObject initialize(ThreadContext context, IRubyObject index, IRubyObject messageBuilder) {
        this.builder = (RubyMessageBuilderContext) messageBuilder;
        this.index = RubyNumeric.num2int(index);

        return this;
    }

    /*
     * call-seq:
     *     OneofBuilderContext.optional(name, type, number, type_class = nil,
     *                                  options = nil)
     *
     * Defines a new optional field in this oneof with the given type, tag number,
     * and type class (for message and enum fields). The type must be a Ruby symbol
     * (as accepted by FieldDescriptor#type=) and the type_class must be a string,
     * if present (as accepted by FieldDescriptor#submsg_name=).
     */
    @JRubyMethod(required = 3, optional = 2)
    public IRubyObject optional(ThreadContext context, IRubyObject[] args) {
        FieldDescriptorProto.Builder fieldBuilder =
                Utils.createFieldBuilder(context, "optional", args);
        fieldBuilder.setOneofIndex(index);
        builder.addFieldBuilder(fieldBuilder);

        return context.nil;
    }

    private RubyMessageBuilderContext builder;
    private int index;
}
