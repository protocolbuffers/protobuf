
#include <linux/kernel.h>

#ifndef UPB_LINUX_ASSERT_H
#define UPB_LINUX_ASSERT_H

#ifdef NDEBUG
#define assert(x)
#else
#define assert(x) \
    if (!(x)) panic("Assertion failed: %s at %s:%d", #x, __FILE__, __LINE__);
#endif

#endif
