
require "upb"

symtab = upb.SymbolTable{
  upb.MessageDef{fqname="A", fields={
    upb.FieldDef{name="a", type=upb.TYPE_INT32, number=1},
    upb.FieldDef{name="b", type=upb.TYPE_DOUBLE, number=2}}
  }
}

symtab = upb.SymbolTable{
  upb.MessageDef{fqname="A", fields={
    upb.FieldDef{name="a", type=upb.TYPE_INT32, number=1},
    upb.FieldDef{name="b", type=upb.TYPE_DOUBLE, number=2}}
  },
  upb.MessageDef{fqname="B"}
}
A, B, C = symtab:lookup("A", "B")
print(A)
print(B)
print(C)

a = A()
a2 = upb.Message(A)
print("YO!  a.a=" .. tostring(a.a) .. ", a2.a=" .. tostring(a2.a))
a.a = 2
a2.a = 3
print("YO!  a.a=" .. tostring(a.a) .. ", a2.a=" .. tostring(a2.a))

A = symtab:lookup("A")
if not A then
  error("Could not find A")
end

f = io.open("../../upb/descriptor.pb")
if not f then
  error("Couldn't open descriptor.pb, try running 'make descriptorgen'")
end
symtab:parsedesc(f:read("*all"))
symtab:load_descriptor()
symtab:load_descriptor_file()

upb.pb.load_descriptor(f:read("*all"))

upb.pb.load_descriptor_file("../../src/descriptor.pb", symtab)

f = io.open("../../benchmarks/google_messages.proto.pb")
if not f then
  error("Couldn't open google_messages.proto.pb, try running 'make benchmarks'")
end
symtab:parsedesc(f:read("*all"))

for _, def in ipairs(symtab:getdefs(-1)) do
  print(def:name())
end

SpeedMessage1 = symtab:lookup("benchmarks.SpeedMessage1")
SpeedMessage2 = symtab:lookup("benchmarks.SpeedMessage2")
print(SpeedMessage1:name())

msg = MyType()
msg:Decode(str)

msg:DecodeJSON(str)

msg = upb.pb.decode(str, MyType)
str = upb.pb.encode(msg)

msg = upb.pb.decode_text(str, MyType)
str = upb.pb.encode_text(msg)

upb.clear(msg)
upb.msgdef(msg)
upb.has(msg, "foo_bar")

msg = upb.json.decode(str, MyType)

msg = upb.pb.DecodeText(str)
msg = upb.pb.EncodeText(msg)
upb.

upb.pb.decode_into(msg, str)

str = upb.json.Encode(msg)
upb.json.DecodeInto(msg, str)
f = assert(io.open("../../benchmarks/google_message1.dat"))
msg:Parse(f:read("*all"))
print(msg:ToText())
print(upb.json.encode(msg))

msg = SpeedMessage2()
f = assert(io.open("../../benchmarks/google_message2.dat"))
msg:Parse(f:read("*all"))
print(msg:ToText())
--msg:Serialize()
--msg:FromText(str)
-- print(msg.field129)
-- print(msg.field271)
--print(msg.field15.field15)
--msg.field15.field15 = "my override"
--print(msg.field15.field15)
-- print(msg.field1)
-- print(msg.field1)
-- msg.field1 = "YEAH BABY!"
-- print(msg.field1)
-- print(msg.field129)
-- msg.field129 = 5
-- print(msg.field129)
--]]
