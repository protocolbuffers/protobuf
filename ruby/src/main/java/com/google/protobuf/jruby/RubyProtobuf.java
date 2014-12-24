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
    }

    /*
     * call-seq:
     *     Google::Protobuf.encode(msg) => bytes
     *
     * Encodes the given message object to protocol buffers wire format. This is an
     * alternative to the #encode method on msg's class.
     */
    @JRubyMethod(meta = true)
    public static IRubyObject encode(ThreadContext context, IRubyObject self, IRubyObject message) {
        return RubyMessage.encode(context, message.getMetaClass(), message);
    }

    /*
     * call-seq:
     *     Google::Protobuf.decode(class, bytes) => msg
     *
     * Decodes the given bytes as protocol buffers wire format under the
     * interpretation given by the given class's message definition. This is an
     * alternative to the #decode method on the given class.
     */
    @JRubyMethod(meta = true)
    public static IRubyObject decode(ThreadContext context, IRubyObject self, IRubyObject klazz, IRubyObject message) {
        return RubyMessage.decode(context, klazz, message);
    }

    /*
     * call-seq:
     *     Google::Protobuf.encode_json(msg) => json_string
     *
     * Encodes the given message object to its JSON representation. This is an
     * alternative to the #encode_json method on msg's class.
     */
    @JRubyMethod(name = "encode_json", meta = true)
    public static IRubyObject encodeJson(ThreadContext context, IRubyObject self, IRubyObject message) {
        return RubyMessage.encodeJson(context, message.getMetaClass(), message);
    }

    /*
     * call-seq:
     *     Google::Protobuf.decode_json(class, json_string) => msg
     *
     * Decodes the given JSON string under the interpretation given by the given
     * class's message definition. This is an alternative to the #decode_json method
     * on the given class.
     */
    @JRubyMethod(name = "decode_json", meta = true)
    public static IRubyObject decodeJson(ThreadContext context, IRubyObject self, IRubyObject klazz, IRubyObject message) {
        return RubyMessage.decodeJson(context, klazz, message);
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
}
