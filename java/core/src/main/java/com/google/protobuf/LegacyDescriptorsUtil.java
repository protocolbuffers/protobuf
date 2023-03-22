// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protobuf;

import com.google.protobuf.Descriptors.FileDescriptor;

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

  private LegacyDescriptorsUtil() {}
}
