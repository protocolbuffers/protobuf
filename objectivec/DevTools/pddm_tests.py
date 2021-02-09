#! /usr/bin/python
#
# Protocol Buffers - Google's data interchange format
# Copyright 2015 Google Inc.  All rights reserved.
# https://developers.google.com/protocol-buffers/
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Tests for pddm.py."""

import io
import unittest

import pddm


class TestParsingMacros(unittest.TestCase):

  def testParseEmpty(self):
    f = io.StringIO(u'')
    result = pddm.MacroCollection(f)
    self.assertEqual(len(result._macros), 0)

  def testParseOne(self):
    f = io.StringIO(u"""PDDM-DEFINE foo( )
body""")
    result = pddm.MacroCollection(f)
    self.assertEqual(len(result._macros), 1)
    macro = result._macros.get('foo')
    self.assertIsNotNone(macro)
    self.assertEquals(macro.name, 'foo')
    self.assertEquals(macro.args, tuple())
    self.assertEquals(macro.body, 'body')

  def testParseGeneral(self):
    # Tests multiple defines, spaces in all places, etc.
    f = io.StringIO(u"""
PDDM-DEFINE noArgs( )
body1
body2

PDDM-DEFINE-END

PDDM-DEFINE oneArg(foo)
body3
PDDM-DEFINE  twoArgs( bar_ , baz )
body4
body5""")
    result = pddm.MacroCollection(f)
    self.assertEqual(len(result._macros), 3)
    macro = result._macros.get('noArgs')
    self.assertIsNotNone(macro)
    self.assertEquals(macro.name, 'noArgs')
    self.assertEquals(macro.args, tuple())
    self.assertEquals(macro.body, 'body1\nbody2\n')
    macro = result._macros.get('oneArg')
    self.assertIsNotNone(macro)
    self.assertEquals(macro.name, 'oneArg')
    self.assertEquals(macro.args, ('foo',))
    self.assertEquals(macro.body, 'body3')
    macro = result._macros.get('twoArgs')
    self.assertIsNotNone(macro)
    self.assertEquals(macro.name, 'twoArgs')
    self.assertEquals(macro.args, ('bar_', 'baz'))
    self.assertEquals(macro.body, 'body4\nbody5')
    # Add into existing collection
    f = io.StringIO(u"""
