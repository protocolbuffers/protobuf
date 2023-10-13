// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
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
//     * Neither the name of Google LLC nor the names of its
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

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <signal.h>

#include "lua/upb.h"

lua_State* L;

static void interrupt(lua_State* L, lua_Debug* ar) {
  (void)ar;
  lua_sethook(L, NULL, 0, 0);
  luaL_error(L, "SIGINT");
}

static void sighandler(int i) {
  fprintf(stderr, "Signal!\n");
  signal(i, SIG_DFL);
  lua_sethook(L, interrupt, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
}

const char* init =
    "package.preload['lupb'] = ... "
    "package.path = '"
    "./?.lua;"
    "./third_party/lunit/?.lua;"
    "external/com_google_protobuf/?.lua;"
    "external/com_google_protobuf/src/?.lua;"
    "bazel-bin/?.lua;"
    "bazel-bin/external/com_google_protobuf/src/?.lua;"
    "bazel-bin/external/com_google_protobuf/?.lua;"
    "lua/?.lua;"
    // These additional paths handle the case where this test is invoked from
    // the protobuf repo's Bazel workspace.
    "external/?.lua;"
    "external/third_party/lunit/?.lua;"
    "src/?.lua;"
    "bazel-bin/external/?.lua;"
    "external/lua/?.lua"
    "'";

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "missing argument with path to .lua file\n");
    return 1;
  }

  int ret = 0;
  L = luaL_newstate();
  luaL_openlibs(L);
  lua_pushcfunction(L, luaopen_lupb);
  ret = luaL_loadstring(L, init);
  lua_pushcfunction(L, luaopen_lupb);

  signal(SIGINT, sighandler);
  ret = ret || lua_pcall(L, 1, LUA_MULTRET, 0) || luaL_dofile(L, argv[1]);
  signal(SIGINT, SIG_DFL);

  if (ret) {
    fprintf(stderr, "error testing Lua: %s\n", lua_tostring(L, -1));
    ret = 1;
  }

  lua_close(L);
  return ret;
}
