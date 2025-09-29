# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Determine which implementation of the protobuf API is used in this process.
"""

import importlib
import os
import sys
import warnings

_GOOGLE3_PYTHON_UPB_DEFAULT = True


def _ApiVersionToImplementationType(api_version):
  if api_version == 2:
    return 'cpp'
  if api_version == 1:
    raise ValueError('api_version=1 is no longer supported.')
  if api_version == 0:
    return 'python'
  return None


_implementation_type = None
try:
  # pylint: disable=g-import-not-at-top
  from google.protobuf.internal import _api_implementation
  # The compile-time constants in the _api_implementation module can be used to
  # switch to a certain implementation of the Python API at build time.
  _implementation_type = _ApiVersionToImplementationType(
      _api_implementation.api_version)
except ImportError:
  pass  # Unspecified by compiler flags.


def _CanImport(mod_name):
  try:
    mod = importlib.import_module(mod_name)
    # Work around a known issue in the classic bootstrap .par import hook.
    if not mod:
      raise ImportError(mod_name + ' import succeeded but was None')
    return True
  except ImportError:
    return False


if _implementation_type is None:
  if _CanImport('google._upb._message'):
    _implementation_type = 'upb'
  elif _CanImport('google.protobuf.pyext._message'):
    _implementation_type = 'cpp'
  else:
    _implementation_type = 'python'


# This environment variable can be used to switch to a certain implementation
# of the Python API, overriding the compile-time constants in the
# _api_implementation module. Right now only 'python', 'cpp' and 'upb' are
# valid values. Any other value will raise error.
_implementation_type = os.getenv('PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION',
                                 _implementation_type)

if _implementation_type not in ('python', 'cpp', 'upb'):
  raise ValueError('PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION {0} is not '
                   'supported. Please set to \'python\', \'cpp\' or '
                   '\'upb\'.'.format(_implementation_type))

if 'PyPy' in sys.version and _implementation_type == 'cpp':
  warnings.warn('PyPy does not work yet with cpp protocol buffers. '
                'Falling back to the python implementation.')
  _implementation_type = 'python'

_c_module = None

if _implementation_type == 'cpp':
  try:
    # pylint: disable=g-import-not-at-top
    from google.protobuf.pyext import _message
    sys.modules['google3.net.proto2.python.internal.cpp._message'] = _message
    _c_module = _message
    del _message
  except ImportError:
    # TODO: fail back to python
    warnings.warn(
        'Selected implementation cpp is not available.')
    pass

if _implementation_type == 'upb':
  try:
    # pylint: disable=g-import-not-at-top
    from google._upb import _message
    _c_module = _message
    del _message
  except ImportError:
    warnings.warn('Selected implementation upb is not available. '
                  'Falling back to the python implementation.')
    _implementation_type = 'python'
    pass

# Detect if serialization should be deterministic by default
try:
  # The presence of this module in a build allows the proto implementation to
  # be upgraded merely via build deps.
  #
  # NOTE: Merely importing this automatically enables deterministic proto
  # serialization for C++ code, but we still need to export it as a boolean so
  # that we can do the same for `_implementation_type == 'python'`.
  #
  # NOTE2: It is possible for C++ code to enable deterministic serialization by
  # default _without_ affecting Python code, if the C++ implementation is not in
  # use by this module.  That is intended behavior, so we don't actually expose
  # this boolean outside of this module.
  #
  # pylint: disable=g-import-not-at-top,unused-import
  from google.protobuf import enable_deterministic_proto_serialization
  _python_deterministic_proto_serialization = True
except ImportError:
  _python_deterministic_proto_serialization = False


# Usage of this function is discouraged. Clients shouldn't care which
# implementation of the API is in use. Note that there is no guarantee
# that differences between APIs will be maintained.
# Please don't use this function if possible.
def Type():
  return _implementation_type


# For internal use only
def IsPythonDefaultSerializationDeterministic():
  return _python_deterministic_proto_serialization
