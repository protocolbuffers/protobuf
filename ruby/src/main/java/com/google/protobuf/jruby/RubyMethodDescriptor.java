// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
