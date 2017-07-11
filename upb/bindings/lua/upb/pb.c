/*
** require("upb.pb") -- A Lua extension for upb.pb.
**
** Exposes all the types defined in upb/pb/{*}.h
** Also defines a few convenience functions on top.
*/

#include "upb/bindings/lua/upb.h"
#include "upb/pb/decoder.h"
#include "upb/pb/encoder.h"

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

  /* Free resources before we potentially bail on error. */
  upb_env_uninit(&env);
  lupb_checkstatus(L, &status);

  /* References the arena at the top of the stack. */
  lupb_msg_pushref(L, lua_upvalueindex(3), msg);
  return 1;
}

static int lupb_pb_messagetostr(lua_State *L) {
  const lupb_msgclass *lmsgclass = lua_touserdata(L, lua_upvalueindex(1));
  const upb_msg *msg = lupb_msg_checkmsg(L, 1, lmsgclass);
  const upb_visitorplan *vp = lua_touserdata(L, lua_upvalueindex(2));
  const upb_handlers *encode_handlers = lua_touserdata(L, lua_upvalueindex(3));

  upb_env env;
  upb_bufsink *bufsink;
  upb_bytessink *bytessink;
  upb_pb_encoder *encoder;
  upb_visitor *visitor;
  const char *buf;
  size_t len;
  upb_status status = UPB_STATUS_INIT;

  upb_env_init(&env);
  upb_env_reporterrorsto(&env, &status);
  bufsink = upb_bufsink_new(&env);
  bytessink = upb_bufsink_sink(bufsink);
  encoder = upb_pb_encoder_create(&env, encode_handlers, bytessink);
  visitor = upb_visitor_create(&env, vp, upb_pb_encoder_input(encoder));

  upb_visitor_visitmsg(visitor, msg);
  buf = upb_bufsink_getdata(bufsink, &len);
  lua_pushlstring(L, buf, len);

  /* Free resources before we potentially bail on error. */
  upb_env_uninit(&env);
  lupb_checkstatus(L, &status);

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

static int lupb_pb_makemsgtostrencoder(lua_State *L) {
  const lupb_msgclass *lmsgclass = lupb_msgclass_check(L, 1);
  const upb_msgdef *md = lupb_msgclass_getmsgdef(lmsgclass);
  upb_msgfactory *factory = lupb_msgclass_getfactory(lmsgclass);
  const upb_handlers *encode_handlers;
  const upb_visitorplan *vp;

  encode_handlers = upb_pb_encoder_newhandlers(md, &encode_handlers);
  vp = upb_msgfactory_getvisitorplan(factory, encode_handlers);

  /* Push upvalues for the closure. */
  lua_pushlightuserdata(L, (void*)lmsgclass);
  lua_pushlightuserdata(L, (void*)vp);
  lua_pushlightuserdata(L, (void*)encode_handlers);

  /* Upvalues for the closure, only to keep the other upvalues alive. */
  lua_pushvalue(L, 1);
  lupb_refcounted_pushnewrapper(
      L, upb_handlers_upcast(encode_handlers), LUPB_PBDECODERMETHOD, &encode_handlers);

  lua_pushcclosure(L, &lupb_pb_messagetostr, 5);

  return 1;  /* The decoder closure. */
}

static const struct luaL_Reg decodermethod_mm[] = {
  {"__gc", lupb_refcounted_gc},
  {NULL, NULL}
};

static const struct luaL_Reg toplevel_m[] = {
  {"MakeStringToMessageDecoder", lupb_pb_makestrtomsgdecoder},
  {"MakeMessageToStringEncoder", lupb_pb_makemsgtostrencoder},
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
