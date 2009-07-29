// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

// TODO(kenton):  Use generics?  E.g. Builder<BuilderType extends Builder>, then
//   mergeFrom*() could return BuilderType for better type-safety.

package com.google.protobuf;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Abstract interface implemented by Protocol Message objects.
 *
 * <p>This interface is implemented by all protocol message objects.  Most
 * users will be more interested in the Message interface, which is a subclass
 * of MessageLite.  Use MessageLite instead when you only need the subset of
 * features which it supports -- namely, nothing that uses descriptors or
 * reflection.  You can instruct the protocol compiler to generate classes
 * which implement only MessageLite, not the full Message interface, by adding
 * the follow line to the .proto file:
 * <pre>
 *   option optimize_for = LITE_RUNTIME;
 * </pre>
 *
 * <p>This is particularly useful on resource-constrained systems where the
 * full protocol buffers runtime library is too big.
 *
 * <p>Note that on non-constrained systems (e.g. servers) when you need to link
 * in lots of protocol definitions, a better way to reduce total code footprint
 * is to use {@code optimize_for = CODE_SIZE}.  This will make the generated
 * code smaller while still supporting all the same features (at the expense of
 * speed).  {@code optimize_for = LITE_RUNTIME} is best when you only have a
 * small number of message types linked into your binary, in which case the
 * size of the protocol buffers runtime itself is the biggest problem.
 *
 * @author kenton@google.com Kenton Varda
 */
public interface MessageLite {
  /**
   * Get an instance of the type with all fields set to their default values.
   * This may or may not be a singleton.  This differs from the
   * {@code getDefaultInstance()} method of generated message classes in that
   * this method is an abstract method of the {@code MessageLite} interface
   * whereas {@code getDefaultInstance()} is a static method of a specific
   * class.  They return the same thing.
   */
  MessageLite getDefaultInstanceForType();

  /**
   * Returns true if all required fields in the message and all embedded
   * messages are set, false otherwise.
   */
  boolean isInitialized();

  /**
   * Serializes the message and writes it to {@code output}.  This does not
   * flush or close the stream.
   */
  void writeTo(CodedOutputStream output) throws IOException;

  /**
   * Get the number of bytes required to encode this message.  The result
   * is only computed on the first call and memoized after that.
   */
  int getSerializedSize();

  // -----------------------------------------------------------------
  // Convenience methods.

  /**
   * Serializes the message to a {@code ByteString} and returns it. This is
   * just a trivial wrapper around
   * {@link #writeTo(CodedOutputStream)}.
   */
  ByteString toByteString();

  /**
   * Serializes the message to a {@code byte} array and returns it.  This is
   * just a trivial wrapper around
   * {@link #writeTo(CodedOutputStream)}.
   */
  byte[] toByteArray();

  /**
   * Serializes the message and writes it to {@code output}.  This is just a
   * trivial wrapper around {@link #writeTo(CodedOutputStream)}.  This does
   * not flush or close the stream.
   * <p>
   * NOTE:  Protocol Buffers are not self-delimiting.  Therefore, if you write
   * any more data to the stream after the message, you must somehow ensure
   * that the parser on the receiving end does not interpret this as being
   * part of the protocol message.  This can be done e.g. by writing the size
   * of the message before the data, then making sure to limit the input to
   * that size on the receiving end (e.g. by wrapping the InputStream in one
   * which limits the input).  Alternatively, just use
   * {@link #writeDelimitedTo(OutputStream)}.
   */
  void writeTo(OutputStream output) throws IOException;

  /**
   * Like {@link #writeTo(OutputStream)}, but writes the size of the message
   * as a varint before writing the data.  This allows more data to be written
   * to the stream after the message without the need to delimit the message
   * data yourself.  Use {@link Builder#mergeDelimitedFrom(InputStream)} (or
   * the static method {@code YourMessageType.parseDelimitedFrom(InputStream)})
   * to parse messages written by this method.
   */
  void writeDelimitedTo(OutputStream output) throws IOException;

  // =================================================================
  // Builders

  /**
   * Constructs a new builder for a message of the same type as this message.
   */
  Builder newBuilderForType();

  /**
   * Constructs a builder initialized with the current message.  Use this to
   * derive a new message from the current one.
   */
  Builder toBuilder();

  /**
   * Abstract interface implemented by Protocol Message builders.
   */
  interface Builder extends Cloneable {
    /** Resets all fields to their default values. */
    Builder clear();

    /**
     * Construct the final message.  Once this is called, the Builder is no
     * longer valid, and calling any other method will result in undefined
     * behavior and may throw a NullPointerException.  If you need to continue
     * working with the builder after calling {@code build()}, {@code clone()}
     * it first.
     * @throws UninitializedMessageException The message is missing one or more
     *         required fields (i.e. {@link #isInitialized()} returns false).
     *         Use {@link #buildPartial()} to bypass this check.
     */
    MessageLite build();

    /**
     * Like {@link #build()}, but does not throw an exception if the message
     * is missing required fields.  Instead, a partial message is returned.
     * Once this is called, the Builder is no longer valid, and calling any
     * will result in undefined behavior and may throw a NullPointerException.
     *
     * If you need to continue working with the builder after calling
     * {@code buildPartial()}, {@code clone()} it first.
     */
    MessageLite buildPartial();

    /**
     * Clones the Builder.
     * @see Object#clone()
     */
    Builder clone();

