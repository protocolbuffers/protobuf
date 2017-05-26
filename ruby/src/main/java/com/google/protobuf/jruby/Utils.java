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

import com.google.protobuf.ByteString;
import com.google.protobuf.DescriptorProtos;
import com.google.protobuf.Descriptors;
import org.jcodings.Encoding;
import org.jcodings.specific.ASCIIEncoding;
import org.jcodings.specific.USASCIIEncoding;
import org.jcodings.specific.UTF8Encoding;
import org.jruby.*;
import org.jruby.runtime.Block;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;

import java.math.BigInteger;

public class Utils {
    public static Descriptors.FieldDescriptor.Type rubyToFieldType(IRubyObject typeClass) {
        return Descriptors.FieldDescriptor.Type.valueOf(typeClass.asJavaString().toUpperCase());
    }

    public static IRubyObject fieldTypeToRuby(ThreadContext context, Descriptors.FieldDescriptor.Type type) {
        return fieldTypeToRuby(context, type.name());
    }

    public static IRubyObject fieldTypeToRuby(ThreadContext context, DescriptorProtos.FieldDescriptorProto.Type type) {
        return fieldTypeToRuby(context, type.name());
    }

    private static IRubyObject fieldTypeToRuby(ThreadContext context, String typeName) {

        return context.runtime.newSymbol(typeName.replace("TYPE_", "").toLowerCase());
    }

    public static IRubyObject checkType(ThreadContext context, Descriptors.FieldDescriptor.Type fieldType,
                                        IRubyObject value, RubyModule typeClass) {
        Ruby runtime = context.runtime;
        Object val;
        switch(fieldType) {
            case INT32:
            case INT64:
            case UINT32:
            case UINT64:
                if (!isRubyNum(value)) {
                    throw runtime.newTypeError("Expected number type for integral field.");
                }
                switch(fieldType) {
                    case INT32:
                        RubyNumeric.num2int(value);
                        break;
                    case INT64:
                        RubyNumeric.num2long(value);
                        break;
                    case UINT32:
                        num2uint(value);
                        break;
                    default:
                        num2ulong(context.runtime, value);
                        break;
                }
                checkIntTypePrecision(context, fieldType, value);
                break;
            case FLOAT:
                if (!isRubyNum(value))
                    throw runtime.newTypeError("Expected number type for float field.");
                break;
            case DOUBLE:
                if (!isRubyNum(value))
                    throw runtime.newTypeError("Expected number type for double field.");
                break;
            case BOOL:
                if (!(value instanceof RubyBoolean))
                    throw runtime.newTypeError("Invalid argument for boolean field.");
                break;
            case BYTES:
            case STRING:
                value = validateStringEncoding(context, fieldType, value);
                break;
            case MESSAGE:
                if (value.getMetaClass() != typeClass) {
                    throw runtime.newTypeError(value, typeClass);
                }
                break;
            case ENUM:
                if (value instanceof RubySymbol) {
                    Descriptors.EnumDescriptor enumDescriptor =
                            ((RubyEnumDescriptor) typeClass.getInstanceVariable(DESCRIPTOR_INSTANCE_VAR)).getDescriptor();
                    val = enumDescriptor.findValueByName(value.asJavaString());
                    if (val == null)
                        throw runtime.newRangeError("Enum value " + value + " is not found.");
                } else if(!isRubyNum(value)) {
                    throw runtime.newTypeError("Expected number or symbol type for enum field.");
                }
                break;
            default:
                break;
        }
        return value;
    }

    public static IRubyObject wrapPrimaryValue(ThreadContext context, Descriptors.FieldDescriptor.Type fieldType, Object value) {
        Ruby runtime = context.runtime;
        switch (fieldType) {
            case INT32:
                return runtime.newFixnum((Integer) value);
            case INT64:
                return runtime.newFixnum((Long) value);
            case UINT32:
                return runtime.newFixnum(((Integer) value) & (-1l >>> 32));
            case UINT64:
                long ret = (Long) value;
                return ret >= 0 ? runtime.newFixnum(ret) :
                        RubyBignum.newBignum(runtime, UINT64_COMPLEMENTARY.add(new BigInteger(ret + "")));
            case FLOAT:
                return runtime.newFloat((Float) value);
            case DOUBLE:
                return runtime.newFloat((Double) value);
            case BOOL:
                return (Boolean) value ? runtime.getTrue() : runtime.getFalse();
            case BYTES: {
                IRubyObject wrapped = runtime.newString(((ByteString) value).toStringUtf8());
                wrapped.setFrozen(true);
                return wrapped;
            }
            case STRING: {
                IRubyObject wrapped = runtime.newString(value.toString());
                wrapped.setFrozen(true);
                return wrapped;
            }
            default:
                return runtime.getNil();
        }
    }

    public static int num2uint(IRubyObject value) {
        long longVal = RubyNumeric.num2long(value);
        if (longVal > UINT_MAX)
            throw value.getRuntime().newRangeError("Integer " + longVal + " too big to convert to 'unsigned int'");
        long num = longVal;
        if (num > Integer.MAX_VALUE || num < Integer.MIN_VALUE)
            // encode to UINT32
            num = (-longVal ^ (-1l >>> 32) ) + 1;
        RubyNumeric.checkInt(value, num);
        return (int) num;
    }

