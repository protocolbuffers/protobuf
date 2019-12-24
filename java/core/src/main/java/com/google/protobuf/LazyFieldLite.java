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

import java.io.IOException;

/**
 * LazyFieldLite encapsulates the logic of lazily parsing message fields. It stores the message in a
 * ByteString initially and then parses it on-demand.
 *
 * <p>LazyFieldLite is thread-compatible: concurrent reads are safe once the proto that this
 * LazyFieldLite is a part of is no longer being mutated by its Builder. However, explicit
 * synchronization is needed under read/write situations.
 *
 * <p>When a LazyFieldLite is used in the context of a MessageLite object, its behavior is
 * considered to be immutable and none of the setter methods in its API are expected to be invoked.
 * All of the getters are expected to be thread-safe. When used in the context of a
 * MessageLite.Builder, setters can be invoked, but there is no guarantee of thread safety.
 *
 * <p>TODO(yatin,dweis): Consider splitting this class's functionality and put the mutable methods
 * into a separate builder class to allow us to give stronger compile-time guarantees.
 *
 * <p>This class is internal implementation detail of the protobuf library, so you don't need to use
 * it directly.
 *
 * @author xiangl@google.com (Xiang Li)
 */
public class LazyFieldLite {
  private static final ExtensionRegistryLite EMPTY_REGISTRY =
      ExtensionRegistryLite.getEmptyRegistry();

  /*
   * The value associated with the LazyFieldLite object is stored in one or more of the following
   * three fields (delayedBytes, value, memoizedBytes). They should together be interpreted as
   * follows.
   *
   * 1) delayedBytes can be non-null, while value and memoizedBytes is null. The object will be in
   * this state while the value for the object has not yet been parsed.
   *
   * 2) Both delayedBytes and value are non-null. The object transitions to this state as soon as
   * some caller needs to access the value (by invoking getValue()).
   *
   * 3) memoizedBytes is merely an optimization for calls to LazyFieldLite.toByteString() to avoid
   * recomputing the ByteString representation on each call. Instead, when the value is parsed from
   * delayedBytes, we will also assign the contents of delayedBytes to memoizedBytes (since that is
   * the ByteString representation of value).
   *
   * 4) Finally, if the LazyFieldLite was created directly with a parsed MessageLite value, then
   * delayedBytes will be null, and memoizedBytes will be initialized only upon the first call to
   * LazyFieldLite.toByteString().
   *
   * <p>Given the above conditions, any caller that needs a serialized representation of this object
   * must first check if the memoizedBytes or delayedBytes ByteString is non-null and use it
   * directly; if both of those are null, it can look at the parsed value field. Similarly, any
   * caller that needs a parsed value must first check if the value field is already non-null, if
   * not it must parse the value from delayedBytes.
   */

  /**
   * A delayed-parsed version of the contents of this field. When this field is non-null, then the
   * "value" field is allowed to be null until the time that the value needs to be read.
   *
   * <p>When delayedBytes is non-null then {@code extensionRegistry} is required to also be
   * non-null. {@code value} and {@code memoizedBytes} will be initialized lazily.
   */
  private ByteString delayedBytes;

  /**
   * An {@code ExtensionRegistryLite} for parsing bytes. It is non-null on a best-effort basis. It
   * is only guaranteed to be non-null if this message was initialized using bytes and an {@code
   * ExtensionRegistry}. If it directly had a value set then it will be null, unless it has been
   * merged with another {@code LazyFieldLite} that had an {@code ExtensionRegistry}.
   */
  private ExtensionRegistryLite extensionRegistry;

  /**
   * The parsed value. When this is null and a caller needs access to the MessageLite value, then
   * {@code delayedBytes} will be parsed lazily at that time.
   */
  protected volatile MessageLite value;

  /**
   * The memoized bytes for {@code value}. This is an optimization for the toByteString() method to
   * not have to recompute its return-value on each invocation. TODO(yatin): Figure out whether this
   * optimization is actually necessary.
   */
  private volatile ByteString memoizedBytes;

  /** Constructs a LazyFieldLite with bytes that will be parsed lazily. */
  public LazyFieldLite(ExtensionRegistryLite extensionRegistry, ByteString bytes) {
    checkArguments(extensionRegistry, bytes);
    this.extensionRegistry = extensionRegistry;
    this.delayedBytes = bytes;
  }

  /** Constructs a LazyFieldLite with no contents, and no ability to parse extensions. */
  public LazyFieldLite() {}

  /**
   * Constructs a LazyFieldLite instance with a value. The LazyFieldLite may not be able to parse
   * the extensions in the value as it has no ExtensionRegistry.
   */
  public static LazyFieldLite fromValue(MessageLite value) {
    LazyFieldLite lf = new LazyFieldLite();
    lf.setValue(value);
    return lf;
  }

