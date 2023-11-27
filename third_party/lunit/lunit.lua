--[[--------------------------------------------------------------------------

    This file is part of lunit 0.5.

    For Details about lunit look at: http://www.mroth.net/lunit/

    Author: Michael Roth <mroth@nessie.de>

    Copyright (c) 2004, 2006-2010 Michael Roth <mroth@nessie.de>

    Permission is hereby granted, free of charge, to any person 
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without restriction,
    including without limitation the rights to use, copy, modify, merge,
    publish, distribute, sublicense, and/or sell copies of the Software,
    and to permit persons to whom the Software is furnished to do so,
    subject to the following conditions:

    The above copyright notice and this permission notice shall be 
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

--]]--------------------------------------------------------------------------


local orig_assert     = assert

local pairs           = pairs
local ipairs          = ipairs
local next            = next
local type            = type
local error           = error
local tostring        = tostring
local setmetatable    = setmetatable
local pcall           = pcall
local xpcall          = xpcall
local require         = require
local loadfile        = loadfile

local string_sub      = string.sub
local string_gsub     = string.gsub
local string_format   = string.format
local string_lower    = string.lower
local string_find     = string.find

local table_concat    = table.concat

local debug_getinfo   = debug.getinfo

local _G = _G

local lunit

if _VERSION >= 'Lua 5.2' then 

    lunit = {}
    _ENV = lunit
    
else

    module("lunit")
    lunit = _M
    
end


local __failure__ = {}    -- Type tag for failed assertions

local typenames = { "nil", "boolean", "number", "string", "table", "function", "thread", "userdata" }


