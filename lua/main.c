// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
