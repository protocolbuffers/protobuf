/*
** require("upb.pb") -- A Lua extension for upb.pb.
**
** Exposes all the types defined in upb/pb/{*}.h
** Also defines a few convenience functions on top.
*/

#include "upb/bindings/lua/upb.h"
#include "upb/pb/decoder.h"

#define LUPB_PBDECODERMETHOD "lupb.pb.decodermethod"

#define MSGDEF_INDEX 1

static upb_pbdecodermethod *lupb_pbdecodermethod_check(lua_State *L, int narg) {
  return lupb_refcounted_check(L, narg, LUPB_PBDECODERMETHOD);
}

static int lupb_pbdecodermethod_new(lua_State *L) {
  const upb_handlers *handlers = lupb_msg_newwritehandlers(L, 1, &handlers);
  const upb_pbdecodermethod *m;

  upb_pbdecodermethodopts opts;
  upb_pbdecodermethodopts_init(&opts, handlers);

  m = upb_pbdecodermethod_new(&opts, &m);
  upb_handlers_unref(handlers, &handlers);
  lupb_refcounted_pushnewrapper(
      L, upb_pbdecodermethod_upcast(m), LUPB_PBDECODERMETHOD, &m);

  /* We need to keep a pointer to the MessageDef (in Lua space) so we can
   * construct new messages in parse(). */
  lua_newtable(L);
  lua_pushvalue(L, 1);
  lua_rawseti(L, -2, MSGDEF_INDEX);
  lua_setuservalue(L, -2);

  return 1;  /* The DecoderMethod wrapper. */
}

/* Unlike most of our exposed Lua functions, this does not correspond to an
 * actual method on the underlying DecoderMethod.  But it's convenient, and
 * important to implement in C because we can do stack allocation and
 * initialization of our runtime structures like the Decoder and Sink. */
static int lupb_pbdecodermethod_parse(lua_State *L) {
  size_t len;
  const upb_pbdecodermethod *method = lupb_pbdecodermethod_check(L, 1);
  const char *pb = lua_tolstring(L, 2, &len);
  void *msg;
  upb_status status = UPB_STATUS_INIT;
  upb_env env;
  upb_sink sink;
  upb_pbdecoder *decoder;

  const upb_handlers *handlers = upb_pbdecodermethod_desthandlers(method);

  lua_getuservalue(L, 1);
  lua_rawgeti(L, -1, MSGDEF_INDEX);
  lupb_assert(L, !lua_isnil(L, -1));
  lupb_msg_pushnew(L, -1);  /* Push new message. */
  msg = lua_touserdata(L, -1);

  /* Handlers need this. */
  lua_getuservalue(L, -1);

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

  lua_pop(L, 1);  /* Uservalue. */

  return 1;
}

static const struct luaL_Reg lupb_pbdecodermethod_m[] = {
  {"parse", lupb_pbdecodermethod_parse},
  {NULL, NULL}
};

static const struct luaL_Reg toplevel_m[] = {
  {"DecoderMethod", lupb_pbdecodermethod_new},
  {NULL, NULL}
};

int luaopen_upb_pb_c(lua_State *L) {
  static char module_key;
  if (lupb_openlib(L, &module_key, "upb.pb_c", toplevel_m)) {
    return 1;
  }

  lupb_register_type(L, LUPB_PBDECODERMETHOD, lupb_pbdecodermethod_m, NULL,
                     true);

  return 1;
}
