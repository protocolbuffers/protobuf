
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <signal.h>

#include "upb/bindings/lua/upb.h"

lua_State *L;

static void interrupt(lua_State *L, lua_Debug *ar) {
  (void)ar;
  lua_sethook(L, NULL, 0, 0);
  luaL_error(L, "SIGINT");
}

static void sighandler(int i) {
  fprintf(stderr, "Signal!\n");
  signal(i, SIG_DFL);
  lua_sethook(L, interrupt, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
}

const char *init =
  "package.preload['lupb'] = ... "
  "package.path = '"
    "./?.lua;"
    "./third_party/lunit/?.lua;"
    "external/com_google_protobuf/?.lua;"
    "external/com_google_protobuf/src/?.lua;"
    "bazel-bin/?.lua;"
    "bazel-bin/external/com_google_protobuf/src/?.lua;"
    "bazel-bin/external/com_google_protobuf/?.lua;"
    "bazel-bin/external/com_google_protobuf/?.lua;"
    "upb/bindings/lua/?.lua"
  "'";

int main(int argc, char **argv) {
  int ret = 0;
  L = luaL_newstate();
  luaL_openlibs(L);
  lua_pushcfunction(L, luaopen_lupb);
  ret = luaL_loadstring(L, init);
  lua_pushcfunction(L, luaopen_lupb);

  signal(SIGINT, sighandler);
  ret = ret ||
        lua_pcall(L, 1, LUA_MULTRET, 0) ||
        luaL_dofile(L, "tests/bindings/lua/test_upb.lua");
  signal(SIGINT, SIG_DFL);

  if (ret) {
    fprintf(stderr, "error testing Lua: %s\n", lua_tostring(L, -1));
    ret = 1;
  }

  lua_close(L);
  return ret;
}
