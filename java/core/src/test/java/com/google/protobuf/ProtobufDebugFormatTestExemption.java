package com.google.protobuf;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * An annotation that, combined with {@link ProtobufDebugFormatTestExemptionRule}, can make Protobuf
 * Message.toString() to always output the Protobuf text format in select libraries for a test
 * method.
 *
 * <p>Historically, Message.toString() outputs the Protobuf text format, a human-readable
 * serializable format. This format includes all the fields in the message, which could lead to
 * privacy incidents where sensitive fields are accidentally logged. A new debug format was
 * introduced to mitigate this problem. Compared to the text format, the debug format redacts the
 * sensitive fields, and contains a randomized prefix, which makes it unserializable. We are in the
 * process of adopting the debug format as the output format for Message.toString() in libraries.
 * This rule only works for libraries in which Message.toString() output is unlikely to be
 * seriailized.
 */
@Retention(RetentionPolicy.RUNTIME)
@Target({ElementType.METHOD})
public @interface ProtobufDebugFormatTestExemption {}
