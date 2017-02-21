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

static int min_region_size = DEFAULT_MIN_REGION_SIZE;


void usage(void)
{
  (void) fprintf(stderr, "USAGE: %s file1 file2\n", program_name);
  (void) fprintf(stderr, "  defaults: min_region_size=%d\n",
      DEFAULT_MIN_REGION_SIZE);
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
  // A NULL at the end enables following iteration scheme
  /*
  for (char **pptr = idx1.paths; *pptr != NULL; pptr++) {
    DBG("Path: %s\n", *pptr);
  }
  */

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


void cleanup_idx(struct index *idx)
{
  free(idx->hashes);
  for (char **pptr = idx->paths; *pptr != NULL; free(*pptr++)) ;
  free(idx->paths);
}

/**
 * An iterator for hash_entries of a specified hash value
 */
struct hash_iter {
  hash_entry_t *ptr;
};


void hash_iter_init(struct hash_iter *it, struct index *idx,
    hash_entry_t *entry)
{
  it->ptr = bsearch(entry, idx->hashes, idx->hash_cnt, sizeof(hash_entry_t),
      (int (*)(const void *, const void *))hash_cmp);

  if (it->ptr != NULL) {
    // find first
    while (hash_cmp(&it->ptr->hash, &(it->ptr-1)->hash) == 0) it->ptr--;
  }
}

hash_entry_t *hash_iter_next(struct hash_iter *it)
{
  hash_entry_t *res = it->ptr;
  if (it->ptr != NULL && hash_cmp(&it->ptr->hash, &(it->ptr+1)->hash) == 0) {
    it->ptr++;
  } else {
    it->ptr = NULL;
  }
  return res;
}


int main(int argc, char *argv[])
{
  int opt_m = 0;
  int c;
  while ((c = getopt(argc, argv, "m:")) != -1) {
    switch (c) {
      case 'm':
        if (opt_m++ > 0) usage();
        min_region_size = parse_num(optarg);
        if (min_region_size < 0) usage();
        break;
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

  // use Tichy's algorithm for finding maximal string moves

  // iterate target in input order
  int k = idx2.hashes[0].next;
  while (k > 0) {
    hash_entry_t *src, *tgt = &idx2.hashes[k];
    DBG("%016lx l%d f%d n%d\n", tgt->hash, tgt->linepos,
        tgt->filecnt, tgt->next);

    // walk through source and find the longest common prefix
    struct hash_iter it;
    hash_iter_init(&it, &idx1, tgt);
    // store the best match (longest chain)
    hash_entry_t *best_src, *best_src_end, *tgt_end = tgt;
    int best_count = 0;
    while ((src = hash_iter_next(&it)) != NULL) {
      DBG("other %016lx l%d f%d n%d\n", src->hash, src->linepos,
          src->filecnt, src->next);

      // follow both chains as long as they're the same, count
      hash_entry_t *t = tgt, *s = src;
      hash_entry_t *tn, *sn;
      int count = 1;
      do {
        if (s->next == 0 || t->next == 0) break;
        sn = &idx1.hashes[s->next];
        tn = &idx2.hashes[t->next];
        if (sn->filecnt == s->filecnt &&
            tn->filecnt == t->filecnt &&
            hash_cmp(&sn->hash, &tn->hash) == 0) {
          // extend the chain
          s = sn;
          t = tn;
          count++;
        } else break;
      } while (1);

      // store the last element in the chain
      if (count > best_count) {
        best_src = src;
        best_src_end = s;
        tgt_end = t;
        best_count = count;
      }
      assert(count >= 0);
      DBG("chain length: %d\n", count);
    }
    DBG("best chain length: %d\n", best_count);
    // record region if it has substantial size
    if (best_count >= min_region_size) {
      DBG("REGION: size %d, ---%s:%d,%d +++%s:%d,%d\n", best_count,
          idx1.paths[best_src->filecnt],
          best_src->linepos, best_src_end->linepos,
          idx2.paths[tgt->filecnt], tgt->linepos, tgt_end->linepos
          );
    }
    k = tgt_end->next;
  }

  cleanup_idx(&idx1);
  cleanup_idx(&idx2);

  exit(EXIT_SUCCESS);
}
