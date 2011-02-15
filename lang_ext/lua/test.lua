
require "upb"

symtab = upb.symtab()

symtab:add_descriptorproto()
for _, def in ipairs(symtab:getdefs(-1)) do
  print(def:name())
end
