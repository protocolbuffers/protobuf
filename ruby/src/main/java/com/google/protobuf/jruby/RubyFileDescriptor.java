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

import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.Descriptors.GenericDescriptor;
import com.google.protobuf.LegacyDescriptorsUtil.LegacyFileDescriptor;
import com.google.protobuf.LegacyDescriptorsUtil.LegacyFileDescriptor.Syntax.*;
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

  /*
   * call-seq:
   *     FileDescriptor.syntax => syntax
   *
   * Returns this file descriptors syntax.
   *
   * Valid syntax versions are:
   *     :proto2 or :proto3.
   */
  @JRubyMethod(name = "syntax")
  public IRubyObject getSyntax(ThreadContext context) {
    switch (LegacyFileDescriptor.getSyntax(fileDescriptor)) {
      case PROTO2:
        return context.runtime.newSymbol("proto2");
      case PROTO3:
        return context.runtime.newSymbol("proto3");
      default:
        return context.nil;
    }
  }

  private static RubyClass cFileDescriptor;

  private FileDescriptor fileDescriptor;
}
