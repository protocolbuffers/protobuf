/*
 * Protocol Buffers - Google's data interchange format
 * Copyright 2024 Google Inc.  All rights reserved.
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

import com.google.protobuf.CodedInputStream;
import com.google.protobuf.Descriptors.MethodDescriptor;
import org.jruby.*;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.Block;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;

@JRubyClass(name = "MethoDescriptor")
public class RubyMethodDescriptor extends RubyObject {
  public static void createRubyMethodDescriptor(Ruby runtime) {
    RubyModule protobuf = runtime.getClassFromPath("Google::Protobuf");
    RubyClass cMethodDescriptor =
        protobuf.defineClassUnder(
            "MethodDescriptor",
            runtime.getObject(),
            new ObjectAllocator() {
              @Override
              public IRubyObject allocate(Ruby runtime, RubyClass klazz) {
                return new RubyMethodDescriptor(runtime, klazz);
              }
            });
    cMethodDescriptor.defineAnnotatedMethods(RubyMethodDescriptor.class);
  }

  public RubyMethodDescriptor(Ruby runtime, RubyClass klazz) {
    super(runtime, klazz);
  }

  /*
   * call-seq:
   *     MethodDescriptor.name => name
   *
   * Returns the name of this method
   */
  @JRubyMethod(name = "name")
  public IRubyObject getName(ThreadContext context) {
    return context.runtime.newString(this.descriptor.getName());
  }

  /*
   * call-seq:
   *    MethodDescriptor.options
   *
   * Returns the options set on this protobuf rpc method
   */
  @JRubyMethod
  public IRubyObject options(ThreadContext context) {
    RubyDescriptorPool pool = (RubyDescriptorPool) RubyDescriptorPool.generatedPool(null, null);
    RubyDescriptor methodOptionsDescriptor =
        (RubyDescriptor)
            pool.lookup(context, context.runtime.newString("google.protobuf.MethodOptions"));
    RubyClass methodOptionsClass = (RubyClass) methodOptionsDescriptor.msgclass(context);
    RubyMessage msg = (RubyMessage) methodOptionsClass.newInstance(context, Block.NULL_BLOCK);
    return msg.decodeBytes(
        context,
        msg,
        CodedInputStream.newInstance(
            descriptor.getOptions().toByteString().toByteArray()), /*freeze*/
        true);
  }

  /*
   * call-seq:
   *      MethodDescriptor.input_type => Descriptor
   *
   * Returns the `Descriptor` for the request message type of this method
   */
  @JRubyMethod(name = "input_type")
  public IRubyObject getInputType(ThreadContext context) {
    return this.pool.lookup(
        context, context.runtime.newString(this.descriptor.getInputType().getFullName()));
  }

  /*
   * call-seq:
   *      MethodDescriptor.output_type => Descriptor
   *
   * Returns the `Descriptor` for the response message type of this method
   */
  @JRubyMethod(name = "output_type")
  public IRubyObject getOutputType(ThreadContext context) {
    return this.pool.lookup(
        context, context.runtime.newString(this.descriptor.getOutputType().getFullName()));
  }

  /*
   * call-seq:
   *      MethodDescriptor.client_streaming => bool
   *
   * Returns whether or not this is a streaming request method
   */
  @JRubyMethod(name = "client_streaming")
  public IRubyObject getClientStreaming(ThreadContext context) {
    return this.descriptor.isClientStreaming()
        ? context.runtime.getTrue()
        : context.runtime.getFalse();
  }

  /*
   * call-seq:
   *      MethodDescriptor.server_streaming => bool
   *
   * Returns whether or not this is a streaming response method
   */
  @JRubyMethod(name = "server_streaming")
  public IRubyObject getServerStreaming(ThreadContext context) {
    return this.descriptor.isServerStreaming()
        ? context.runtime.getTrue()
        : context.runtime.getFalse();
  }

  /*
   * call-seq:
   *     MethodDescriptor.to_proto => MethodDescriptorProto
   *
   * Returns the `MethodDescriptorProto` of this `MethodDescriptor`.
   */
  @JRubyMethod(name = "to_proto")
  public IRubyObject toProto(ThreadContext context) {
    RubyDescriptorPool pool = (RubyDescriptorPool) RubyDescriptorPool.generatedPool(null, null);
    RubyDescriptor methodDescriptorProto =
        (RubyDescriptor)
            pool.lookup(
                context, context.runtime.newString("google.protobuf.MethodDescriptorProto"));
    RubyClass msgClass = (RubyClass) methodDescriptorProto.msgclass(context);
    RubyMessage msg = (RubyMessage) msgClass.newInstance(context, Block.NULL_BLOCK);
    return msg.decodeBytes(
        context,
        msg,
        CodedInputStream.newInstance(descriptor.toProto().toByteString().toByteArray()), /*freeze*/
        true);
  }

  protected void setDescriptor(
      ThreadContext context, MethodDescriptor descriptor, RubyDescriptorPool pool) {
    this.descriptor = descriptor;
    this.pool = pool;
  }

  private MethodDescriptor descriptor;
  private IRubyObject name;
  private RubyDescriptorPool pool;
}
