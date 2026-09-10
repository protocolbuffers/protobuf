# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

import datetime

from typing_extensions import reveal_type

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
reveal_type(step.ok)
reveal_type(step.detail)
reveal_type(step.payload)
reveal_type(step.imported.value)
reveal_type(step.state)
nested = fixture_pb2.Step.Nested(started="started")
reveal_type(nested.started)
self_fields = self_fields_pb2.SelfFields(
    something=1,
    self="self",
    self_="self_",
)
reveal_type(self_fields.something)
reveal_type(self_fields.self)
reveal_type(self_fields.self_)

fixture_pb2.Step(payload="not bytes")
step.payload = "not bytes"
fixture_pb2.Step(not_a_field="unknown")
