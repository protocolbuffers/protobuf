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

import org.jruby.RubyModule;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;

public class RubyEnum {
  /*
   * call-seq:
   *     Enum.lookup(number) => name
   *
   * This module method, provided on each generated enum module, looks up an enum
   * value by number and returns its name as a Ruby symbol, or nil if not found.
   */
  @JRubyMethod(meta = true)
  public static IRubyObject lookup(ThreadContext context, IRubyObject recv, IRubyObject number) {
    RubyEnumDescriptor rubyEnumDescriptor = (RubyEnumDescriptor) getDescriptor(context, recv);
    return rubyEnumDescriptor.numberToName(context, number);
  }

  /*
   * call-seq:
   *     Enum.resolve(name) => number
   *
   * This module method, provided on each generated enum module, looks up an enum
   * value by name (as a Ruby symbol) and returns its name, or nil if not found.
   */
  @JRubyMethod(meta = true)
  public static IRubyObject resolve(ThreadContext context, IRubyObject recv, IRubyObject name) {
    RubyEnumDescriptor rubyEnumDescriptor = (RubyEnumDescriptor) getDescriptor(context, recv);
    return rubyEnumDescriptor.nameToNumber(context, name);
  }

  /*
   * call-seq:
   *     Enum.descriptor
   *
   * This module method, provided on each generated enum module, returns the
   * EnumDescriptor corresponding to this enum type.
   */
  @JRubyMethod(meta = true, name = "descriptor")
  public static IRubyObject getDescriptor(ThreadContext context, IRubyObject recv) {
    return ((RubyModule) recv).getInstanceVariable(Utils.DESCRIPTOR_INSTANCE_VAR);
  }
}
