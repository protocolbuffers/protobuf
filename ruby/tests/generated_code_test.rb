#!/usr/bin/ruby

# generated_code.rb is in the same directory as this test.
$LOAD_PATH.unshift(File.expand_path(File.dirname(__FILE__)))

require 'generated_code_pb'
require 'test_import_pb'
require 'test_ruby_package_pb'
require 'test/unit'

class GeneratedCodeTest < Test::Unit::TestCase
  def test_generated_msg
    # just test that we can instantiate the message. The purpose of this test
    # is to ensure that the output of the code generator is valid Ruby and
    # successfully creates message definitions and classes, not to test every
    # aspect of the extension (basic.rb is for that).
    A::B::C::TestMessage.new
    FooBar::TestImportedMessage.new
    A::B::TestRubyPackageMessage.new
  end
end
