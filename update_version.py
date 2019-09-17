#!/usr/bin/env python
# Usage: ./update_version.py <MAJOR>.<MINOR>.<MICRO> [<RC version>]
#
# Example:
# ./update_version.py 3.7.1 2
#   => Version will become 3.7.1-rc-2 (beta)
# ./update_version.py 3.7.1
#   => Version will become 3.7.1 (stable)

import datetime
import re
import sys
from xml.dom import minidom

if len(sys.argv) < 2 or len(sys.argv) > 3:
  print """
[ERROR] Please specify a version.

./update_version.py <MAJOR>.<MINOR>.<MICRO> [<RC version>]

Example:
./update_version.py 3.7.1 2
"""
  exit(1)

NEW_VERSION = sys.argv[1]
NEW_VERSION_INFO = [int(x) for x in NEW_VERSION.split('.')]
if len(NEW_VERSION_INFO) != 3:
  print """
[ERROR] Version must be in the format <MAJOR>.<MINOR>.<MICRO>

Example:
./update_version.py 3.7.3
"""
  exit(1)

RC_VERSION = 0
if len(sys.argv) > 2:
  RC_VERSION = int(sys.argv[2])


def Find(elem, tagname):
  for child in elem.childNodes:
    if child.nodeName == tagname:
      return child
  return None


def FindAndClone(elem, tagname):
  return Find(elem, tagname).cloneNode(True)


def ReplaceText(elem, text):
  elem.firstChild.replaceWholeText(text)


def GetFullVersion(rc_suffix = '-rc-'):
  if RC_VERSION == 0:
    return NEW_VERSION
  else:
    return '%s%s%s' % (NEW_VERSION, rc_suffix, RC_VERSION)


def RewriteXml(filename, rewriter, add_xml_prefix=True):
  document = minidom.parse(filename)
  rewriter(document)
  # document.toxml() always prepend the XML version without inserting new line.
  # We wants to preserve as much of the original formatting as possible, so we
  # will remove the default XML version and replace it with our custom one when
  # whever necessary.
  content = document.toxml().replace('<?xml version="1.0" ?>', '')
  file_handle = open(filename, 'wb')
  if add_xml_prefix:
    file_handle.write('<?xml version="1.0" encoding="UTF-8"?>\n')
  file_handle.write(content)
  file_handle.write('\n')
  file_handle.close()


def RewriteTextFile(filename, line_rewriter):
  lines = open(filename, 'r').readlines()
  updated_lines = []
  for line in lines:
    updated_lines.append(line_rewriter(line))
  if lines == updated_lines:
    print '%s was not updated. Please double check.' % filename
  f = open(filename, 'w')
  f.write(''.join(updated_lines))
  f.close()


def UpdateConfigure():
  RewriteTextFile('configure.ac',
    lambda line : re.sub(
      r'^AC_INIT\(\[Protocol Buffers\],\[.*\],\[protobuf@googlegroups.com\],\[protobuf\]\)$',
      ('AC_INIT([Protocol Buffers],[%s],[protobuf@googlegroups.com],[protobuf])'
        % GetFullVersion()),
      line))