  @Override
  public boolean equals(Object o) {
    if (this == o) {
      return true;
    }

    if (!(o instanceof LazyFieldLite)) {
      return false;
    }

    LazyFieldLite other = (LazyFieldLite) o;

    // Lazy fields do not work well with equals... If both are delayedBytes, we do not have a
    // mechanism to deserialize them so we rely on bytes equality. Otherwise we coerce into an
    // actual message (if necessary) and call equals on the message itself. This implies that two
    // messages can by unequal but then be turned equal simply be invoking a getter on a lazy field.
    MessageLite value1 = value;
    MessageLite value2 = other.value;
    if (value1 == null && value2 == null) {
      return toByteString().equals(other.toByteString());
    } else if (value1 != null && value2 != null) {
      return value1.equals(value2);
    } else if (value1 != null) {
      return value1.equals(other.getValue(value1.getDefaultInstanceForType()));
    } else {
      return getValue(value2.getDefaultInstanceForType()).equals(value2);
    }
  }

  @Override
  public int hashCode() {
    // We can't provide a memoizable hash code for lazy fields. The byte strings may have different
    // hash codes but evaluate to equivalent messages. And we have no facility for constructing
    // a message here if we were not already holding a value.
    return 1;
  }

  /**
   * Determines whether this LazyFieldLite instance represents the default instance of this type.
   */
  public boolean containsDefaultInstance() {
    return memoizedBytes == ByteString.EMPTY
        || value == null && (delayedBytes == null || delayedBytes == ByteString.EMPTY);
  }

  /**
   * Clears the value state of this instance.
   *
   * <p>LazyField is not thread-safe for write access. Synchronizations are needed under read/write
   * situations.
   */
  public void clear() {
    // Don't clear the ExtensionRegistry. It might prove useful later on when merging in another
    // value, but there is no guarantee that it will contain all extensions that were directly set
    // on the values that need to be merged.
    delayedBytes = null;
    value = null;
    memoizedBytes = null;
  }

  /**
   * Overrides the contents of this LazyField.
   *
   * <p>LazyField is not thread-safe for write access. Synchronizations are needed under read/write
   * situations.
   */
  public void set(LazyFieldLite other) {
    this.delayedBytes = other.delayedBytes;
    this.value = other.value;
    this.memoizedBytes = other.memoizedBytes;
    // If the other LazyFieldLite was created by directly setting the value rather than first by
    // parsing, then it will not have an extensionRegistry. In this case we hold on to the existing
    // extensionRegistry, which has no guarantees that it has all the extensions that will be
    // directly set on the value.
    if (other.extensionRegistry != null) {
      this.extensionRegistry = other.extensionRegistry;
    }
  }

  /**
   * Returns message instance. It may do some thread-safe delayed parsing of bytes.
   *
   * @param defaultInstance its message's default instance. It's also used to get parser for the
   *     message type.
   */
  public MessageLite getValue(MessageLite defaultInstance) {
    ensureInitialized(defaultInstance);
    return value;
  }

  /**
   * Sets the value of the instance and returns the old value without delay parsing anything.
   *
   * <p>LazyField is not thread-safe for write access. Synchronizations are needed under read/write
   * situations.
   */
  public MessageLite setValue(MessageLite value) {
    MessageLite originalValue = this.value;
    this.delayedBytes = null;
    this.memoizedBytes = null;
    this.value = value;
    return originalValue;
  }

  /**
   * Merges another instance's contents. In some cases may drop some extensions if both fields
   * contain data. If the other field has an {@code ExtensionRegistry} but this does not, then this
   * field will copy over that {@code ExtensionRegistry}.
   *
   * <p>LazyField is not thread-safe for write access. Synchronizations are needed under read/write
   * situations.
   */
  public void merge(LazyFieldLite other) {
    if (other.containsDefaultInstance()) {
      return;
    }

    if (this.containsDefaultInstance()) {
      set(other);
      return;
    }

    // If the other field has an extension registry but this does not, copy over the other extension
    // registry.
    if (this.extensionRegistry == null) {
      this.extensionRegistry = other.extensionRegistry;
    }

    // In the case that both of them are not parsed we simply concatenate the bytes to save time. In
    // the (probably rare) case that they have different extension registries there is a chance that
    // some of the extensions may be dropped, but the tradeoff of making this operation fast seems
    // to outway the benefits of combining the extension registries, which is not normally done for
    // lite protos anyways.
    if (this.delayedBytes != null && other.delayedBytes != null) {
      this.delayedBytes = this.delayedBytes.concat(other.delayedBytes);
      return;
    }

    // At least one is parsed and both contain data. We won't drop any extensions here directly, but
    // in the case that the extension registries are not the same then we might in the future if we
    // need to serialze and parse a message again.
    if (this.value == null && other.value != null) {
      setValue(mergeValueAndBytes(other.value, this.delayedBytes, this.extensionRegistry));
      return;
    } else if (this.value != null && other.value == null) {
      setValue(mergeValueAndBytes(this.value, other.delayedBytes, other.extensionRegistry));
      return;
    }

    // At this point we have two fully parsed messages.
    setValue(this.value.toBuilder().mergeFrom(other.value).build());
  }

