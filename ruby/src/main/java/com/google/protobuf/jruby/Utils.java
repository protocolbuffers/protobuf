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
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.Descriptors.FieldDescriptor;
import org.jcodings.Encoding;
import org.jcodings.specific.ASCIIEncoding;
import org.jruby.*;
import org.jruby.exceptions.RaiseException;
import org.jruby.ext.bigdecimal.RubyBigDecimal;
import org.jruby.runtime.Block;
import org.jruby.runtime.Helpers;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;

import java.math.BigInteger;

public class Utils {
    public static FieldDescriptor.Type rubyToFieldType(IRubyObject typeClass) {
        return FieldDescriptor.Type.valueOf(typeClass.asJavaString().toUpperCase());
    }

    public static IRubyObject fieldTypeToRuby(ThreadContext context, FieldDescriptor.Type type) {
        return fieldTypeToRuby(context, type.name());
    }

    public static IRubyObject fieldTypeToRuby(ThreadContext context, FieldDescriptorProto.Type type) {
        return fieldTypeToRuby(context, type.name());
    }

    private static IRubyObject fieldTypeToRuby(ThreadContext context, String typeName) {

        return context.runtime.newSymbol(typeName.replace("TYPE_", "").toLowerCase());
    }

    public static IRubyObject checkType(ThreadContext context, FieldDescriptor.Type fieldType,
                                        String fieldName, IRubyObject value, RubyModule typeClass) {
        Ruby runtime = context.runtime;

        switch(fieldType) {
            case INT32:
            case INT64:
            case UINT32:
            case UINT64:
                if (!isRubyNum(value))
                    throw createExpectedTypeError(context, "number", "integral", fieldName, value);

                if (value instanceof RubyFloat) {
                    double doubleVal = RubyNumeric.num2dbl(value);
                    if (Math.floor(doubleVal) != doubleVal) {
                        throw runtime.newRangeError("Non-integral floating point value assigned to integer field '" + fieldName + "' (given " + value.getMetaClass() + ").");
                    }
                }
                if (fieldType == FieldDescriptor.Type.UINT32 || fieldType == FieldDescriptor.Type.UINT64) {
                    if (((RubyNumeric) value).isNegative()) {
                        throw runtime.newRangeError("Assigning negative value to unsigned integer field '" + fieldName + "' (given " + value.getMetaClass() + ").");
                    }
                }

                switch(fieldType) {
                    case INT32:
                        RubyNumeric.num2int(value);
                        break;
                    case UINT32:
                        num2uint(value);
                        break;
                    case UINT64:
                        num2ulong(context.runtime, value);
                        break;
                    default:
                        RubyNumeric.num2long(value);
                        break;
                }
                break;
            case FLOAT:
                if (!isRubyNum(value))
                    throw createExpectedTypeError(context, "number", "float", fieldName, value);
                break;
            case DOUBLE:
                if (!isRubyNum(value))
                    throw createExpectedTypeError(context, "number", "double", fieldName, value);
                break;
            case BOOL:
                if (!(value instanceof RubyBoolean))
                    throw createInvalidTypeError(context, "boolean", fieldName, value);
                break;
            case BYTES:
                value = validateAndEncodeString(context, "bytes", fieldName, value, "Encoding::ASCII_8BIT");
                break;
            case STRING:
                value = validateAndEncodeString(context, "string", fieldName, symToString(value), "Encoding::UTF_8");
                break;
            case MESSAGE:
                if (value.getMetaClass() != typeClass) {
                    // See if we can convert the value before flagging it as invalid
                    String className = typeClass.getName();

                    if (className.equals("Google::Protobuf::Timestamp") && value instanceof RubyTime) {
                        RubyTime rt = (RubyTime) value;
                        RubyHash timestampArgs =
                            Helpers.constructHash(runtime,
                                runtime.newString("nanos"), rt.nsec(), false,
                                runtime.newString("seconds"), rt.to_i(), false);
                        return ((RubyClass) typeClass).newInstance(context, timestampArgs, Block.NULL_BLOCK);

                    } else if (className.equals("Google::Protobuf::Duration") && value instanceof RubyNumeric) {
                        IRubyObject seconds;
                        if (value instanceof RubyFloat) {
                            seconds = ((RubyFloat) value).truncate(context);
                        } else if (value instanceof RubyRational) {
                            seconds = ((RubyRational) value).to_i(context);
                        } else if (value instanceof RubyBigDecimal) {
                            seconds = ((RubyBigDecimal) value).to_int(context);
                        } else {
                            seconds = ((RubyInteger) value).to_i();
                        }

                        IRubyObject nanos = ((RubyNumeric) value).remainder(context, RubyFixnum.one(runtime));
                        if (nanos instanceof RubyFloat) {
                            nanos = ((RubyFloat) nanos).op_mul(context, 1000000000);
                        } else if (nanos instanceof RubyRational) {
                            nanos = ((RubyRational) nanos).op_mul(context, runtime.newFixnum(1000000000));
                        } else if (nanos instanceof RubyBigDecimal) {
                            nanos = ((RubyBigDecimal) nanos).op_mul(context, runtime.newFixnum(1000000000));
                        } else {
                            nanos = ((RubyInteger) nanos).op_mul(context, 1000000000);
                        }

                        RubyHash durationArgs =
                            Helpers.constructHash(runtime,
                                runtime.newString("nanos"), ((RubyNumeric) nanos).round(context), false,
                                runtime.newString("seconds"), seconds, false);
                        return ((RubyClass) typeClass).newInstance(context, durationArgs, Block.NULL_BLOCK);
                    }

                    // Not able to convert so flag as invalid
                    throw createTypeError(context, "Invalid type " + value.getMetaClass() + " to assign to submessage field '" + fieldName + "'.");
                }

                break;
            case ENUM:
                boolean isValid = ((RubyEnumDescriptor) typeClass.getInstanceVariable(DESCRIPTOR_INSTANCE_VAR)).isValidValue(context, value);
                if (!isValid) {
                    throw runtime.newRangeError("Unknown symbol value for enum field '" + fieldName + "'.");
                }
                break;
            default:
                break;
        }
        return value;
    }