def UpdateCpp():
  cpp_version = '%d%03d%03d' % (
    NEW_VERSION_INFO[0], NEW_VERSION_INFO[1], NEW_VERSION_INFO[2])
  def RewriteCommon(line):
    line = re.sub(
      r'^#define GOOGLE_PROTOBUF_VERSION .*$',
      '#define GOOGLE_PROTOBUF_VERSION %s' % cpp_version,
      line)
    line = re.sub(
      r'^#define PROTOBUF_VERSION .*$',
      '#define PROTOBUF_VERSION %s' % cpp_version,
      line)
    if NEW_VERSION_INFO[2] == 0:
      line = re.sub(
        r'^#define PROTOBUF_MIN_HEADER_VERSION_FOR_PROTOC .*$',
        '#define PROTOBUF_MIN_HEADER_VERSION_FOR_PROTOC %s' % cpp_version,
        line)
      line = re.sub(
        r'^#define GOOGLE_PROTOBUF_MIN_PROTOC_VERSION .*$',
        '#define GOOGLE_PROTOBUF_MIN_PROTOC_VERSION %s' % cpp_version,
        line)
      line = re.sub(
        r'^static const int kMinHeaderVersionForLibrary = .*$',
        'static const int kMinHeaderVersionForLibrary = %s;' % cpp_version,
        line)
      line = re.sub(
        r'^static const int kMinHeaderVersionForProtoc = .*$',
        'static const int kMinHeaderVersionForProtoc = %s;' % cpp_version,
        line)
    return line
  
  def RewritePortDef(line):
    line = re.sub(
      r'^#define PROTOBUF_VERSION .*$',
      '#define PROTOBUF_VERSION %s' % cpp_version,
      line)
    if NEW_VERSION_INFO[2] == 0:
      line = re.sub(
        r'^#define PROTOBUF_MIN_HEADER_VERSION_FOR_PROTOC .*$',
        '#define PROTOBUF_MIN_HEADER_VERSION_FOR_PROTOC %s' % cpp_version,
        line)
      line = re.sub(
        r'^#define PROTOBUF_MIN_PROTOC_VERSION .*$',
        '#define PROTOBUF_MIN_PROTOC_VERSION %s' % cpp_version,
        line)
      line = re.sub(
        r'^#define GOOGLE_PROTOBUF_MIN_LIBRARY_VERSION .*$',
        '#define GOOGLE_PROTOBUF_MIN_LIBRARY_VERSION %s' % cpp_version,
        line)
    return line

  def RewritePbH(line):
    line = re.sub(
        r'^#if PROTOBUF_VERSION < .*$',
        '#if PROTOBUF_VERSION < %s' % cpp_version,
        line)
    line = re.sub(
        r'^#if .* < PROTOBUF_MIN_PROTOC_VERSION$',
        '#if %s < PROTOBUF_MIN_PROTOC_VERSION' % cpp_version,
        line)
    return line
    
  RewriteTextFile('src/google/protobuf/stubs/common.h', RewriteCommon)
  RewriteTextFile('src/google/protobuf/port_def.inc', RewritePortDef)
  RewriteTextFile('src/google/protobuf/any.pb.h', RewritePbH)
  RewriteTextFile('src/google/protobuf/api.pb.h', RewritePbH)
  RewriteTextFile('src/google/protobuf/descriptor.pb.h', RewritePbH)
  RewriteTextFile('src/google/protobuf/duration.pb.h', RewritePbH)
  RewriteTextFile('src/google/protobuf/empty.pb.h', RewritePbH)
  RewriteTextFile('src/google/protobuf/field_mask.pb.h', RewritePbH)
  RewriteTextFile('src/google/protobuf/source_context.pb.h', RewritePbH)
  RewriteTextFile('src/google/protobuf/struct.pb.h', RewritePbH)
  RewriteTextFile('src/google/protobuf/timestamp.pb.h', RewritePbH)
  RewriteTextFile('src/google/protobuf/type.pb.h', RewritePbH)
  RewriteTextFile('src/google/protobuf/wrappers.pb.h', RewritePbH)
  RewriteTextFile('src/google/protobuf/compiler/plugin.pb.h', RewritePbH)


def UpdateCsharp():
  RewriteXml('csharp/src/Google.Protobuf/Google.Protobuf.csproj',
    lambda document : ReplaceText(
      Find(Find(document.documentElement, 'PropertyGroup'), 'VersionPrefix'),
      GetFullVersion(rc_suffix = '-rc')),
    add_xml_prefix=False)

  RewriteXml('csharp/Google.Protobuf.Tools.nuspec',
    lambda document : ReplaceText(
      Find(Find(document.documentElement, 'metadata'), 'version'),
      GetFullVersion(rc_suffix = '-rc')))


def UpdateJava():
  RewriteXml('java/pom.xml',
    lambda document : ReplaceText(
      Find(document.documentElement, 'version'), GetFullVersion()))

  RewriteXml('java/bom/pom.xml',
    lambda document : ReplaceText(
      Find(document.documentElement, 'version'), GetFullVersion()))

  RewriteXml('java/core/pom.xml',
    lambda document : ReplaceText(
      Find(Find(document.documentElement, 'parent'), 'version'),
      GetFullVersion()))

  RewriteXml('java/lite/pom.xml',
    lambda document : ReplaceText(
      Find(Find(document.documentElement, 'parent'), 'version'),
      GetFullVersion()))

  RewriteXml('java/util/pom.xml',
    lambda document : ReplaceText(
      Find(Find(document.documentElement, 'parent'), 'version'),
      GetFullVersion()))

  RewriteXml('protoc-artifacts/pom.xml',
    lambda document : ReplaceText(
      Find(document.documentElement, 'version'), GetFullVersion()))


def UpdateJavaScript():
  RewriteTextFile('js/package.json',
    lambda line : re.sub(
      r'^  "version": ".*",$',
      '  "version": "%s",' % GetFullVersion(rc_suffix = '-rc.'),
      line))


