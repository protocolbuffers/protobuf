"""Regression test for self or indirect recursive messages with pybind11."""

from google.protobuf.internal import pybind11_test_module
from google.protobuf.internal import self_recursive_from_py_pb2
from google3.testing.pybase import unittest


class RecursiveMessagePybind11Test(unittest.TestCase):

  def test_self_recursive_message_callback(self):
    called = False

    def callback(
        msg: self_recursive_from_py_pb2.ContainsSelfRecursive,
    ) -> None:
      nonlocal called
      called = True

    # Without proper handling of message factories (in pyext/message.cc New)
    # this will stack overflow
    pybind11_test_module.invoke_callback_on_message(
        callback, self_recursive_from_py_pb2.ContainsSelfRecursive()
    )
    self.assertTrue(called)

  def test_indirect_recursive_message_callback(self):
    called = False

    def callback(
        msg: self_recursive_from_py_pb2.ContainsIndirectRecursive,
    ) -> None:
      nonlocal called
      called = True

    # Without proper handling of message factories (in pyext/message.cc New)
    # this will stack overflow
    pybind11_test_module.invoke_callback_on_message(
        callback, self_recursive_from_py_pb2.ContainsIndirectRecursive()
    )
    self.assertTrue(called)


if __name__ == "__main__":
  unittest.main()
