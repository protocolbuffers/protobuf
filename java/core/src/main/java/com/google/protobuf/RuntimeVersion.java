// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.util.logging.Logger;

/**
 * Provides the version of this Protobuf Java runtime, and methods for Protobuf Java gencode to
 * validate that versions are compatible. Fields and methods in this class should be only accessed
 * by related unit tests and Protobuf Java gencode, and should not be used elsewhere.
 */
public class RuntimeVersion extends RuntimeVersionLite {

  private static final Logger logger = Logger.getLogger(RuntimeVersion.class.getName());

  public enum RuntimeDomain {
    GOOGLE_INTERNAL,
    PUBLIC,
  }

  public static final RuntimeDomain DOMAIN = RuntimeDomain.GOOGLE_INTERNAL;

  public static void validateProtobufGencodeVersion(
      RuntimeVersionLite.RuntimeDomain domain,
      int major,
      int minor,
      int patch,
      String suffix,
      String location) {
    if (checkDisabled()) {
      return;
    }
    synchronized (RuntimeVersionLite.class) {
      if (prevFullness == Fullness.LITE) {
        // TODO: b/298200443 - Turn the warning into runtime exception.
        logger.warning(
            String.format(
                "Protobuf Java version checker saw both Lite and Full gencode during runtime, which"
                    + " is disallowed. Full gencode @%s; Lite gencode @%s",
                location, prevCheckedLocation));
      }
      prevCheckedLocation = location;
      prevFullness = Fullness.FULL;
    }
    validateProtobufGencodeVersionImpl(domain, major, minor, patch, suffix, location);
  }
}
