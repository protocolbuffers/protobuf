
local upb = require "upb"
local lunit = require "lunitx"

if _VERSION >= 'Lua 5.2' then
  _ENV = lunit.module("testupb", "seeall")
else
  module("testupb", lunit.testcase, package.seeall)
end

function test_fielddef()
  local f = upb.FieldDef()
  assert_false(f:is_frozen())
  assert_nil(f:number())
  assert_nil(f:name())
  assert_equal(upb.LABEL_OPTIONAL, f:label())

  f:set_name("foo_field")
  f:set_number(3)
  f:set_label(upb.LABEL_REPEATED)
  f:set_type(upb.TYPE_FLOAT)

  assert_equal("foo_field", f:name())
  assert_equal(3, f:number())
  assert_equal(upb.LABEL_REPEATED, f:label())
  assert_equal(upb.TYPE_FLOAT, f:type())

  local f2 = upb.FieldDef{
    name = "foo", number = 5, type = upb.TYPE_DOUBLE, label = upb.LABEL_REQUIRED
  }

  assert_equal("foo", f2:name())
  assert_equal(5, f2:number())
  assert_equal(upb.TYPE_DOUBLE, f2:type())
  assert_equal(upb.LABEL_REQUIRED, f2:label())
end

function test_enumdef()
  local e = upb.EnumDef()
  assert_equal(0, #e)
  assert_nil(e:value(5))
  assert_nil(e:value("NONEXISTENT_NAME"))

  for name, value in e:values() do
    fail()
  end

  e:add("VAL1", 1)
  e:add("VAL2", 2)

  local values = {}
  for name, value in e:values() do
    values[name] = value
  end

  assert_equal(1, values["VAL1"])
  assert_equal(2, values["VAL2"])

  local e2 = upb.EnumDef{
    values = {
      {"FOO", 1},
      {"BAR", 77},
    }
  }

  assert_equal(1, e2:value("FOO"))
  assert_equal(77, e2:value("BAR"))
  assert_equal("FOO", e2:value(1))
  assert_equal("BAR", e2:value(77))
end

function test_empty_msgdef()
  local md = upb.MessageDef()
  assert_nil(md:full_name())  -- Def without name is anonymous.
  assert_false(md:is_frozen())
  assert_equal(0, #md)
  assert_nil(md:field("nonexistent_field"))
  assert_nil(md:field(3))
  for field in md:fields() do
    fail()
  end

  upb.freeze(md)
  assert_true(md:is_frozen())
  assert_equal(0, #md)
  assert_nil(md:field("nonexistent_field"))
  assert_nil(md:field(3))
  for field in md:fields() do
    fail()
  end
end

function test_msgdef_constructor()
  local f1 = upb.FieldDef{name = "field1", number = 7, type = upb.TYPE_INT32}
  local f2 = upb.FieldDef{name = "field2", number = 8, type = upb.TYPE_INT32}
  local md = upb.MessageDef{
    full_name = "TestMessage",
    fields = {f1, f2}
  }
  assert_equal("TestMessage", md:full_name())
  assert_false(md:is_frozen())
  assert_equal(2, #md)
  assert_equal(f1, md:field("field1"))
  assert_equal(f2, md:field("field2"))
  assert_equal(f1, md:field(7))
  assert_equal(f2, md:field(8))
  local count = 0
  local found = {}
  for field in md:fields() do
    count = count + 1
    found[field] = true
  end
  assert_equal(2, count)
  assert_true(found[f1])
  assert_true(found[f2])

  upb.freeze(md)
end

function test_msgdef_setters()
  local md = upb.MessageDef()
  md:set_full_name("Message1")
  assert_equal("Message1", md:full_name())
  local f = upb.FieldDef{name = "field1", number = 3, type = upb.TYPE_DOUBLE}
  md:add{f}
  assert_equal(1, #md)
  assert_equal(f, md:field("field1"))
end

function test_msgdef_errors()
  assert_error(function() upb.MessageDef{bad_initializer_key = 5} end)
  local md = upb.MessageDef()
  assert_error(function()
    -- Duplicate field number.
    upb.MessageDef{
      fields = {
        upb.FieldDef{name = "field1", number = 1, type = upb.TYPE_INT32},
        upb.FieldDef{name = "field2", number = 1, type = upb.TYPE_INT32}
      }
    }
  end)
  assert_error(function()
    -- Duplicate field name.
    upb.MessageDef{
      fields = {
        upb.FieldDef{name = "field1", number = 1, type = upb.TYPE_INT32},
        upb.FieldDef{name = "field1", number = 2, type = upb.TYPE_INT32}
      }
    }
  end)

  -- attempt to set a name with embedded NULLs.
  assert_error_match("names cannot have embedded NULLs", function()
    md:set_full_name("abc\0def")
  end)

  upb.freeze(md)
  -- Attempt to mutate frozen MessageDef.
  -- TODO(haberman): better error message and test for message.
  assert_error(function()
    md:add{upb.FieldDef{name = "field1", number = 1, type = upb.TYPE_INT32}}
  end)
  assert_error(function()
    md:set_full_name("abc")
  end)

  -- Attempt to freeze a msgdef without freezing its subdef.
  assert_error_match("is not frozen or being frozen", function()
    m1 = upb.MessageDef()
    upb.freeze(
      upb.MessageDef{
        fields = {
          upb.FieldDef{name = "f1", number = 1, type = upb.TYPE_MESSAGE,
                       subdef = m1}
        }
      }
    )
  end)
end

function test_symtab()
  local empty = upb.SymbolTable()
  assert_equal(0, #empty:getdefs(upb.DEF_ANY))

  local symtab = upb.SymbolTable{
    upb.MessageDef{full_name = "TestMessage"},
    upb.MessageDef{full_name = "ContainingMessage", fields = {
      upb.FieldDef{name = "field1", number = 1, type = upb.TYPE_INT32},
      upb.FieldDef{name = "field2", number = 2, type = upb.TYPE_MESSAGE,
                   subdef_name = ".TestMessage"}
      }
    }
  }

  local msgdef1 = symtab:lookup("TestMessage")
  local msgdef2 = symtab:lookup("ContainingMessage")
  assert_not_nil(msgdef1)
  assert_not_nil(msgdef2)
  assert_equal(msgdef1, msgdef2:field("field2"):subdef())
  assert_true(msgdef1:is_frozen())
  assert_true(msgdef2:is_frozen())

  symtab:add{
    upb.MessageDef{full_name = "ContainingMessage2", fields = {
      upb.FieldDef{name = "field5", number = 5, type = upb.TYPE_MESSAGE,
                   subdef = msgdef2}
      }
    }
  }

  local msgdef3 = symtab:lookup("ContainingMessage2")
  assert_not_nil(msgdef3)
  assert_equal(msgdef3:field("field5"):subdef(), msgdef2)
end

-- Lua 5.1 and 5.2 have slightly different semantics for how a finalizer
-- can be defined in Lua.
if _VERSION >= 'Lua 5.2' then
  function defer(fn)
    setmetatable({}, { __gc = fn })
  end
else
  function defer(fn)
    getmetatable(newproxy(true)).__gc = fn
  end
end

function test_finalizer()
  -- Tests that we correctly handle a call into an already-finalized object.
  -- Collectible objects are finalized in the opposite order of creation.
  do
    local t = {}
    defer(function()
      assert_error_match("called into dead def", function()
        -- Generic def call.
        t[1]:full_name()
      end)
      assert_error_match("called into dead msgdef", function()
        -- Specific msgdef call.
        t[1]:add()
      end)
      assert_error_match("called into dead enumdef", function()
        t[2]:values()
      end)
      assert_error_match("called into dead fielddef", function()
        t[3]:number()
      end)
      assert_error_match("called into dead symtab",
        function() t[4]:lookup()
      end)
    end)
    t = {
      upb.MessageDef(),
      upb.EnumDef(),
      upb.FieldDef(),
      upb.SymbolTable(),
    }
  end
  collectgarbage()
end

lunit.main()
