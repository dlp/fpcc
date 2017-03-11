/**
 * fpcc-paths - Print paths to files contained in an index.
 *
 * (c) 2017, Daniel Prokesch <daniel.prokesch@gmail.com>
 */
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "common.h"

const char *program_name = "fpcc-paths";


void usage(void)
{
  (void) fprintf(stderr, "USAGE: %s file...\n", program_name);
  exit(EXIT_FAILURE);
}


/**
 * Load the fingerprint index from a file to the provided idx.
 */
void load_file(const char *fname)
{
  FILE *f = fopen(fname, "r");
  uint32_t hash_cnt, path_cnt;

  if (f == NULL) {
    goto fail;
  }
  // skip the hashes
  if (fread(&hash_cnt, sizeof hash_cnt, 1, f) != 1) {
    goto fail;
  }
  if (fseek(f, hash_cnt * sizeof(hash_entry_t), SEEK_CUR) != 0) {
    goto fail;
  }
  // read the paths
  if (fread(&path_cnt, sizeof path_cnt, 1, f) != 1) {
    goto fail;
  }
  char *path_buf = malloc(PATH_MAX);
  size_t len = PATH_MAX;
  for (int i = 0; i < path_cnt; i++) {
    if (getdelim(&path_buf, &len, '\0', f) == -1) {
      goto fail;
    }
    if (puts(path_buf) == EOF) {
      error_exit("cannot print path");
    }
  }
  free(path_buf);
  (void) fclose(f);
  return;

fail: ;
  char msg[PATH_MAX];
  (void) snprintf(msg, sizeof msg, "error reading '%s'", fname);
  error_exit(msg);
}


int main(int argc, char *argv[])
{
  if (argc > 0) program_name = argv[0];

  int c;
  while ((c = getopt(argc, argv, "")) != -1) {
    switch (c) {
      case '?':
      default:
        usage();
    }
  }
  // at least one argument
  if (argc - optind < 1) usage();

  while (optind < argc) {
    load_file(argv[optind++]);
  }
  exit(EXIT_SUCCESS);
}

