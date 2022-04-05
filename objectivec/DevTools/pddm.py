#! /usr/bin/env python3
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

"""PDDM - Poor Developers' Debug-able Macros

A simple markup that can be added in comments of source so they can then be
expanded out into code. Most of this could be done with CPP macros, but then
developers can't really step through them in the debugger, this way they are
expanded to the same code, but you can debug them.

Any file can be processed, but the syntax is designed around a C based compiler.
Processed lines start with "//%".  There are three types of sections you can
create: Text (left alone), Macro Definitions, and Macro Expansions.  There is
no order required between definitions and expansions, all definitions are read
before any expansions are processed (thus, if desired, definitions can be put
at the end of the file to keep them out of the way of the code).

Macro Definitions are started with "//%PDDM-DEFINE Name(args)" and all lines
afterwards that start with "//%" are included in the definition.  Multiple
macros can be defined in one block by just using a another "//%PDDM-DEFINE"
line to start the next macro.  Optionally, a macro can be ended with
"//%PDDM-DEFINE-END", this can be useful when you want to make it clear that
trailing blank lines are included in the macro.  You can also end a definition
with an expansion.

Macro Expansions are started by single lines containing
"//%PDDM-EXPAND Name(args)" and then with "//%PDDM-EXPAND-END" or another
expansions.  All lines in-between are replaced by the result of the expansion.
The first line of the expansion is always a blank like just for readability.

Expansion itself is pretty simple, one macro can invoke another macro, but
you cannot nest the invoke of a macro in another macro (i.e. - can't do
"foo(bar(a))", but you can define foo(a) and bar(b) where bar invokes foo()
within its expansion.

When macros are expanded, the arg references can also add "$O" suffix to the
name (i.e. - "NAME$O") to specify an option to be applied. The options are:

    $S - Replace each character in the value with a space.
    $l - Lowercase the first letter of the value.
    $L - Lowercase the whole value.
    $u - Uppercase the first letter of the value.
    $U - Uppercase the whole value.

Within a macro you can use ## to cause things to get joined together after
expansion (i.e. - "a##b" within a macro will become "ab").

Example:

    int foo(MyEnum x) {
    switch (x) {
    //%PDDM-EXPAND case(Enum_Left, 1)
    //%PDDM-EXPAND case(Enum_Center, 2)
    //%PDDM-EXPAND case(Enum_Right, 3)
    //%PDDM-EXPAND-END
    }

    //%PDDM-DEFINE case(_A, _B)
    //%  case _A:
    //%    return _B;

  A macro ends at the start of the next one, or an optional %PDDM-DEFINE-END
  can be used to avoid adding extra blank lines/returns (or make it clear when
  it is desired).

  One macro can invoke another by simply using its name NAME(ARGS). You cannot
  nest an invoke inside another (i.e. - NAME1(NAME2(ARGS)) isn't supported).

  Within a macro you can use ## to cause things to get joined together after
  processing (i.e. - "a##b" within a macro will become "ab").


"""

import optparse
import os
import re
import sys


# Regex for macro definition.
_MACRO_RE = re.compile(r'(?P<name>\w+)\((?P<args>.*?)\)')
# Regex for macro's argument definition.
_MACRO_ARG_NAME_RE = re.compile(r'^\w+$')

# Line inserted after each EXPAND.
_GENERATED_CODE_LINE = (
  '// This block of code is generated, do not edit it directly.'
)


def _MacroRefRe(macro_names):
  # Takes in a list of macro names and makes a regex that will match invokes
  # of those macros.
  return re.compile(r'\b(?P<macro_ref>(?P<name>(%s))\((?P<args>.*?)\))' %
                    '|'.join(macro_names))


def _MacroArgRefRe(macro_arg_names):
  # Takes in a list of macro arg names and makes a regex that will match
  # uses of those args.
  return re.compile(r'\b(?P<name>(%s))(\$(?P<option>.))?\b' %
                    '|'.join(macro_arg_names))


class PDDMError(Exception):
  """Error thrown by pddm."""

  def __init__(self, message="Error"):
    self.message = message
    super().__init__(self.message)


