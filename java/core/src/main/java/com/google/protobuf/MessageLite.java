// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Abstract interface implemented by Protocol Message objects.
 *
 * <p>This interface is intended to only be implemented by protoc created gencode. It is not
 * intended or supported to implement this interface manually.
 *
 * <p>This interface is implemented by all protocol message objects. Non-lite messages additionally
 * implement the Message interface, which is a subclass of MessageLite. Use MessageLite instead when
 * you only need the subset of features which it supports -- namely, nothing that uses descriptors
 * or reflection. You can instruct the protocol compiler to generate classes which implement only
 * MessageLite, not the full Message interface, by adding the follow line to the .proto file:
 *
 * <pre>
 *   option optimize_for = LITE_RUNTIME;
 * </pre>
 *
 * <p>This is particularly useful on resource-constrained systems where the full protocol buffers
 * runtime library is too big.
 *
 * <p>Note that on non-constrained systems (e.g. servers) when you need to link in lots of protocol
 * definitions, a better way to reduce total code footprint is to use {@code optimize_for =
 * CODE_SIZE}. This will make the generated code smaller while still supporting all the same
 * features (at the expense of speed). {@code optimize_for = LITE_RUNTIME} is best when you only
 * have a small number of message types linked into your binary, in which case the size of the
 * protocol buffers runtime itself is the biggest problem.
 *
 * @author kenton@google.com Kenton Varda
 */
@CheckReturnValue
public interface MessageLite extends MessageLiteOrBuilder {

  /**
   * Serializes the message and writes it to {@code output}. This does not flush or close the
   * stream.
   */
  void writeTo(CodedOutputStream output) throws IOException;

  /**
   * Get the number of bytes required to encode this message. The result is only computed on the
   * first call and memoized after that.
   *
   * If this message requires more than Integer.MAX_VALUE bytes to encode, the return value will
   * be smaller than the actual number of bytes required and might be negative.
   */
  int getSerializedSize();

  /** Gets the parser for a message of the same type as this message. */
  Parser<? extends MessageLite> getParserForType();

  // -----------------------------------------------------------------
  // Convenience methods.

  /**
   * Serializes the message to a {@code ByteString} and returns it. This is just a trivial wrapper
   * around {@link #writeTo(CodedOutputStream)}.
   *
   * If this message requires more than Integer.MAX_VALUE bytes to encode, the behavior is
   * unpredictable. It may throw a runtime exception or truncate or slice the data.
   */
  ByteString toByteString();

  /**
   * Serializes the message to a {@code byte} array and returns it. This is just a trivial wrapper
   * around {@link #writeTo(CodedOutputStream)}.
   *
   * If this message requires more than Integer.MAX_VALUE bytes to encode, the behavior is
   * unpredictable. It may throw a runtime exception or truncate or slice the data.
   */
  byte[] toByteArray();

  /**
   * Serializes the message and writes it to {@code output}. This is just a trivial wrapper around
   * {@link #writeTo(CodedOutputStream)}. This does not flush or close the stream.
   *
   * <p>NOTE: Protocol Buffers are not self-delimiting. Therefore, if you write any more data to the
   * stream after the message, you must somehow ensure that the parser on the receiving end does not
   * interpret this as being part of the protocol message. This can be done, for instance, by
   *  writing the size of the message before the data, then making sure to limit the input to that
   * size on the receiving end by wrapping the InputStream in one which limits the input.
   * Alternatively, just use {@link #writeDelimitedTo(OutputStream)}.
   */
  void writeTo(OutputStream output) throws IOException;

  /**
   * Like {@link #writeTo(OutputStream)}, but writes the size of the message as a varint before
   * writing the data. This allows more data to be written to the stream after the message without
   * the need to delimit the message data yourself. Use {@link
   * Builder#mergeDelimitedFrom(InputStream)} (or the static method {@code
   * YourMessageType.parseDelimitedFrom(InputStream)}) to parse messages written by this method.
   */
  void writeDelimitedTo(OutputStream output) throws IOException;

  // =================================================================
  // Builders

  /** Constructs a new builder for a message of the same type as this message. */
  Builder newBuilderForType();

  /**
   * Constructs a builder initialized with the current message. Use this to derive a new message
   * from the current one.
   */
  Builder toBuilder();

  /** Abstract interface implemented by Protocol Message builders. */
  interface Builder extends MessageLiteOrBuilder, Cloneable {
    /** Resets all fields to their default values. */
    @CanIgnoreReturnValue
    Builder clear();

