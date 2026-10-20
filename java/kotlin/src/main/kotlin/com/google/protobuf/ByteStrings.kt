// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.kotlin

import com.google.protobuf.ByteString
import java.nio.ByteBuffer

/** Encodes this String into a sequence of UTF-8 bytes and returns the result as a [ByteString]. */
fun String.toByteStringUtf8(): ByteString =
  ByteString.copyFromUtf8(this) // symmetric from ByteString.toStringUtf8()

/** Concatenates the given [ByteString] to this one. */
operator fun ByteString.plus(other: ByteString): ByteString = concat(other)

/** Gets the byte at [index]. */
operator fun ByteString.get(index: Int): Byte = byteAt(index)

/** Checks if this is not empty. */
fun ByteString.isNotEmpty(): Boolean = !isEmpty()

/** Returns a copy of this [ByteArray] as an immutable [ByteString]. */
fun ByteArray.toByteString(): ByteString = ByteString.copyFrom(this)

/** Copies the remaining bytes from this [ByteBuffer] to a [ByteString]. */
fun ByteBuffer.toByteString(): ByteString = ByteString.copyFrom(this)