def UpdateMakefile():
  protobuf_version_offset = 11
  expected_major_version = 3
  if NEW_VERSION_INFO[0] != expected_major_version:
    print """[ERROR] Major protobuf version has changed. Please update
update_version.py to readjust the protobuf_version_offset and
expected_major_version such that the PROTOBUF_VERSION in src/Makefile.am is
always increasing.
    """
    exit(1)

  protobuf_version_info = '%d:%d:0' % (
    NEW_VERSION_INFO[1] + protobuf_version_offset, NEW_VERSION_INFO[2])
  RewriteTextFile('src/Makefile.am',
    lambda line : re.sub(
      r'^PROTOBUF_VERSION = .*$',
      'PROTOBUF_VERSION = %s' % protobuf_version_info,
      line))


def UpdateObjectiveC():
  RewriteTextFile('Protobuf.podspec',
    lambda line : re.sub(
      r"^  s.version  = '.*'$",
      "  s.version  = '%s'" % GetFullVersion(rc_suffix = '-rc'),
      line))
  RewriteTextFile('Protobuf-C++.podspec',
    lambda line : re.sub(
      r"^  s.version  = '.*'$",
      "  s.version  = '%s'" % GetFullVersion(rc_suffix = '-rc'),
      line))


def UpdatePhp():
  def Callback(document):
    def CreateNode(tagname, indent, children):
      elem = document.createElement(tagname)
      indent += 1
      for child in children:
        elem.appendChild(document.createTextNode('\n' + (' ' * indent)))
        elem.appendChild(child)
      indent -= 1
      elem.appendChild(document.createTextNode('\n' + (' ' * indent)))
      return elem

    root = document.documentElement
    now = datetime.datetime.now()
    ReplaceText(Find(root, 'date'), now.strftime('%Y-%m-%d'))
    ReplaceText(Find(root, 'time'), now.strftime('%H:%M:%S'))
    version = Find(root, 'version')
    ReplaceText(Find(version, 'release'), GetFullVersion(rc_suffix = 'RC'))
    ReplaceText(Find(version, 'api'), NEW_VERSION)
    stability = Find(root, 'stability')
    ReplaceText(Find(stability, 'release'),
        'stable' if RC_VERSION == 0 else 'beta')
    ReplaceText(Find(stability, 'api'), 'stable' if RC_VERSION == 0 else 'beta')
    changelog = Find(root, 'changelog')
    for old_version in changelog.getElementsByTagName('version'):
      if Find(old_version, 'release').firstChild.nodeValue == NEW_VERSION:
        print ('[WARNING] Version %s already exists in the change log.'
          % NEW_VERSION)
        return
    changelog.appendChild(document.createTextNode(' '))
    release = CreateNode('release', 2, [
        CreateNode('version', 3, [
          FindAndClone(version, 'release'),
          FindAndClone(version, 'api')
        ]),
        CreateNode('stability', 3, [
          FindAndClone(stability, 'release'),
          FindAndClone(stability, 'api')
        ]),
        FindAndClone(root, 'date'),
        FindAndClone(root, 'time'),
        FindAndClone(root, 'license'),
        FindAndClone(root, 'notes')
      ])
    changelog.appendChild(release)
    changelog.appendChild(document.createTextNode('\n '))
  RewriteXml('php/ext/google/protobuf/package.xml', Callback)
  RewriteTextFile('php/ext/google/protobuf/protobuf.h',
    lambda line : re.sub(
      r'PHP_PROTOBUF_VERSION ".*"$',
      'PHP_PROTOBUF_VERSION "%s"' % NEW_VERSION,
      line))

  RewriteTextFile('php/ext/google/protobuf/protobuf.h',
    lambda line : re.sub(
      r"^#define PHP_PROTOBUF_VERSION .*$",
      "#define PHP_PROTOBUF_VERSION \"%s\"" % GetFullVersion(rc_suffix = 'RC'),
      line))

  RewriteTextFile('php/ext/google/protobuf/protobuf.h',
    lambda line : re.sub(
      r"^#define PHP_PROTOBUF_VERSION .*$",
      "#define PHP_PROTOBUF_VERSION \"%s\"" % GetFullVersion(rc_suffix = 'RC'),
      line))

def UpdatePython():
  RewriteTextFile('python/google/protobuf/__init__.py',
    lambda line : re.sub(
      r"^__version__ = '.*'$",
      "__version__ = '%s'" % GetFullVersion(rc_suffix = 'rc'),
      line))

def UpdateRuby():
  RewriteTextFile('ruby/google-protobuf.gemspec',
    lambda line : re.sub(
      r'^  s.version     = ".*"$',
      '  s.version     = "%s"' % GetFullVersion(rc_suffix = '.rc.'),
      line))


UpdateConfigure()
UpdateCsharp()
UpdateCpp()
UpdateJava()
UpdateJavaScript()
UpdateMakefile()
UpdateObjectiveC()
UpdatePhp()
UpdatePython()
UpdateRuby()