PDDM-DEFINE another(a,b,c)
body1
body2""")
    result.ParseInput(f)
    self.assertEqual(len(result._macros), 4)
    macro = result._macros.get('another')
    self.assertIsNotNone(macro)
    self.assertEquals(macro.name, 'another')
    self.assertEquals(macro.args, ('a', 'b', 'c'))
    self.assertEquals(macro.body, 'body1\nbody2')

  def testParseDirectiveIssues(self):
    test_list = [
      # Unknown directive
      (u'PDDM-DEFINE foo()\nbody\nPDDM-DEFINED foo\nbaz',
       'Hit a line with an unknown directive: '),
      # End without begin
      (u'PDDM-DEFINE foo()\nbody\nPDDM-DEFINE-END\nPDDM-DEFINE-END\n',
       'Got DEFINE-END directive without an active macro: '),
      # Line not in macro block
      (u'PDDM-DEFINE foo()\nbody\nPDDM-DEFINE-END\nmumble\n',
       'Hit a line that wasn\'t a directive and no open macro definition: '),
      # Redefine macro
      (u'PDDM-DEFINE foo()\nbody\nPDDM-DEFINE foo(a)\nmumble\n',
       'Attempt to redefine macro: '),
    ]
    for idx, (input_str, expected_prefix) in enumerate(test_list, 1):
      f = io.StringIO(input_str)
      try:
        result = pddm.MacroCollection(f)
        self.fail('Should throw exception, entry %d' % idx)
      except pddm.PDDMError as e:
        self.assertTrue(e.message.startswith(expected_prefix),
                        'Entry %d failed: %r' % (idx, e))

  def testParseBeginIssues(self):
    test_list = [
      # 1. No name
      (u'PDDM-DEFINE\nmumble',
       'Failed to parse macro definition: '),
      # 2. No name (with spaces)
      (u'PDDM-DEFINE  \nmumble',
       'Failed to parse macro definition: '),
      # 3. No open paren
      (u'PDDM-DEFINE  foo\nmumble',
       'Failed to parse macro definition: '),
      # 4. No close paren
      (u'PDDM-DEFINE foo(\nmumble',
       'Failed to parse macro definition: '),
      # 5. No close paren (with args)
      (u'PDDM-DEFINE foo(a, b\nmumble',
       'Failed to parse macro definition: '),
      # 6. No name before args
      (u'PDDM-DEFINE  (a, b)\nmumble',
       'Failed to parse macro definition: '),
      # 7. No name before args
      (u'PDDM-DEFINE foo bar(a, b)\nmumble',
       'Failed to parse macro definition: '),
      # 8. Empty arg name
      (u'PDDM-DEFINE foo(a, ,b)\nmumble',
       'Empty arg name in macro definition: '),
      (u'PDDM-DEFINE foo(a,,b)\nmumble',
       'Empty arg name in macro definition: '),
      # 10. Duplicate name
      (u'PDDM-DEFINE foo(a,b,a,c)\nmumble',
       'Arg name "a" used more than once in macro definition: '),
      # 11. Invalid arg name
      (u'PDDM-DEFINE foo(a b,c)\nmumble',
       'Invalid arg name "a b" in macro definition: '),
      (u'PDDM-DEFINE foo(a.b,c)\nmumble',
       'Invalid arg name "a.b" in macro definition: '),
      (u'PDDM-DEFINE foo(a-b,c)\nmumble',
       'Invalid arg name "a-b" in macro definition: '),
      (u'PDDM-DEFINE foo(a,b,c.)\nmumble',
       'Invalid arg name "c." in macro definition: '),
      # 15. Extra stuff after the name
      (u'PDDM-DEFINE foo(a,c) foo\nmumble',
       'Failed to parse macro definition: '),
      (u'PDDM-DEFINE foo(a,c) foo)\nmumble',
       'Failed to parse macro definition: '),
    ]
    for idx, (input_str, expected_prefix) in enumerate(test_list, 1):
      f = io.StringIO(input_str)
      try:
        result = pddm.MacroCollection(f)
        self.fail('Should throw exception, entry %d' % idx)
      except pddm.PDDMError as e:
        self.assertTrue(e.message.startswith(expected_prefix),
                        'Entry %d failed: %r' % (idx, e))


class TestExpandingMacros(unittest.TestCase):

  def testExpandBasics(self):
    f = io.StringIO(u"""
PDDM-DEFINE noArgs( )
body1
body2

PDDM-DEFINE-END

PDDM-DEFINE oneArg(a)
body3 a

PDDM-DEFINE-END

PDDM-DEFINE twoArgs(b,c)
body4 b c
body5
PDDM-DEFINE-END

""")
    mc = pddm.MacroCollection(f)
    test_list = [
      (u'noArgs()',
       'body1\nbody2\n'),
      (u'oneArg(wee)',
       'body3 wee\n'),
      (u'twoArgs(having some, fun)',
       'body4 having some fun\nbody5'),
      # One arg, pass empty.
      (u'oneArg()',
       'body3 \n'),
      # Two args, gets empty in each slot.
      (u'twoArgs(, empty)',
       'body4  empty\nbody5'),
      (u'twoArgs(empty, )',
       'body4 empty \nbody5'),
      (u'twoArgs(, )',
       'body4  \nbody5'),
    ]
    for idx, (input_str, expected) in enumerate(test_list, 1):
      result = mc.Expand(input_str)
      self.assertEqual(result, expected,
                       'Entry %d --\n       Result: %r\n     Expected: %r' %
                       (idx, result, expected))

  def testExpandArgOptions(self):
    f = io.StringIO(u"""
