# Protocol Buffers - Google's data interchange format
# Copyright 2022 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# A to_native DataConverter method that raises an error if the value is not of the same type.
# Adapted from to https://www.varvet.com/blog/advanced-topics-in-ruby-ffi/
module Google
  module Protobuf
    module Internal
      module TypeSafety
        def to_native(value, ctx = nil)
          if value.kind_of?(self) or value.nil?
            super
          else
            raise TypeError.new "Expected a kind of #{name}, was #{value.class}"
          end
        end
      end
    end
  end
end

