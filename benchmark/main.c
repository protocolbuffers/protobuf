
#include <stdbool.h>
#include <time.h>
#include <stdio.h>

static bool initialize();
static void cleanup();
static size_t run();

int main (int argc, char *argv[])
{
  if(!initialize()) {
    fprintf(stderr, "%s: failed to initialize\n", argv[0]);
    return 1;
  }

  size_t total_bytes = 0;
  clock_t before = clock();
  for(int i = 0; true; i++) {
    if((i & 0xFF) == 0 && (clock() - before > CLOCKS_PER_SEC)) break;
    size_t bytes = run();
    if(bytes == 0) {
      fprintf(stderr, "%s: failed.\n", argv[0]);
      return 2;
    }
    total_bytes += bytes;
  }
  double elapsed = ((double)clock() - before) / CLOCKS_PER_SEC;
  printf("%s: %d\n", argv[0], (int)(total_bytes / elapsed / (1 << 20)));
  cleanup();
  return 0;
}
