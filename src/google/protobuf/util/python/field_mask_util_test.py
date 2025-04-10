from google.protobuf import field_mask_pb2
from google.protobuf import timestamp_pb2
from google3.testing.pybase import unittest
from google.protobuf.util.python import field_mask_util


class FieldMaskUtilTest(unittest.TestCase):

  def test_merge_message_to(self):
    source = timestamp_pb2.Timestamp(seconds=123, nanos=456)
    mask = field_mask_pb2.FieldMask(paths=["seconds"])
    options = field_mask_util.FieldMaskUtil.MergeOptions()
    destination = timestamp_pb2.Timestamp()
    field_mask_util.FieldMaskUtil.MergeMessageTo(
        source, mask, options, destination
    )
    self.assertEqual(destination.seconds, 123)
    self.assertEqual(destination.nanos, 0)


if __name__ == "__main__":
  unittest.main()
