# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

import datetime
from typing import assert_type

from google.protobuf.message import Message
from python.pyi_test import fixture_import_pb2, fixture_pb2, self_fields_pb2


step = fixture_pb2.Step(
    a="a",
    ok=None,
    detail="detail",
    labels=["label"],
    children=[fixture_pb2.Child(value="child")],
    children_by_name={"child": fixture_pb2.Child(value="child")},
    state=fixture_pb2.STATE_READY,
    payload=b"payload",
    imported=fixture_import_pb2.Imported(value="imported"),
    created_at=datetime.datetime(2025, 1, 1, tzinfo=datetime.timezone.utc),
    elapsed=datetime.timedelta(seconds=1),
)
message: Message = step
assert_type(step.ok, bool)
assert_type(step.detail, str)
assert_type(step.payload, bytes)
assert_type(step.imported.value, str)
assert_type(step.state, fixture_pb2.State)
assert_type(step.labels[0], str)
assert_type(step.children[0], fixture_pb2.Child)
assert_type(step.children_by_name["child"], fixture_pb2.Child)
assert_type(step.created_at.seconds, int)
assert_type(step.elapsed.nanos, int)
nested = fixture_pb2.Step.Nested(started="started")
assert_type(nested.started, str)
self_fields = self_fields_pb2.SelfFields(
    something=1,
    self="self",
    self_="self_",
)
assert_type(self_fields.something, int)
assert_type(self_fields.self, str)
assert_type(self_fields.self_, str)

step.payload = b"updated"
step.labels.append("updated")
