#!/usr/bin/env python
# Usage: ./update_version.py

import datetime
import json
import re
import sys
from xml.dom import minidom

versions = json.load(open('versions.json', 'r'))
protoc_version = versions['versions'][0]['protoc_version'].split('-')
RC_VERSION = len(protoc_version) > 1 and int(protoc_version[1][2:]) or -1
NEW_VERSION = protoc_version[0]
NEW_VERSION_INFO = [int(x) for x in NEW_VERSION.split('.')]

major_versions = {}
for (language, version) in versions['versions'][0]['languages'].items():
  major_versions[language] = int(version.split('.')[0])

def Find(elem, tagname):
  for child in elem.childNodes:
    if child.nodeName == tagname:
      return child
  return None


def FindAndClone(elem, tagname):
  return Find(elem, tagname).cloneNode(True)


def ReplaceText(elem, text):
  elem.firstChild.replaceWholeText(text)


def GetFullVersion(lang, rc_suffix = '-rc-'):
  if RC_VERSION < 0:
    return '%d.%s' % (major_versions[lang], NEW_VERSION)
  else:
    return '%d.%s%s%s' % (major_versions[lang], NEW_VERSION, rc_suffix, RC_VERSION)


def GetSharedObjectVersion():
  protobuf_version_offset = 11
  return [NEW_VERSION_INFO[0] + protobuf_version_offset, NEW_VERSION_INFO[1], 0]


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
    file_handle.write(b'<?xml version="1.0" encoding="UTF-8"?>\n')
  file_handle.write(content.encode('utf-8'))
  file_handle.write(b'\n')
  file_handle.close()


def RewriteTextFile(filename, line_rewriter):
  lines = open(filename, 'r').readlines()
  updated_lines = []
  for line in lines:
    updated_lines.append(line_rewriter(line))
  if lines == updated_lines:
    print('%s was not updated. Please double check.' % filename)
  f = open(filename, 'w')
  f.write(''.join(updated_lines))
  f.close()


def UpdateCMake():
  cmake_files = (
    'cmake/libprotobuf.cmake',
    'cmake/libprotobuf-lite.cmake',
    'cmake/libprotoc.cmake'
  )
  for cmake_file in cmake_files:
    RewriteTextFile(cmake_file,
      lambda line : re.sub(
        r'SOVERSION ([0-9]+)$',
        'SOVERSION %s' % GetSharedObjectVersion()[0],
        line))


def UpdateConfigure():
  RewriteTextFile('configure.ac',
    lambda line : re.sub(
      r'^AC_INIT\(\[Protocol Buffers\],\[.*\],\[protobuf@googlegroups.com\],\[protobuf\]\)$',
      ('AC_INIT([Protocol Buffers],[%s],[protobuf@googlegroups.com],[protobuf])'
        % GetFullVersion('cpp')),
      line))


def UpdateCpp():
  cpp_version = '%d%03d%03d' % (
    major_versions['cpp'], NEW_VERSION_INFO[0], NEW_VERSION_INFO[1])
  version_suffix = ''
  if RC_VERSION != -1:
    version_suffix = '-rc%s' % RC_VERSION
  def RewriteCommon(line):
    line = re.sub(
      r'^#define GOOGLE_PROTOBUF_VERSION .*$',
      '#define GOOGLE_PROTOBUF_VERSION %s' % cpp_version,
      line)
    line = re.sub(
      r'^#define PROTOBUF_VERSION .*$',
      '#define PROTOBUF_VERSION %s' % cpp_version,
      line)
    line = re.sub(
        r'^#define GOOGLE_PROTOBUF_VERSION_SUFFIX .*$',
        '#define GOOGLE_PROTOBUF_VERSION_SUFFIX "%s"' % version_suffix,
        line)
    line = re.sub(
        r'^#define PROTOBUF_VERSION_SUFFIX .*$',
        '#define PROTOBUF_VERSION_SUFFIX "%s"' % version_suffix,
        line)
    if NEW_VERSION_INFO[1] == 0:
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
    line = re.sub(
        r'^#define PROTOBUF_VERSION_SUFFIX .*$',
        '#define PROTOBUF_VERSION_SUFFIX "%s"' % version_suffix,
        line)
    if NEW_VERSION_INFO[1] == 0:
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
      GetFullVersion('csharp', rc_suffix = '-rc')),
    add_xml_prefix=False)

  RewriteXml('csharp/Google.Protobuf.Tools.nuspec',
    lambda document : ReplaceText(
      Find(Find(document.documentElement, 'metadata'), 'version'),
      GetFullVersion('csharp', rc_suffix = '-rc')))


