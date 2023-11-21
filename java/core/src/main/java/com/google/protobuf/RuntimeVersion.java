// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/**
 * Provides the version of this Protobuf Java runtime, and methods for Protobuf Java gencode to
 * validate that versions are compatible.
 */
public final class RuntimeVersion {

  /** Indicates the domain of the Protobuf artifact. */
  public enum RuntimeDomain {
    GOOGLE_INTERNAL,
    PUBLIC,
  }

  // The version information for this runtime.
  // Automatically updated by Protobuf release process. Do not edit manually.
  private static final RuntimeDomain DOMAIN = RuntimeDomain.PUBLIC;
  private static final int MAJOR = 3;
  private static final int MINOR = 26;
  private static final int PATCH = 0;
  private static final String SUFFIX = "-dev";

  /**
   * Validates that the gencode version is compatible with this runtime version. Currently, no
   * validation takes place, but only checks that version numbers valid.
   *
   * <p>This method is only for Protobuf Java gencode; do not call it elsewhere.
   *
   * <p>In the future, we will validate Protobuf Java versions according to
   * https://protobuf.dev/support/cross-version-runtime-guarantee/
   *
   * @param domain the domain where Protobuf Java code was generated. Currently unused.
   * @param major the major version of Protobuf Java gencode.
   * @param minor the minor version of Protobuf Java gencode.
   * @param patch the micro/patch version of Protobuf Java gencode.
   * @param suffix the version suffix e.g. "-rc2", "-dev", etc. Currently unused.
   */
  public static void validateProtobufGencodeVersion(
      RuntimeDomain domain, int major, int minor, int patch, String suffix) {
    // TODO: b/298200443 - Add cross-version validations.
    if (major < 0 || minor < 0 || patch < 0) {
      throw new ProtobufRuntimeVersionException(
          String.format("Invalid gencode version: %d.%d.%d", major, minor, patch));
    }
  }

  /**
   * A runtime exception to be thrown by the version validator if version is not well defined or
   * versions mismatch.
   */
  public static final class ProtobufRuntimeVersionException extends RuntimeException {
    public ProtobufRuntimeVersionException(String message) {
      super(message);
    }
  }
}
