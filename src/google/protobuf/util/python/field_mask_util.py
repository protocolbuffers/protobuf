"""Utilities for working with FieldMasks."""

from typing import TypeVar

from google.protobuf import message

from google.protobuf import field_mask_pb2
from google.protobuf.util.python import _field_mask_util as field_mask_util_clif


_T = TypeVar("_T", bound=message.Message)


class FieldMaskUtil:
  """Utilities for working with FieldMasks.

  This class mirrors the C++ API structure that uses static methods
  and has a nested MergeOptions class.
  """

  MergeOptions = field_mask_util_clif.FieldMaskUtil.MergeOptions  # pylint: disable=invalid-name

  @classmethod
  def MergeMessageTo(
      cls,
      source: _T,
      mask: field_mask_pb2.FieldMask,
      options: MergeOptions,
      destination: _T,
  ) -> _T:
    """Merges fields specified by a FieldMask from a source message to a copy of a destination message and returns the merged message.

    This function wraps the C++ FieldMaskUtil::MergeMessageTo implementation.
    It merges fields from the source message into a copy of the destination
    message according to the provided mask and options. Neither the source nor
    the destination message object passed to this function is modified.

    Note that this behavior is different from the C++ implementation, which
    modifies the destination message in place.

    Args:
      source: The source protocol buffer message.
      mask: A FieldMask specifying which fields to merge.
      options: An instance of MergeOptions controlling the merge behavior (e.g.,
        replace_message_fields, replace_repeated_fields).
      destination: The destination protocol buffer message.

    Returns:
      The merged message.
    """
    # Call the CLIF-wrapped C++ function
    # It returns a PyObject* which is converted to 'object' in CLIF,
    # and specifically PyBytes in the C++ implementation, so we expect 'bytes'.
    merged_bytes = field_mask_util_clif.MergeMessageToBytes(
        source=source, mask=mask, options=options, destination=destination  # pytype: disable=wrong-arg-types
    )
    return destination.__class__().FromString(merged_bytes)
