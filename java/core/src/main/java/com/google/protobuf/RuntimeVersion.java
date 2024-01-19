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

  /**
   * Validates that the gencode version is compatible with this runtime version according to
   * https://protobuf.dev/support/cross-version-runtime-guarantee/.
   *
   * <p>This method is currently only used by Protobuf Java **full version** gencode. Do not call it
   * elsewhere.
   *
   * @param domain the domain where Protobuf Java code was generated.
   * @param major the major version of Protobuf Java gencode.
   * @param minor the minor version of Protobuf Java gencode.
   * @param patch the micro/patch version of Protobuf Java gencode.
   * @param suffix the version suffix e.g. "-rc2", "-dev", etc.
   * @param location the debugging location e.g. message full name to put in the error messages.
   * @throws ProtobufRuntimeVersionException if versions are incompatible.
   */
  public static void validateProtobufGencodeVersion(
      RuntimeVersionLite.RuntimeDomain domain,
      int major,
      int minor,
      int patch,
      String suffix,
      String location) {
    // TODO: b/298200443 - Check that lite version validation should not happen in full runtime and
    // vice versa.
    validateProtobufLiteGencodeVersion(domain, major, minor, patch, suffix, location);
  }
}
