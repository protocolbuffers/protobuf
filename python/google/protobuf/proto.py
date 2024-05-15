# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Contains the Nextgen Pythonic protobuf APIs."""

import typing

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

def parse(message_class: typing.Type[Message], payload: bytes) -> Message:
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
