
local upb = require("lupb")

upb.generated_pool = upb.SymbolTable()

local module_metatable = {
  __index = function(t, k)
    local package = t._filedef:package()
    if package then
      k = package .. "." .. k
    end
    local pool = upb.generated_pool
    local def = pool:lookup_msg(k) or pool:lookup_enum(k)
    local v = nil
    if def and def:file():name() == t._filedef:name() then
      v = def
      t[k] = v
    end
    return v
  end
}

function upb._generated_module(desc_string)
  local file = upb.generated_pool:add_file(desc_string)
  local module = {_filedef = file}
  setmetatable(module, module_metatable)
  return module
end

return upb