class MacroCollection(object):
  """Hold a set of macros and can resolve/expand them."""

  def __init__(self, a_file=None):
    """Initializes the collection.

    Args:
      a_file: The file like stream to parse.

    Raises:
      PDDMError if there are any issues.
    """
    self._macros = dict()
    if a_file:
      self.ParseInput(a_file)

  class MacroDefinition(object):
    """Holds a macro definition."""

    def __init__(self, name, arg_names):
      self._name = name
      self._args = tuple(arg_names)
      self._body = ''
      self._needNewLine = False

    def AppendLine(self, line):
      if self._needNewLine:
        self._body += '\n'
      self._body += line
      self._needNewLine = not line.endswith('\n')

    @property
    def name(self):
      return self._name

    @property
    def args(self):
      return self._args

    @property
    def body(self):
      return self._body

  def ParseInput(self, a_file):
    """Consumes input extracting definitions.

    Args:
      a_file: The file like stream to parse.

    Raises:
      PDDMError if there are any issues.
    """
    input_lines = a_file.read().splitlines()
    self.ParseLines(input_lines)

  def ParseLines(self, input_lines):
    """Parses list of lines.

    Args:
      input_lines: A list of strings of input to parse (no newlines on the
                   strings).

    Raises:
      PDDMError if there are any issues.
    """
    current_macro = None
    for line in input_lines:
      if line.startswith('PDDM-'):
        directive = line.split(' ', 1)[0]
        if directive == 'PDDM-DEFINE':
          name, args = self._ParseDefineLine(line)
          if self._macros.get(name):
            raise PDDMError('Attempt to redefine macro: "%s"' % line)
          current_macro = self.MacroDefinition(name, args)
          self._macros[name] = current_macro
          continue
        if directive == 'PDDM-DEFINE-END':
          if not current_macro:
            raise PDDMError('Got DEFINE-END directive without an active macro:'
                            ' "%s"' % line)
          current_macro = None
          continue
        raise PDDMError('Hit a line with an unknown directive: "%s"' % line)

      if current_macro:
        current_macro.AppendLine(line)
        continue

      # Allow blank lines between macro definitions.
      if line.strip() == '':
        continue

      raise PDDMError('Hit a line that wasn\'t a directive and no open macro'
                      ' definition: "%s"' % line)

  def _ParseDefineLine(self, input_line):
    assert input_line.startswith('PDDM-DEFINE')
    line = input_line[12:].strip()
    match = _MACRO_RE.match(line)
    # Must match full line
    if match is None or match.group(0) != line:
      raise PDDMError('Failed to parse macro definition: "%s"' % input_line)
    name = match.group('name')
    args_str = match.group('args').strip()
    args = []
    if args_str:
      for part in args_str.split(','):
        arg = part.strip()
        if arg == '':
          raise PDDMError('Empty arg name in macro definition: "%s"'
                          % input_line)
        if not _MACRO_ARG_NAME_RE.match(arg):
          raise PDDMError('Invalid arg name "%s" in macro definition: "%s"'
                          % (arg, input_line))
        if arg in args:
          raise PDDMError('Arg name "%s" used more than once in macro'
                          ' definition: "%s"' % (arg, input_line))
        args.append(arg)
    return (name, tuple(args))

  def Expand(self, macro_ref_str):
    """Expands the macro reference.

    Args:
      macro_ref_str: String of a macro reference (i.e. foo(a, b)).

    Returns:
      The text from the expansion.

    Raises:
      PDDMError if there are any issues.
    """
    match = _MACRO_RE.match(macro_ref_str)
    if match is None or match.group(0) != macro_ref_str:
      raise PDDMError('Failed to parse macro reference: "%s"' % macro_ref_str)
    if match.group('name') not in self._macros:
      raise PDDMError('No macro named "%s".' % match.group('name'))
    return self._Expand(match, [], macro_ref_str)

  def _FormatStack(self, macro_ref_stack):
    result = ''
    for _, macro_ref in reversed(macro_ref_stack):
      result += '\n...while expanding "%s".' % macro_ref
    return result

  def _Expand(self, macro_ref_match, macro_stack, macro_ref_str=None):
    if macro_ref_str is None:
      macro_ref_str = macro_ref_match.group('macro_ref')
    name = macro_ref_match.group('name')
    for prev_name, prev_macro_ref in macro_stack:
      if name == prev_name:
        raise PDDMError('Found macro recursion, invoking "%s":%s' %
                        (macro_ref_str, self._FormatStack(macro_stack)))
    macro = self._macros[name]
    args_str = macro_ref_match.group('args').strip()
    args = []
    if args_str or len(macro.args):
      args = [x.strip() for x in args_str.split(',')]
    if len(args) != len(macro.args):
      raise PDDMError('Expected %d args, got: "%s".%s' %
                      (len(macro.args), macro_ref_str,
                       self._FormatStack(macro_stack)))
    # Replace args usages.
    result = self._ReplaceArgValues(macro, args, macro_ref_str, macro_stack)
    # Expand any macro invokes.
    new_macro_stack = macro_stack + [(name, macro_ref_str)]
    while True:
      eval_result = self._EvalMacrosRefs(result, new_macro_stack)
      # Consume all ## directives to glue things together.
      eval_result = eval_result.replace('##', '')
      if eval_result == result:
        break
      result = eval_result
    return result

  def _ReplaceArgValues(self,
                        macro, arg_values, macro_ref_to_report, macro_stack):
    if len(arg_values) == 0:
      # Nothing to do
      return macro.body
    assert len(arg_values) == len(macro.args)
    args = dict(list(zip(macro.args, arg_values)))

    def _lookupArg(match):
      val = args[match.group('name')]
      opt = match.group('option')
      if opt:
        if opt == 'S':  # Spaces for the length
          return ' ' * len(val)
        elif opt == 'l':  # Lowercase first character
          if val:
            return val[0].lower() + val[1:]
          else:
            return val
        elif opt == 'L':  # All Lowercase
          return val.lower()
        elif opt == 'u':  # Uppercase first character
          if val:
            return val[0].upper() + val[1:]
          else:
            return val
        elif opt == 'U':  # All Uppercase
          return val.upper()
        else:
          raise PDDMError('Unknown arg option "%s$%s" while expanding "%s".%s'
                          % (match.group('name'), match.group('option'),
                             macro_ref_to_report,
                             self._FormatStack(macro_stack)))
      return val
    # Let the regex do the work!
    macro_arg_ref_re = _MacroArgRefRe(macro.args)
    return macro_arg_ref_re.sub(_lookupArg, macro.body)

  def _EvalMacrosRefs(self, text, macro_stack):
    macro_ref_re = _MacroRefRe(list(self._macros.keys()))

    def _resolveMacro(match):
      return self._Expand(match, macro_stack)
    return macro_ref_re.sub(_resolveMacro, text)


