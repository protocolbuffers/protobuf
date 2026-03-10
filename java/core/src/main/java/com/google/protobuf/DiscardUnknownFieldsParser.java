// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/** Parsers to discard unknown fields during parsing. */
public final class DiscardUnknownFieldsParser {

  /**
   * Wraps a given {@link Parser} into a new {@link Parser} that discards unknown fields during
   * parsing.
   *
   * <p>Usage example:
   *
   * <pre>{@code
   * private final static Parser<Foo> FOO_PARSER = DiscardUnknownFieldsParser.wrap(Foo.parser());
   * Foo parseFooDiscardUnknown(ByteBuffer input) throws IOException {
   *   return FOO_PARSER.parseFrom(input);
   * }
   * }</pre>
   *
   * <p>Like all other implementations of {@code Parser}, this parser is stateless and thread-safe.
   *
   * @param parser The delegated parser that parses messages.
   * @return a {@link Parser} that will discard unknown fields during parsing.
   */
  public static final <T extends Message> Parser<T> wrap(final Parser<T> parser) {
    return new AbstractParser<T>() {
      @Override
      public T parsePartialFrom(CodedInputStream input, ExtensionRegistryLite extensionRegistry)
          throws InvalidProtocolBufferException {
        try {
          input.discardUnknownFields();
          return parser.parsePartialFrom(input, extensionRegistry);
        } finally {
          input.unsetDiscardUnknownFields();
        }
      }
    };
  }

  private DiscardUnknownFieldsParser() {}
}
