
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "upb/bindings/lua/upb.h"

int main() {
  int ret = 0;
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  lua_pushcfunction(L, luaopen_lupb);

  if (luaL_dostring(L, "package.preload['lupb'] = ...[1]") ||
      luaL_dofile(L, "tests/bindings/lua/test_upb.lua")) {
    fprintf(stderr, "error testing Lua: %s\n", lua_tostring(L, -1));
    ret = 1;
  }

  lua_close(L);
  return ret;
}
