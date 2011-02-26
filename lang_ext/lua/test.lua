
require "upb"

symtab = upb.symtab()

f = io.open("../../src/descriptor.pb")
if not f then
  error("Couldn't open descriptor.pb, try running 'make descriptorgen'")
end
symtab:parsedesc(f:read("*all"))

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

msg = SpeedMessage1()
f = assert(io.open("../../benchmarks/google_message1.dat"))
msg:Parse(f:read("*all"))
print(msg:ToText())

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

