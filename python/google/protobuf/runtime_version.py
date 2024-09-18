# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Protobuf Runtime versions and validators.

It should only be accessed by Protobuf gencodes and tests. DO NOT USE it
elsewhere.
"""

__author__ = 'shaod@google.com (Dennis Shao)'

from enum import Enum
import os


class Domain(Enum):
  GOOGLE_INTERNAL = 1
  PUBLIC = 2


class VersionError(Exception):
  """Exception class for version violation."""


# The versions of this Python Protobuf runtime to be changed automatically by
# the Protobuf release process. Do not edit them manually.
DOMAIN = Domain.PUBLIC
MAJOR = 5
MINOR = 27
PATCH = 5
SUFFIX = ''


def ValidateProtobufRuntimeVersion(
    gen_domain, gen_major, gen_minor, gen_patch, gen_suffix, location
):
  """Function to validate versions.

  Args:
    gen_domain: The domain where the code was generated from.
    gen_major: The major version number of the gencode.
    gen_minor: The minor version number of the gencode.
    gen_patch: The patch version number of the gencode.
    gen_suffix: The version suffix e.g. '-dev', '-rc1' of the gencode.
    location: The proto location that causes the version violation.

  Raises:
    VersionError: if gencode version is invalid or incompatible with the
    runtime.
  """

  disable_flag = os.getenv('TEMORARILY_DISABLE_PROTOBUF_VERSION_CHECK')
  if disable_flag is not None and disable_flag.lower() == 'true':
    return

  version = f'{MAJOR}.{MINOR}.{PATCH}{SUFFIX}'
  gen_version = f'{gen_major}.{gen_minor}.{gen_patch}{gen_suffix}'

  if gen_major < 0 or gen_minor < 0 or gen_patch < 0:
    raise VersionError(f'Invalid gencode version: {gen_version}')

  error_prompt = (
      'See Protobuf version guarantees at'
      ' https://protobuf.dev/support/cross-version-runtime-guarantee.'
  )

  if gen_domain != DOMAIN:
    raise VersionError(
        'Detected mismatched Protobuf Gencode/Runtime domains when loading'
        f' {location}: gencode {gen_domain.name} runtime {DOMAIN.name}.'
        ' Cross-domain usage of Protobuf is not supported.'
    )

  if gen_major != MAJOR:
    raise VersionError(
        'Detected mismatched Protobuf Gencode/Runtime major versions when'
        f' loading {location}: gencode {gen_version} runtime {version}.'
        f' Same major version is required. {error_prompt}'
    )

  if MINOR < gen_minor or (MINOR == gen_minor and PATCH < gen_patch):
    raise VersionError(
        'Detected incompatible Protobuf Gencode/Runtime versions when loading'
        f' {location}: gencode {gen_version} runtime {version}. Runtime version'
        f' cannot be older than the linked gencode version. {error_prompt}'
    )

  if gen_suffix != SUFFIX:
    raise VersionError(
        'Detected mismatched Protobuf Gencode/Runtime version suffixes when'
        f' loading {location}: gencode {gen_version} runtime {version}.'
        f' Version suffixes must be the same. {error_prompt}'
    )
