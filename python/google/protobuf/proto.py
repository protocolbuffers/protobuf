# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Contains the Nextgen Pythonic protobuf APIs."""

import io
from typing import Type, Union

from google.protobuf.internal import decoder
from google.protobuf.internal import encoder
from google.protobuf.message import Message

def serialize(message: Message, deterministic: bool=None) -> bytes:
  """Return the serialized proto.

  Args:
    message: The proto message to be serialized.
    deterministic: If true, requests deterministic serialization
        of the protobuf, with predictable ordering of map keys.

  Returns:
    A binary bytes representation of the message.
  """
  return message.SerializeToString(deterministic=deterministic)


def parse(message_class: Type[Message], payload: bytes) -> Message:
  """Given a serialized data in binary form, deserialize it into a Message.

  Args:
    message_class: The message meta class.
    payload: A serialized bytes in binary form.

  Returns:
    A new message deserialized from payload.
  """
  new_message = message_class()
  new_message.ParseFromString(payload)
  return new_message


def serialize_length_prefixed(message: Message, output: io.BytesIO) -> None:
  """Writes the size of the message as a varint and the serialized message.

  Writes the size of the message as a varint and then the serialized message.
  This allows more data to be written to the output after the message. Use
  parse_length_prefixed to parse messages written by this method.

  The output stream must be buffered, e.g. using
  https://docs.python.org/3/library/io.html#buffered-streams.

  Args:
    message: The protocol buffer message that should be serialized.
    output: BytesIO or custom buffered IO that data should be written to.
  """
  size = message.ByteSize()
  encoder._VarintEncoder()(output.write, size)
  out_size = output.write(serialize(message))
  if out_size is None:
    raise TypeError('Output stream does not extend io.RawIOBase')
  if out_size != size:
    raise TypeError(
        'Failed to write complete message (wrote: %d, expected: %d)'
        '. Ensure output is using buffered IO.' % (out_size, size)
    )


def parse_length_prefixed(
    message_class: Type[Message], input_bytes: Union[io.BytesIO, bytes]
) -> Message:
  """Parse a message from input_bytes.

  Args:
    message_class: The protocol buffer message class that parser should parse.
    input_bytes: A buffered input.

  Returns:
    A parsed message if successful. None if input_bytes is at EOF.
  """

  def _read_varint(input_bytes):
    """Read a varint from input_bytes.

    Args:
      input_bytes: A buffered input.

    Returns:
      The varint value or None if we read EOF
    """
    size = 0
    shift = 0
    mask = (1 << 64) - 1

    while 1:
      try:
        b = input_bytes.read(1)[0]
      except IndexError as e:
        if shift == 0:
          return None
        else:
          raise ValueError('Fail to read message size %s' % str(e))
      size |= (b & 0x7F) << shift
      if not b & 0x80:
        size &= mask
        break
      shift += 7
      if shift >= 64:
        raise ValueError(
            'Fail to read message size: Too many bytes when decoding varint.'
        )
    return size

  buffer = (
      io.BytesIO(input_bytes) if isinstance(input_bytes, bytes) else input_bytes
  )
  size = _read_varint(buffer)
  if size is None:
    return None

  if size == 0:
    message = message_class()
    return message

  message = parse(message_class, buffer.read(size))
  parsed_size = message.ByteSize()
  if parsed_size != size:
    raise ValueError(
        'Truncated message or non-buffered input_bytes: '
        'Expected {0} bytes but only {1} bytes parsed for '
        '{2}.'.format(size, parsed_size, message.DESCRIPTOR.name)
    )
  return message
