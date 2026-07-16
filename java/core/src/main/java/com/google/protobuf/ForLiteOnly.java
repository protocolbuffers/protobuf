// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Indicates that the annotated type is intended for the Protobuf Java Lite runtime only. If you are
 * on a server you should not use these types, and you should avoid using the Lite gencode which
 * uses these types.
 *
 * <p>The Lite runtime is designed specifically for mobile clients (especially Android) and makes
 * different trade-offs compared to the Full runtime.
 *
 * <p><b>Important Constraints & Trade-offs:</b>
 *
 * <ul>
 *   <li><b>Client-Only:</b> It should not be used on the server-side. The tradeoffs it makes are
 *       not suited for server environments.
 *   <li><b>Code Size Optimization:</b> It is optimized for small binary size and lower peak memory
 *       usage, potentially at the cost of slower runtime performance.
 *   <li><b>DoS Security Hardening:</b> It may be less hardened against certain categories of DoS
 *       security issues. These issues are often of high importance on servers but of much less
 *       concern on mobile clients.
 *   <li><b>No API/ABI Stability:</b> API/ABI stability is not guaranteed for Java Lite.
 *   <li><b>Unsafe Dependency:</b> It relies on {@code sun.misc.Unsafe} for performance. It cannot
 *       be used in environments where {@code sun.misc.Unsafe} is unavailable.
 * </ul>
 *
 * <p>For more detailed information, see <a
 * href="https://github.com/protocolbuffers/protobuf/blob/main/java/lite.md">https://github.com/protocolbuffers/protobuf/blob/main/java/lite.md</a>.
 */
@Retention(RetentionPolicy.SOURCE)
@Target(ElementType.TYPE)
public @interface ForLiteOnly {}
