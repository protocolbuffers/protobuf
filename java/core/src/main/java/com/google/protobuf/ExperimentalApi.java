package com.google.protobuf;

import java.lang.annotation.Documented;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Indicates a public API that can change at any time, and has no guarantee of API stability and
 * backward-compatibility.
 *
 * <p>Usage guidelines:
 * <ol>
 * <li>This annotation is used only on public API. Internal interfaces should not use it.</li>
 * <li>This annotation should only be added to new APIs. Adding it to an existing API is
 * considered API-breaking.</li>
 * <li>Removing this annotation from an API gives it stable status.</li>
 * </ol>
 */
@Retention(RetentionPolicy.SOURCE)
@Target({
    ElementType.ANNOTATION_TYPE,
    ElementType.CONSTRUCTOR,
    ElementType.FIELD,
    ElementType.METHOD,
    ElementType.PACKAGE,
    ElementType.TYPE})
@Documented
public @interface ExperimentalApi {
  /**
   * Context information such as links to discussion thread, tracking issue etc.
   */
  String value() default "";
}

