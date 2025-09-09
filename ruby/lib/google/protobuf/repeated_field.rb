# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

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
        :assoc, :bsearch, :bsearch_index, :combination, :compact, :count,
        :cycle, :difference, :dig, :drop, :drop_while, :eql?, :fetch, :find_index, :flatten,
        :include?, :index, :inspect, :intersection, :join,
        :pack, :permutation, :product, :pretty_print, :pretty_print_cycle,
        :rassoc, :repeated_combination, :repeated_permutation, :reverse,
        :rindex, :rotate, :sample, :shuffle, :shelljoin,
        :to_s, :transpose, :union, :uniq, :|


      def first(n=nil)
        if n.nil?
          return self[0]
        elsif n < 0
          raise ArgumentError, "negative array size"
        else
          return self[0...n]
        end
      end


      def last(n=nil)
        if n.nil?
          return self[-1]
        elsif n < 0
          raise ArgumentError, "negative array size"
        else
          start = [self.size-n, 0].max
          return self[start...self.size]
        end
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


      %w(delete delete_at shift slice! unshift).each do |method_name|
        define_array_wrapper_method(method_name)
      end


      %w(collect! compact! delete_if each_index fill flatten! insert reverse!
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
            #nil means no change occurred from yield; usually occurs when #to_a is called
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