    /**
     * Constructs the message based on the state of the Builder. Subsequent changes to the Builder
     * will not affect the returned message.
     *
     * @throws UninitializedMessageException The message is missing one or more required fields
     *     (i.e. {@link #isInitialized()} returns false). Use {@link #buildPartial()} to bypass this
     *     check.
     */
    MessageLite build();

    /**
     * Like {@link #build()}, but does not throw an exception if the message is missing required
     * fields. Instead, a partial message is returned. Subsequent changes to the Builder will not
     * affect the returned message.
     */
    MessageLite buildPartial();

    /**
     * Clones the Builder.
     *
     * @see Object#clone()
     */
    Builder clone();

    /**
     * Parses a message of this type from the input and merges it with this message.
     *
     * <p>Warning: This does not verify that all required fields are present in the input message.
     * If you call {@link #build()} without setting all required fields, it will throw an {@link
     * UninitializedMessageException}, which is a {@code RuntimeException} and thus might not be
     * caught. There are a few good ways to deal with this:
     *
     * <ul>
     *   <li>Call {@link #isInitialized()} to verify that all required fields are set before
     *       building.
     *   <li>Use {@code buildPartial()} to build, which ignores missing required fields.
     * </ul>
     *
     * <p>Note: The caller should call {@link CodedInputStream#checkLastTagWas(int)} after calling
     * this to verify that the last tag seen was the appropriate end-group tag, or zero for EOF.
     *
     * @throws InvalidProtocolBufferException the bytes read are not syntactically correct according
     *     to the protobuf wire format specification. The data is corrupt, incomplete, or was never
     *     a protobuf in the first place.
     * @throws IOException an I/O error reading from the stream
     */
    @CanIgnoreReturnValue
    Builder mergeFrom(CodedInputStream input) throws IOException;

    /**
     * Like {@link Builder#mergeFrom(CodedInputStream)}, but also parses extensions. The extensions
     * that you want to be able to parse must be registered in {@code extensionRegistry}. Extensions
     * not in the registry will be treated as unknown fields.
     *
     * @throws InvalidProtocolBufferException the bytes read are not syntactically correct according
     *     to the protobuf wire format specification. The data is corrupt, incomplete, or was never
     *     a protobuf in the first place.
     * @throws IOException an I/O error reading from the stream
     */
    @CanIgnoreReturnValue
    Builder mergeFrom(CodedInputStream input, ExtensionRegistryLite extensionRegistry)
        throws IOException;

    // ---------------------------------------------------------------
    // Convenience methods.

    /**
     * Parse {@code data} as a message of this type and merge it with the message being built. This
     * is just a small wrapper around {@link #mergeFrom(CodedInputStream)}.
     *
     * @throws InvalidProtocolBufferException the bytes in data are not syntactically correct
     *     according to the protobuf wire format specification. The data is corrupt, incomplete, or
     *     was never a protobuf in the first place.
     * @return this
     */
    @CanIgnoreReturnValue
    Builder mergeFrom(ByteString data) throws InvalidProtocolBufferException;

