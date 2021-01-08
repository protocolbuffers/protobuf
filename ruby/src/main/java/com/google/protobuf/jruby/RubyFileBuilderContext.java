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

import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumDescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumValueDescriptorProtoOrBuilder;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.Descriptors.FieldDescriptor;
import org.jruby.Ruby;
import org.jruby.RubyClass;
import org.jruby.RubyHash;
import org.jruby.RubyModule;
import org.jruby.RubyObject;
import org.jruby.anno.JRubyClass;
import org.jruby.anno.JRubyMethod;
import org.jruby.runtime.Block;
import org.jruby.runtime.ObjectAllocator;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;

import java.util.HashMap;
import java.util.List;
import java.util.TreeMap;

@JRubyClass(name = "FileBuilderContext")
public class RubyFileBuilderContext extends RubyObject {
    public static void createRubyFileBuilderContext(Ruby runtime) {
        RubyModule internal = runtime.getClassFromPath("Google::Protobuf::Internal");
        RubyClass cFileBuilderContext = internal.defineClassUnder("FileBuilderContext", runtime.getObject(), new ObjectAllocator() {
            @Override
            public IRubyObject allocate(Ruby runtime, RubyClass klazz) {
                return new RubyFileBuilderContext(runtime, klazz);
            }
        });
        cFileBuilderContext.defineAnnotatedMethods(RubyFileBuilderContext.class);

        cDescriptor = (RubyClass) runtime.getClassFromPath("Google::Protobuf::Descriptor");
        cEnumBuilderContext = (RubyClass) runtime.getClassFromPath("Google::Protobuf::Internal::EnumBuilderContext");
        cEnumDescriptor = (RubyClass) runtime.getClassFromPath("Google::Protobuf::EnumDescriptor");
        cMessageBuilderContext = (RubyClass) runtime.getClassFromPath("Google::Protobuf::Internal::MessageBuilderContext");
    }

    public RubyFileBuilderContext(Ruby runtime, RubyClass klazz) {
        super(runtime, klazz);
    }

    /*
     * call-seq:
     *     FileBuilderContext.new(descriptor_pool, name, options = nil) => context
     *
     * Create a new file builder context for the given file descriptor and
     * builder context. This class is intended to serve as a DSL context to be used
     * with #instance_eval.
     */
    @JRubyMethod(required = 2, optional = 1)
    public IRubyObject initialize(ThreadContext context, IRubyObject[] args) {
        this.descriptorPool = (RubyDescriptorPool) args[0];
        this.builder = FileDescriptorProto.newBuilder();
        this.builder.setName(args[1].asJavaString());
        this.builder.setSyntax("proto3");

        if (args.length > 2) {
            RubyHash options = (RubyHash) args[2];
            IRubyObject syntax = options.fastARef(context.runtime.newSymbol("syntax"));

            if (syntax != null) {
                String syntaxStr = syntax.asJavaString();
                this.builder.setSyntax(syntaxStr);
                this.proto3 = syntaxStr.equals("proto3");
            }
        }

        return this;
    }

    /*
     * call-seq:
     *     FileBuilderContext.add_enum(name, &block)
     *
     * Creates a new, empty enum descriptor with the given name, and invokes the
     * block in the context of an EnumBuilderContext on that descriptor. The block
     * can then call EnumBuilderContext#add_value to define the enum values.
     *
     * This is the recommended, idiomatic way to build enum definitions.
     */
    @JRubyMethod(name = "add_enum")
    public IRubyObject addEnum(ThreadContext context, IRubyObject name, Block block) {
        RubyObject ctx = (RubyObject) cEnumBuilderContext.newInstance(context, this, name, Block.NULL_BLOCK);
        ctx.instance_eval(context, block);

        return context.nil;
    }

    /*
     * call-seq:
     *     FileBuilderContext.add_message(name, &block)
     *
     * Creates a new, empty descriptor with the given name, and invokes the block in
     * the context of a MessageBuilderContext on that descriptor. The block can then
     * call, e.g., MessageBuilderContext#optional and MessageBuilderContext#repeated
     * methods to define the message fields.
     *
     * This is the recommended, idiomatic way to build message definitions.
     */
    @JRubyMethod(name = "add_message")
    public IRubyObject addMessage(ThreadContext context, IRubyObject name, Block block) {
        RubyObject ctx = (RubyObject) cMessageBuilderContext.newInstance(context, this, name, Block.NULL_BLOCK);
        ctx.instance_eval(context, block);

        return context.nil;
    }