class SourceFile(object):
  """Represents a source file with PDDM directives in it."""

  def __init__(self, a_file, import_resolver=None):
    """Initializes the file reading in the file.

    Args:
      a_file: The file to read in.
      import_resolver: a function that given a path will return a stream for
        the contents.

    Raises:
      PDDMError if there are any issues.
    """
    self._sections = []
    self._original_content = a_file.read()
    self._import_resolver = import_resolver
    self._processed_content = None

  class SectionBase(object):

    def __init__(self, first_line_num):
      self._lines = []
      self._first_line_num = first_line_num

    def TryAppend(self, line, line_num):
      """Try appending a line.

      Args:
        line: The line to append.
        line_num: The number of the line.

      Returns:
        A tuple of (SUCCESS, CAN_ADD_MORE).  If SUCCESS if False, the line
        wasn't append.  If SUCCESS is True, then CAN_ADD_MORE is True/False to
        indicate if more lines can be added after this one.
      """
      assert False, "subclass should have overridden"
      return (False, False)

    def HitEOF(self):
      """Called when the EOF was reached for for a given section."""
      pass

    def BindMacroCollection(self, macro_collection):
      """Binds the chunk to a macro collection.

      Args:
        macro_collection: The collection to bind too.
      """
      pass

    def Append(self, line):
      self._lines.append(line)

    @property
    def lines(self):
      return self._lines

    @property
    def num_lines_captured(self):
      return len(self._lines)

    @property
    def first_line_num(self):
      return self._first_line_num

    @property
    def first_line(self):
      if not self._lines:
        return ''
      return self._lines[0]

    @property
    def text(self):
      return '\n'.join(self.lines) + '\n'

  class TextSection(SectionBase):
    """Text section that is echoed out as is."""

    def TryAppend(self, line, line_num):
      if line.startswith('//%PDDM'):
        return (False, False)
      self.Append(line)
      return (True, True)

  class ExpansionSection(SectionBase):
    """Section that is the result of an macro expansion."""

    def __init__(self, first_line_num):
      SourceFile.SectionBase.__init__(self, first_line_num)
      self._macro_collection = None

    def TryAppend(self, line, line_num):
      if line.startswith('//%PDDM'):
        directive = line.split(' ', 1)[0]
        if directive == '//%PDDM-EXPAND':
          self.Append(line)
          return (True, True)
        if directive == '//%PDDM-EXPAND-END':
          assert self.num_lines_captured > 0
          return (True, False)
        raise PDDMError('Ran into directive ("%s", line %d) while in "%s".' %
                        (directive, line_num, self.first_line))
      # Eat other lines.
      return (True, True)

    def HitEOF(self):
      raise PDDMError('Hit the end of the file while in "%s".' %
                      self.first_line)

    def BindMacroCollection(self, macro_collection):
      self._macro_collection = macro_collection

    @property
    def lines(self):
      captured_lines = SourceFile.SectionBase.lines.fget(self)
      directive_len = len('//%PDDM-EXPAND')
      result = []
      for line in captured_lines:
        result.append(line)
        if self._macro_collection:
          # Always add a blank line, seems to read better. (If need be, add an
          # option to the EXPAND to indicate if this should be done.)
          result.extend([_GENERATED_CODE_LINE, '// clang-format off', ''])
          macro = line[directive_len:].strip()
          try:
            expand_result = self._macro_collection.Expand(macro)
            # Since expansions are line oriented, strip trailing whitespace
            # from the lines.
            lines = [x.rstrip() for x in expand_result.split('\n')]
            lines.append('// clang-format on')
            result.append('\n'.join(lines))
          except PDDMError as e:
            raise PDDMError('%s\n...while expanding "%s" from the section'
                            ' that started:\n   Line %d: %s' %
                            (e.message, macro,
                             self.first_line_num, self.first_line))

      # Add the ending marker.
      if len(captured_lines) == 1:
        result.append('//%%PDDM-EXPAND-END %s' %
                      captured_lines[0][directive_len:].strip())
      else:
        result.append('//%%PDDM-EXPAND-END (%s expansions)' %
                      len(captured_lines))
      return result

  class DefinitionSection(SectionBase):
    """Section containing macro definitions"""

    def TryAppend(self, line, line_num):
      if not line.startswith('//%'):
        return (False, False)
      if line.startswith('//%PDDM'):
        directive = line.split(' ', 1)[0]
        if directive == "//%PDDM-EXPAND":
          return False, False
        if directive not in ('//%PDDM-DEFINE', '//%PDDM-DEFINE-END'):
          raise PDDMError('Ran into directive ("%s", line %d) while in "%s".' %
                          (directive, line_num, self.first_line))
      self.Append(line)
      return (True, True)

    def BindMacroCollection(self, macro_collection):
      if macro_collection:
        try:
          # Parse the lines after stripping the prefix.
          macro_collection.ParseLines([x[3:] for x in self.lines])
        except PDDMError as e:
          raise PDDMError('%s\n...while parsing section that started:\n'
                          '  Line %d: %s' %
                          (e.message, self.first_line_num, self.first_line))

  class ImportDefinesSection(SectionBase):
    """Section containing an import of PDDM-DEFINES from an external file."""

    def __init__(self, first_line_num, import_resolver):
      SourceFile.SectionBase.__init__(self, first_line_num)
      self._import_resolver = import_resolver

    def TryAppend(self, line, line_num):
      if not line.startswith('//%PDDM-IMPORT-DEFINES '):
        return (False, False)
      assert self.num_lines_captured == 0
      self.Append(line)
      return (True, False)

    def BindMacroCollection(self, macro_colletion):
      if not macro_colletion:
        return
      if self._import_resolver is None:
        raise PDDMError('Got an IMPORT-DEFINES without a resolver (line %d):'
                        ' "%s".' % (self.first_line_num, self.first_line))
      import_name = self.first_line.split(' ', 1)[1].strip()
      imported_file = self._import_resolver(import_name)
      if imported_file is None:
        raise PDDMError('Resolver failed to find "%s" (line %d):'
                        ' "%s".' %
                        (import_name, self.first_line_num, self.first_line))
      try:
        imported_src_file = SourceFile(imported_file, self._import_resolver)
        imported_src_file._ParseFile()
        for section in imported_src_file._sections:
          section.BindMacroCollection(macro_colletion)
      except PDDMError as e:
        raise PDDMError('%s\n...while importing defines:\n'
                        '  Line %d: %s' %
                        (e.message, self.first_line_num, self.first_line))

  def _ParseFile(self):
    self._sections = []
    lines = self._original_content.splitlines()
    cur_section = None
    for line_num, line in enumerate(lines, 1):
      if not cur_section:
        cur_section = self._MakeSection(line, line_num)
      was_added, accept_more = cur_section.TryAppend(line, line_num)
      if not was_added:
        cur_section = self._MakeSection(line, line_num)
        was_added, accept_more = cur_section.TryAppend(line, line_num)
        assert was_added
      if not accept_more:
        cur_section = None

    if cur_section:
      cur_section.HitEOF()

  def _MakeSection(self, line, line_num):
    if not line.startswith('//%PDDM'):
      section = self.TextSection(line_num)
    else:
      directive = line.split(' ', 1)[0]
      if directive == '//%PDDM-EXPAND':
        section = self.ExpansionSection(line_num)
      elif directive == '//%PDDM-DEFINE':
        section = self.DefinitionSection(line_num)
      elif directive == '//%PDDM-IMPORT-DEFINES':
        section = self.ImportDefinesSection(line_num, self._import_resolver)
      else:
        raise PDDMError('Unexpected line %d: "%s".' % (line_num, line))
    self._sections.append(section)
    return section

  def ProcessContent(self, strip_expansion=False):
    """Processes the file contents."""
    self._ParseFile()
    if strip_expansion:
      # Without a collection the expansions become blank, removing them.
      collection = None
    else:
      collection = MacroCollection()
    for section in self._sections:
      section.BindMacroCollection(collection)
    result = ''
    for section in self._sections:
      result += section.text
    self._processed_content = result

  @property
  def original_content(self):
    return self._original_content

  @property
  def processed_content(self):
    return self._processed_content


