#include "protobuf_cpp.h"
#include "upb.h"

// -----------------------------------------------------------------------------
// Define static methods
// -----------------------------------------------------------------------------

// Stack-allocated context during an encode/decode operation. Contains the upb
// environment and its stack-based allocator, an initial buffer for allocations
// to avoid malloc() when possible, and a template for PHP exception messages
// if any error occurs.
#define STACK_ENV_STACKBYTES 4096
typedef struct {
  upb_env env;
  const char *php_error_template;
  char allocbuf[STACK_ENV_STACKBYTES];
} stackenv;

static void stackenv_init(stackenv* se, const char* errmsg);
static void stackenv_uninit(stackenv* se);

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

static void stackenv_init(stackenv* se, const char* errmsg) {
  se->php_error_template = errmsg;
  upb_env_init2(&se->env, se->allocbuf, sizeof(se->allocbuf), NULL);
  upb_env_seterrorfunc(&se->env, env_error_func, se);
}

static void stackenv_uninit(stackenv* se) {
  upb_env_uninit(&se->env);
}


// -----------------------------------------------------------------------------
// Message
// -----------------------------------------------------------------------------

void Message_init_c_instance(
    Message *intern TSRMLS_DC) {
}

void Message_free_c(
    Message *intern TSRMLS_DC) {
  // void *msg = upb_msg_uninit(intern->msg, intern->layout);
  // upb_gfree(msg);
  // if (intern->arena) {
  //   upb_alloc *alloc = upb_arena_alloc(intern->arena);
  //   upb_arena_uninit(intern->arena);
  //   FREE(intern->arena);
  // } else {
  //   // FREE(msg);
  // }
}

void Message___construct(Message* intern, const upb_msgdef* msgdef) {
  // Create layout
  const upb_msglayout* layout =
      upb_msgfactory_getlayout(message_factory, msgdef);

  // Alloc message
  intern->msgdef = msgdef;
  intern->layout = layout;
  // intern->arena = reinterpret_cast<upb_arena*>(ALLOC(upb_arena));
  // upb_arena_init(intern->arena);
  // upb_alloc *alloc = upb_arena_alloc(intern->arena);
  intern->msg = (upb_msg*)upb_gmalloc(upb_msg_sizeof(layout));
  intern->msg = upb_msg_init(intern->msg, layout, &upb_alloc_global);
}

void Message_wrap(Message* intern, upb_msg *msg, const upb_msgdef *msgdef) {
  // Create layout
  const upb_msglayout* layout =
      upb_msgfactory_getlayout(message_factory, msgdef);

  // Alloc message
  intern->msgdef = msgdef;
  intern->layout = layout;
  intern->msg = msg;
}

const char* Message_serializeToString(Message* intern, size_t* size) {
  stackenv se;
  stackenv_init(&se, "Error occurred during encoding: %s");
  const char* data = upb_encode2(intern->msg, intern->layout, &se.env, size);
  stackenv_uninit(&se);
  return data;
}

void Message_mergeFromString(
    Message* intern, const char* data, size_t size) {
  upb_env env;
  void *allocbuf = malloc(STACK_ENV_STACKBYTES);
  upb_alloc *alloc = upb_msg_alloc(intern->msg);
  upb_env_init2(&env, allocbuf, sizeof(allocbuf), alloc);

  upb_decode2(upb_stringview_make(data, size),
              intern->msg, intern->layout, &env);
}
