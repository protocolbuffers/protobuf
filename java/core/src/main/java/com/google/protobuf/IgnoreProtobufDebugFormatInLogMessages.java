package com.google.protobuf;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * An annotation for integration test methods that tells the test backend to ignore the Protobuf
 * debug format prefix in log messages before matching those logs messages with the predefined
 * regexes. This applies to both Tin and Itest Lamprey test methods.
 *
 * Historically, Protobuf Message.toString() converts the proto message into the Protobuf Text
 * Format, which contains all the field names and values, including the sensitive fields like
 * passwords.
 */
@Retention(RetentionPolicy.RUNTIME)
@Target({ElementType.METHOD})
public @interface IgnoreProtobufDebugFormatInLogMessages {}
