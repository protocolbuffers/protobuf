/*
** require("upb.pb") -- A Lua extension for upb.pb.
**
** Exposes all the types defined in upb/pb/{*}.h
** Also defines a few convenience functions on top.
*/

#include "upb/bindings/lua/upb.h"
#include "upb/decode.h"
#include "upb/encode.h"

#define LUPB_PBDECODERMETHOD "lupb.pb.decodermethod"

static int lupb_pb_decode(lua_State *L) {
  size_t len;
  const upb_msglayout *layout;
  upb_msg *msg = lupb_msg_checkmsg2(L, 1, &layout);
  const char *pb = lua_tolstring(L, 2, &len);

  upb_decode(pb, len, msg, layout, lupb_arena_get(L));
  /* TODO(haberman): check for error. */

  return 0;
}

static int lupb_pb_encode(lua_State *L) {
  const upb_msglayout *layout;
  const upb_msg *msg = lupb_msg_checkmsg2(L, 1, &layout);
  upb_arena *arena = upb_arena_new();
  size_t size;
  char *result;

  result = upb_encode(msg, (const void*)layout, arena, &size);

  /* Free resources before we potentially bail on error. */
  lua_pushlstring(L, result, size);
  upb_arena_free(arena);
  /* TODO(haberman): check for error. */

  return 1;
}

static const struct luaL_Reg toplevel_m[] = {
  {"decode", lupb_pb_decode},
  {"encode", lupb_pb_encode},
  {NULL, NULL}
};

int luaopen_upb_pb_c(lua_State *L) {
  static char module_key;
  if (lupb_openlib(L, &module_key, "upb.pb_c", toplevel_m)) {
    return 1;
  }

  return 1;
}
