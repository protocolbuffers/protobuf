package com.google.protobuf.kotlin

/**
 * Indicates an API that is part of a DSL to generate protocol buffer messages.
 */
@DslMarker
@Target(AnnotationTarget.CLASS)
@Retention(AnnotationRetention.BINARY)
@OnlyForUseByGeneratedProtoCode
annotation class ProtoDslMarker
