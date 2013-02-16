
local actions = {}

local atexit

if _VERSION >= 'Lua 5.2' then 

    atexit = function (fn)
        actions[#actions+1] = setmetatable({}, { __gc = fn })
    end
    
else

    local newproxy = newproxy
    local debug = debug
    local assert = assert
    local setmetatable = setmetatable

    local function gc(fn)
        local p = assert(newproxy())
        assert(debug.setmetatable(p, { __gc = fn }))
        return p
    end

    atexit = function (fn)
        actions[#actions+1] = gc(fn)
    end
    
end

return atexit

