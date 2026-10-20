// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
