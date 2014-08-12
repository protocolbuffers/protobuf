# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
# http://code.google.com/p/protobuf/
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

"""Determine which implementation of the protobuf API is used in this process.
"""

import os
import sys

try:
  # pylint: disable=g-import-not-at-top
  from google.protobuf.internal import _api_implementation
  # The compile-time constants in the _api_implementation module can be used to
  # switch to a certain implementation of the Python API at build time.
  _api_version = _api_implementation.api_version
  del _api_implementation
except ImportError:
  _api_version = 0

_default_implementation_type = (
    'python' if _api_version == 0 else 'cpp')
_default_version_str = (
    '1' if _api_version <= 1 else '2')

# This environment variable can be used to switch to a certain implementation
# of the Python API, overriding the compile-time constants in the
# _api_implementation module. Right now only 'python' and 'cpp' are valid
# values. Any other value will be ignored.
_implementation_type = os.getenv('PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION',
                                 _default_implementation_type)

if _implementation_type != 'python':
  _implementation_type = 'cpp'

# This environment variable can be used to switch between the two
# 'cpp' implementations, overriding the compile-time constants in the
# _api_implementation module. Right now only 1 and 2 are valid values. Any other
# value will be ignored.
_implementation_version_str = os.getenv(
    'PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION_VERSION',
    _default_version_str)

if _implementation_version_str not in ('1', '2'):
  raise ValueError(
      "unsupported PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION_VERSION: '" +
      _implementation_version_str + "' (supported versions: 1, 2)"
      )

_implementation_version = int(_implementation_version_str)


# Usage of this function is discouraged. Clients shouldn't care which
# implementation of the API is in use. Note that there is no guarantee
# that differences between APIs will be maintained.
# Please don't use this function if possible.
def Type():
  return _implementation_type


# See comment on 'Type' above.
def Version():
  return _implementation_version
