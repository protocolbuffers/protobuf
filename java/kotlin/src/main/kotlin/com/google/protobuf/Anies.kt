// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf.kotlin

import com.google.protobuf.Any as ProtoAny
import com.google.protobuf.Message

/** Returns `true` if this [com.google.protobuf.Any] contains a message of type `T`. */
inline fun <reified T : Message> ProtoAny.isA(): Boolean = this.`is`(T::class.java)

/**
 * Returns the message of type `T` encoded in this [com.google.protobuf.Any].
 *
 * @throws InvalidProtocolBufferException if this [com.google.protobuf.Any] does not contain a `T`
 *   message.
 */
inline fun <reified T : Message> ProtoAny.unpack(): T = unpack(T::class.java)
