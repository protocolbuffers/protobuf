// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.Descriptors.OneofDescriptor;

/**
 * This file is meant to be a temporary housing for legacy descriptor APIs we want to deprecate and
 * remove. This will help prevent backslide by allowing us to control visibility.
 */
public final class LegacyDescriptorsUtil {

  /** Wraps FileDescriptor */
  public static final class LegacyFileDescriptor {

    /** The syntax of the .proto file. */
    public static enum Syntax {
      UNKNOWN("unknown"),
      PROTO2("proto2"),
      PROTO3("proto3");

      Syntax(String name) {
        this.name = name;
      }

      final String name;
    }

    public static Syntax getSyntax(FileDescriptor descriptor) {
      switch (descriptor.getSyntax()) {
        case UNKNOWN:
          return Syntax.UNKNOWN;
        case PROTO2:
          return Syntax.PROTO2;
        case PROTO3:
          return Syntax.PROTO3;
      }
      throw new IllegalArgumentException("Unexpected syntax");
    }

    private LegacyFileDescriptor() {}
  }

  /** Wraps FieldDescriptor */
  public static final class LegacyFieldDescriptor {

    public static boolean hasOptionalKeyword(FieldDescriptor descriptor) {
      return descriptor.hasOptionalKeyword();
    }

    private LegacyFieldDescriptor() {}
  }

  /** Wraps OneofDescriptor */
  public static final class LegacyOneofDescriptor {

    public static boolean isSynthetic(OneofDescriptor descriptor) {
      return descriptor.isSynthetic();
    }

    private LegacyOneofDescriptor() {}
  }

  private LegacyDescriptorsUtil() {}
}