  /**
   * Merges another instance's contents from a stream.
   *
   * <p>LazyField is not thread-safe for write access. Synchronizations are needed under read/write
   * situations.
   */
  public void mergeFrom(CodedInputStream input, ExtensionRegistryLite extensionRegistry)
      throws IOException {
    if (this.containsDefaultInstance()) {
      setByteString(input.readBytes(), extensionRegistry);
      return;
    }

    // If the other field has an extension registry but this does not, copy over the other extension
    // registry.
    if (this.extensionRegistry == null) {
      this.extensionRegistry = extensionRegistry;
    }

    // In the case that both of them are not parsed we simply concatenate the bytes to save time. In
    // the (probably rare) case that they have different extension registries there is a chance that
    // some of the extensions may be dropped, but the tradeoff of making this operation fast seems
    // to outway the benefits of combining the extension registries, which is not normally done for
    // lite protos anyways.
    if (this.delayedBytes != null) {
      setByteString(this.delayedBytes.concat(input.readBytes()), this.extensionRegistry);
      return;
    }

    // We are parsed and both contain data. We won't drop any extensions here directly, but in the
    // case that the extension registries are not the same then we might in the future if we
    // need to serialize and parse a message again.
    try {
      setValue(value.toBuilder().mergeFrom(input, extensionRegistry).build());
    } catch (InvalidProtocolBufferException e) {
      // Nothing is logged and no exceptions are thrown. Clients will be unaware that a proto
      // was invalid.
    }
  }

  private static MessageLite mergeValueAndBytes(
      MessageLite value, ByteString otherBytes, ExtensionRegistryLite extensionRegistry) {
    try {
      return value.toBuilder().mergeFrom(otherBytes, extensionRegistry).build();
    } catch (InvalidProtocolBufferException e) {
      // Nothing is logged and no exceptions are thrown. Clients will be unaware that a proto
      // was invalid.
      return value;
    }
  }

  /** Sets this field with bytes to delay-parse. */
  public void setByteString(ByteString bytes, ExtensionRegistryLite extensionRegistry) {
    checkArguments(extensionRegistry, bytes);
    this.delayedBytes = bytes;
    this.extensionRegistry = extensionRegistry;
    this.value = null;
    this.memoizedBytes = null;
  }

  /**
   * Due to the optional field can be duplicated at the end of serialized bytes, which will make the
   * serialized size changed after LazyField parsed. Be careful when using this method.
   */
  public int getSerializedSize() {
    // We *must* return delayed bytes size if it was ever set because the dependent messages may
    // have memoized serialized size based off of it.
    if (memoizedBytes != null) {
      return memoizedBytes.size();
    } else if (delayedBytes != null) {
      return delayedBytes.size();
    } else if (value != null) {
      return value.getSerializedSize();
    } else {
      return 0;
    }
  }

  /** Returns a BytesString for this field in a thread-safe way. */
  public ByteString toByteString() {
    if (memoizedBytes != null) {
      return memoizedBytes;
    }
    // We *must* return delayed bytes if it was set because the dependent messages may have
    // memoized serialized size based off of it.
    if (delayedBytes != null) {
      return delayedBytes;
    }
    synchronized (this) {
      if (memoizedBytes != null) {
        return memoizedBytes;
      }
      if (value == null) {
        memoizedBytes = ByteString.EMPTY;
      } else {
        memoizedBytes = value.toByteString();
      }
      return memoizedBytes;
    }
  }

  /** Writes this lazy field into a {@link Writer}. */
  void writeTo(Writer writer, int fieldNumber) throws IOException {
    if (memoizedBytes != null) {
      writer.writeBytes(fieldNumber, memoizedBytes);
    } else if (delayedBytes != null) {
      writer.writeBytes(fieldNumber, delayedBytes);
    } else if (value != null) {
      writer.writeMessage(fieldNumber, value);
    } else {
      writer.writeBytes(fieldNumber, ByteString.EMPTY);
    }
  }

  /** Might lazily parse the bytes that were previously passed in. Is thread-safe. */
  protected void ensureInitialized(MessageLite defaultInstance) {
    if (value != null) {
      return;
    }
    synchronized (this) {
      if (value != null) {
        return;
      }
      try {
        if (delayedBytes != null) {
          // The extensionRegistry shouldn't be null here since we have delayedBytes.
          MessageLite parsedValue =
              defaultInstance.getParserForType().parseFrom(delayedBytes, extensionRegistry);
          this.value = parsedValue;
          this.memoizedBytes = delayedBytes;
        } else {
          this.value = defaultInstance;
          this.memoizedBytes = ByteString.EMPTY;
        }
      } catch (InvalidProtocolBufferException e) {
        // Nothing is logged and no exceptions are thrown. Clients will be unaware that this proto
        // was invalid.
        this.value = defaultInstance;
        this.memoizedBytes = ByteString.EMPTY;
      }
    }
  }

  private static void checkArguments(ExtensionRegistryLite extensionRegistry, ByteString bytes) {
    if (extensionRegistry == null) {
      throw new NullPointerException("found null ExtensionRegistry");
    }
    if (bytes == null) {
      throw new NullPointerException("found null ByteString");
    }
  }
}