    /**
     * Parse {@code data} as a message of this type and merge it with the message being built. This
     * is just a small wrapper around {@link #mergeFrom(CodedInputStream,ExtensionRegistryLite)}.
     *
     * @throws InvalidProtocolBufferException the bytes in data are not syntactically correct
     *     according to the protobuf wire format specification. The data is corrupt, incomplete, or
     *     was never a protobuf in the first place.
     * @return this
     */
    @CanIgnoreReturnValue
    Builder mergeFrom(ByteString data, ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException;

    /**
     * Parse {@code data} as a message of this type and merge it with the message being built. This
     * is just a small wrapper around {@link #mergeFrom(CodedInputStream)}.
     *
     * @throws InvalidProtocolBufferException the bytes in data are not syntactically correct
     *     according to the protobuf wire format specification. The data is corrupt, incomplete, or
     *     was never a protobuf in the first place.
     * @return this
     */
    @CanIgnoreReturnValue
    Builder mergeFrom(byte[] data) throws InvalidProtocolBufferException;

    /**
     * Parse {@code data} as a message of this type and merge it with the message being built. This
     * is just a small wrapper around {@link #mergeFrom(CodedInputStream)}.
     *
     * @throws InvalidProtocolBufferException the bytes in data are not syntactically correct
     *     according to the protobuf wire format specification. The data is corrupt, incomplete, or
     *     was never a protobuf in the first place.
     * @return this
     */
    @CanIgnoreReturnValue
    Builder mergeFrom(byte[] data, int off, int len) throws InvalidProtocolBufferException;

    /**
     * Parse {@code data} as a message of this type and merge it with the message being built. This
     * is just a small wrapper around {@link #mergeFrom(CodedInputStream,ExtensionRegistryLite)}.
     *
     * @throws InvalidProtocolBufferException the bytes in data are not syntactically correct
     *     according to the protobuf wire format specification. The data is corrupt, incomplete, or
     *     was never a protobuf in the first place.
     * @return this
     */
    @CanIgnoreReturnValue
    Builder mergeFrom(byte[] data, ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException;

    /**
     * Parse {@code data} as a message of this type and merge it with the message being built. This
     * is just a small wrapper around {@link #mergeFrom(CodedInputStream,ExtensionRegistryLite)}.
     *
     * @throws InvalidProtocolBufferException the bytes in data are not syntactically correct
     *     according to the protobuf wire format specification. The data is corrupt, incomplete, or
     *     was never a protobuf in the first place.
     * @return this
     */
    @CanIgnoreReturnValue
    Builder mergeFrom(byte[] data, int off, int len, ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException;

    /**
     * Parse a message of this type from {@code input} and merge it with the message being built.
     * This is just a small wrapper around {@link #mergeFrom(CodedInputStream)}. Note that this
     * method always reads the <i>entire</i> input (unless it throws an exception). If you want it
     * to stop earlier, you will need to wrap your input in some wrapper stream that limits reading.
     * Or, use {@link MessageLite#writeDelimitedTo(OutputStream)} to write your message and {@link
     * #mergeDelimitedFrom(InputStream)} to read it.
     *
     * <p>Despite usually reading the entire input, this does not close the stream.
     *
     * @throws InvalidProtocolBufferException the bytes read are not syntactically correct according
     *     to the protobuf wire format specification. The data is corrupt, incomplete, or was never
     *     a protobuf in the first place.
     * @throws IOException an I/O error reading from the stream
     * @return this
     */
    @CanIgnoreReturnValue
    Builder mergeFrom(InputStream input) throws IOException;

    /**
     * Parse a message of this type from {@code input} and merge it with the message being built.
     * This is just a small wrapper around {@link
     * #mergeFrom(CodedInputStream,ExtensionRegistryLite)}.
     *
     * @return this
     */
    @CanIgnoreReturnValue
    Builder mergeFrom(InputStream input, ExtensionRegistryLite extensionRegistry)
        throws IOException;

    /**
     * Merge {@code other} into the message being built. {@code other} must have the exact same type
     * as {@code this} (i.e. {@code getClass().equals(getDefaultInstanceForType().getClass())}).
     *
     * <p>Merging occurs as follows. For each field:<br>
     * * For singular primitive fields, if the field is set in {@code other}, then {@code other}'s
     * value overwrites the value in this message.<br>
     * * For singular message fields, if the field is set in {@code other}, it is merged into the
     * corresponding sub-message of this message using the same merging rules.<br>
     * * For repeated fields, the elements in {@code other} are concatenated with the elements in
     * this message. * For oneof groups, if the other message has one of the fields set, the group
     * of this message is cleared and replaced by the field of the other message, so that the oneof
     * constraint is preserved.
     *
     * <p>This is equivalent to the {@code Message::MergeFrom} method in C++.
     */
    @CanIgnoreReturnValue
    Builder mergeFrom(MessageLite other);

    /**
     * Like {@link #mergeFrom(InputStream)}, but does not read until EOF. Instead, the size of the
     * message (encoded as a varint) is read first, then the message data. Use {@link
     * MessageLite#writeDelimitedTo(OutputStream)} to write messages in this format.
     *
     * @return true if successful, or false if the stream is at EOF when the method starts. Any
     *     other error (including reaching EOF during parsing) causes an exception to be thrown.
     * @throws InvalidProtocolBufferException the bytes read are not syntactically correct according
     *     to the protobuf wire format specification. The data is corrupt, incomplete, or was never
     *     a protobuf in the first place.
     * @throws IOException an I/O error reading from the stream
     */
    boolean mergeDelimitedFrom(InputStream input) throws IOException;

    /**
     * Like {@link #mergeDelimitedFrom(InputStream)} but supporting extensions.
     *
     * @return true if successful, or false if the stream is at EOF when the method starts. Any
     *     other error (including reaching EOF during parsing) causes an exception to be thrown.
     * @throws InvalidProtocolBufferException the bytes read are not syntactically correct according
     *     to the protobuf wire format specification. The data is corrupt, incomplete, or was never
     *     a protobuf in the first place.
     * @throws IOException an I/O error reading from the stream
     */
    boolean mergeDelimitedFrom(InputStream input, ExtensionRegistryLite extensionRegistry)
        throws IOException;
  }
}
