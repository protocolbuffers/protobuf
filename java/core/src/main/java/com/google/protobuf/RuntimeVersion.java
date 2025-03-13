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
public final class RuntimeVersion {

  /** Indicates the domain of the Protobuf artifact. */
  public enum RuntimeDomain {
    GOOGLE_INTERNAL,
    PUBLIC,
  }

  // The version of this runtime.
  // Automatically updated by Protobuf release process. Do not edit manually.
  // These OSS versions are not stripped to avoid merging conflicts.
  public static final RuntimeDomain OSS_DOMAIN = RuntimeDomain.PUBLIC;
  public static final int OSS_MAJOR = 4;
  public static final int OSS_MINOR = 30;
  public static final int OSS_PATCH = 1;
  public static final String OSS_SUFFIX = "";

  public static final RuntimeDomain DOMAIN = OSS_DOMAIN;
  public static final int MAJOR = OSS_MAJOR;
  public static final int MINOR = OSS_MINOR;
  public static final int PATCH = OSS_PATCH;
  public static final String SUFFIX = OSS_SUFFIX;

  private static final int MAX_WARNING_COUNT = 20;

  @SuppressWarnings("NonFinalStaticField")
  static int majorWarningLoggedCount = 0;

  @SuppressWarnings("NonFinalStaticField")
  static int minorWarningLoggedCount = 0;

  private static final String VERSION_STRING = versionString(MAJOR, MINOR, PATCH, SUFFIX);
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
   * @param location the debugging location e.g. generated Java class to put in the error messages.
   * @throws ProtobufRuntimeVersionException if versions are incompatible.
   */
  public static void validateProtobufGencodeVersion(
      RuntimeDomain domain, int major, int minor, int patch, String suffix, String location) {
    validateProtobufGencodeVersionImpl(domain, major, minor, patch, suffix, location);
  }

  /** The actual implementation of version validation. */
  private static void validateProtobufGencodeVersionImpl(
      RuntimeDomain domain, int major, int minor, int patch, String suffix, String location) {
    if (checkDisabled()) {
      return;
    }
    // Check that version numbers are valid.
    if (major < 0 || minor < 0 || patch < 0) {
      throw new ProtobufRuntimeVersionException(
          "Invalid gencode version: " + versionString(major, minor, patch, suffix));
    }

    // Check that runtime domain is the same as the gencode domain.
    if (domain != DOMAIN) {
      throw new ProtobufRuntimeVersionException(
          String.format(
              "Detected mismatched Protobuf Gencode/Runtime domains when loading %s: gencode %s,"
                  + " runtime %s. Cross-domain usage of Protobuf is not supported.",
              location, domain, DOMAIN));
    }

    String gencodeVersionString = null;

    // Check that runtime major version is the same as the gencode major version.
    if (major != MAJOR) {
      if (major == MAJOR - 1 && majorWarningLoggedCount < MAX_WARNING_COUNT) {
        gencodeVersionString = versionString(major, minor, patch, suffix);
        logger.warning(
            String.format(
                " Protobuf gencode version %s is exactly one major version older than the runtime"
                    + " version %s at %s. Please update the gencode to avoid compatibility"
                    + " violations in the next runtime release.",
                gencodeVersionString, VERSION_STRING, location));
        majorWarningLoggedCount++;
      } else {
        throw new ProtobufRuntimeVersionException(
            String.format(
                "Detected mismatched Protobuf Gencode/Runtime major versions when loading %s:"
                    + " gencode %s, runtime %s. Same major version is required.",
                location, versionString(major, minor, patch, suffix), VERSION_STRING));
      }
    }

    // Check that runtime version is newer than the gencode version.
    if (MINOR < minor || (minor == MINOR && PATCH < patch)) {
      if (gencodeVersionString == null) {
        gencodeVersionString = versionString(major, minor, patch, suffix);
      }
      throw new ProtobufRuntimeVersionException(
          String.format(
              "Detected incompatible Protobuf Gencode/Runtime versions when loading %s: gencode %s,"
                  + " runtime %s. Runtime version cannot be older than the linked gencode version.",
              location, gencodeVersionString, VERSION_STRING));
    }

    // Check that runtime version suffix is the same as the gencode version suffix.
    if (!suffix.equals(SUFFIX)) {
      if (gencodeVersionString == null) {
        gencodeVersionString = versionString(major, minor, patch, suffix);
      }
      throw new ProtobufRuntimeVersionException(
          String.format(
              "Detected mismatched Protobuf Gencode/Runtime version suffixes when loading %s:"
                  + " gencode %s, runtime %s. Version suffixes must be the same.",
              location, gencodeVersionString, VERSION_STRING));
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

  private static boolean checkDisabled() {
    // Check the environmental variable, and temporarily disable validation if it's set to true.
    String disableFlag = java.lang.System.getenv("TEMPORARILY_DISABLE_PROTOBUF_VERSION_CHECK");
    if ((disableFlag != null && disableFlag.equals("true"))) {
      return true;
    }

    return false;
  }

  private RuntimeVersion() {}
}
