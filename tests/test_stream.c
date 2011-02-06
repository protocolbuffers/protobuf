
#undef NDEBUG  /* ensure tests always assert. */
#include "upb_stream.h"
#include "upb_string.h"

typedef struct {
  upb_string *str;
  bool should_delegate;
} test_data;

extern upb_handlerset test_handlers;

static void strappendf(upb_string *s, const char *format, ...) {
  upb_string *str = upb_string_new();
  va_list args;
  va_start(args, format);
  upb_string_vprintf(str, format, args);
  va_end(args);
  upb_strcat(s, str);
  upb_string_unref(str);
}

static upb_flow_t startmsg(void *closure) {
  test_data *d = closure;
  strappendf(d->str, "startmsg\n");
  return UPB_CONTINUE;
}

static upb_flow_t endmsg(void *closure) {
  test_data *d = closure;
  strappendf(d->str, "endmsg\n");
  return UPB_CONTINUE;
}

static upb_flow_t value(void *closure, struct _upb_fielddef *f, upb_value val) {
  (void)f;
  test_data *d = closure;
  strappendf(d->str, "value, %lld\n", upb_value_getint64(val));
  return UPB_CONTINUE;
}

static upb_flow_t startsubmsg(void *closure, struct _upb_fielddef *f,
                              upb_handlers *delegate_to) {
  (void)f;
  test_data *d = closure;
  strappendf(d->str, "startsubmsg\n");
  if (d->should_delegate) {
    upb_register_handlerset(delegate_to, &test_handlers);
    upb_set_handler_closure(delegate_to, closure, NULL);
    return UPB_DELEGATE;
  } else {
    return UPB_CONTINUE;
  }
}

static upb_flow_t endsubmsg(void *closure) {
  test_data *d = closure;
  strappendf(d->str, "endsubmsg\n");
  return UPB_CONTINUE;
}

static upb_flow_t unknownval(void *closure, upb_field_number_t fieldnum,
                             upb_value val) {
  (void)val;
  test_data *d = closure;
  strappendf(d->str, "unknownval, %d\n", fieldnum);
  return UPB_CONTINUE;
}

upb_handlerset test_handlers = {
  &startmsg,
  &endmsg,
  &value,
  &startsubmsg,
  &endsubmsg,
  &unknownval,
};

static void test_dispatcher() {
  test_data data;
  data.should_delegate = false;
  data.str = upb_string_new();
  upb_handlers h;
  upb_handlers_init(&h);
  upb_handlers_reset(&h);
  upb_register_handlerset(&h, &test_handlers);
  upb_set_handler_closure(&h, &data, NULL);
  upb_dispatcher d;
  upb_dispatcher_init(&d);
  upb_dispatcher_reset(&d, &h, false);

  upb_dispatch_startmsg(&d);
  upb_value val;
  upb_value_setint64(&val, 5);
  upb_dispatch_value(&d, NULL, val);
  upb_dispatch_startsubmsg(&d, NULL);
  data.should_delegate = true;
  upb_dispatch_startsubmsg(&d, NULL);
  data.should_delegate = false;
  upb_dispatch_startsubmsg(&d, NULL);
  upb_dispatch_value(&d, NULL, val);
  upb_dispatch_endsubmsg(&d);
  upb_dispatch_endsubmsg(&d);
  upb_dispatch_endsubmsg(&d);
  upb_dispatch_endmsg(&d);

  upb_string expected = UPB_STACK_STRING(
      "startmsg\n"
      "value, 5\n"
      "startsubmsg\n"
      "startsubmsg\n"
      "startmsg\n"  // Because of the delegation.
      "startsubmsg\n"
      "value, 5\n"
      "endsubmsg\n"
      "endmsg\n"   // Because of the delegation.
      "endsubmsg\n"
      "endsubmsg\n"
      "endmsg\n");
  assert(upb_streql(data.str, &expected));
  upb_string_unref(data.str);
}

int main() {
  test_dispatcher();
  return 0;
}
