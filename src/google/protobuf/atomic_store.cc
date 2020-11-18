// --- start __atomic_store
#if defined (__MVS__)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CSG(_op1, _op2, _op3)                                                  \
  __asm(" csg %0,%2,%1 \n " : "+r"(_op1), "+m"(_op2) : "r"(_op3) :)

#define CS(_op1, _op2, _op3)                                                   \
  __asm(" cs %0,%2,%1 \n " : "+r"(_op1), "+m"(_op2) : "r"(_op3) :)

extern void __atomic_store_real(int size,
                                    void* ptr,
                                    void* val,
                                    int memorder) asm("__atomic_store");
void __atomic_store_real(int size, void* ptr, void* val, int memorder) {
  if (size == 4) {
    unsigned int new_val = *(unsigned int*)val;
    unsigned int* stor = (unsigned int*)ptr;
    unsigned int org;
    unsigned int old_val;
    do {
      org = *(unsigned int*)ptr;
      old_val = org;
      CS(old_val, *stor, new_val);
    } while (old_val != org);
  } else if (size == 8) {
    unsigned long new_val = *(unsigned long*)val;
    unsigned long* stor = (unsigned long*)ptr;
    unsigned long org;
    unsigned long old_val;
    do {
      org = *(unsigned long*)ptr;
      old_val = org;
      CSG(old_val, *stor, new_val);
    } while (old_val != org);
  } else if (0x40 & *(const char*)209) {
    long cc;
    int retry = 10000;
    while (retry--) {
      __asm(" TBEGIN 0,X'FF00'\n"
            " IPM      %0\n"
            " LLGTR    %0,%0\n"
            " SRLG     %0,%0,28\n"
            : "=r"(cc)::);
      if (0 == cc) {
        memcpy(ptr, val, size);
        __asm(" TEND\n"
              " IPM      %0\n"
              " LLGTR    %0,%0\n"
              " SRLG     %0,%0,28\n"
              : "=r"(cc)::);
        if (0 == cc) break;
      }
    }
    if (retry < 1) {
      fprintf(stderr,
              "%s:%s:%d size=%d target=%p source=%p store failed\n",
              __FILE__,
              __FUNCTION__,
              __LINE__,
              size,
              ptr,
              val);
      abort();
    }
  } else {
    fprintf(stderr,
            "%s:%s:%d size=%d target=%p source=%p not implimented\n",
            __FILE__,
            __FUNCTION__,
            __LINE__,
            size,
            ptr,
            val);
    abort();
  }
}

#endif
// --- end __atomic_store