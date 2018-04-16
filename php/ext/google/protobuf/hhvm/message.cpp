#include "protobuf_cpp.h"
#include "upb.h"

// -----------------------------------------------------------------------------
// Define static methods
// -----------------------------------------------------------------------------

// Callback invoked by upb if any error occurs during parsing or serialization.
static bool env_error_func(void* ud, const upb_status* status) {
    char err_msg[100] = "";
    stackenv* se = static_cast<stackenv*>(ud);
    // Free the env -- zend_error will longjmp up the stack past the
    // encode/decode function so it would not otherwise have been freed.
    stackenv_uninit(se);

    // TODO(teboring): have a way to verify that this is actually a parse error,
    // instead of just throwing "parse error" unconditionally.
    // sprintf(err_msg, se->php_error_template, upb_status_errmsg(status));
    // zend_throw_exception(NULL, err_msg, 0 TSRMLS_CC);
    // Never reached.
    return false;
}

void stackenv_init(stackenv* se, const char* errmsg) {
  se->php_error_template = errmsg;
  upb_env_init2(&se->env, se->allocbuf, sizeof(se->allocbuf), NULL);
  upb_env_seterrorfunc(&se->env, env_error_func, se);
}

void stackenv_uninit(stackenv* se) {
  upb_env_uninit(&se->env);
}


// -----------------------------------------------------------------------------
// Message
// -----------------------------------------------------------------------------

void Message_init_c_instance(
    Message *intern TSRMLS_DC) {
  intern->msgdef = NULL;
  intern->layout = NULL;
  intern->msg = NULL;
  intern->arena = NULL;
}

void Message_deepclean(upb_msg *msg, const upb_msgdef *m) {
}

void Message_free_c(
    Message *intern TSRMLS_DC) {
  ARENA_DTOR(intern->arena);
}

void Message___construct(Message* intern, const upb_msgdef* msgdef) {
  // Create layout
  const upb_msglayout* layout =
      upb_msgfactory_getlayout(message_factory, msgdef);

  // Alloc message
  intern->msgdef = msgdef;
  intern->layout = layout;

  upb_arena* arena;
  ARENA_INIT(intern->arena, arena);

  upb_alloc *alloc = upb_arena_alloc(arena);
  intern->msg = (upb_msg*)upb_malloc(alloc, upb_msg_sizeof(layout));
  intern->msg = upb_msg_init(intern->msg, layout, alloc);
}

void Message_wrap(Message* intern, upb_msg *msg,
                  const upb_msgdef *msgdef, ARENA arena) {
  // Create layout
  const upb_msglayout* layout =
      upb_msgfactory_getlayout(message_factory, msgdef);

  intern->msgdef = msgdef;
  intern->layout = layout;
  intern->msg = msg;

  intern->arena = arena;
  ARENA_ADDREF(arena);
}

void Message_clear(Message* intern) {
  upb_alloc* a = upb_msg_alloc(intern->msg);
  void* mem = upb_msg_uninit(intern->msg, intern->layout);
  upb_msg_init(mem, intern->layout, a);
}

void Message_mergeFrom(Message* from, Message* to) {
  UPB_ASSERT(from->msgdef == to->msgdef);

  stackenv se;
  stackenv_init(&se, "Error occurred during encoding: %s");
  size_t size;
  const char* data = upb_encode2(from->msg, from->layout, &se.env, &size);

  Message_mergeFromString(to, data, size);
  stackenv_uninit(&se);
}

void Message_mergeFromString(
    Message* intern, const char* data, size_t size) {
  upb_env env;
  upb_alloc *alloc = upb_msg_alloc(intern->msg);
  upb_env_init2(&env, NULL, 0, alloc);
  upb_decode2(upb_stringview_make(data, size),
              intern->msg, intern->layout, &env);
}
