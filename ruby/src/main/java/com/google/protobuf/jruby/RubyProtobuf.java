// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.jruby;

import org.jruby.Ruby;
import org.jruby.RubyModule;
import org.jruby.anno.JRubyMethod;
import org.jruby.anno.JRubyModule;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;

@JRubyModule(name = "Protobuf")
public class RubyProtobuf {

  public static void createProtobuf(Ruby runtime) {
    RubyModule mGoogle = runtime.getModule("Google");
    RubyModule mProtobuf = mGoogle.defineModuleUnder("Protobuf");
    mProtobuf.defineAnnotatedMethods(RubyProtobuf.class);
    RubyModule mInternal = mProtobuf.defineModuleUnder("Internal");
  }

  /*
   * call-seq:
   *     Google::Protobuf.deep_copy(obj) => copy_of_obj
   *
   * Performs a deep copy of either a RepeatedField instance or a message object,
   * recursively copying its members.
   */
  @JRubyMethod(name = "deep_copy", meta = true)
  public static IRubyObject deepCopy(ThreadContext context, IRubyObject self, IRubyObject message) {
    if (message instanceof RubyMessage) {
      return ((RubyMessage) message).deepCopy(context);
    } else if (message instanceof RubyRepeatedField) {
      return ((RubyRepeatedField) message).deepCopy(context);
    } else {
      return ((RubyMap) message).deepCopy(context);
    }
  }

  /*
   * call-seq:
   *     Google::Protobuf.discard_unknown(msg)
   *
   * Discard unknown fields in the given message object and recursively discard
   * unknown fields in submessages.
   */
  @JRubyMethod(name = "discard_unknown", meta = true)
  public static IRubyObject discardUnknown(
      ThreadContext context, IRubyObject self, IRubyObject message) {
    ((RubyMessage) message).discardUnknownFields(context);
    return context.nil;
  }
}
