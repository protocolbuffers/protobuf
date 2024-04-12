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

import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import org.jruby.*;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;

@JRubyClass(name = "FieldDescriptor")
public class RubyFieldDescriptor extends RubyObject {
  public static void createRubyFieldDescriptor(Ruby runtime) {
    RubyModule mProtobuf = runtime.getClassFromPath("Google::Protobuf");
    RubyClass cFieldDescriptor =
        mProtobuf.defineClassUnder(
            "FieldDescriptor",
            runtime.getObject(),
            new ObjectAllocator() {
              @Override
              public IRubyObject allocate(Ruby runtime, RubyClass klazz) {
                return new RubyFieldDescriptor(runtime, klazz);
              }
            });
    cFieldDescriptor.defineAnnotatedMethods(RubyFieldDescriptor.class);
  }

  public RubyFieldDescriptor(Ruby runtime, RubyClass klazz) {
    super(runtime, klazz);
  }

  /*
   * call-seq:
   *     FieldDescriptor.default => default
   *
   * Returns this field's default, as a Ruby object, or nil if not yet set.
   */
  // VALUE FieldDescriptor_default(VALUE _self) {
  //   DEFINE_SELF(FieldDescriptor, self, _self);
  //   return layout_get_default(self->fielddef);
  // }

  /*
   * call-seq:
   *     FieldDescriptor.label => label
   *
   * Returns this field's label (i.e., plurality), as a Ruby symbol.
   *
   * Valid field labels are:
   *     :optional, :repeated
   */
  @JRubyMethod(name = "label")
  public IRubyObject getLabel(ThreadContext context) {
    if (label == null) {
      calculateLabel(context);
    }
    return label;
  }

  /*
   * call-seq:
   *     FieldDescriptor.name => name
   *
   * Returns the name of this field as a Ruby String, or nil if it is not set.
   */
  @JRubyMethod(name = "name")
  public IRubyObject getName(ThreadContext context) {
    return this.name;
  }

  /*
   * call-seq:
   *     FieldDescriptor.subtype => message_or_enum_descriptor
   *
   * Returns the message or enum descriptor corresponding to this field's type if
   * it is a message or enum field, respectively, or nil otherwise. Cannot be
   * called *until* the containing message type is added to a pool (and thus
   * resolved).
   */
  @JRubyMethod(name = "subtype")
  public IRubyObject getSubtype(ThreadContext context) {
    if (subtype == null) {
      calculateSubtype(context);
    }
    return subtype;
  }

  /*
   * call-seq:
   *     FieldDescriptor.type => type
   *
   * Returns this field's type, as a Ruby symbol, or nil if not yet set.
   *
   * Valid field types are:
   *     :int32, :int64, :uint32, :uint64, :float, :double, :bool, :string,
   *     :bytes, :message.
   */
  @JRubyMethod(name = "type")
  public IRubyObject getType(ThreadContext context) {
    return Utils.fieldTypeToRuby(context, descriptor.getType());
  }

  /*
   * call-seq:
   *     FieldDescriptor.number => number
   *
   * Returns the tag number for this field.
   */
  @JRubyMethod(name = "number")
  public IRubyObject getNumber(ThreadContext context) {
    return this.number;
  }