    public static long num2ulong(Ruby runtime, IRubyObject value) {
        if (value instanceof RubyFloat) {
            RubyBignum bignum = RubyBignum.newBignum(runtime, ((RubyFloat) value).getDoubleValue());
            return RubyBignum.big2ulong(bignum);
        } else if (value instanceof RubyBignum) {
            return RubyBignum.big2ulong((RubyBignum) value);
        } else {
            return RubyNumeric.num2long(value);
        }
    }

    public static IRubyObject validateStringEncoding(ThreadContext context, Descriptors.FieldDescriptor.Type type, IRubyObject value) {
        if (!(value instanceof RubyString))
            throw context.runtime.newTypeError("Invalid argument for string field.");
        switch(type) {
            case BYTES:
                value = ((RubyString)value).encode(context, context.runtime.evalScriptlet("Encoding::ASCII_8BIT"));
                break;
            case STRING:
                value = ((RubyString)value).encode(context, context.runtime.evalScriptlet("Encoding::UTF_8"));
                break;
            default:
                break;
        }
        value.setFrozen(true);
        return value;
    }

    public static void checkNameAvailability(ThreadContext context, String name) {
        if (context.runtime.getObject().getConstantAt(name) != null)
            throw context.runtime.newNameError(name + " is already defined", name);
    }

    /**
     * Replace invalid "." in descriptor with __DOT__
     * @param name
     * @return
     */
    public static String escapeIdentifier(String name) {
        return name.replace(".", BADNAME_REPLACEMENT);
    }

    /**
     * Replace __DOT__ in descriptor name with "."
     * @param name
     * @return
     */
    public static String unescapeIdentifier(String name) {
        return name.replace(BADNAME_REPLACEMENT, ".");
    }

    public static boolean isMapEntry(Descriptors.FieldDescriptor fieldDescriptor) {
        return fieldDescriptor.getType() == Descriptors.FieldDescriptor.Type.MESSAGE &&
                fieldDescriptor.isRepeated() &&
                fieldDescriptor.getMessageType().getOptions().getMapEntry();
    }

    public static RubyFieldDescriptor msgdefCreateField(ThreadContext context, String label, IRubyObject name,
                                      IRubyObject type, IRubyObject number, IRubyObject typeClass, RubyClass cFieldDescriptor) {
        Ruby runtime = context.runtime;
        RubyFieldDescriptor fieldDef = (RubyFieldDescriptor) cFieldDescriptor.newInstance(context, Block.NULL_BLOCK);
        fieldDef.setLabel(context, runtime.newString(label));
        fieldDef.setName(context, name);
        fieldDef.setType(context, type);
        fieldDef.setNumber(context, number);

        if (!typeClass.isNil()) {
            if (!(typeClass instanceof RubyString)) {
                throw runtime.newArgumentError("expected string for type class");
            }
            fieldDef.setSubmsgName(context, typeClass);
        }
        return fieldDef;
    }

    protected static void checkIntTypePrecision(ThreadContext context, Descriptors.FieldDescriptor.Type type, IRubyObject value) {
        if (value instanceof RubyFloat) {
            double doubleVal = RubyNumeric.num2dbl(value);
            if (Math.floor(doubleVal) != doubleVal) {
                throw context.runtime.newRangeError("Non-integral floating point value assigned to integer field.");
            }
        }
        if (type == Descriptors.FieldDescriptor.Type.UINT32 || type == Descriptors.FieldDescriptor.Type.UINT64) {
            if (RubyNumeric.num2dbl(value) < 0) {
                throw context.runtime.newRangeError("Assigning negative value to unsigned integer field.");
            }
        }
    }

    protected static boolean isRubyNum(Object value) {
        return value instanceof RubyFixnum || value instanceof RubyFloat || value instanceof RubyBignum;
    }

    protected static void validateTypeClass(ThreadContext context, Descriptors.FieldDescriptor.Type type, IRubyObject value) {
        Ruby runtime = context.runtime;
        if (!(value instanceof RubyModule)) {
            throw runtime.newArgumentError("TypeClass has incorrect type");
        }
        RubyModule klass = (RubyModule) value;
        IRubyObject descriptor = klass.getInstanceVariable(DESCRIPTOR_INSTANCE_VAR);
        if (descriptor.isNil()) {
            throw runtime.newArgumentError("Type class has no descriptor. Please pass a " +
                    "class or enum as returned by the DescriptorPool.");
        }
        if (type == Descriptors.FieldDescriptor.Type.MESSAGE) {
            if (! (descriptor instanceof RubyDescriptor)) {
                throw runtime.newArgumentError("Descriptor has an incorrect type");
            }
        } else if (type == Descriptors.FieldDescriptor.Type.ENUM) {
            if (! (descriptor instanceof RubyEnumDescriptor)) {
                throw runtime.newArgumentError("Descriptor has an incorrect type");
            }
        }
    }

    public static String BADNAME_REPLACEMENT = "__DOT__";

    public static String DESCRIPTOR_INSTANCE_VAR = "@descriptor";

    public static String EQUAL_SIGN = "=";

    private static BigInteger UINT64_COMPLEMENTARY = new BigInteger("18446744073709551616"); //Math.pow(2, 64)

    private static long UINT_MAX = 0xffffffffl;
}