    public static IRubyObject wrapPrimaryValue(ThreadContext context, FieldDescriptor.Type fieldType, Object value) {
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
                IRubyObject wrapped = RubyString.newString(runtime, ((ByteString) value).toStringUtf8(), ASCIIEncoding.INSTANCE);
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

    /*
     * Helper to make it easier to support symbols being passed instead of strings
     */
    public static IRubyObject symToString(IRubyObject sym) {
        if (sym instanceof RubySymbol) {
            return ((RubySymbol) sym).id2name();
        }
        return sym;
    }

    public static void checkNameAvailability(ThreadContext context, String name) {
        if (context.runtime.getObject().getConstantAt(name) != null)
            throw context.runtime.newNameError(name + " is already defined", name);
    }

    public static boolean isMapEntry(FieldDescriptor fieldDescriptor) {
        return fieldDescriptor.getType() == FieldDescriptor.Type.MESSAGE &&
                fieldDescriptor.isRepeated() &&
                fieldDescriptor.getMessageType().getOptions().getMapEntry();
    }

    /*
     * call-seq:
     *     Utils.createFieldBuilder(context, label, name, type, number, typeClass = nil, options = nil)
     *
     * Most places calling this are already dealing with an optional number of
     * arguments so dealing with them here. This helper is a standard way to
     * create a FieldDescriptor builder that handles some of the options that
     * are used in different places.
     */
    public static FieldDescriptorProto.Builder createFieldBuilder(ThreadContext context,
            String label, IRubyObject[] args) {

        Ruby runtime = context.runtime;
        IRubyObject options = context.nil;
        IRubyObject typeClass = context.nil;

        if (args.length > 4) {
            options = args[4];
            typeClass = args[3];
        } else if (args.length > 3) {
            if (args[3] instanceof RubyHash) {
                options = args[3];
            } else {
                typeClass = args[3];
            }
        }

        FieldDescriptorProto.Builder builder = FieldDescriptorProto.newBuilder();

        builder.setLabel(FieldDescriptorProto.Label.valueOf("LABEL_" + label.toUpperCase()))
            .setName(args[0].asJavaString())
            .setNumber(RubyNumeric.num2int(args[2]))
            .setType(FieldDescriptorProto.Type.valueOf("TYPE_" + args[1].asJavaString().toUpperCase()));

        if (!typeClass.isNil()) {
            if (!(typeClass instanceof RubyString)) {
                throw runtime.newArgumentError("expected string for type class");
            }
            builder.setTypeName("." + typeClass.asJavaString());
        }

        if (options instanceof RubyHash) {
            IRubyObject defaultValue = ((RubyHash) options).fastARef(runtime.newSymbol("default"));
            if (defaultValue != null) {
                builder.setDefaultValue(defaultValue.toString());
            }
        }

        return builder;
    }


    public static RaiseException createTypeError(ThreadContext context, String message) {
        if (cTypeError == null) {
            cTypeError = (RubyClass) context.runtime.getClassFromPath("Google::Protobuf::TypeError");
        }
        return RaiseException.from(context.runtime, cTypeError, message);
    }

    public static RaiseException createExpectedTypeError(ThreadContext context, String type, String fieldType, String fieldName, IRubyObject value) {
        return createTypeError(context, String.format(EXPECTED_TYPE_ERROR_FORMAT, type, fieldType, fieldName, value.getMetaClass()));
    }

    public static RaiseException createInvalidTypeError(ThreadContext context, String fieldType, String fieldName, IRubyObject value) {
        return createTypeError(context, String.format(INVALID_TYPE_ERROR_FORMAT, fieldType, fieldName, value.getMetaClass()));
    }

    protected static boolean isRubyNum(Object value) {
        return value instanceof RubyFixnum || value instanceof RubyFloat || value instanceof RubyBignum;
    }

    protected static void validateTypeClass(ThreadContext context, FieldDescriptor.Type type, IRubyObject value) {
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
        if (type == FieldDescriptor.Type.MESSAGE) {
            if (! (descriptor instanceof RubyDescriptor)) {
                throw runtime.newArgumentError("Descriptor has an incorrect type");
            }
        } else if (type == FieldDescriptor.Type.ENUM) {
            if (! (descriptor instanceof RubyEnumDescriptor)) {
                throw runtime.newArgumentError("Descriptor has an incorrect type");
            }
        }
    }

    private static IRubyObject validateAndEncodeString(ThreadContext context, String fieldType, String fieldName, IRubyObject value, String encoding) {
        if (!(value instanceof RubyString))
            throw createInvalidTypeError(context, fieldType, fieldName, value);

        value = ((RubyString) value).encode(context, context.runtime.evalScriptlet(encoding));
        value.setFrozen(true);
        return value;
    }

    public static final String DESCRIPTOR_INSTANCE_VAR = "@descriptor";

    public static final String EQUAL_SIGN = "=";

    private static final BigInteger UINT64_COMPLEMENTARY = new BigInteger("18446744073709551616"); //Math.pow(2, 64)

    private static final String EXPECTED_TYPE_ERROR_FORMAT = "Expected %s type for %s field '%s' (given %s).";
    private static final String INVALID_TYPE_ERROR_FORMAT = "Invalid argument for %s field '%s' (given %s).";

    private static final long UINT_MAX = 0xffffffffl;

    private static RubyClass cTypeError;
}
