
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
