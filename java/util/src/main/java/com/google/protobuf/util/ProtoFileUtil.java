// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.util;

import com.google.protobuf.DescriptorProtos.Edition;

/** Utility helper functions to work with {@link com.google.protobuf.FileDescriptorProto}. */
public final class ProtoFileUtil {

  private ProtoFileUtil() {}

  /** Converts an Edition to its string representation as specified in ".proto" file. */
  public static String getEditionString(Edition edition) {
    return edition.toString().substring("EDITION_".length());
  }
}
