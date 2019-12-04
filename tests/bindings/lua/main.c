
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "upb/bindings/lua/upb.h"

const char *init =
  "package.preload['lupb'] = ... "
  "package.path = './?.lua;./third_party/lunit/?.lua'";

int main() {
  int ret = 0;
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  lua_pushcfunction(L, luaopen_lupb);
  ret = luaL_loadstring(L, init);
  lua_pushcfunction(L, luaopen_lupb);

  if (ret || lua_pcall(L, 1, LUA_MULTRET, 0) ||
      luaL_dofile(L, "tests/bindings/lua/test_upb.lua")) {
    fprintf(stderr, "error testing Lua: %s\n", lua_tostring(L, -1));
    ret = 1;
  }

  lua_close(L);
  return ret;
}