PDDM-DEFINE bar(a)
a-a$S-a$l-a$L-a$u-a$U
PDDM-DEFINE-END
""")
    mc = pddm.MacroCollection(f)

    self.assertEqual(mc.Expand('bar(xYz)'), 'xYz-   -xYz-xyz-XYz-XYZ')
    self.assertEqual(mc.Expand('bar(MnoP)'), 'MnoP-    -mnoP-mnop-MnoP-MNOP')
    # Test empty
    self.assertEqual(mc.Expand('bar()'), '-----')

  def testExpandSimpleMacroErrors(self):
    f = io.StringIO(u"""
PDDM-DEFINE foo(a, b)
<a-z>
PDDM-DEFINE baz(a)
a - a$z
""")
    mc = pddm.MacroCollection(f)
    test_list = [
      # 1. Unknown macro
      (u'bar()',
       'No macro named "bar".'),
      (u'bar(a)',
       'No macro named "bar".'),
      # 3. Arg mismatch
      (u'foo()',
       'Expected 2 args, got: "foo()".'),
      (u'foo(a b)',
       'Expected 2 args, got: "foo(a b)".'),
      (u'foo(a,b,c)',
       'Expected 2 args, got: "foo(a,b,c)".'),
      # 6. Unknown option in expansion
      (u'baz(mumble)',
       'Unknown arg option "a$z" while expanding "baz(mumble)".'),
    ]
    for idx, (input_str, expected_err) in enumerate(test_list, 1):
      try:
        result = mc.Expand(input_str)
        self.fail('Should throw exception, entry %d' % idx)
      except pddm.PDDMError as e:
        self.assertEqual(e.message, expected_err,
                        'Entry %d failed: %r' % (idx, e))

  def testExpandReferences(self):
    f = io.StringIO(u"""
PDDM-DEFINE StartIt()
foo(abc, def)
foo(ghi, jkl)
PDDM-DEFINE foo(a, b)
bar(a, int)
bar(b, NSString *)
PDDM-DEFINE bar(n, t)
- (t)n;
- (void)set##n$u##:(t)value;

""")
    mc = pddm.MacroCollection(f)
    expected = """- (int)abc;
- (void)setAbc:(int)value;

- (NSString *)def;
- (void)setDef:(NSString *)value;

- (int)ghi;
- (void)setGhi:(int)value;

- (NSString *)jkl;
- (void)setJkl:(NSString *)value;
"""
    self.assertEqual(mc.Expand('StartIt()'), expected)

  def testCatchRecursion(self):
    f = io.StringIO(u"""
