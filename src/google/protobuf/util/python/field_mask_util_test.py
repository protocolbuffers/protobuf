import hypothesis as hp
from hypothesis import strategies as st

from google.protobuf import field_mask_pb2
from google.protobuf import struct_pb2
from google.protobuf import timestamp_pb2
from google3.testing.pybase import unittest
from absl.testing import parameterized
from google.protobuf.util.python import field_mask_util
from google.protobuf.util.python import nested_pb2


_ORIGINAL_VALUE = 42


@st.composite
def timestamp_strategy(draw):
  return timestamp_pb2.Timestamp(
      seconds=draw(st.integers(0, 100000)), nanos=draw(st.integers(0, 100000))
  )


@st.composite
def field_mask_strategy(draw):
  return field_mask_pb2.FieldMask(
      paths=draw(st.lists(st.sampled_from(["seconds", "nanos"])))
  )


class FieldMaskUtilTest(parameterized.TestCase):

  def test_merge_message_to_simple(self):
    source = timestamp_pb2.Timestamp(seconds=1, nanos=2)
    mask = field_mask_pb2.FieldMask(paths=["seconds"])
    options = field_mask_util.FieldMaskUtil.MergeOptions()
    destination = timestamp_pb2.Timestamp(seconds=3, nanos=4)

    result = field_mask_util.FieldMaskUtil.MergeMessageTo(
        source, mask, options, destination
    )

    self.assertEqual(result.seconds, 1)

  @hp.given(
      source=timestamp_strategy(),
      mask=field_mask_strategy(),
  )
  def test_merge_message_to_hp(
      self, source: timestamp_pb2.Timestamp, mask: field_mask_pb2.FieldMask
  ):
    options = field_mask_util.FieldMaskUtil.MergeOptions()
    destination = timestamp_pb2.Timestamp(
        seconds=_ORIGINAL_VALUE, nanos=_ORIGINAL_VALUE
    )

    result = field_mask_util.FieldMaskUtil.MergeMessageTo(
        source, mask, options, destination
    )

    if "seconds" in mask.paths:
      self.assertEqual(result.seconds, source.seconds)
    else:
      self.assertEqual(result.seconds, _ORIGINAL_VALUE)
    if "nanos" in mask.paths:
      self.assertEqual(result.nanos, source.nanos)
    else:
      self.assertEqual(result.nanos, _ORIGINAL_VALUE)

  @parameterized.named_parameters(
      dict(
          testcase_name="replace_repeated_fields",
          replace_repeated_fields=True,
          expected_values=[struct_pb2.Value(string_value="hello")],
      ),
      dict(
          testcase_name="not_replace_repeated_fields",
          replace_repeated_fields=False,
          expected_values=[
              struct_pb2.Value(string_value="world"),
              struct_pb2.Value(string_value="hello"),
          ],
      ),
  )
  def test_merge_message_to_repeated_fields(
      self,
      replace_repeated_fields,
      expected_values,
  ):
    source = struct_pb2.ListValue(
        values=[struct_pb2.Value(string_value="hello")]
    )
    mask = field_mask_pb2.FieldMask(paths=["values"])
    options = field_mask_util.FieldMaskUtil.MergeOptions()
    options.replace_repeated_fields = replace_repeated_fields
    destination = struct_pb2.ListValue(
        values=[struct_pb2.Value(string_value="world")]
    )

    result = field_mask_util.FieldMaskUtil.MergeMessageTo(
        source, mask, options, destination
    )

    self.assertListEqual(list(result.values), expected_values)

  @parameterized.named_parameters(
      dict(
          testcase_name="replace_message_fields",
          replace_message_fields=True,
          expected_nanos=0,
      ),
      dict(
          testcase_name="not_replace_message_fields",
          replace_message_fields=False,
          expected_nanos=4,
      ),
  )
  def test_merge_message_to_message_fields(
      self,
      replace_message_fields,
      expected_nanos,
  ):
    source = nested_pb2.TimestampWrapper(
        timestamp=timestamp_pb2.Timestamp(seconds=1)
    )
    mask = field_mask_pb2.FieldMask(paths=["timestamp"])
    options = field_mask_util.FieldMaskUtil.MergeOptions()
    options.replace_message_fields = replace_message_fields
    destination = nested_pb2.TimestampWrapper(
        timestamp=timestamp_pb2.Timestamp(seconds=3, nanos=4)
    )

    result = field_mask_util.FieldMaskUtil.MergeMessageTo(
        source, mask, options, destination
    )

    self.assertEqual(result.timestamp.seconds, 1)
    self.assertEqual(result.timestamp.nanos, expected_nanos)


if __name__ == "__main__":
  unittest.main()