def UpdateJava():
  RewriteXml('java/pom.xml',
    lambda document : ReplaceText(
      Find(document.documentElement, 'version'), GetFullVersion('java')))

  RewriteXml('java/bom/pom.xml',
    lambda document : ReplaceText(
      Find(document.documentElement, 'version'), GetFullVersion('java')))

  RewriteXml('java/core/pom.xml',
    lambda document : ReplaceText(
      Find(Find(document.documentElement, 'parent'), 'version'),
      GetFullVersion('java')))

  RewriteXml('java/lite/pom.xml',
    lambda document : ReplaceText(
      Find(Find(document.documentElement, 'parent'), 'version'),
      GetFullVersion('java')))

  RewriteXml('java/util/pom.xml',
    lambda document : ReplaceText(
      Find(Find(document.documentElement, 'parent'), 'version'),
      GetFullVersion('java')))

  RewriteXml('java/kotlin/pom.xml',
    lambda document : ReplaceText(
      Find(Find(document.documentElement, 'parent'), 'version'),
      GetFullVersion('kotlin')))

  RewriteXml('java/kotlin-lite/pom.xml',
    lambda document : ReplaceText(
      Find(Find(document.documentElement, 'parent'), 'version'),
      GetFullVersion('kotlin')))

  RewriteXml('protoc-artifacts/pom.xml',
    lambda document : ReplaceText(
      Find(document.documentElement, 'version'), GetFullVersion('java')))

  RewriteTextFile('java/README.md',
    lambda line : re.sub(
      r'<version>.*</version>',
      '<version>%s</version>' % GetFullVersion('java'),
      line))

  RewriteTextFile('java/README.md',
    lambda line : re.sub(
      r'implementation \'com.google.protobuf:protobuf-java:.*\'',
      'implementation \'com.google.protobuf:protobuf-java:%s\'' % GetFullVersion('java'),
      line))

  RewriteTextFile('java/lite.md',
    lambda line : re.sub(
      r'<version>.*</version>',
      '<version>%s</version>' % GetFullVersion('java'),
      line))


def UpdateJavaScript():
  RewriteTextFile('js/package.json',
    lambda line : re.sub(
      r'^  "version": ".*",$',
      '  "version": "%s",' % GetFullVersion('javascript', rc_suffix = '-rc.'),
      line))


def UpdateMakefile():
  RewriteTextFile('src/Makefile.am',
    lambda line : re.sub(
      r'^PROTOBUF_VERSION = .*$',
      'PROTOBUF_VERSION = %s' % ":".join(map(str,GetSharedObjectVersion())),
      line))


def UpdateObjectiveC():
  RewriteTextFile('Protobuf.podspec',
    lambda line : re.sub(
      r"^  s.version  = '.*'$",
      "  s.version  = '%s'" % GetFullVersion('objectivec', rc_suffix = '-rc'),
      line))
  RewriteTextFile('Protobuf-C++.podspec',
    lambda line : re.sub(
      r"^  s.version  = '.*'$",
      "  s.version  = '%s'" % GetFullVersion('cpp', rc_suffix = '-rc'),
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
    ReplaceText(Find(version, 'release'), GetFullVersion('php', rc_suffix = 'RC'))
    ReplaceText(Find(version, 'api'), NEW_VERSION)
    stability = Find(root, 'stability')
    ReplaceText(Find(stability, 'release'),
        'stable' if RC_VERSION < 0 else 'beta')
    ReplaceText(Find(stability, 'api'), 'stable' if RC_VERSION < 0 else 'beta')
    changelog = Find(root, 'changelog')
    for old_version in changelog.getElementsByTagName('version'):
      if Find(old_version, 'release').firstChild.nodeValue == NEW_VERSION:
        print ('[WARNING] Version %s already exists in the change log.'
          % NEW_VERSION)
        return
    if RC_VERSION != 0:
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
          CreateNode('notes', 3, []),
        ])
      changelog.appendChild(release)
      changelog.appendChild(document.createTextNode('\n '))
  RewriteXml('php/ext/google/protobuf/package.xml', Callback)
  RewriteTextFile('php/ext/google/protobuf/protobuf.h',
    lambda line : re.sub(
      r"^#define PHP_PROTOBUF_VERSION .*$",
      "#define PHP_PROTOBUF_VERSION \"%s\"" % GetFullVersion('php', rc_suffix = 'RC'),
      line))

def UpdatePython():
  RewriteTextFile('python/google/protobuf/__init__.py',
    lambda line : re.sub(
      r"^__version__ = '.*'$",
      "__version__ = '%s'" % GetFullVersion('python', rc_suffix = 'rc'),
      line))

def UpdateRuby():
  RewriteXml('ruby/pom.xml',
             lambda document : ReplaceText(
                 Find(document.documentElement, 'version'), GetFullVersion('ruby')))
  RewriteXml('ruby/pom.xml',
             lambda document : ReplaceText(
                 Find(Find(Find(document.documentElement, 'dependencies'), 'dependency'), 'version'),
                 GetFullVersion('ruby')))
  RewriteTextFile('ruby/google-protobuf.gemspec',
    lambda line : re.sub(
      r'^  s.version     = ".*"$',
      '  s.version     = "%s"' % GetFullVersion('ruby', rc_suffix = '.rc.'),
      line))

def UpdateBazel():
  RewriteTextFile('protobuf_version.bzl',
    lambda line : re.sub(
     r"^PROTOBUF_JAVA_VERSION = '.*'$",
     "PROTOBUF_JAVA_VERSION = '%s'" % GetFullVersion('java'),
     line))
  RewriteTextFile('protobuf_version.bzl',
    lambda line : re.sub(
     r"^PROTOBUF_KOTLIN_VERSION = '.*'$",
     "PROTOBUF_KOTLIN_VERSION = '%s'" % GetFullVersion('java'),
     line))
  RewriteTextFile('protobuf_version.bzl',
    lambda line : re.sub(
     r"^PROTOC_VERSION = '.*'$",
     "PROTOC_VERSION = '%s'" % versions['versions'][0]['protoc_version'],
     line))


UpdateCMake()
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
UpdateBazel()
