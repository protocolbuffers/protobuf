# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

module Google
  module Protobuf
    module MessageExts

      #this is only called in jruby; mri loades the ClassMethods differently
      def self.included(klass)
        klass.extend(ClassMethods)
      end

      module ClassMethods
      end

      def to_json(options = {})
        self.class.encode_json(self, options)
      end

      def to_proto(options = {})
        self.class.encode(self, options)
      end

      def to_hash
        self.to_h
      end

    end
    class AbstractMessage
      include MessageExts
      extend MessageExts::ClassMethods
    end
    private_constant :AbstractMessage
  end
end
