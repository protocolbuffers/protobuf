
#include "upb/upb.h"
#include "upb/port_def.inc"

#ifdef UPB_MSVC_VSNPRINTF
/* Visual C++ earlier than 2015 doesn't have standard C99 snprintf and
 * vsnprintf. To support them, missing functions are manually implemented
 * using the existing secure functions. */
int msvc_vsnprintf(char* s, size_t n, const char* format, va_list arg) {
  if (!s) {
    return _vscprintf(format, arg);
  }
  int ret = _vsnprintf_s(s, n, _TRUNCATE, format, arg);
  if (ret < 0) {
	ret = _vscprintf(format, arg);
  }
  return ret;
}

int msvc_snprintf(char* s, size_t n, const char* format, ...) {
  va_list arg;
  va_start(arg, format);
  int ret = msvc_vsnprintf(s, n, format, arg);
  va_end(arg);
  return ret;
}
#endif