PDDM-DEFINE foo(a, b)
bar(1, a)
bar(2, b)
PDDM-DEFINE bar(x, y)
foo(x, y)
""")
    mc = pddm.MacroCollection(f)
    try:
      result = mc.Expand('foo(A,B)')
      self.fail('Should throw exception! Test failed to catch recursion.')
    except pddm.PDDMError as e:
      self.assertEqual(e.message,
                       'Found macro recursion, invoking "foo(1, A)":\n...while expanding "bar(1, A)".\n...while expanding "foo(A,B)".')


class TestParsingSource(unittest.TestCase):

  def testBasicParse(self):
    test_list = [
      # 1. no directives
      (u'a\nb\nc',
       (3,) ),
      # 2. One define
      (u'a\n//%PDDM-DEFINE foo()\n//%body\nc',
       (1, 2, 1) ),
      # 3. Two defines
      (u'a\n//%PDDM-DEFINE foo()\n//%body\n//%PDDM-DEFINE bar()\n//%body2\nc',
       (1, 4, 1) ),
      # 4. Two defines with ends
      (u'a\n//%PDDM-DEFINE foo()\n//%body\n//%PDDM-DEFINE-END\n'
       u'//%PDDM-DEFINE bar()\n//%body2\n//%PDDM-DEFINE-END\nc',
       (1, 6, 1) ),
      # 5. One expand, one define (that runs to end of file)
      (u'a\n//%PDDM-EXPAND foo()\nbody\n//%PDDM-EXPAND-END\n'
       u'//%PDDM-DEFINE bar()\n//%body2\n',
       (1, 1, 2) ),
      # 6. One define ended with an expand.
      (u'a\nb\n//%PDDM-DEFINE bar()\n//%body2\n'
       u'//%PDDM-EXPAND bar()\nbody2\n//%PDDM-EXPAND-END\n',
       (2, 2, 1) ),
      # 7. Two expands (one end), one define.
      (u'a\n//%PDDM-EXPAND foo(1)\nbody\n//%PDDM-EXPAND foo(2)\nbody2\n//%PDDM-EXPAND-END\n'
       u'//%PDDM-DEFINE foo()\n//%body2\n',
       (1, 2, 2) ),
    ]
    for idx, (input_str, line_counts) in enumerate(test_list, 1):
      f = io.StringIO(input_str)
      sf = pddm.SourceFile(f)
      sf._ParseFile()
      self.assertEqual(len(sf._sections), len(line_counts),
                       'Entry %d -- %d != %d' %
                       (idx, len(sf._sections), len(line_counts)))
      for idx2, (sec, expected) in enumerate(zip(sf._sections, line_counts), 1):
        self.assertEqual(sec.num_lines_captured, expected,
                         'Entry %d, section %d -- %d != %d' %
                         (idx, idx2, sec.num_lines_captured, expected))

  def testErrors(self):
    test_list = [
      # 1. Directive within expansion
      (u'//%PDDM-EXPAND a()\n//%PDDM-BOGUS',
       'Ran into directive ("//%PDDM-BOGUS", line 2) while in "//%PDDM-EXPAND a()".'),
      (u'//%PDDM-EXPAND a()\n//%PDDM-DEFINE a()\n//%body\n',
       'Ran into directive ("//%PDDM-DEFINE", line 2) while in "//%PDDM-EXPAND a()".'),
      # 3. Expansion ran off end of file
      (u'//%PDDM-EXPAND a()\na\nb\n',
       'Hit the end of the file while in "//%PDDM-EXPAND a()".'),
      # 4. Directive within define
      (u'//%PDDM-DEFINE a()\n//%body\n//%PDDM-BOGUS',
       'Ran into directive ("//%PDDM-BOGUS", line 3) while in "//%PDDM-DEFINE a()".'),
      (u'//%PDDM-DEFINE a()\n//%body\n//%PDDM-EXPAND-END a()',
       'Ran into directive ("//%PDDM-EXPAND-END", line 3) while in "//%PDDM-DEFINE a()".'),
      # 6. Directives that shouldn't start sections
      (u'a\n//%PDDM-DEFINE-END a()\n//a\n',
       'Unexpected line 2: "//%PDDM-DEFINE-END a()".'),
      (u'a\n//%PDDM-EXPAND-END a()\n//a\n',
       'Unexpected line 2: "//%PDDM-EXPAND-END a()".'),
      (u'//%PDDM-BOGUS\n//a\n',
       'Unexpected line 1: "//%PDDM-BOGUS".'),
    ]
    for idx, (input_str, expected_err) in enumerate(test_list, 1):
      f = io.StringIO(input_str)
      try:
        pddm.SourceFile(f)._ParseFile()
        self.fail('Should throw exception, entry %d' % idx)
      except pddm.PDDMError as e:
        self.assertEqual(e.message, expected_err,
                        'Entry %d failed: %r' % (idx, e))

class TestProcessingSource(unittest.TestCase):

  def testBasics(self):
    self.maxDiff = None
    input_str = u"""
//%PDDM-IMPORT-DEFINES ImportFile
foo
//%PDDM-EXPAND mumble(abc)
//%PDDM-EXPAND-END
bar
//%PDDM-EXPAND mumble(def)
//%PDDM-EXPAND mumble(ghi)
//%PDDM-EXPAND-END
baz
//%PDDM-DEFINE mumble(a_)
//%a_: getName(a_)
"""
    input_str2 = u"""