local traceback_hide      -- Traceback function which hides lunit internals
local mypcall             -- Protected call to a function with own traceback
do
  local _tb_hide = setmetatable( {}, {__mode="k"} )

  function traceback_hide(func)
    _tb_hide[func] = true
  end

  local function my_traceback(errobj)
    if is_table(errobj) and errobj.type == __failure__ then
      local info = debug_getinfo(5, "Sl")   -- FIXME: Hardcoded integers are bad...
      errobj.where = string_format( "%s:%d", info.short_src, info.currentline)
    else
      errobj = { msg = tostring(errobj) }
      errobj.tb = {}
      local i = 2
      while true do
        local info = debug_getinfo(i, "Snlf")
        if not is_table(info) then
          break
        end
        if not _tb_hide[info.func] then
          local line = {}       -- Ripped from ldblib.c...
          line[#line+1] = string_format("%s:", info.short_src)
          if info.currentline > 0 then
            line[#line+1] = string_format("%d:", info.currentline)
          end
          if info.namewhat ~= "" then
            line[#line+1] = string_format(" in function '%s'", info.name)
          else
            if info.what == "main" then
              line[#line+1] = " in main chunk"
            elseif info.what == "C" or info.what == "tail" then
              line[#line+1] = " ?"
            else
              line[#line+1] = string_format(" in function <%s:%d>", info.short_src, info.linedefined)
            end
          end
          errobj.tb[#errobj.tb+1] = table_concat(line)
        end
        i = i + 1
      end
    end
    return errobj
  end

  function mypcall(func)
    orig_assert( is_function(func) )
    local ok, errobj = xpcall(func, my_traceback)
    if not ok then
      return errobj
    end
  end
  traceback_hide(mypcall)
end


-- Type check functions

for _, typename in ipairs(typenames) do
  lunit["is_"..typename] = function(x)
    return type(x) == typename
  end
end

local is_nil      = is_nil
local is_boolean  = is_boolean
local is_number   = is_number
local is_string   = is_string
local is_table    = is_table
local is_function = is_function
local is_thread   = is_thread
local is_userdata = is_userdata


local function failure(name, usermsg, defaultmsg, ...)
  local errobj = {
    type    = __failure__,
    name    = name,
    msg     = string_format(defaultmsg,...),
    usermsg = usermsg
  }
  error(errobj, 0)
end
traceback_hide( failure )


local function format_arg(arg)
  local argtype = type(arg)
  if argtype == "string" then
    return "'"..arg.."'"
  elseif argtype == "number" or argtype == "boolean" or argtype == "nil" then
    return tostring(arg)
  else
    return "["..tostring(arg).."]"
  end
end


local function selected(map, name)
    if not map then
        return true
    end

    local m = {}
    for k,v in pairs(map) do
        m[k] = lunitpat2luapat(v)
    end
    return in_patternmap(m, name)
end


function fail(msg)
  stats.assertions = stats.assertions + 1
  failure( "fail", msg, "failure" )
end
traceback_hide( fail )


function assert(assertion, msg)
  stats.assertions = stats.assertions + 1
  if not assertion then
    failure( "assert", msg, "assertion failed" )
  end
  return assertion
end
traceback_hide( assert )


function assert_true(actual, msg)
  stats.assertions = stats.assertions + 1
  if actual ~= true then
    failure( "assert_true", msg, "true expected but was %s", format_arg(actual) )
  end
  return actual
end
traceback_hide( assert_true )


function assert_false(actual, msg)
  stats.assertions = stats.assertions + 1
  if actual ~= false then
    failure( "assert_false", msg, "false expected but was %s", format_arg(actual) )
  end
  return actual
end
traceback_hide( assert_false )


function assert_equal(expected, actual, msg)
  stats.assertions = stats.assertions + 1
  if expected ~= actual then
    failure( "assert_equal", msg, "expected %s but was %s", format_arg(expected), format_arg(actual) )
  end
  return actual
end
traceback_hide( assert_equal )


function assert_not_equal(unexpected, actual, msg)
  stats.assertions = stats.assertions + 1
  if unexpected == actual then
    failure( "assert_not_equal", msg, "%s not expected but was one", format_arg(unexpected) )
  end
  return actual
end
traceback_hide( assert_not_equal )


function assert_match(pattern, actual, msg)
  stats.assertions = stats.assertions + 1
  if type(pattern) ~= "string" then
    failure( "assert_match", msg, "expected a string as pattern but was %s", format_arg(pattern) )
  end
  if type(actual) ~= "string" then
    failure( "assert_match", msg, "expected a string to match pattern '%s' but was a %s", pattern, format_arg(actual) )
  end
  if not string_find(actual, pattern) then
    failure( "assert_match", msg, "expected '%s' to match pattern '%s' but doesn't", actual, pattern )
  end
  return actual
end
traceback_hide( assert_match )


function assert_not_match(pattern, actual, msg)
  stats.assertions = stats.assertions + 1
  if type(pattern) ~= "string" then
    failure( "assert_not_match", msg, "expected a string as pattern but was %s", format_arg(pattern) )
  end
  if type(actual) ~= "string" then
    failure( "assert_not_match", msg, "expected a string to not match pattern '%s' but was %s", pattern, format_arg(actual) )
  end
  if string_find(actual, pattern) then
    failure( "assert_not_match", msg, "expected '%s' to not match pattern '%s' but it does", actual, pattern )
  end
  return actual
end
traceback_hide( assert_not_match )


function assert_error(msg, func)
  stats.assertions = stats.assertions + 1
  if func == nil then
    func, msg = msg, nil
  end
  if type(func) ~= "function" then
    failure( "assert_error", msg, "expected a function as last argument but was %s", format_arg(func) )
  end
  local ok, errmsg = pcall(func)
  if ok then
    failure( "assert_error", msg, "error expected but no error occurred" )
  end
end
traceback_hide( assert_error )


function assert_error_match(msg, pattern, func)
  stats.assertions = stats.assertions + 1
  if func == nil then
    msg, pattern, func = nil, msg, pattern
  end
  if type(pattern) ~= "string" then
    failure( "assert_error_match", msg, "expected the pattern as a string but was %s", format_arg(pattern) )
  end
  if type(func) ~= "function" then
    failure( "assert_error_match", msg, "expected a function as last argument but was %s", format_arg(func) )
  end
  local ok, errmsg = pcall(func)
  if ok then
    failure( "assert_error_match", msg, "error expected but no error occurred" )
  end
  if type(errmsg) ~= "string" then
    failure( "assert_error_match", msg, "error as string expected but was %s", format_arg(errmsg) )
  end
  if not string_find(errmsg, pattern) then
    failure( "assert_error_match", msg, "expected error '%s' to match pattern '%s' but doesn't", errmsg, pattern )
  end
end
traceback_hide( assert_error_match )


function assert_pass(msg, func)
  stats.assertions = stats.assertions + 1
  if func == nil then
    func, msg = msg, nil
  end
  if type(func) ~= "function" then
    failure( "assert_pass", msg, "expected a function as last argument but was %s", format_arg(func) )
  end
  local ok, errmsg = pcall(func)
  if not ok then
    failure( "assert_pass", msg, "no error expected but error was: '%s'", errmsg )
  end
end
traceback_hide( assert_pass )


-- lunit.assert_typename functions

for _, typename in ipairs(typenames) do
  local assert_typename = "assert_"..typename
  lunit[assert_typename] = function(actual, msg)
    stats.assertions = stats.assertions + 1
    if type(actual) ~= typename then
      failure( assert_typename, msg, "%s expected but was %s", typename, format_arg(actual) )
    end
    return actual
  end
  traceback_hide( lunit[assert_typename] )
end


-- lunit.assert_not_typename functions

for _, typename in ipairs(typenames) do
  local assert_not_typename = "assert_not_"..typename
  lunit[assert_not_typename] = function(actual, msg)
    stats.assertions = stats.assertions + 1
    if type(actual) == typename then
      failure( assert_not_typename, msg, typename.." not expected but was one" )
    end
  end
  traceback_hide( lunit[assert_not_typename] )
end


function lunit.clearstats()
  stats = {
    assertions  = 0;
    passed      = 0;
    failed      = 0;
    errors      = 0;
  }
end


local report, reporterrobj
do
  local testrunner

  function lunit.setrunner(newrunner)
    if not ( is_table(newrunner) or is_nil(newrunner) ) then
      return error("lunit.setrunner: Invalid argument", 0)
    end
    local oldrunner = testrunner
    testrunner = newrunner
    return oldrunner
  end

  function lunit.loadrunner(name)
    if not is_string(name) then
      return error("lunit.loadrunner: Invalid argument", 0)
    end
    local ok, runner = pcall( require, name )
    if not ok then
      return error("lunit.loadrunner: Can't load test runner: "..runner, 0)
    end
    return setrunner(runner)
  end

  function lunit.getrunner()
    return testrunner
  end

  function report(event, ...)
    local f = testrunner and testrunner[event]
    if is_function(f) then
      pcall(f, ...)
    end
  end

  function reporterrobj(context, tcname, testname, errobj)
    local fullname = tcname .. "." .. testname
    if context == "setup" then
      fullname = fullname .. ":" .. setupname(tcname, testname)
    elseif context == "teardown" then
      fullname = fullname .. ":" .. teardownname(tcname, testname)
    end
    if errobj.type == __failure__ then
      stats.failed = stats.failed + 1
      report("fail", fullname, errobj.where, errobj.msg, errobj.usermsg)
    else
      stats.errors = stats.errors + 1
      report("err", fullname, errobj.msg, errobj.tb)
    end
  end
end



local function key_iter(t, k)
    return (next(t,k))
end


local testcase
do
  -- Array with all registered testcases
  local _testcases = {}

  -- Marks a module as a testcase.
  -- Applied over a module from module("xyz", lunit.testcase).
  function lunit.testcase(m)
    orig_assert( is_table(m) )
    --orig_assert( m._M == m )
    orig_assert( is_string(m._NAME) )
    --orig_assert( is_string(m._PACKAGE) )

    -- Register the module as a testcase
    _testcases[m._NAME] = m

    -- Import lunit, fail, assert* and is_* function to the module/testcase
    m.lunit = lunit
    m.fail = lunit.fail
    for funcname, func in pairs(lunit) do
      if "assert" == string_sub(funcname, 1, 6) or "is_" == string_sub(funcname, 1, 3) then
        m[funcname] = func
      end
    end
  end
  
  function lunit.module(name,seeall)
    local m = {}
    if seeall == "seeall" then
      setmetatable(m, { __index = _G })
    end
    m._NAME = name
    lunit.testcase(m)
    return m
  end

  -- Iterator (testcasename) over all Testcases
  function lunit.testcases()
    -- Make a copy of testcases to prevent confusing the iterator when
    -- new testcase are defined
    local _testcases2 = {}
    for k,v in pairs(_testcases) do
        _testcases2[k] = true
    end
    return key_iter, _testcases2, nil
  end

  function testcase(tcname)
    return _testcases[tcname]
  end
end


do
  -- Finds a function in a testcase case insensitive
  local function findfuncname(tcname, name)
    for key, value in pairs(testcase(tcname)) do
      if is_string(key) and is_function(value) and string_lower(key) == name then
        return key
      end
    end
  end

  function lunit.setupname(tcname)
    return findfuncname(tcname, "setup")
  end

  function lunit.teardownname(tcname)
    return findfuncname(tcname, "teardown")
  end

  -- Iterator over all test names in a testcase.
  -- Have to collect the names first in case one of the test
  -- functions creates a new global and throws off the iteration.
  function lunit.tests(tcname)
    local testnames = {}
    for key, value in pairs(testcase(tcname)) do
      if is_string(key) and is_function(value) then
        local lfn = string_lower(key)
        if string_sub(lfn, 1, 4) == "test" or string_sub(lfn, -4) == "test" then
          testnames[key] = true
        end
      end
    end
    return key_iter, testnames, nil
  end
end




function lunit.runtest(tcname, testname)
  orig_assert( is_string(tcname) )
  orig_assert( is_string(testname) )

  if (not getrunner()) then
    loadrunner("console")
  end

  local function callit(context, func)
    if func then
      local err = mypcall(func)
      if err then
        reporterrobj(context, tcname, testname, err)
        return false
      end
    end
    return true
  end
  traceback_hide(callit)

  report("run", tcname, testname)

  local tc          = testcase(tcname)
  local setup       = tc[setupname(tcname)]
  local test        = tc[testname]
  local teardown    = tc[teardownname(tcname)]

  local setup_ok    =              callit( "setup", setup )
  local test_ok     = setup_ok and callit( "test", test )
  local teardown_ok = setup_ok and callit( "teardown", teardown )

  if setup_ok and test_ok and teardown_ok then
    stats.passed = stats.passed + 1
    report("pass", tcname, testname)
  end
end
traceback_hide(runtest)



function lunit.run(testpatterns)
  clearstats()
  report("begin")
  for testcasename in lunit.testcases() do
    -- Run tests in the testcases
    for testname in lunit.tests(testcasename) do
      if selected(testpatterns, testname) then
        runtest(testcasename, testname)
      end
    end
  end
  report("done")
  return stats
end
traceback_hide(run)


function lunit.loadonly()
  clearstats()
  report("begin")
  report("done")
  return stats
end









local lunitpat2luapat
do 
  local conv = {
    ["^"] = "%^",
    ["$"] = "%$",
    ["("] = "%(",
    [")"] = "%)",
    ["%"] = "%%",
    ["."] = "%.",
    ["["] = "%[",
    ["]"] = "%]",
    ["+"] = "%+",
    ["-"] = "%-",
    ["?"] = ".",
    ["*"] = ".*"
  }
  function lunitpat2luapat(str)
    --return "^" .. string.gsub(str, "%W", conv) .. "$"
    -- Above was very annoying, if I want to run all the tests having to do with
    -- RSS, I want to be able to do "-t rss"   not "-t \*rss\*".
    return string_gsub(str, "%W", conv)
  end
end



local function in_patternmap(map, name)
  if map[name] == true then
    return true
  else
    for _, pat in ipairs(map) do
      if string_find(name, pat) then
        return true
      end
    end
  end
  return false
end








-- Called from 'lunit' shell script.

function main(argv)
  argv = argv or {}

  -- FIXME: Error handling and error messages aren't nice.

  local function checkarg(optname, arg)
    if not is_string(arg) then
      return error("lunit.main: option "..optname..": argument missing.", 0)
    end
  end

  local function loadtestcase(filename)
    if not is_string(filename) then
      return error("lunit.main: invalid argument")
    end
    local chunk, err = loadfile(filename)
    if err then
      return error(err)
    else
      chunk()
    end
  end

  local testpatterns = nil
  local doloadonly = false

  local i = 0
  while i < #argv do
    i = i + 1
    local arg = argv[i]
    if arg == "--loadonly" then
      doloadonly = true
    elseif arg == "--runner" or arg == "-r" then
      local optname = arg; i = i + 1; arg = argv[i]
      checkarg(optname, arg)
      loadrunner(arg)
    elseif arg == "--test" or arg == "-t" then
      local optname = arg; i = i + 1; arg = argv[i]
      checkarg(optname, arg)
      testpatterns = testpatterns or {}
      testpatterns[#testpatterns+1] = arg
    elseif arg == "--help" or arg == "-h" then
        print[[
lunit 0.5
Copyright (c) 2004-2009 Michael Roth <mroth@nessie.de>
This program comes WITHOUT WARRANTY OF ANY KIND.

Usage: lua test [OPTIONS] [--] scripts

Options:

  -r, --runner RUNNER         Testrunner to use, defaults to 'lunit-console'.
  -t, --test PATTERN          Which tests to run, may contain * or ? wildcards.
      --loadonly              Only load the tests.
  -h, --help                  Print this help screen.

Please report bugs to <mroth@nessie.de>.
]]
        return
    elseif arg == "--" then
      while i < #argv do
        i = i + 1; arg = argv[i]
        loadtestcase(arg)
      end
    else
      loadtestcase(arg)
    end
  end

  if doloadonly then
    return loadonly()
  else
    return run(testpatterns)
  end
end

clearstats()

return lunit