  /*
   * call-seq:
   *     FieldDescriptor.submsg_name => submsg_name
   *
   * Returns the name of the message or enum type corresponding to this field, if
   * it is a message or enum field (respectively), or nil otherwise. This type
   * name will be resolved within the context of the pool to which the containing
   * message type is added.
   */
  // VALUE FieldDescriptor_submsg_name(VALUE _self) {
  //   DEFINE_SELF(FieldDescriptor, self, _self);
  //   switch (upb_fielddef_type(self->fielddef)) {
  //     case UPB_TYPE_ENUM:
  //       return rb_str_new2(
  //           upb_enumdef_fullname(upb_fielddef_enumsubdef(self->fielddef)));
  //     case UPB_TYPE_MESSAGE:
  //       return rb_str_new2(
  //           upb_msgdef_fullname(upb_fielddef_msgsubdef(self->fielddef)));
  //     default:
  //       return Qnil;
  //   }
  // }
  /*
   * call-seq:
   *     FieldDescriptor.submsg_name = submsg_name
   *
   * Sets the name of the message or enum type corresponding to this field, if it
   * is a message or enum field (respectively). This type name will be resolved
   * within the context of the pool to which the containing message type is added.
   * Cannot be called on field that are not of message or enum type, or on fields
   * that are part of a message type already added to a pool.
   */
  // @JRubyMethod(name = "submsg_name=")
  // public IRubyObject setSubmsgName(ThreadContext context, IRubyObject name) {
  //     this.builder.setTypeName("." + Utils.escapeIdentifier(name.asJavaString()));
  //     return context.runtime.getNil();
  // }

  /*
   * call-seq:
   *     FieldDescriptor.clear(message)
   *
   * Clears the field from the message if it's set.
   */
  @JRubyMethod(name = "clear")
  public IRubyObject clearValue(ThreadContext context, IRubyObject message) {
    return ((RubyMessage) message).clearField(context, descriptor);
  }

  /*
   * call-seq:
   *     FieldDescriptor.get(message) => value
   *
   * Returns the value set for this field on the given message. Raises an
   * exception if message is of the wrong type.
   */
  @JRubyMethod(name = "get")
  public IRubyObject getValue(ThreadContext context, IRubyObject message) {
    return ((RubyMessage) message).getField(context, descriptor);
  }

  /*
   * call-seq:
   *     FieldDescriptor.has?(message) => boolean
   *
   * Returns whether the value is set on the given message. Raises an
   * exception when calling for fields that do not have presence.
   */
  @JRubyMethod(name = "has?")
  public IRubyObject has(ThreadContext context, IRubyObject message) {
    return ((RubyMessage) message).hasField(context, descriptor);
  }

  /*
   * call-seq:
   *     FieldDescriptor.set(message, value)
   *
   * Sets the value corresponding to this field to the given value on the given
   * message. Raises an exception if message is of the wrong type. Performs the
   * ordinary type-checks for field setting.
   */
  @JRubyMethod(name = "set")
  public IRubyObject setValue(ThreadContext context, IRubyObject message, IRubyObject value) {
    ((RubyMessage) message).setField(context, descriptor, value);
    return context.nil;
  }

  protected void setDescriptor(
      ThreadContext context, FieldDescriptor descriptor, RubyDescriptorPool pool) {
    if (descriptor.isRequired()
        && descriptor.getFile().getSyntax() == FileDescriptor.Syntax.PROTO3) {
      throw Utils.createTypeError(
          context,
          descriptor.getName()
              + " is labeled required but required fields are unsupported in proto3");
    }
    this.descriptor = descriptor;
    this.name = context.runtime.newString(descriptor.getName());
    this.pool = pool;
  }

  private void calculateLabel(ThreadContext context) {
    if (descriptor.isRepeated()) {
      this.label = context.runtime.newSymbol("repeated");
    } else if (descriptor.isOptional()) {
      this.label = context.runtime.newSymbol("optional");
    } else {
      this.label = context.nil;
    }
  }

  private void calculateSubtype(ThreadContext context) {
    FieldDescriptor.Type fdType = descriptor.getType();
    if (fdType == FieldDescriptor.Type.MESSAGE) {
      RubyString messageName = context.runtime.newString(descriptor.getMessageType().getFullName());
      this.subtype = pool.lookup(context, messageName);
    } else if (fdType == FieldDescriptor.Type.ENUM) {
      RubyString enumName = context.runtime.newString(descriptor.getEnumType().getFullName());
      this.subtype = pool.lookup(context, enumName);
    } else {
      this.subtype = context.nil;
    }
  }

  private static final String DOT = ".";

  private FieldDescriptor descriptor;
  private IRubyObject name;
  private IRubyObject label;
  private IRubyObject number;
  private IRubyObject subtype;
  private RubyDescriptorPool pool;
}
