// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/** Verifies that an object is mutable, throwing if not. */
interface MutabilityOracle {
  static final MutabilityOracle IMMUTABLE =
      new MutabilityOracle() {
        @Override
        public void ensureMutable() {
          throw new UnsupportedOperationException();
        }
      };

  /** Throws an {@link UnsupportedOperationException} if not mutable. */
  void ensureMutable();
}
