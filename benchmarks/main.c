
#include <stdbool.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

/* Cycle between a bunch of different messages, to avoid performance
 * variations due to memory effects of a particular allocation pattern. */
#ifndef NUM_MESSAGES
#define NUM_MESSAGES 32
#endif

static bool initialize();
static void cleanup();
static size_t run(int i);

int main (int argc, char *argv[])
{
  (void)argc;

  /* Change cwd to where the binary is. */
  char *lastslash = strrchr(argv[0], '/');
  char *progname = argv[0];
  if(lastslash) {
    *lastslash = '\0';
    if(chdir(argv[0]) < 0) {
      fprintf(stderr, "Error changing directory to %s.\n", argv[0]);
      return 1;
    }
    *lastslash = '/';
    progname = lastslash + 3;  /* "/b_" */
  }

  if(!initialize()) {
    fprintf(stderr, "%s: failed to initialize\n", argv[0]);
    return 1;
  }

  size_t total_bytes = 0;
  clock_t before = clock();
  for(int i = 0; true; i++) {
    if((i & 0xFF) == 0 && (clock() - before > CLOCKS_PER_SEC)) break;
    size_t bytes = run(i);
    if(bytes == 0) {
      fprintf(stderr, "%s: failed.\n", argv[0]);
      return 2;
    }
    total_bytes += bytes;
  }
  double elapsed = ((double)clock() - before) / CLOCKS_PER_SEC;
  printf("%s:%d\n", progname, (int)(total_bytes / elapsed / (1 << 20)));
  cleanup();
  return 0;
}
