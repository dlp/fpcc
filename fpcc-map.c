/**
 * fpcc-map - Show similar regions in files.
 *
 *
 * Author: Daniel Prokesch <daniel.prokesch@gmail.com>
 */
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "common.h"

const char *program_name = "fpcc-map";

struct hashes {
  uint32_t count;
  hash_entry_t *arr;
} hashes1, hashes2;

struct paths {
  uint32_t count;
  char **arr;
} paths1, paths2;


void usage(void)
{
  (void) fprintf(stderr, "USAGE: %s file1 file2\n", program_name);
  exit(EXIT_FAILURE);
}


void load_file(const char *fname, struct hashes *hashes, struct paths *paths)
{
  FILE *f = fopen(fname, "r");
  if (f == NULL) {
    goto fail;
  }
  if (fread(&hashes->count, sizeof hashes->count, 1, f) != 1) {
    goto fail;
  }
  hashes->arr = malloc((hashes->count) * sizeof(hash_entry_t));
  if (hashes->arr == NULL) {
    error_exit("can't allocate buffer");
  }
  if (fread(hashes->arr, sizeof(hash_entry_t),
        hashes->count, f) != hashes->count) {
    goto fail;
  }
  // now the paths
  if (fread(&paths->count, sizeof paths->count, 1, f) != 1) {
    goto fail;
  }
  paths->arr = calloc(paths->count, sizeof(char *));
  if (paths->arr == NULL) {
    error_exit("can't allocate buffer");
  }
  size_t len = 0;
  char **pptr = paths->arr;
  while (getdelim(pptr++, &len, '\0', f) != -1);

  (void) fclose(f);
  return;

fail: ;
  char msg[PATH_MAX];
  (void) snprintf(msg, sizeof msg, "error reading '%s'", fname);
  error_exit(msg);
}



int main(int argc, char *argv[])
{
  int c;
  while ((c = getopt(argc, argv, "")) != -1) {
    switch (c) {
      case '?':
      default:
        usage();
    }
  }
  // exactly two arguments
  if (argc - optind != 2) usage();

  load_file(argv[optind], &hashes1, &paths1);



  // iterate in input order
  int k = hashes1.arr[0].next;
  while (k > 0) {
    hash_entry_t *he = &hashes1.arr[k];
    DBG("%016lx l%d f%d n%d\n", he->hash, he->linepos,
        he->filecnt, he->next);
    k = he->next;
  }
  free(hashes1.arr);


  for (int i = 0; i < paths1.count; i++) {
    DBG("Path: %s\n", paths1.arr[i]);
    free(paths1.arr[i]);
  }
  free(paths1.arr);

  exit(EXIT_SUCCESS);
}
