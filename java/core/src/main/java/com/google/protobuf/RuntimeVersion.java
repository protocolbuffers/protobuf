// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/**
 * Provides the version of this Protobuf Java runtime, and methods for Protobuf Java gencode to
 * validate that versions are compatible. Fields and methods in this class should be only accessed
 * by related unit tests and Protobuf Java gencode, and should not be used elsewhere.
 */
public final class RuntimeVersion {

  /** Indicates the domain of the Protobuf artifact. */
  public enum RuntimeDomain {
    GOOGLE_INTERNAL,
    PUBLIC,
  }

  // The version of this runtime.
  // Automatically updated by Protobuf release process. Do not edit manually.
  public static final RuntimeDomain DOMAIN = RuntimeDomain.PUBLIC;
  public static final int MAJOR = 3;
  public static final int MINOR = 26;
  public static final int PATCH = 0;
  public static final String SUFFIX = "-dev";
  private static final String VERSION_STRING = versionString(MAJOR, MINOR, PATCH, SUFFIX);

  /**
   * Validates that the gencode version is compatible with this runtime version according to
   * https://protobuf.dev/support/cross-version-runtime-guarantee/.
   *
   * <p>This method is currently only used by Protobuf Java gencode in OSS.
   *
   * <p>This method is only for Protobuf Java gencode; do not call it elsewhere.
   *
   * @param domain the domain where Protobuf Java code was generated. Currently ignored.
   * @param major the major version of Protobuf Java gencode.
   * @param minor the minor version of Protobuf Java gencode.
   * @param patch the micro/patch version of Protobuf Java gencode.
   * @param suffix the version suffix e.g. "-rc2", "-dev", etc.
   * @throws ProtobufRuntimeVersionException if versions are incompatible.
   */
  public static void validateProtobufGencodeVersion(
      RuntimeDomain domain, int major, int minor, int patch, String suffix) {

    // Check the environmental variable, and temporarily disable poison pills if it's set to true.
    String disableFlag = java.lang.System.getenv("TEMORARILY_DISABLE_PROTOBUF_VERSION_CHECK");
    if (disableFlag != null && disableFlag.equals("true")) {
      return;
    }

    // Check that version numbers are valid.
    if (major < 0 || minor < 0 || patch < 0) {
      throw new ProtobufRuntimeVersionException(
          "Invalid gencode version: " + versionString(major, minor, patch, suffix));
    }

    String gencodeVersionString = versionString(major, minor, patch, suffix);
    // Check that runtime major version is the same as the gencode major version.
    if (major != MAJOR) {
      throw new ProtobufRuntimeVersionException(
          String.format(
              "Mismatched Protobuf Gencode/Runtime major versions: gencode %s, runtime %s. Same"
                  + " major version is required.",
              gencodeVersionString, VERSION_STRING));
    }

    // Check that runtime version is newer than the gencode version.
    if (MINOR < minor || (MINOR == minor && PATCH < patch)) {
      throw new ProtobufRuntimeVersionException(
          String.format(
              "Protobuf Java runtime version cannot be older than the gencode version:"
                  + "gencode %s, runtime %s.",
              gencodeVersionString, VERSION_STRING));
    }

    // Check that runtime version suffix is the same as the gencode version suffix.
    if (!suffix.equals(SUFFIX)) {
      throw new ProtobufRuntimeVersionException(
          String.format(
              "Mismatched Protobuf Gencode/Runtime version suffixes: gencode %s, runtime %s."
                  + " Version suffixes must be the same.",
              gencodeVersionString, VERSION_STRING));
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

  /** Gets the version string given the version segments. */
  private static String versionString(int major, int minor, int patch, String suffix) {
    return String.format("%d.%d.%d%s", major, minor, patch, suffix);
  }
}
