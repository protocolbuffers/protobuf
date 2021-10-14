// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "names.h"

#include <stdlib.h>

#include "protobuf.h"

/* stringsink *****************************************************************/

typedef struct {
  char *ptr;
  size_t len, size;
} stringsink;

static size_t stringsink_string(stringsink *sink, const char *ptr, size_t len) {
  size_t new_size = sink->size;

  while (sink->len + len > new_size) {
    new_size *= 2;
  }

  if (new_size != sink->size) {
    sink->ptr = realloc(sink->ptr, new_size);
    sink->size = new_size;
  }

  memcpy(sink->ptr + sink->len, ptr, len);
  sink->len += len;

  return len;
}

static void stringsink_init(stringsink *sink) {
  sink->size = 32;
  sink->ptr = malloc(sink->size);
  PBPHP_ASSERT(sink->ptr != NULL);
  sink->len = 0;
}

static void stringsink_uninit(stringsink *sink) { free(sink->ptr); }

/* def name -> classname ******************************************************/

const char *const kReservedNames[] = {
    "abstract",   "and",          "array",      "as",           "break",
    "callable",   "case",         "catch",      "class",        "clone",
    "const",      "continue",     "declare",    "default",      "die",
    "do",         "echo",         "else",       "elseif",       "empty",
    "enddeclare", "endfor",       "endforeach", "endif",        "endswitch",
    "endwhile",   "eval",         "exit",       "extends",      "final",
    "finally",    "fn",           "for",        "foreach",      "function",
    "if",         "implements",   "include",    "include_once", "instanceof",
    "global",     "goto",         "insteadof",  "interface",    "isset",
    "list",       "match",        "namespace",  "new",          "object",
    "or",         "print",        "private",    "protected",    "public",
    "require",    "require_once", "return",     "static",       "switch",
    "throw",      "trait",        "try",        "unset",        "use",
    "var",        "while",        "xor",        "yield",        "int",
    "float",      "bool",         "string",     "true",         "false",
    "null",       "void",         "iterable",   NULL};

bool is_reserved_name(const char* name) {
  int i;
  for (i = 0; kReservedNames[i]; i++) {
    if (strcmp(kReservedNames[i], name) == 0) {
      return true;
    }
  }
  return false;
}

static char nolocale_tolower(char ch) {
  if (ch >= 'A' && ch <= 'Z') {
    return ch - ('A' - 'a');
  } else {
    return ch;
  }
}

static char nolocale_toupper(char ch) {
  if (ch >= 'a' && ch <= 'z') {
    return ch - ('a' - 'A');
  } else {
    return ch;
  }
}

static bool is_reserved(const char *segment, int length) {
  bool result;
  char* lower = calloc(1, length + 1);
  memcpy(lower, segment, length);
  int i = 0;
  while(lower[i]) {
    lower[i] = nolocale_tolower(lower[i]);
    i++;
  }
  lower[length] = 0;
  result = is_reserved_name(lower);
  free(lower);
  return result;
}

static void fill_prefix(const char *segment, int length,
                        const char *prefix_given,
                        const char *package_name,
                        stringsink *classname) {
  if (prefix_given != NULL && strcmp(prefix_given, "") != 0) {
    stringsink_string(classname, prefix_given, strlen(prefix_given));
  } else {
    if (is_reserved(segment, length)) {
      if (package_name != NULL &&
          strcmp("google.protobuf", package_name) == 0) {
        stringsink_string(classname, "GPB", 3);
      } else {
        stringsink_string(classname, "PB", 2);
      }
    }
  }
}

static void fill_segment(const char *segment, int length,
                         stringsink *classname, bool use_camel) {
  if (use_camel && (segment[0] < 'A' || segment[0] > 'Z')) {
    char first = nolocale_toupper(segment[0]);
    stringsink_string(classname, &first, 1);
    stringsink_string(classname, segment + 1, length - 1);
  } else {
    stringsink_string(classname, segment, length);
  }
}

static void fill_namespace(const char *package, const char *php_namespace,
                           stringsink *classname) {
  if (php_namespace != NULL) {
    if (strlen(php_namespace) != 0) {
      stringsink_string(classname, php_namespace, strlen(php_namespace));
      stringsink_string(classname, "\\", 1);
    }
  } else if (package != NULL) {
    int i = 0, j = 0;
    size_t package_len = strlen(package);
    while (i < package_len) {
      j = i;
      while (j < package_len && package[j] != '.') {
        j++;
      }
      fill_prefix(package + i, j - i, "", package, classname);
      fill_segment(package + i, j - i, classname, true);
      stringsink_string(classname, "\\", 1);
      i = j + 1;
    }
  }
}

static void fill_classname(const char *fullname,
                           const char *package,
                           const char *prefix,
                           stringsink *classname) {
  int classname_start = 0;
  if (package != NULL) {
    size_t package_len = strlen(package);
    classname_start = package_len == 0 ? 0 : package_len + 1;
  }
  size_t fullname_len = strlen(fullname);

  int i = classname_start, j;
  while (i < fullname_len) {
    j = i;
    while (j < fullname_len && fullname[j] != '.') {
      j++;
    }
    fill_prefix(fullname + i, j - i, prefix, package, classname);
    fill_segment(fullname + i, j - i, classname, false);
    if (j != fullname_len) {
      stringsink_string(classname, "\\", 1);
    }
    i = j + 1;
  }
}

char *GetPhpClassname(const upb_filedef *file, const char *fullname) {
  // Prepend '.' to package name to make it absolute. In the 5 additional
  // bytes allocated, one for '.', one for trailing 0, and 3 for 'GPB' if
  // given message is google.protobuf.Empty.
  const char *package = upb_filedef_package(file);
  const char *php_namespace = upb_filedef_phpnamespace(file);
  const char *prefix = upb_filedef_phpprefix(file);
  char *ret;
  stringsink namesink;
  stringsink_init(&namesink);

  fill_namespace(package, php_namespace, &namesink);
  fill_classname(fullname, package, prefix, &namesink);
  stringsink_string(&namesink, "\0", 1);
  ret = strdup(namesink.ptr);
  stringsink_uninit(&namesink);
  return ret;
}
