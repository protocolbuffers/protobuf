// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.jruby;

import com.google.protobuf.CodedInputStream;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.Descriptors.GenericDescriptor;
import org.jruby.*;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.Block;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;

@JRubyClass(name = "FileDescriptor")
public class RubyFileDescriptor extends RubyObject {
  public static void createRubyFileDescriptor(Ruby runtime) {
    RubyModule mProtobuf = runtime.getClassFromPath("Google::Protobuf");
    cFileDescriptor =
        mProtobuf.defineClassUnder(
            "FileDescriptor",
            runtime.getObject(),
            new ObjectAllocator() {
              @Override
              public IRubyObject allocate(Ruby runtime, RubyClass klazz) {
                return new RubyFileDescriptor(runtime, klazz);
              }
            });
    cFileDescriptor.defineAnnotatedMethods(RubyFileDescriptor.class);
  }

  public static RubyFileDescriptor getRubyFileDescriptor(
      ThreadContext context, GenericDescriptor descriptor) {
    RubyFileDescriptor rfd =
        (RubyFileDescriptor) cFileDescriptor.newInstance(context, Block.NULL_BLOCK);
    rfd.fileDescriptor = descriptor.getFile();
    return rfd;
  }

  public RubyFileDescriptor(Ruby runtime, RubyClass klazz) {
    super(runtime, klazz);
  }

  /*
   * call-seq:
   *     FileDescriptor.name => name
   *
   * Returns the name of the file.
   */
  @JRubyMethod(name = "name")
  public IRubyObject getName(ThreadContext context) {
    String name = fileDescriptor.getName();
    return name == null ? context.nil : context.runtime.newString(name);
  }

  @JRubyMethod
  public IRubyObject options(ThreadContext context) {
    RubyDescriptorPool pool = (RubyDescriptorPool) RubyDescriptorPool.generatedPool(null, null);
    RubyDescriptor fileOptionsDescriptor =
        (RubyDescriptor)
            pool.lookup(context, context.runtime.newString("google.protobuf.FileOptions"));
    RubyClass fileOptionsClass = (RubyClass) fileOptionsDescriptor.msgclass(context);
    RubyMessage msg = (RubyMessage) fileOptionsClass.newInstance(context, Block.NULL_BLOCK);
    return msg.decodeBytes(
        context,
        msg,
        CodedInputStream.newInstance(
            fileDescriptor.getOptions().toByteString().toByteArray()), /*freeze*/
        true);
  }

  /*
   * call-seq:
   *     FileDescriptor.to_proto => FileDescriptorProto
   *
   * Returns the `FileDescriptorProto` of this `FileDescriptor`.
   */
  @JRubyMethod(name = "to_proto")
  public IRubyObject toProto(ThreadContext context) {
    RubyDescriptorPool pool = (RubyDescriptorPool) RubyDescriptorPool.generatedPool(null, null);
    RubyDescriptor fileDescriptorProto =
        (RubyDescriptor)
            pool.lookup(context, context.runtime.newString("google.protobuf.FileDescriptorProto"));
    RubyClass msgClass = (RubyClass) fileDescriptorProto.msgclass(context);
    RubyMessage msg = (RubyMessage) msgClass.newInstance(context, Block.NULL_BLOCK);
    return msg.decodeBytes(
        context,
        msg,
        CodedInputStream.newInstance(
            fileDescriptor.toProto().toByteString().toByteArray()), /*freeze*/
        true);
  }

  private static RubyClass cFileDescriptor;

  private FileDescriptor fileDescriptor;
}
