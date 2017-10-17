#!/usr/bin/ruby

# generated_code.rb is in the same directory as this test.
$LOAD_PATH.unshift(File.expand_path(File.dirname(__FILE__)))

require 'generated_code_pb'
require 'test_import_pb'
require 'test/unit'

def bin_to_hex(s)
  s.each_byte.map { |b| b.to_s(16) }.join
end

def hex_to_bin(s)
  s.scan(/../).map { |x| x.hex.chr }.join
end

class GeneratedCodeTest < Test::Unit::TestCase
  def test_generated_msg
    # just test that we can instantiate the message. The purpose of this test
    # is to ensure that the output of the code generator is valid Ruby and
    # successfully creates message definitions and classes, not to test every
    # aspect of the extension (basic.rb is for that).
    m = A::B::C::TestMessage.new()
    m2 = FooBar::TestImportedMessage.new()
    m3 = A::B::C::TestMessage.decode(hex_to_bin('A81F00'))
    puts bin_to_hex(hex_to_bin('A81F'))
    serialized = A::B::C::TestMessage.encode(m3)
    puts bin_to_hex(serialized)
    puts 'finish'
  end
end