//%PDDM-DEFINE getName(x_)
//%do##x_$u##(int x_);

"""
    expected = u"""
//%PDDM-IMPORT-DEFINES ImportFile
foo
//%PDDM-EXPAND mumble(abc)
// This block of code is generated, do not edit it directly.
// clang-format off

abc: doAbc(int abc);
// clang-format on
//%PDDM-EXPAND-END mumble(abc)
bar
//%PDDM-EXPAND mumble(def)
// This block of code is generated, do not edit it directly.
// clang-format off

def: doDef(int def);
// clang-format on
//%PDDM-EXPAND mumble(ghi)
// This block of code is generated, do not edit it directly.
// clang-format off

ghi: doGhi(int ghi);
// clang-format on
//%PDDM-EXPAND-END (2 expansions)
baz
//%PDDM-DEFINE mumble(a_)
//%a_: getName(a_)
"""
    expected_stripped = u"""
//%PDDM-IMPORT-DEFINES ImportFile
foo
//%PDDM-EXPAND mumble(abc)
//%PDDM-EXPAND-END mumble(abc)
bar
//%PDDM-EXPAND mumble(def)
//%PDDM-EXPAND mumble(ghi)
//%PDDM-EXPAND-END (2 expansions)
baz
//%PDDM-DEFINE mumble(a_)
//%a_: getName(a_)
"""
    def _Resolver(name):
      self.assertEqual(name, 'ImportFile')
      return io.StringIO(input_str2)
    f = io.StringIO(input_str)
    sf = pddm.SourceFile(f, _Resolver)
    sf.ProcessContent()
    self.assertEqual(sf.processed_content, expected)
    # Feed it through and nothing should change.
    f2 = io.StringIO(sf.processed_content)
    sf2 = pddm.SourceFile(f2, _Resolver)
    sf2.ProcessContent()
    self.assertEqual(sf2.processed_content, expected)
    self.assertEqual(sf2.processed_content, sf.processed_content)
    # Test stripping (with the original input and expanded version).
    f2 = io.StringIO(input_str)
    sf2 = pddm.SourceFile(f2)
    sf2.ProcessContent(strip_expansion=True)
    self.assertEqual(sf2.processed_content, expected_stripped)
    f2 = io.StringIO(sf.processed_content)
    sf2 = pddm.SourceFile(f2, _Resolver)
    sf2.ProcessContent(strip_expansion=True)
    self.assertEqual(sf2.processed_content, expected_stripped)

  def testProcessFileWithMacroParseError(self):
    input_str = u"""
foo
//%PDDM-DEFINE mumble(a_)
//%body
//%PDDM-DEFINE mumble(x_)
//%body2

"""
    f = io.StringIO(input_str)
    sf = pddm.SourceFile(f)
    try:
      sf.ProcessContent()
      self.fail('Should throw exception! Test failed to catch macro parsing error.')
    except pddm.PDDMError as e:
      self.assertEqual(e.message,
                       'Attempt to redefine macro: "PDDM-DEFINE mumble(x_)"\n'
                       '...while parsing section that started:\n'
                       '  Line 3: //%PDDM-DEFINE mumble(a_)')

  def testProcessFileWithExpandError(self):
    input_str = u"""
foo
//%PDDM-DEFINE mumble(a_)
//%body
//%PDDM-EXPAND foobar(x_)
//%PDDM-EXPAND-END

"""
    f = io.StringIO(input_str)
    sf = pddm.SourceFile(f)
    try:
      sf.ProcessContent()
      self.fail('Should throw exception! Test failed to catch expand error.')
    except pddm.PDDMError as e:
      self.assertEqual(e.message,
                       'No macro named "foobar".\n'
                       '...while expanding "foobar(x_)" from the section that'
                       ' started:\n   Line 5: //%PDDM-EXPAND foobar(x_)')


if __name__ == '__main__':
  unittest.main()
