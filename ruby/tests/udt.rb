# Protocol Buffers - Google's data interchange format
# Copyright 2014 Google Inc.  All rights reserved.
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

require 'google/protobuf'
require 'test/unit'

require 'time'  # for Time UDT below

# TODO: implement UDT API in JRuby extension as well!
if RUBY_PLATFORM != "java"
  module UdtTest
    Epoch = Time.parse('1970-01-01 00:00:00 UTC')

    pool = Google::Protobuf::DescriptorPool.new
    pool.build do
      add_message "Container" do
        optional :optional, :message, 1, "ProtoTime"
        repeated :repeated, :message, 2, "ProtoTime"
        map :map, :string, :message, 3, "ProtoTime"
      end

      add_message "ProtoTime" do
        optional :rfc822, :string, 1

        udt_parse_hook do |wiremsg|
          # Parse the `rfc822` string field from the wire message into a Ruby
          # `Time` object and return that.
          Time.parse(wiremsg.rfc822)
        end
        udt_serialize_hook do |t, klass|
          # Create a new ProtoTime message from `t`, which is a ruby `Time`
          # object, using `Time#rfc822` to get an RFC-822 string.
          klass.new(:rfc822 => t.rfc822)
        end
        udt_verify_hook do |t|
          if t.class != Time
            raise TypeError.new('Invalid type for ProtoTime field')
          end
        end
      end

      # UDT containing embedded UDTs. This tests the conversion order: at parse,
      # submessages should be converted from wire to user types first, so the
      # parent message sees user types. At serialize, submessages should be
      # converted from user to wire types last, so the parent message can produce
      # user types and they will be consumed properly.
      #
      # The corresponding user type for this message class is a simple Ruby hash
      # with :start_time and :end_time keys, but it could easily be some more
      # elaborate `Duration` or `TimeSpan` class.
      add_message "ProtoTimeSpan" do
        optional :start, :message, 1, "ProtoTime"
        optional :end, :message, 2, "ProtoTime"

        udt_parse_hook do |wiremsg|
          # note that `wiremsg.start` and `wiremsg.end` are already converted to
          # their user types (Ruby `Time` objects) at this point.
          { :start_time => wiremsg.start || Epoch,
            :end_time => wiremsg.end || Epoch }
        end
        udt_serialize_hook do |t, klass|
          klass.new(:start => t[:start_time] || Epoch,
                    :end => t[:end_time] || Epoch)
        end
        udt_verify_hook do |t|
          if t[:start_time].class != Time || t[:end_time].class != Time
            raise TypeError.new('Invalid value for ProtoTimeSpan field')
          end
          if t[:end_time] < t[:start_time]
            raise ArgumentError.new('TimeSpan start/end times are reversed')
          end
        end
      end

      add_message "ProtoTimeSpanContainer" do
        optional :m, :message, 1, "ProtoTimeSpan"
      end
    end

    Container = pool.lookup("Container").msgclass
    ProtoTime = pool.lookup("ProtoTime").msgclass
    ProtoTimeSpan = pool.lookup("ProtoTimeSpan").msgclass
    ProtoTimeSpanContainer = pool.lookup("ProtoTimeSpanContainer").msgclass

  # ------------ test cases ---------------

    class UDTTest < Test::Unit::TestCase
      def test_udt_defaults
        # Creating a message with UDT-typed fields gives default-valued (nil)
        # fields.
        c = Container.new
        assert_equal c.optional, nil
        assert_equal c.repeated, []
        assert_equal c.map.length, 0

        # Creating the UDT directly still gives us the raw class.
        p = ProtoTime.new
        assert_equal p.class, ProtoTime

        c = ProtoTimeSpanContainer.new
        assert_equal c.m, nil
      end

      def test_udt_ctor_arg
        t1 = Time.parse('2015-01-01 12:34:56 -0800')
        t2 = Time.parse('2015-01-01 12:35:00 -0800')
        c = Container.new(:optional => t1, :repeated => [t2, t1],
                          :map => { "a" => t1, "b" => t2 })
        assert_equal c.optional, t1
        assert_equal c.repeated.to_a, [t2, t1]
        assert_equal c.map["a"], t1
        assert_equal c.map["b"], t2
      end

      def test_inspect
        t1 = Time.parse('2015-01-01 12:34:56 -0800')
        t2 = Time.parse('2015-01-01 12:35:00 -0800')
        c = Container.new(:optional => t1, :repeated => [t2, t1],
                          # only one map arg for deterministic output
                          :map => { "a" => t1 })
        assert_equal c.inspect,
          '<UdtTest::Container: optional: 2015-01-01 12:34:56 -0800, ' +
          'repeated: [2015-01-01 12:35:00 -0800, 2015-01-01 12:34:56 -0800], ' +
          'map: {"a"=>2015-01-01 12:34:56 -0800}>'
      end

      def test_udt_nil
        c = Container.new
        assert_equal c.optional, nil
        t1 = Time.parse('2015-01-01 12:34:56 -0800')
        c.optional = t1
        assert_equal(c.optional, t1)
        c.optional = nil
        assert_equal(c.optional, nil)

        assert_raise TypeError do
          c.repeated.push nil
        end
        assert_raise TypeError do
          c.map["a"] = nil
        end
      end

      def test_udt_fieldassign
        c = Container.new
        t1 = Time.parse('2015-01-01 12:34:56 -0800')
        c.optional = t1
        assert_equal c.optional, t1
        c.repeated.push t1
        assert_equal c.repeated.to_a, [t1]
        c.repeated.replace([t1, t1, t1])
        assert_equal c.repeated.to_a, [t1, t1, t1]
        c.map["a"] = t1
        assert_equal c.map["a"], t1

        assert_raise TypeError do
          # *only* user-visible types are valid on this side of the
          # parse/serialize converter hooks
          c.optional = ProtoTime.new
        end
      end

      def test_udt_rptfield
        r = Google::Protobuf::RepeatedField.new(:message, ProtoTime)
        r.push(Time.parse('1970-07-01 00:00:00'))
        r.push(Time.parse('1980-07-01 00:00:00'))
        assert_equal r.length, 2
        assert_equal r[0].year, 1970
        assert_equal r[1].year, 1980
        assert_raise TypeError do
          r[0] = "asdf"
        end
        assert_raise TypeError do
          # *only* user-visible types are valid on this side of the
          # parse/serialize converter hooks
          r[0] = ProtoTime.new
        end
      end

      def test_udt_map
        m = Google::Protobuf::Map.new(:string, :message, ProtoTime)
        m["a"] = Time.parse('1970-07-01 00:00:00')
        m["b"] = Time.parse('1980-07-01 00:00:00')
        assert_equal m.length, 2
        assert_equal m["a"].year, 1970
        assert_equal m["b"].year, 1980
        assert_raise TypeError do
          m["c"] = "asdf"
        end
        assert_raise TypeError do
          # *only* user-visible types are valid on this side of the
          # parse/serialize converter hooks
          m["c"] = ProtoTime.new
        end
      end

      def test_udt_verify
        t1 = Time.parse('1970-07-01 00:00:00')
        t2 = Time.parse('1970-08-01 00:00:00')
        c = ProtoTimeSpanContainer.new
        c.m = { :start_time => t1, :end_time => t2 }
        assert_raise TypeError do
          c.m = { :start_time => nil, :end_time => t2 }
        end
        assert_raise TypeError do
          c.m = { :start_time => "asdf", :end_time => t2 }
        end
        assert_raise ArgumentError do
          # reversed start/end
          c.m = { :start_time => t2, :end_time => t1 }
        end
      end

      def test_udt_roundtrip
        t1 = Time.parse('1970-07-01 00:00:00')
        t2 = Time.parse('1970-08-01 00:00:00')
        c = ProtoTimeSpanContainer.new(
          :m => { :start_time => t1, :end_time => t2 }
        )
        encoded = ProtoTimeSpanContainer.encode(c)
        decoded = ProtoTimeSpanContainer.decode(encoded)
        assert_equal c, decoded
      end

      def test_incomplete_hooks
        p = Google::Protobuf::DescriptorPool.new
        assert_raise RuntimeError do
          p.build do
            add_message "MyUDT" do
              optional :foo, :int32, 1
              udt_verify_hook do |t|
              end
              # no udt_parse_hook or udt_serialize_hook
            end
          end
          udt = p.lookup("MyUDT").msgclass
        end
      end
    end
  end
end
