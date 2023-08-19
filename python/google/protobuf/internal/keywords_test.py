# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
# https://developers.google.com/protocol-buffers/
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Tests for google.protobuf.internal.keywords."""

import pytest

from google.protobuf.internal import more_messages_pb2
from google.protobuf import descriptor_pool


@pytest.fixture
def pool():
    return descriptor_pool.Default()

def test_message(pool):
    message = getattr(more_messages_pb2, 'class')()
    message.int_field = 123
    assert message.int_field == 123
    des = pool.FindMessageTypeByName('google.protobuf.internal.class')
    assert des.name == 'class'

def test_nested_message(pool):
    message = getattr(more_messages_pb2, 'class')()
    message.nested_message.field = 234
    assert message.nested_message.field == 234
    des = pool.FindMessageTypeByName('google.protobuf.internal.class.try')
    assert des.name == 'try'

def test_field():
    message = getattr(more_messages_pb2, 'class')()
    setattr(message, 'if', 123)
    setattr(message, 'as', 1)
    assert getattr(message, 'if') == 123
    assert getattr(message, 'as') == 1

def test_enum():
    class_ = getattr(more_messages_pb2, 'class')
    message = class_()
    # Normal enum value.
    message.enum_field = more_messages_pb2.default
    assert message.enum_field == more_messages_pb2.default
    # Top level enum value.
    message.enum_field = getattr(more_messages_pb2, 'else')
    assert message.enum_field == 1
    # Nested enum value
    message.nested_enum_field = getattr(class_, 'True')
    assert message.nested_enum_field == 1

def test_extension():
    message = getattr(more_messages_pb2, 'class')()
    # Top level extension
    extension1 = getattr(more_messages_pb2, 'continue')
    message.Extensions[extension1] = 456
    assert message.Extensions[extension1] == 456
    # None top level extension
    extension2 = getattr(more_messages_pb2.ExtendClass, 'return')
    message.Extensions[extension2] = 789
    assert message.Extensions[extension2] == 789

def test_extension_for_nested_message():
    message = getattr(more_messages_pb2, 'class')()
    extension = getattr(more_messages_pb2, 'with')
    message.nested_message.Extensions[extension] = 999
    assert message.nested_message.Extensions[extension] == 999

def test_full_keyword_used():
    message = more_messages_pb2.TestFullKeyword()
    message.field2.int_field = 123
