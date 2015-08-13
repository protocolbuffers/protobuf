# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
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

require 'forwardable'

#
# This class makes RepeatedField act (almost-) like a Ruby Array.
# It has convenience methods that extend the core C or Java based
# methods.
#
# This is a best-effort to mirror Array behavior.  Two comments:
#  1) patches always welcome :)
#  2) if performance is an issue, feel free to rewrite the method
#     in jruby and C.  The source code has plenty of examples
#
# KNOWN ISSUES
#   - #[]= doesn't allow less used approaches such as `arr[1, 2] = 'fizz'`
#   - #concat should return the orig array
#   - #push should accept multiple arguments and push them all at the same time
#
module Google
  module Protobuf
    class RepeatedField
      extend Forwardable

      # methods defined in C or Java:
      #   +
      #   [], at
      #   []=
      #   concat
      #   clear
      #   dup, clone
      #   each
      #   push, <<
      #   replace
      #   length, size
      #   ==
      #   to_ary, to_a
      #   also all enumerable
      #
      # NOTE:  using delegators rather than method_missing to make the
      #        relationship explicit instead of implicit
      def_delegators :to_ary,
        :&, :*, :-, :'<=>',
        :assoc, :bsearch, :combination, :compact, :count, :cycle,
        :drop, :drop_while, :eql?, :fetch, :find_index, :flatten,
        :include?, :index, :inspect, :join,
        :pack, :permutation, :product, :pretty_print, :pretty_print_cycle,
        :rassoc, :repeated_combination, :repeated_permutation, :reverse,
        :rindex, :rotate, :sample, :shuffle, :shelljoin, :slice,
        :to_s, :transpose, :uniq, :|


      def first(n=nil)
        n ? self[0..n] : self[0]
      end


      def last(n=nil)
        n ? self[(self.size-n-1)..-1] : self[-1]
      end


      def pop(n=nil)
        if n
          results = []
          n.times{ results << pop_one }
          return results
        else
          return pop_one
        end
      end


      def empty?
        self.size == 0
      end

      # array aliases into enumerable
      alias_method :each_index, :each_with_index
      alias_method :slice, :[]
      alias_method :values_at, :select
      alias_method :map, :collect


      class << self
        def define_array_wrapper_method(method_name)
          define_method(method_name) do |*args, &block|
            arr = self.to_a
            result = arr.send(method_name, *args)
            self.replace(arr)
            return result if result
            return block ? block.call : result
          end
        end
        private :define_array_wrapper_method


        def define_array_wrapper_with_result_method(method_name)
          define_method(method_name) do |*args, &block|
            # result can be an Enumerator, Array, or nil
            # Enumerator can sometimes be returned if a block is an optional argument and it is not passed in
            # nil usually specifies that no change was made
            result = self.to_a.send(method_name, *args, &block)
            if result
              new_arr = result.to_a
              self.replace(new_arr)
              if result.is_a?(Enumerator)
                # generate a fresh enum; rewinding the exiting one, in Ruby 2.2, will
                # reset the enum with the same length, but all the #next calls will
                # return nil
                result = new_arr.to_enum
                # generate a wrapper enum so any changes which occur by a chained
                # enum can be captured
                ie = ProxyingEnumerator.new(self, result)
                result = ie.to_enum
              end
            end
            result
          end
        end
        private :define_array_wrapper_with_result_method
      end


      %w(delete delete_at delete_if shift slice! unshift).each do |method_name|
        define_array_wrapper_method(method_name)
      end


      %w(collect! compact! fill flatten! insert reverse!
        rotate! select! shuffle! sort! sort_by! uniq!).each do |method_name|
        define_array_wrapper_with_result_method(method_name)
      end
      alias_method :keep_if, :select!
      alias_method :map!, :collect!
      alias_method :reject!, :delete_if


      # propagates changes made by user of enumerator back to the original repeated field.
      # This only applies in cases where the calling function which created the enumerator,
      # such as #sort!, modifies itself rather than a new array, such as #sort
      class ProxyingEnumerator < Struct.new(:repeated_field, :external_enumerator)
        def each(*args, &block)
          results = []
          external_enumerator.each_with_index do |val, i|
            result = yield(val)
            results << result
            #nil means no change occured from yield; usually occurs when #to_a is called
            if result
              repeated_field[i] = result if result != val
            end
          end
          results
        end
      end


    end
  end
end
