# Protocol Buffers - Google's data interchange format
# Copyright 2024 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
"""proto_plugin rule"""

load("//bazel/private:proto_plugin_rule.bzl", _ProtoPluginInfo = "ProtoPluginInfo", _proto_plugin = "proto_plugin")

ProtoPluginInfo = _ProtoPluginInfo
proto_plugin = _proto_plugin
