// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import com.google.protobuf.Descriptors.Descriptor;

/**
 * Resolves a descriptor for a given type URL.
 *
 * @see com.google.protobuf.Any#getTypeUrl()
 */
public interface DescriptorResolver {
  /**
   * Returns the {@link Descriptor} for a typeUrl, or {@code null} if no match is found.
   *
   * @param typeUrl The {@link com.google.protobuf.Any#getTypeUrl()} to find the default instance
   *     for.
   */
  Descriptor getDescriptorForTypeUrl(String typeUrl);
}
