#!/usr/bin/ruby

require 'google/protobuf'
require 'benchmark'

puts "Protobuf Benchmarks for Ruby Platform #{RUBY_PLATFORM}"

pool = Google::Protobuf::DescriptorPool.new
pool.build do
  add_message "BenchmarkMessage" do
    repeated :a,  :message,      1, "BenchmarkSubMessage"
    optional :b,  :int32,        2
    optional :c,  :uint32,       3
    optional :d,  :int64,        4
    optional :e,  :uint64,       5
    optional :f,  :float,        6
    optional :g,  :double,       7
    optional :h,  :enum,         8, "BenchmarkEnum"
    optional :i,  :string,       9
    optional :j,  :bytes,       10
  end
  add_enum "BenchmarkEnum" do
    value :Default, 0
    value :A, 1
    value :B, 2
    value :C, 3
  end
  add_message "BenchmarkSubMessage" do
    optional :foo, :string, 1
  end
end

BenchmarkMessage = pool.lookup("BenchmarkMessage").msgclass
BenchmarkSubMessage = pool.lookup("BenchmarkSubMessage").msgclass

def get_msg
  BenchmarkMessage.new(
                  a: [BenchmarkSubMessage.new(:foo => "hello"),
                      BenchmarkSubMessage.new(:foo => "world")],
                  b: -1000,
                  c: 1000,
                  d: -10_000_000_000,
                  e: 10_000_000_000,
                  f: 1.123,
                  g: 1.123456,
                  h: :C,
                  i: "❤️",
                  j: "\0\0\0"
  )
end

foo = get_msg
string = "hi"
byte_string = "\0\0\0\0\0\0"
n = 1000
Benchmark.bmbm do |x|
  x.report("inspect") { n.times { foo.inspect } }
  x.report("to_h") { n.times { foo.to_h } }
  x.report("encode_json") { n.times { foo.to_json } }
  x.report("new") { n.times { get_msg } }
  x.report("dup") { n.times { foo.dup } }
  x.report("repeated field accessor") { n.times { foo.a } }
  x.report("repeated field index accessor") { n.times { foo.a[1] } }
  x.report("repeated field index + string accessor") { n.times { foo.a[1].foo } }
  x.report("int32 accessor") { n.times { foo.b } }
  x.report("uint32 accessor") { n.times { foo.c } }
  x.report("int64 accessor") { n.times { foo.d } }
  x.report("uint64 accessor") { n.times { foo.e } }
  x.report("float accessor") { n.times { foo.f } }
  x.report("double accessor") { n.times { foo.g } }
  x.report("enum accessor") { n.times { foo.h } }
  x.report("string accessor") { n.times { foo.i } }
  x.report("bytes accessor") { n.times { foo.j } }

  x.report("int32 writer") { n.times { foo.b = -42 } }
  x.report("uint32 writer") { n.times { foo.c = 42 } }
  x.report("int64 writer") { n.times { foo.d = -42_000_000_000 } }
  x.report("uint64 writer") { n.times { foo.e = 42_000_000_000 } }
  x.report("float writer") { n.times { foo.f = 9.8765 } }
  x.report("double writer") { n.times { foo.g = -9.8765432109 } }
  x.report("enum writer") { n.times { foo.h = :B } }
  x.report("string writer") { n.times { foo.i = string } }
  x.report("bytes writer") { n.times { foo.j = byte_string } }
end
