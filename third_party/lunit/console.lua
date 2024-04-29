
--[[--------------------------------------------------------------------------

    This file is part of lunit 0.5.

    For Details about lunit look at: http://www.mroth.net/lunit/

    Author: Michael Roth <mroth@nessie.de>

    Copyright (c) 2006-2008 Michael Roth <mroth@nessie.de>

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



--[[

      begin()
        run(testcasename, testname)
          err(fullname, message, traceback)
          fail(fullname, where, message, usermessage)
          pass(testcasename, testname)
      done()

      Fullname:
        testcase.testname
        testcase.testname:setupname
        testcase.testname:teardownname

--]]


lunit = require "lunit"

local lunit_console

if _VERSION >= 'Lua 5.2' then 

    lunit_console = setmetatable({},{__index = _ENV})
    _ENV = lunit_console
    
else

    module( "lunit-console", package.seeall )
    lunit_console = _M
    
end



local function printformat(format, ...)
  io.write( string.format(format, ...) )
end


local columns_printed = 0

local function writestatus(char)
  if columns_printed == 0 then
    io.write("    ")
  end
  if columns_printed == 60 then
    io.write("\n    ")
    columns_printed = 0
  end
  io.write(char)
  io.flush()
  columns_printed = columns_printed + 1
end


local msgs = {}


function begin()
  local total_tc = 0
  local total_tests = 0
  
  msgs = {} -- e

  for tcname in lunit.testcases() do
    total_tc = total_tc + 1
    for testname, test in lunit.tests(tcname) do
      total_tests = total_tests + 1
    end
  end

  printformat("Loaded testsuite with %d tests in %d testcases.\n\n", total_tests, total_tc)
end


function run(testcasename, testname)
  -- NOP
end


function err(fullname, message, traceback)
  writestatus("E")
  msgs[#msgs+1] = "Error! ("..fullname.."):\n"..message.."\n\t"..table.concat(traceback, "\n\t") .. "\n"
end


function fail(fullname, where, message, usermessage)
  writestatus("F")
  local text =  "Failure ("..fullname.."):\n"..
                where..": "..message.."\n"

  if usermessage then
    text = text .. where..": "..usermessage.."\n"
  end

  msgs[#msgs+1] = text
end


function pass(testcasename, testname)
  writestatus(".")
end



function done()
  printformat("\n\n%d Assertions checked.\n", lunit.stats.assertions )
  print()

  for i, msg in ipairs(msgs) do
    printformat( "%3d) %s\n", i, msg )
  end

  printformat("Testsuite finished (%d passed, %d failed, %d errors).\n",
      lunit.stats.passed, lunit.stats.failed, lunit.stats.errors )
end


return lunit_console


