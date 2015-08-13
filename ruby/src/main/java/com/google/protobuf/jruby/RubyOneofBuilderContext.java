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

import org.jruby.Ruby;
import org.jruby.RubyClass;
import org.jruby.RubyModule;
import org.jruby.RubyObject;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;

@JRubyClass(name = "OneofBuilderContext")
public class RubyOneofBuilderContext extends RubyObject {
    public static void createRubyOneofBuilderContext(Ruby runtime) {
        RubyModule protobuf = runtime.getClassFromPath("Google::Protobuf");
        RubyModule internal = protobuf.defineModuleUnder("Internal");
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

    @JRubyMethod
    public IRubyObject initialize(ThreadContext context, IRubyObject oneofdef) {
        this.descriptor = (RubyOneofDescriptor) oneofdef;
        this.cFieldDescriptor = (RubyClass) context.runtime.getClassFromPath("Google::Protobuf::FieldDescriptor");
        return this;
    }

    @JRubyMethod(required = 3, optional = 1)
    public IRubyObject optional(ThreadContext context, IRubyObject[] args) {
        IRubyObject name = args[0];
        IRubyObject type = args[1];
        IRubyObject number = args[2];
        IRubyObject typeClass = args.length > 3 ? args[3] : context.runtime.getNil();
        RubyFieldDescriptor fieldDescriptor = Utils.msgdefCreateField(context, "optional",
                name, type, number, typeClass, cFieldDescriptor);
        descriptor.addField(context, fieldDescriptor);
        return this;
    }

    private RubyOneofDescriptor descriptor;
    private RubyClass cFieldDescriptor;
}
