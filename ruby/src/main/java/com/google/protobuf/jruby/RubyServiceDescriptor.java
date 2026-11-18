// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.jruby;

import com.google.protobuf.CodedInputStream;
import com.google.protobuf.Descriptors.MethodDescriptor;
import com.google.protobuf.Descriptors.ServiceDescriptor;
import java.util.LinkedHashMap;
import java.util.Map;
import org.jruby.*;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.Block;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;

@JRubyClass(name = "ServiceDescriptor")
public class RubyServiceDescriptor extends RubyObject {
  public static void createRubyServiceDescriptor(Ruby runtime) {
    RubyModule protobuf = runtime.getClassFromPath("Google::Protobuf");
    RubyClass cServiceDescriptor =
        protobuf.defineClassUnder(
            "ServiceDescriptor",
            runtime.getObject(),
            new ObjectAllocator() {
              @Override
              public IRubyObject allocate(Ruby runtime, RubyClass klazz) {
                return new RubyServiceDescriptor(runtime, klazz);
              }
            });
    cServiceDescriptor.includeModule(runtime.getEnumerable());
    cServiceDescriptor.defineAnnotatedMethods(RubyServiceDescriptor.class);
    cMethodDescriptor = (RubyClass) runtime.getClassFromPath("Google::Protobuf::MethodDescriptor");
  }

  public RubyServiceDescriptor(Ruby runtime, RubyClass klazz) {
    super(runtime, klazz);
  }

  /*
   * call-seq:
   *     ServiceDescriptor.name => name
   *
   * Returns the name of this service type as a fully-qualified string (e.g.,
   * My.Package.Service).
   */
  @JRubyMethod(name = "name")
  public IRubyObject getName(ThreadContext context) {
    return name;
  }

  /*
   * call-seq:
   *    ServiceDescriptor.file_descriptor
   *
   * Returns the FileDescriptor object this service belongs to.
   */
  @JRubyMethod(name = "file_descriptor")
  public IRubyObject getFileDescriptor(ThreadContext context) {
    return RubyFileDescriptor.getRubyFileDescriptor(context, descriptor);
  }

  /*
   * call-seq:
   *    ServiceDescriptor.options
   *
   * Returns the options set on this protobuf service
   */
  @JRubyMethod
  public IRubyObject options(ThreadContext context) {
    RubyDescriptorPool pool = (RubyDescriptorPool) RubyDescriptorPool.generatedPool(null, null);
    RubyDescriptor serviceOptionsDescriptor =
        (RubyDescriptor)
            pool.lookup(context, context.runtime.newString("google.protobuf.ServiceOptions"));
    RubyClass serviceOptionsClass = (RubyClass) serviceOptionsDescriptor.msgclass(context);
    RubyMessage msg = (RubyMessage) serviceOptionsClass.newInstance(context, Block.NULL_BLOCK);
    return msg.decodeBytes(
        context,
        msg,
        CodedInputStream.newInstance(
            descriptor.getOptions().toByteString().toByteArray()), /*freeze*/
        true);
  }

  /*
   * call-seq:
   *     ServiceDescriptor.each(&block)
   *
   * Iterates over methods in this service, yielding to the block on each one.
   */
  @JRubyMethod(name = "each")
  public IRubyObject each(ThreadContext context, Block block) {
    for (Map.Entry<IRubyObject, RubyMethodDescriptor> entry : methodDescriptors.entrySet()) {
      block.yield(context, entry.getValue());
    }
    return context.nil;
  }

  /*
   * call-seq:
   *     ServiceDescriptor.to_proto => ServiceDescriptorProto
   *
   * Returns the `ServiceDescriptorProto` of this `ServiceDescriptor`.
   */
  @JRubyMethod(name = "to_proto")
  public IRubyObject toProto(ThreadContext context) {
    RubyDescriptorPool pool = (RubyDescriptorPool) RubyDescriptorPool.generatedPool(null, null);
    RubyDescriptor serviceDescriptorProto =
        (RubyDescriptor)
            pool.lookup(
                context, context.runtime.newString("google.protobuf.ServiceDescriptorProto"));
    RubyClass msgClass = (RubyClass) serviceDescriptorProto.msgclass(context);
    RubyMessage msg = (RubyMessage) msgClass.newInstance(context, Block.NULL_BLOCK);
    return msg.decodeBytes(
        context,
        msg,
        CodedInputStream.newInstance(descriptor.toProto().toByteString().toByteArray()), /*freeze*/
        true);
  }

  protected void setDescriptor(
      ThreadContext context, ServiceDescriptor descriptor, RubyDescriptorPool pool) {
    this.descriptor = descriptor;

    // Populate the methods (and preserve the order by using LinkedHashMap)
    methodDescriptors = new LinkedHashMap<IRubyObject, RubyMethodDescriptor>();

    for (MethodDescriptor methodDescriptor : descriptor.getMethods()) {
      RubyMethodDescriptor md =
          (RubyMethodDescriptor) cMethodDescriptor.newInstance(context, Block.NULL_BLOCK);
      md.setDescriptor(context, methodDescriptor, pool);
      methodDescriptors.put(context.runtime.newString(methodDescriptor.getName()), md);
    }
  }

  protected void setName(IRubyObject name) {
    this.name = name;
  }

  private static RubyClass cMethodDescriptor;

  private ServiceDescriptor descriptor;
  private Map<IRubyObject, RubyMethodDescriptor> methodDescriptors;
  private IRubyObject name;
}
