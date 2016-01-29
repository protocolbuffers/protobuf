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

/**
 * LazyFieldLite encapsulates the logic of lazily parsing message fields. It stores
 * the message in a ByteString initially and then parse it on-demand.
 *
 * LazyField is thread-compatible e.g. concurrent read are safe, however,
 * synchronizations are needed under read/write situations.
 *
 * This class is internal implementation detail, so you don't need to use it directly.
 *
 * @author xiangl@google.com (Xiang Li)
 */
public class LazyFieldLite {
  private static final ExtensionRegistryLite EMPTY_REGISTRY =
      ExtensionRegistryLite.getEmptyRegistry();

  /**
   * A delayed-parsed version of the bytes. When this is non-null then {@code extensionRegistry } is
   * also non-null and {@code value} and {@code memoizedBytes} are null.
   */
  private ByteString delayedBytes;

  /**
   * An {@code ExtensionRegistryLite} for parsing bytes. It is non-null on a best-effort basis. It
   * is only guaranteed to be non-null if this message was initialized using bytes and an
   * {@code ExtensionRegistry}. If it directly had a value set then it will be null, unless it has
   * been merged with another {@code LazyFieldLite} that had an {@code ExtensionRegistry}.
   */
  private ExtensionRegistryLite extensionRegistry;

  /**
   * The parsed value. When this is non-null then {@code delayedBytes} will be null.
   */
  protected volatile MessageLite value;

  /**
   * The memoized bytes for {@code value}. Will be null when {@code value} is null.
   */
  private volatile ByteString memoizedBytes;

  /**
   * Constructs a LazyFieldLite with bytes that will be parsed lazily.
   */
  public LazyFieldLite(ExtensionRegistryLite extensionRegistry, ByteString bytes) {
    checkArguments(extensionRegistry, bytes);
    this.extensionRegistry = extensionRegistry;
    this.delayedBytes = bytes;
  }

  /**
   * Constructs a LazyFieldLite with no contents, and no ability to parse extensions.
   */
  public LazyFieldLite() {
  }

  /**
   * Constructs a LazyFieldLite instance with a value. The LazyFieldLite may not be able to parse
   * the extensions in the value as it has no ExtensionRegistry.
   */
  public static LazyFieldLite fromValue(MessageLite value) {
    LazyFieldLite lf = new LazyFieldLite();
    lf.setValue(value);
    return lf;
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
   * <p>LazyField is not thread-safe for write access. Synchronizations are needed
   * under read/write situations.
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
   * <p>LazyField is not thread-safe for write access. Synchronizations are needed
   * under read/write situations.
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
   * message type.
   */
  public MessageLite getValue(MessageLite defaultInstance) {
    ensureInitialized(defaultInstance);
    return value;
  }

  /**
   * Sets the value of the instance and returns the old value without delay parsing anything.
   *
   * <p>LazyField is not thread-safe for write access. Synchronizations are needed
   * under read/write situations.
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
   * <p>LazyField is not thread-safe for write access. Synchronizations are needed
   * under read/write situations.
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

    // At this point we have two fully parsed messages. We can't merge directly from one to the
    // other because only generated builder code contains methods to mergeFrom another parsed
    // message. We have to serialize one instance and then merge the bytes into the other. This may
    // drop extensions from one of the messages if one of the values had an extension set on it
    // directly.
    //
    // To mitigate this we prefer serializing a message that has an extension registry, and
    // therefore a chance that all extensions set on it are in that registry.
    //
    // NOTE: The check for other.extensionRegistry not being null must come first because at this
    // point in time if other.extensionRegistry is not null then this.extensionRegistry will not be
    // null either.
    if (other.extensionRegistry != null) {
      setValue(mergeValueAndBytes(this.value, other.toByteString(), other.extensionRegistry));
      return;
    } else if (this.extensionRegistry != null) {
      setValue(mergeValueAndBytes(other.value, this.toByteString(), this.extensionRegistry));
      return;
    } else {
      // All extensions from the other message will be dropped because we have no registry.
      setValue(mergeValueAndBytes(this.value, other.toByteString(), EMPTY_REGISTRY));
      return;
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

  /**
   * Sets this field with bytes to delay-parse.
   */
  public void setByteString(ByteString bytes, ExtensionRegistryLite extensionRegistry) {
    checkArguments(extensionRegistry, bytes);
    this.delayedBytes = bytes;
    this.extensionRegistry = extensionRegistry;
    this.value = null;
    this.memoizedBytes = null;
  }

  /**
   * Due to the optional field can be duplicated at the end of serialized
   * bytes, which will make the serialized size changed after LazyField
   * parsed. Be careful when using this method.
   */
  public int getSerializedSize() {
    if (delayedBytes != null) {
      return delayedBytes.size();
    } else if (memoizedBytes != null) {
      return memoizedBytes.size();
    } else if (value != null) {
      return value.getSerializedSize();
    } else {
      return 0;
    }
  }

  /**
   * Returns a BytesString for this field in a thread-safe way.
   */
  public ByteString toByteString() {
    if (delayedBytes != null) {
      return delayedBytes;
    }
    if (memoizedBytes != null) {
      return memoizedBytes;
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

  /**
   * Might lazily parse the bytes that were previously passed in. Is thread-safe.
   */
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
          MessageLite parsedValue = defaultInstance.getParserForType()
              .parseFrom(delayedBytes, extensionRegistry);
          this.value = parsedValue;
          this.memoizedBytes = delayedBytes;
          this.delayedBytes = null;
        } else {
          this.value = defaultInstance;
          this.memoizedBytes = ByteString.EMPTY;
          this.delayedBytes = null;
        }
      } catch (InvalidProtocolBufferException e) {
        // Nothing is logged and no exceptions are thrown. Clients will be unaware that this proto
        // was invalid.
        this.value = defaultInstance;
        this.memoizedBytes = ByteString.EMPTY;
        this.delayedBytes = null;
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
