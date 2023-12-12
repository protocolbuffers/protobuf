// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/** Provides methods to check Protobuf Java runtime version matches the gencode version. */
public class RuntimeVersion extends RuntimeVersionLite {
  /**
   * Validates that the gencode version is compatible with this runtime version according to
   * https://protobuf.dev/support/cross-version-runtime-guarantee/.
   *
   * <p>This method is currently only used by Protobuf Java Full-version gencode in OSS.
   *
   * <p>This method is only for Protobuf Java gencode; do not call it elsewhere.
   *
   * @param domain the domain where Protobuf Java code was generated.
   * @param major the major version of Protobuf Java gencode.
   * @param minor the minor version of Protobuf Java gencode.
   * @param patch the micro/patch version of Protobuf Java gencode.
   * @param suffix the version suffix e.g. "-rc2", "-dev", etc.
   * @throws ProtobufRuntimeVersionException if versions are incompatible.
   */
  public static void validateProtobufGencodeVersion(
      RuntimeDomain domain, int major, int minor, int patch, String suffix) {
    validateProtobufGencodeVersionImpl(domain, major, minor, patch, suffix);
  }
}
