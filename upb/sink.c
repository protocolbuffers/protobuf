
#include "upb/sink.h"

bool upb_bufsrc_putbuf(const char *buf, size_t len, upb_bytessink *sink) {
  void *subc;
  bool ret;
  upb_bufhandle handle;
  upb_bufhandle_init(&handle);
  upb_bufhandle_setbuf(&handle, buf, 0);
  ret = upb_bytessink_start(sink, len, &subc);
  if (ret && len != 0) {
    ret = (upb_bytessink_putbuf(sink, subc, buf, len, &handle) >= len);
  }
  if (ret) {
    ret = upb_bytessink_end(sink);
  }
  upb_bufhandle_uninit(&handle);
  return ret;
}

struct upb_bufsink {
  upb_byteshandler handler;
  upb_bytessink sink;
  upb_env *env;
  char *ptr;
  size_t len, size;
};

static void *upb_bufsink_start(void *_sink, const void *hd, size_t size_hint) {
  upb_bufsink *sink = _sink;
  UPB_UNUSED(hd);
  UPB_UNUSED(size_hint);
  sink->len = 0;
  return sink;
}

static size_t upb_bufsink_string(void *_sink, const void *hd, const char *ptr,
                                size_t len, const upb_bufhandle *handle) {
  upb_bufsink *sink = _sink;
  size_t new_size = sink->size;

  UPB_ASSERT(new_size > 0);
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);

  while (sink->len + len > new_size) {
    new_size *= 2;
  }

  if (new_size != sink->size) {
    sink->ptr = upb_env_realloc(sink->env, sink->ptr, sink->size, new_size);
    sink->size = new_size;
  }

  memcpy(sink->ptr + sink->len, ptr, len);
  sink->len += len;

  return len;
}

upb_bufsink *upb_bufsink_new(upb_env *env) {
  upb_bufsink *sink = upb_env_malloc(env, sizeof(upb_bufsink));
  upb_byteshandler_init(&sink->handler);
  upb_byteshandler_setstartstr(&sink->handler, upb_bufsink_start, NULL);
  upb_byteshandler_setstring(&sink->handler, upb_bufsink_string, NULL);

  upb_bytessink_reset(&sink->sink, &sink->handler, sink);

  sink->env = env;
  sink->size = 32;
  sink->ptr = upb_env_malloc(env, sink->size);
  sink->len = 0;

  return sink;
}

void upb_bufsink_free(upb_bufsink *sink) {
  upb_env_free(sink->env, sink->ptr);
  upb_env_free(sink->env, sink);
}

upb_bytessink *upb_bufsink_sink(upb_bufsink *sink) {
  return &sink->sink;
}

const char *upb_bufsink_getdata(const upb_bufsink *sink, size_t *len) {
  *len = sink->len;
  return sink->ptr;
}
