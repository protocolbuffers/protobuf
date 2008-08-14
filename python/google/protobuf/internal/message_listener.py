# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.
# http://code.google.com/p/protobuf/
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Defines a listener interface for observing certain
state transitions on Message objects.

Also defines a null implementation of this interface.
"""

__author__ = 'robinson@google.com (Will Robinson)'


class MessageListener(object):

  """Listens for transitions to nonempty and for invalidations of cached
  byte sizes.  Meant to be registered via Message._SetListener().
  """

  def TransitionToNonempty(self):
    """Called the *first* time that this message becomes nonempty.
    Implementations are free (but not required) to call this method multiple
    times after the message has become nonempty.
    """
    raise NotImplementedError

  def ByteSizeDirty(self):
    """Called *every* time the cached byte size value
    for this object is invalidated (transitions from being
    "clean" to "dirty").
    """
    raise NotImplementedError


class NullMessageListener(object):

  """No-op MessageListener implementation."""

  def TransitionToNonempty(self):
    pass

  def ByteSizeDirty(self):
    pass
