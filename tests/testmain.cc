
#include <stdlib.h>
#ifdef USE_GOOGLE
#include "base/init_google.h"
#endif

extern "C" {
int run_tests(int argc, char *argv[]);
}

int main(int argc, char *argv[]) {
#ifdef USE_GOOGLE
  InitGoogle(NULL, &argc, &argv, true);
#endif
  run_tests(argc, argv);
}
