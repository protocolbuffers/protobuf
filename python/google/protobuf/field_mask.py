# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Contains the FieldMask helper APIs."""

from typing import Optional

from google.protobuf import descriptor

from google.protobuf.field_mask_pb2 import FieldMask


def to_json_string(mask_msg: FieldMask) -> str:
  """Converts FieldMask to string according to proto3 JSON spec."""
  return mask_msg.ToJsonString()


def from_json_string(value: str) -> FieldMask:
  """Converts string to FieldMask according to proto3 JSON spec."""
  msg = FieldMask()
  msg.FromJsonString(value)
  return msg


def is_valid_for_descriptor(
    mask_msg: FieldMask, message_descriptor: descriptor.Descriptor
) -> bool:
  """Checks whether the FieldMask is valid for Message Descriptor."""
  return mask_msg.IsValidForDescriptor(message_descriptor)


def all_fields_from_descriptor(
    message_descriptor: descriptor.Descriptor,
) -> FieldMask:
  """Gets all direct fields of Message Descriptor to FieldMask."""
  msg = FieldMask()
  msg.AllFieldsFromDescriptor(message_descriptor)
  return msg


def canonical_form_from_mask(mask_msg: FieldMask) -> FieldMask:
  """Converts a FieldMask to the canonical form FieldMask."""
  msg = FieldMask()
  msg.CanonicalFormFromMask(mask_msg)
  return msg


def union(mask1: FieldMask, mask2: FieldMask) -> FieldMask:
  """Merges mask1 and mask2 into a new FieldMask."""
  msg = FieldMask()
  msg.Union(mask1, mask2)
  return msg


def intersect(mask1: FieldMask, mask2: FieldMask) -> FieldMask:
  """Intersects mask1 and mask2 into a new FieldMask."""
  msg = FieldMask()
  msg.Intersect(mask1, mask2)
  return msg


def merge_message(
    mask_msg: FieldMask,
    source: FieldMask,
    destination: FieldMask,
    replace_message_field: Optional[bool] = False,
    replace_repeated_field: Optional[bool] = False,
) -> None:
  """Merges fields specified in field_mask from source to destination."""
  return mask_msg.MergeMessage(
      source, destination, replace_message_field, replace_repeated_field
  )
