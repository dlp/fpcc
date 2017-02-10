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

struct index {
  uint32_t hash_cnt, path_cnt;
  hash_entry_t *hashes;
  char **paths;
};


void usage(void)
{
  (void) fprintf(stderr, "USAGE: %s file1 file2\n", program_name);
  exit(EXIT_FAILURE);
}


void load_file(const char *fname, struct index *idx)
{
  FILE *f = fopen(fname, "r");
  if (f == NULL) {
    goto fail;
  }
  if (fread(&idx->hash_cnt, sizeof idx->hash_cnt, 1, f) != 1) {
    goto fail;
  }
  idx->hashes = malloc((idx->hash_cnt) * sizeof(hash_entry_t));
  if (idx->hashes == NULL) {
    error_exit("can't allocate buffer");
  }
  if (fread(idx->hashes, sizeof(hash_entry_t),
        idx->hash_cnt, f) != idx->hash_cnt) {
    goto fail;
  }
  // now the paths
  if (fread(&idx->path_cnt, sizeof idx->path_cnt, 1, f) != 1) {
    goto fail;
  }
  // one more field that stays NULL
  idx->paths = calloc(idx->path_cnt + 1, sizeof(char *));
  if (idx->paths == NULL) {
    error_exit("can't allocate buffer");
  }
  size_t len = 0;
  for (int i = 0; i < idx->path_cnt; i++) {
    (void) getdelim(&idx->paths[i], &len, '\0', f);
  }

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

  struct index idx1, idx2;
  load_file(argv[optind], &idx1);
  load_file(argv[optind + 1], &idx2);



  // iterate in input order
  int k = idx1.hashes[0].next;
  while (k > 0) {
    hash_entry_t *he = &idx1.hashes[k];
    DBG("%016lx l%d f%d n%d\n", he->hash, he->linepos,
        he->filecnt, he->next);
    k = he->next;
  }
  free(idx1.hashes);

  //for (int i = 0; i < idx1.path_cnt; i++) {
  for (char **pptr = idx1.paths; *pptr != NULL; pptr++) {
    //DBG("Path: %s\n", idx1.paths[i]);
    DBG("Path: %s\n", *pptr);
    //free(idx1.paths[i]);
    free(*pptr);
  }
  free(idx1.paths);

  exit(EXIT_SUCCESS);
}
