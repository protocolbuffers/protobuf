// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/**
 * Lite interface that generated extensions implement.
 *
 * <p>Methods are for use by generated code only. You can hold a reference to extensions using this
 * type name.
 */
public abstract class ExtensionLite<ContainingType extends MessageLite, Type> {

  /** Returns the field number of the extension. */
  public abstract int getNumber();

  /** Returns the type of the field. */
  public abstract WireFormat.FieldType getLiteType();

  /** Returns whether it is a repeated field. */
  public abstract boolean isRepeated();

  /** Returns the default value of the extension field. */
  public abstract Type getDefaultValue();

  /** Returns the default instance of the extension field, if it's a message extension. */
  public abstract MessageLite getMessageDefaultInstance();

  /** Returns whether or not this extension is a Lite Extension. */
  boolean isLite() {
    return true;
  }
}
