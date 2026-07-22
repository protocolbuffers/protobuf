# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""allocation_count helper for testing OOM/allocation failures."""

_upb_allocation_count = None
try:
  from google.protobuf.internal import api_implementation
  if api_implementation.Type() == 'upb':
    from google._upb import _message
    _upb_allocation_count = _message
except ImportError:
  pass


def is_available():
  if _upb_allocation_count is not None:
    return _upb_allocation_count._AllocationCount_IsAvailable()
  return False


def get():
  if _upb_allocation_count is not None:
    return _upb_allocation_count._AllocationCount_Get()
  return 0


def reset():
  if _upb_allocation_count is not None:
    _upb_allocation_count._AllocationCount_Reset()


def fail_on(n):
  if _upb_allocation_count is not None:
    _upb_allocation_count._AllocationCount_FailOn(n)