    protected void build(ThreadContext context) {
        Ruby runtime = context.runtime;
        List<DescriptorProto.Builder> messageBuilderList = builder.getMessageTypeBuilderList();
        List<EnumDescriptorProto.Builder> enumBuilderList = builder.getEnumTypeBuilderList();

        // Get the package name from defined names
        String packageName = getPackageName(messageBuilderList, enumBuilderList);

        if (!packageName.isEmpty()) {
            builder.setPackage(packageName);
        }

        // Make an index of the message builders so we can easily nest them
        TreeMap<String, DescriptorProto.Builder> messageBuilderMap = new TreeMap();
        for (DescriptorProto.Builder messageBuilder : messageBuilderList) {
            messageBuilderMap.put(messageBuilder.getName(), messageBuilder);
        }

        // Make an index of the enum builders so we can easily nest them
        HashMap<String, EnumDescriptorProto.Builder> enumBuilderMap = new HashMap();
        for (EnumDescriptorProto.Builder enumBuilder : enumBuilderList) {
            enumBuilderMap.put("." + enumBuilder.getName(), enumBuilder);
        }

        // Rename and properly nest messages and create associated ruby objects
        int packageNameLength = packageName.length();
        int currentMessageIndex = 0;
        int currentEnumIndex = 0;

        // Need to get a static list because we are potentially deleting some of them from the collection
        DescriptorProto.Builder[] messageBuilders = new DescriptorProto.Builder[messageBuilderList.size()];
        messageBuilderList.toArray(messageBuilders);
        EnumDescriptorProto.Builder[] enumBuilders = new EnumDescriptorProto.Builder[enumBuilderList.size()];
        enumBuilderList.toArray(enumBuilders);

        for (EnumDescriptorProto.Builder enumBuilder : enumBuilders) {
            String name = enumBuilder.getName();
            int lastDot = name.lastIndexOf('.');

            if (lastDot > packageNameLength) {
                String parentName = name.substring(0, lastDot);
                String shortName = name.substring(lastDot + 1);

                enumBuilder.setName(shortName);
                messageBuilderMap.get(parentName).addEnumType(enumBuilder);

                builder.removeEnumType(currentEnumIndex);

            } else {
                if (packageNameLength > 0) {
                    // Remove the package name
                    String shortName = name.substring(packageNameLength + 1);
                    enumBuilder.setName(shortName);
                }

                currentEnumIndex++;
            }

            // Ensure we have a default value if using proto3 syntax
            if (proto3) {
                boolean foundDefault = false;
                for (EnumValueDescriptorProtoOrBuilder enumValue : enumBuilder.getValueOrBuilderList()) {
                    if (enumValue.getNumber() == 0) {
                        foundDefault = true;
                        break;
                    }
                }

                if (!foundDefault) {
                    throw Utils.createTypeError(context, "Enum definition " + enumBuilder.getName() + " does not contain a value for '0'");
                }
            }
        }

        // Wipe out top level message builders so we can insert only the ones that should be there
        builder.clearMessageType();

        /*
         * This block is done in this order because calling
         * `addNestedType` and `addMessageType` makes a copy of the builder
         * so the objects that our maps point to are no longer the objects
         * that are being used to build the descriptions.
         */
        for (HashMap.Entry<String, DescriptorProto.Builder> entry : messageBuilderMap.descendingMap().entrySet()) {
            DescriptorProto.Builder messageBuilder = entry.getValue();

            // Rewrite any enum defaults needed
            for(FieldDescriptorProto.Builder field : messageBuilder.getFieldBuilderList()) {
                String typeName = field.getTypeName();

                if (typeName == null || !field.hasDefaultValue()) continue;

                EnumDescriptorProto.Builder enumBuilder = enumBuilderMap.get(typeName);

                if (enumBuilder == null) continue;

                int defaultValue = Integer.parseInt(field.getDefaultValue());

                for (EnumValueDescriptorProtoOrBuilder enumValue : enumBuilder.getValueOrBuilderList()) {
                    if (enumValue.getNumber() == defaultValue) {
                        field.setDefaultValue(enumValue.getName());
                        break;
                    }
                }
            }

            // Turn Foo.Bar.Baz into a correctly nested structure with the correct name
            String name = messageBuilder.getName();
            int lastDot = name.lastIndexOf('.');

            if (lastDot > packageNameLength) {
                String parentName = name.substring(0, lastDot);
                String shortName = name.substring(lastDot + 1);
                messageBuilder.setName(shortName);
                messageBuilderMap.get(parentName).addNestedType(messageBuilder);

            } else {
                if (packageNameLength > 0) {
                    // Remove the package name
                    messageBuilder.setName(name.substring(packageNameLength + 1));
                }

                // Add back in top level message definitions
                builder.addMessageType(messageBuilder);

                currentMessageIndex++;
            }
        }

        descriptorPool.registerFileDescriptor(context, builder);
    }

    protected EnumDescriptorProto.Builder getNewEnumBuilder() {
        return builder.addEnumTypeBuilder();
    }

    protected DescriptorProto.Builder getNewMessageBuilder() {
        return builder.addMessageTypeBuilder();
    }

    protected boolean isProto3() {
        return proto3;
    }

    private String getPackageName(List<DescriptorProto.Builder> messages, List<EnumDescriptorProto.Builder> enums) {
        String shortest = null;
        String longest = null;

        /*
         * The >= in the longest string comparisons below makes it so we replace
         * the name in case all the names are the same length. This makes it so
         * that the shortest and longest aren't the same name to prevent
         * finding a "package" that isn't correct
         */

        for (DescriptorProto.Builder message : messages) {
            String name = message.getName();
            int nameLength = name.length();
            if (shortest == null) {
                shortest = name;
                longest = name;
            } else if (nameLength < shortest.length()) {
                shortest = name;
            } else if (nameLength >= longest.length()) {
                longest = name;
            }
        }

        for (EnumDescriptorProto.Builder item : enums) {
            String name = item.getName();
            int nameLength = name.length();
            if (shortest == null) {
                shortest = name;
                longest = name;
            } else if (nameLength < shortest.length()) {
                shortest = name;
            } else if (nameLength >= longest.length()) {
                longest = name;
            }
        }

        if (shortest == null) {
            return "";
        }

        int lastCommonDot = 0;
        for (int i = 0; i < shortest.length(); i++) {
            char nextChar = shortest.charAt(i);
            if (nextChar != longest.charAt(i)) break;
            if (nextChar == '.') lastCommonDot = i;
        }

        return shortest.substring(0, lastCommonDot);
    }

    private static RubyClass cDescriptor;
    private static RubyClass cEnumBuilderContext;
    private static RubyClass cEnumDescriptor;
    private static RubyClass cMessageBuilderContext;

    private FileDescriptorProto.Builder builder;
    private RubyDescriptorPool descriptorPool;
    private boolean proto3 = true;
}