def main(args):
  usage = '%prog [OPTIONS] PATH ...'
  description = (
      'Processes PDDM directives in the given paths and write them back out.'
  )
  parser = optparse.OptionParser(usage=usage, description=description)
  parser.add_option('--dry-run',
                    default=False, action='store_true',
                    help='Don\'t write back to the file(s), just report if the'
                    ' contents needs an update and exit with a value of 1.')
  parser.add_option('--verbose',
                    default=False, action='store_true',
                    help='Reports is a file is already current.')
  parser.add_option('--collapse',
                    default=False, action='store_true',
                    help='Removes all the generated code.')
  opts, extra_args = parser.parse_args(args)

  if not extra_args:
    parser.error('Need at least one file to process')

  result = 0
  for a_path in extra_args:
    if not os.path.exists(a_path):
      sys.stderr.write('ERROR: File not found: %s\n' % a_path)
      return 100

    def _ImportResolver(name):
      # resolve based on the file being read.
      a_dir = os.path.dirname(a_path)
      import_path = os.path.join(a_dir, name)
      if not os.path.exists(import_path):
        return None
      return open(import_path, 'r')

    with open(a_path, 'r') as f:
      src_file = SourceFile(f, _ImportResolver)

    try:
      src_file.ProcessContent(strip_expansion=opts.collapse)
    except PDDMError as e:
      sys.stderr.write('ERROR: %s\n...While processing "%s"\n' %
                       (e.message, a_path))
      return 101

    if src_file.processed_content != src_file.original_content:
      if not opts.dry_run:
        print('Updating for "%s".' % a_path)
        with open(a_path, 'w') as f:
          f.write(src_file.processed_content)
      else:
        # Special result to indicate things need updating.
        print('Update needed for "%s".' % a_path)
        result = 1
    elif opts.verbose:
      print('No update for "%s".' % a_path)

  return result


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
