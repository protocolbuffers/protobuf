# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file or at https://developers.google.com/open-source/licenses/bsd

"""Run against an installed experimental wheel, without source-tree runfiles.

Usage: /path/to/fresh-venv/bin/python python/free_threading/wheel_smoke_test.py
Run with an external timeout to bound worker/executor teardown on failure.
"""

from concurrent import futures
from datetime import datetime, timezone
import json
import sys
import threading

from google._upb import _message
from google.protobuf import descriptor_pb2
from google.protobuf import descriptor_pool
from google.protobuf import json_format
from google.protobuf import message_factory
from google.protobuf import timestamp_pb2
from google.protobuf.internal import api_implementation


def main():
  assert api_implementation.Type() == 'upb'
  assert not sys._is_gil_enabled()
  print(json.dumps({'python': sys.version, 'extension': _message.__file__,
                    'backend': api_implementation.Type(),
                    'gil_enabled': sys._is_gil_enabled()}), flush=True)
  proto = descriptor_pb2.FileDescriptorProto(
      name='wheel_smoke.proto', dependency=['google/protobuf/timestamp.proto'])
  child = proto.message_type.add(name='Child')
  child.field.add(name='time', number=1, type=11, label=1,
                  type_name='.google.protobuf.Timestamp')
  parent = proto.message_type.add(name='Parent')
  parent.field.add(name='values', number=1, type=5, label=3)
  parent.field.add(name='children', number=2, type=11, label=3,
                   type_name='.Child')
  pool = descriptor_pool.DescriptorPool()
  pool.AddSerializedFile(timestamp_pb2.DESCRIPTOR.serialized_pb)
  file = pool.AddSerializedFile(proto.SerializeToString())
  cls = message_factory.GetMessageClass(file.message_types_by_name['Parent'])
  time = datetime(2026, 1, 2, 3, 4, 5, tzinfo=timezone.utc)
  shared = cls(values=range(32))
  shared.children.add(time=time)
  wire = shared.SerializeToString()
  barrier = threading.Barrier(8)

  def worker(index):
    barrier.wait(timeout=30)
    for iteration in range(500):
      message = cls.FromString(wire)
      message.values.append(index + iteration)
      message.children.extend(message.children)
      assert len(message.children) == 2
      assert message.children[1].time.ToDatetime(tzinfo=timezone.utc) == time
      dictionary = json_format.MessageToDict(message)
      assert json_format.ParseDict(dictionary, cls()) == message
      assert cls.FromString(message.SerializeToString()) == message
      assert shared.SerializeToString() == wire
    return 500

  with futures.ThreadPoolExecutor(max_workers=8) as workers:
    count = sum(workers.map(worker, range(8)))
  assert not sys._is_gil_enabled()
  print(f'{count} installed-wheel parse/JSON/alias/read checks passed', flush=True)


if __name__ == '__main__':
  main()
