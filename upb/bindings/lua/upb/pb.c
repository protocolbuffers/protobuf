/*
** require("upb.pb") -- A Lua extension for upb.pb.
**
** Exposes all the types defined in upb/pb/{*}.h
** Also defines a few convenience functions on top.
*/

#include "upb/bindings/lua/upb.h"
#include "upb/pb/decoder.h"

#define LUPB_PBDECODERMETHOD "lupb.pb.decodermethod"

static int lupb_pb_strtomessage(lua_State *L) {
  size_t len;
  upb_status status = UPB_STATUS_INIT;
  const char *pb = lua_tolstring(L, 1, &len);
  const upb_msglayout *layout = lua_touserdata(L, lua_upvalueindex(1));
  const upb_pbdecodermethod *method = lua_touserdata(L, lua_upvalueindex(2));
  const upb_handlers *handlers = upb_pbdecodermethod_desthandlers(method);

  upb_arena *msg_arena;
  upb_env env;
  upb_sink sink;
  upb_pbdecoder *decoder;
  void *msg;

  lupb_arena_new(L);
  msg_arena = lupb_arena_check(L, -1);

  msg = upb_msg_new(layout, upb_arena_alloc(msg_arena));
  upb_env_init(&env);
  upb_env_reporterrorsto(&env, &status);
  upb_sink_reset(&sink, handlers, msg);
  decoder = upb_pbdecoder_create(&env, method, &sink);
  upb_bufsrc_putbuf(pb, len, upb_pbdecoder_input(decoder));

  /* TODO: This won't get called in the error case, which longjmp's across us.
   * This will cause the memory to leak.  To remedy this, we should make the
   * upb_env wrapped in a userdata that guarantees this will get called. */
  upb_env_uninit(&env);

  lupb_checkstatus(L, &status);

  /* References the arena at the top of the stack. */
  lupb_msg_pushref(L, lua_upvalueindex(3), msg);
  return 1;
}

static int lupb_pb_makestrtomsgdecoder(lua_State *L) {
  const upb_msglayout *layout = lupb_msgclass_getlayout(L, 1);
  const upb_handlers *handlers = lupb_msgclass_getmergehandlers(L, 1);
  const upb_pbdecodermethod *m;

  upb_pbdecodermethodopts opts;
  upb_pbdecodermethodopts_init(&opts, handlers);

  m = upb_pbdecodermethod_new(&opts, &m);

  /* Push upvalues for the closure. */
  lua_pushlightuserdata(L, (void*)layout);
  lua_pushlightuserdata(L, (void*)m);
  lua_pushvalue(L, 1);

  /* Upvalue for the closure, only to keep the decodermethod alive. */
  lupb_refcounted_pushnewrapper(
      L, upb_pbdecodermethod_upcast(m), LUPB_PBDECODERMETHOD, &m);

  lua_pushcclosure(L, &lupb_pb_strtomessage, 4);

  return 1;  /* The decoder closure. */
}

static const struct luaL_Reg decodermethod_mm[] = {
  {"__gc", lupb_refcounted_gc},
  {NULL, NULL}
};

static const struct luaL_Reg toplevel_m[] = {
  {"MakeStringToMessageDecoder", lupb_pb_makestrtomsgdecoder},
  {NULL, NULL}
};

int luaopen_upb_pb_c(lua_State *L) {
  static char module_key;
  if (lupb_openlib(L, &module_key, "upb.pb_c", toplevel_m)) {
    return 1;
  }

  lupb_register_type(L, LUPB_PBDECODERMETHOD, NULL, decodermethod_mm);

  return 1;
}