    /**
     * Returns true if all required fields in the message and all embedded
     * messages are set, false otherwise.
     */
    boolean isInitialized();

    /**
     * Parses a message of this type from the input and merges it with this
     * message, as if using {@link Builder#mergeFrom(MessageLite)}.
     *
     * <p>Warning:  This does not verify that all required fields are present in
     * the input message.  If you call {@link #build()} without setting all
     * required fields, it will throw an {@link UninitializedMessageException},
     * which is a {@code RuntimeException} and thus might not be caught.  There
     * are a few good ways to deal with this:
     * <ul>
     *   <li>Call {@link #isInitialized()} to verify that all required fields
     *       are set before building.
     *   <li>Parse the message separately using one of the static
     *       {@code parseFrom} methods, then use {@link #mergeFrom(MessageLite)}
     *       to merge it with this one.  {@code parseFrom} will throw an
     *       {@link InvalidProtocolBufferException} (an {@code IOException})
     *       if some required fields are missing.
     *   <li>Use {@code buildPartial()} to build, which ignores missing
     *       required fields.
     * </ul>
     *
     * <p>Note:  The caller should call
     * {@link CodedInputStream#checkLastTagWas(int)} after calling this to
     * verify that the last tag seen was the appropriate end-group tag,
     * or zero for EOF.
     */
    Builder mergeFrom(CodedInputStream input) throws IOException;

    /**
     * Like {@link Builder#mergeFrom(CodedInputStream)}, but also
     * parses extensions.  The extensions that you want to be able to parse
     * must be registered in {@code extensionRegistry}.  Extensions not in
     * the registry will be treated as unknown fields.
     */
    Builder mergeFrom(CodedInputStream input,
                      ExtensionRegistryLite extensionRegistry)
                      throws IOException;

    /**
     * Get the message's type's default instance.
     * See {@link MessageLite#getDefaultInstanceForType()}.
     */
    MessageLite getDefaultInstanceForType();

    // ---------------------------------------------------------------
    // Convenience methods.

    /**
     * Parse {@code data} as a message of this type and merge it with the
     * message being built.  This is just a small wrapper around
     * {@link #mergeFrom(CodedInputStream)}.
     */
    Builder mergeFrom(ByteString data) throws InvalidProtocolBufferException;

    /**
     * Parse {@code data} as a message of this type and merge it with the
     * message being built.  This is just a small wrapper around
     * {@link #mergeFrom(CodedInputStream,ExtensionRegistry)}.
     */
    Builder mergeFrom(ByteString data,
                      ExtensionRegistryLite extensionRegistry)
                      throws InvalidProtocolBufferException;

    /**
     * Parse {@code data} as a message of this type and merge it with the
     * message being built.  This is just a small wrapper around
     * {@link #mergeFrom(CodedInputStream)}.
     */
    Builder mergeFrom(byte[] data) throws InvalidProtocolBufferException;

    /**
     * Parse {@code data} as a message of this type and merge it with the
     * message being built.  This is just a small wrapper around
     * {@link #mergeFrom(CodedInputStream)}.
     */
    Builder mergeFrom(byte[] data, int off, int len)
                      throws InvalidProtocolBufferException;

    /**
     * Parse {@code data} as a message of this type and merge it with the
     * message being built.  This is just a small wrapper around
     * {@link #mergeFrom(CodedInputStream,ExtensionRegistry)}.
     */
    Builder mergeFrom(byte[] data,
                      ExtensionRegistryLite extensionRegistry)
                      throws InvalidProtocolBufferException;

    /**
     * Parse {@code data} as a message of this type and merge it with the
     * message being built.  This is just a small wrapper around
     * {@link #mergeFrom(CodedInputStream,ExtensionRegistry)}.
     */
    Builder mergeFrom(byte[] data, int off, int len,
                      ExtensionRegistryLite extensionRegistry)
                      throws InvalidProtocolBufferException;

    /**
     * Parse a message of this type from {@code input} and merge it with the
     * message being built.  This is just a small wrapper around
     * {@link #mergeFrom(CodedInputStream)}.  Note that this method always
     * reads the <i>entire</i> input (unless it throws an exception).  If you
     * want it to stop earlier, you will need to wrap your input in some
     * wrapper stream that limits reading.  Or, use
     * {@link MessageLite#writeDelimitedTo(OutputStream)} to write your message
     * and {@link #mergeDelimitedFrom(InputStream)} to read it.
     * <p>
     * Despite usually reading the entire input, this does not close the stream.
     */
    Builder mergeFrom(InputStream input) throws IOException;

    /**
     * Parse a message of this type from {@code input} and merge it with the
     * message being built.  This is just a small wrapper around
     * {@link #mergeFrom(CodedInputStream,ExtensionRegistry)}.
     */
    Builder mergeFrom(InputStream input,
                      ExtensionRegistryLite extensionRegistry)
                      throws IOException;

    /**
     * Like {@link #mergeFrom(InputStream)}, but does not read until EOF.
     * Instead, the size of the message (encoded as a varint) is read first,
     * then the message data.  Use
     * {@link MessageLite#writeDelimitedTo(OutputStream)} to write messages in
     * this format.
     */
    Builder mergeDelimitedFrom(InputStream input)
                               throws IOException;

    /**
     * Like {@link #mergeDelimitedFrom(InputStream)} but supporting extensions.
     */
    Builder mergeDelimitedFrom(InputStream input,
                               ExtensionRegistryLite extensionRegistry)
                               throws IOException;
  }
}
