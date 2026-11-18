// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.jruby;

import com.google.protobuf.CodedInputStream;
import com.google.protobuf.DescriptorProtos.EnumDescriptorProto;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import org.jruby.Ruby;
import org.jruby.RubyClass;
import org.jruby.RubyModule;
import org.jruby.RubyNumeric;
import org.jruby.RubyObject;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.Block;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;

@JRubyClass(name = "EnumDescriptor", include = "Enumerable")
public class RubyEnumDescriptor extends RubyObject {
  public static void createRubyEnumDescriptor(Ruby runtime) {
    RubyModule mProtobuf = runtime.getClassFromPath("Google::Protobuf");
    RubyClass cEnumDescriptor =
        mProtobuf.defineClassUnder(
            "EnumDescriptor",
            runtime.getObject(),
            new ObjectAllocator() {
              @Override
              public IRubyObject allocate(Ruby runtime, RubyClass klazz) {
                return new RubyEnumDescriptor(runtime, klazz);
              }
            });
    cEnumDescriptor.includeModule(runtime.getEnumerable());
    cEnumDescriptor.defineAnnotatedMethods(RubyEnumDescriptor.class);
  }

  public RubyEnumDescriptor(Ruby runtime, RubyClass klazz) {
    super(runtime, klazz);
  }

  /*
   * call-seq:
   *     EnumDescriptor.name => name
   *
   * Returns the name of this enum type.
   */
  @JRubyMethod(name = "name")
  public IRubyObject getName(ThreadContext context) {
    return this.name;
  }

  /*
   * call-seq:
   *     EnumDescriptor.each(&block)
   *
   * Iterates over key => value mappings in this enum's definition, yielding to
   * the block with (key, value) arguments for each one.
   */
  @JRubyMethod
  public IRubyObject each(ThreadContext context, Block block) {
    Ruby runtime = context.runtime;
    for (EnumValueDescriptor enumValueDescriptor : descriptor.getValues()) {
      block.yield(
          context,
          runtime.newArray(
              runtime.newSymbol(enumValueDescriptor.getName()),
              runtime.newFixnum(enumValueDescriptor.getNumber())));
    }
    return context.nil;
  }

  /*
   * call-seq:
   *     EnumDescriptor.enummodule => module
   *
   * Returns the Ruby module corresponding to this enum type. Cannot be called
   * until the enum descriptor has been added to a pool.
   */
  @JRubyMethod
  public IRubyObject enummodule(ThreadContext context) {
    return module;
  }

  /*
   * call-seq:
   *    EnumDescriptor.file_descriptor
   *
   * Returns the FileDescriptor object this enum belongs to.
   */
  @JRubyMethod(name = "file_descriptor")
  public IRubyObject getFileDescriptor(ThreadContext context) {
    return RubyFileDescriptor.getRubyFileDescriptor(context, descriptor);
  }

  @JRubyMethod
  public IRubyObject options(ThreadContext context) {
    RubyDescriptorPool pool = (RubyDescriptorPool) RubyDescriptorPool.generatedPool(null, null);
    RubyDescriptor enumOptionsDescriptor =
        (RubyDescriptor)
            pool.lookup(context, context.runtime.newString("google.protobuf.EnumOptions"));
    RubyClass enumOptionsClass = (RubyClass) enumOptionsDescriptor.msgclass(context);
    RubyMessage msg = (RubyMessage) enumOptionsClass.newInstance(context, Block.NULL_BLOCK);
    return msg.decodeBytes(
        context,
        msg,
        CodedInputStream.newInstance(
            descriptor.getOptions().toByteString().toByteArray()), /*freeze*/
        true);
  }

  /*
   * call-seq:
   *     EnumDescriptor.to_proto => EnumDescriptorProto
   *
   * Returns the `EnumDescriptorProto` of this `EnumDescriptor`.
   */
  @JRubyMethod(name = "to_proto")
  public IRubyObject toProto(ThreadContext context) {
    RubyDescriptorPool pool = (RubyDescriptorPool) RubyDescriptorPool.generatedPool(null, null);
    RubyDescriptor enumDescriptorProto =
        (RubyDescriptor)
            pool.lookup(context, context.runtime.newString("google.protobuf.EnumDescriptorProto"));
    RubyClass msgClass = (RubyClass) enumDescriptorProto.msgclass(context);
    RubyMessage msg = (RubyMessage) msgClass.newInstance(context, Block.NULL_BLOCK);
    return msg.decodeBytes(
        context,
        msg,
        CodedInputStream.newInstance(descriptor.toProto().toByteString().toByteArray()), /*freeze*/
        true);
  }

  public boolean isValidValue(ThreadContext context, IRubyObject value) {
    EnumValueDescriptor enumValue;

    if (Utils.isRubyNum(value)) {
      enumValue = descriptor.findValueByNumberCreatingIfUnknown(RubyNumeric.num2int(value));
    } else {
      enumValue = descriptor.findValueByName(value.asJavaString());
    }

    return enumValue != null;
  }

  protected IRubyObject nameToNumber(ThreadContext context, IRubyObject name) {
    EnumValueDescriptor value = descriptor.findValueByName(name.asJavaString());
    return value == null ? context.nil : context.runtime.newFixnum(value.getNumber());
  }

  protected IRubyObject numberToName(ThreadContext context, IRubyObject number) {
    EnumValueDescriptor value = descriptor.findValueByNumber(RubyNumeric.num2int(number));
    return value == null ? context.nil : context.runtime.newSymbol(value.getName());
  }

  protected void setDescriptor(ThreadContext context, EnumDescriptor descriptor) {
    this.descriptor = descriptor;
    this.module = buildModuleFromDescriptor(context);
  }

  protected void setName(IRubyObject name) {
    this.name = name;
  }

  private RubyModule buildModuleFromDescriptor(ThreadContext context) {
    Ruby runtime = context.runtime;

    RubyModule enumModule = RubyModule.newModule(runtime);
    boolean defaultValueRequiredButNotFound = !descriptor.isClosed();
    for (EnumValueDescriptor value : descriptor.getValues()) {
      String name = fixEnumConstantName(value.getName());
      // Make sure it's a valid constant name before trying to create it
      int ch = name.codePointAt(0);
      if (Character.isUpperCase(ch)) {
        enumModule.defineConstant(name, runtime.newFixnum(value.getNumber()));
      } else {
        runtime
            .getWarnings()
            .warn(
                "Enum value "
                    + name
                    + " does not start with an uppercase letter as is required for Ruby"
                    + " constants.");
      }
      if (value.getNumber() == 0) {
        defaultValueRequiredButNotFound = false;
      }
    }

    if (defaultValueRequiredButNotFound) {
      throw Utils.createTypeError(
          context, "Enum definition " + name + " does not contain a value for '0'");
    }
    enumModule.instance_variable_set(runtime.newString(Utils.DESCRIPTOR_INSTANCE_VAR), this);
    enumModule.defineAnnotatedMethods(RubyEnum.class);
    return enumModule;
  }

  private static String fixEnumConstantName(String name) {
    if (name != null && name.length() > 0) {
      int ch = name.codePointAt(0);
      if (ch >= 'a' && ch <= 'z') {
        // Protobuf enums can start with lowercase letters, while Ruby's constant should
        // always start with uppercase letters. We tolerate this case by capitalizing
        // the first character if possible.
        return new StringBuilder()
            .appendCodePoint(Character.toUpperCase(ch))
            .append(name.substring(1))
            .toString();
      }
    }
    return name;
  }

  private EnumDescriptor descriptor;
  private EnumDescriptorProto.Builder builder;
  private IRubyObject name;
  private RubyModule module;
}
