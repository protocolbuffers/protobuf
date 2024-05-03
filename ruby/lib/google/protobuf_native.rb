# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

if RUBY_PLATFORM == "java"
  require 'json'
  require 'google/protobuf_java'
else
  begin
    require "google/#{RUBY_VERSION.sub(/\.\d+$/, '')}/protobuf_c"
  rescue LoadError
    require 'google/protobuf_c'
  end
end

require 'google/protobuf/descriptor_dsl'
require 'google/protobuf/repeated_field'
