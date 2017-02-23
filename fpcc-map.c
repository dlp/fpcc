/**
 * fpcc-map - Find similar regions in files.
 *
 * Takes two fingerprint indices and finds common regions,
 * that is, consecutive matching hashes.
 *
 * The regions are printed to stdout, one line for each region, in the format:
 * file1:start1,count1 -- file2:start2,count2
 * where start and count are line numbers.
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

/**
 * The fingerprint index as stored in a file by fpcc-idx.
 */
struct index {
  uint32_t hash_cnt, path_cnt;
  hash_entry_t *hashes;
  char **paths;
};


/**
 * An iterator for hash_entries of a specified hash value
 */
struct hash_iter {
  // invariant: ptr points to the next element,
  // last to the last of all available hashes
  hash_entry_t *ptr, *last;
};

// Matching regions (consecutive hashes) below this value are not emitted.
// The units are number of hashes.
static int min_region_size = DEFAULT_MIN_REGION_SIZE;


void usage(void)
{
  (void) fprintf(stderr, "USAGE: %s [-m min_region_size] file1 file2\n",
      program_name);
  (void) fprintf(stderr, "\nOptions:\n"
      "  min_region_size ... matching regions (consecutive hashes) below\n"
      "                      this value are not emitted. Default: %d\n",
      DEFAULT_MIN_REGION_SIZE);
  exit(EXIT_FAILURE);
}


/**
 * Load the fingerprint index from a file to the provided idx.
 */
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
 * Initialize an iterator for a certain hash value of a given index.
 * An index can contain more than one hash of the same value.
 *
 * Q: Why is an entry passed as argument and not just the hash value?
 * A: Because we can utilize hash_cmp which operates on hash_entry_t*.
 */
void hash_iter_init(struct hash_iter *it, struct index *idx,
    hash_entry_t *entry)
{
  it->ptr = bsearch(entry, idx->hashes, idx->hash_cnt, sizeof(hash_entry_t),
      (int (*)(const void *, const void *))hash_cmp);
  // find the first hash of a certain value (bsearch found any)
  if (it->ptr != NULL) {
    while (hash_cmp(it->ptr, it->ptr-1) == 0) it->ptr--;
  }

  // store the last of the hashes to avoid indexing out of bounds
  // while iterating
  it->last = (idx->hash_cnt > 0) ? &idx->hashes[idx->hash_cnt - 1] : NULL;
}

/**
 * Get the next element from the iterator, or NULL if there is no more.
 */
hash_entry_t *hash_iter_next(struct hash_iter *it)
{
  hash_entry_t *res = it->ptr;
  if (it->ptr != NULL && it->ptr != it->last &&
      hash_cmp(it->ptr, it->ptr+1) == 0) {
    it->ptr++;
  } else {
    it->ptr = NULL;
  }
  return res;
}


/**
 * Print a matching region to stdout in the format:
 * file1:start1,count1 -- file2:start2,count2
 *
 * start and count are line numbers.
 */
void record(int match_size,
    char *fname1, int beg1, int end1,
    char *fname2, int beg2, int end2)
{
  ssize_t res = printf("%s:%d,%d -- %s:%d,%d\n",
      fname1, beg1, end1-beg1, fname2, beg2, end2-beg2);
  if (res < 0) {
    error_exit("cannot output match");
  }
}


/**
 * "Construct" idx_tgt from idx_src by finding matching regions for prefixes
 * of idx_tgt in idx_src.
 *
 * The altorihm is based on following report, except we have binary search
 * for finding prefixes in the source.
 * Walter Tichy, "The String-to-String Correction Problem with
 * Block Moves" (1983).
 */
void construct_target(struct index *idx_src, struct index *idx_tgt)
{
  // iterate target in input order
  int k = idx_tgt->hashes[0].next;
  while (k > 0) {
    hash_entry_t *src, *tgt = &idx_tgt->hashes[k];
    DBG("target %016lx l%d f%d n%d\n", tgt->hash, tgt->linepos,
        tgt->filecnt, tgt->next);

    // walk through source and find the longest common prefix
    struct hash_iter it;
    hash_entry_t *tgt_end = tgt;
    // store the best match (longest chain)
    hash_entry_t *best_src, *best_src_end;
    int best_count = 0;
    hash_iter_init(&it, idx_src, tgt);
    while ((src = hash_iter_next(&it)) != NULL) {
      DBG("source %016lx l%d f%d n%d\n", src->hash, src->linepos,
          src->filecnt, src->next);

      // follow both chains as long as they're the same, count
      hash_entry_t *t = tgt, *s = src;
      hash_entry_t *tn, *sn; // next elements
      int count = 1;
      while (s->next != 0 && t->next != 0) {
        sn = &idx_src->hashes[s->next];
        tn = &idx_tgt->hashes[t->next];
        if (sn->filecnt == s->filecnt &&
            tn->filecnt == t->filecnt &&
            hash_cmp(sn, tn) == 0) {
          // extend the chain
          s = sn;
          t = tn;
          count++;
        } else break;
      }

      // store the start/end of the longest common chain
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
      record(best_count,
          idx_src->paths[best_src->filecnt],
          best_src->linepos, best_src_end->linepos,
          idx_tgt->paths[tgt->filecnt], tgt->linepos, tgt_end->linepos
          );
    }
    k = tgt_end->next;
  }
}

void common_substring(struct index *idx_src, struct index *idx_tgt)
{
  int longest;
  hash_entry_t *hs = idx_src->hashes, *ht = idx_tgt->hashes;


  // actual and last row of the dynamic programming table
  int *dp0 = malloc(idx_tgt->hash_cnt * sizeof(int));
  int *dp1 = malloc(idx_tgt->hash_cnt * sizeof(int));

  // prepare supplemental data structures
  struct suppl {
    int prev;
    int term;
  };

  struct suppl *suppls = malloc(idx_src->hash_cnt * sizeof(struct suppl));
  struct suppl *supplt = malloc(idx_tgt->hash_cnt * sizeof(struct suppl));
  for (int ps = 0, ks = hs[0].next; ks > 0; ps = ks, ks = hs[ks].next) {
    suppls[ks].prev = ps;
    suppls[ps].term = (hs[ps].filecnt != hs[ks].filecnt);
  }
  for (int pt = 0, kt = ht[0].next; kt > 0; pt = kt, kt = ht[kt].next) {
    supplt[kt].prev = pt;
    supplt[pt].term = (ht[pt].filecnt != ht[kt].filecnt);
  }


  do {
    int ls, lt;
    longest = 0;
    for (int ps = 0, ks = hs[0].next; ks > 0; ps = ks, ks = hs[ks].next) {
      // swap pointers
      int *tmp = dp0; dp0 = dp1; dp1 = tmp;
      for (int pt = 0, kt = ht[0].next; kt > 0; pt = kt, kt = ht[kt].next) {
        if (hash_cmp(&hs[ks], &ht[kt]) == 0) {
          if (suppls[ps].term != 0 || supplt[pt].term != 0) {
            dp1[kt] = 1;
          } else {
            dp1[kt] = dp0[pt] + 1;
          }
          if (dp1[kt] >= longest) {
            longest = dp1[kt];
            // store the indices of the end of the longest match
            ls = ks;
            lt = kt;
          }
        } else {
          dp1[kt] = 0;
        }
      }
    }
    // record region if it has substantial size
    if (longest >= min_region_size) {
      // discover matching region from back to front
      int ks = ls, kt = lt;
      for (int i = 1; i < longest; i++) {
        ks = suppls[ks].prev;
        kt = supplt[kt].prev;
      }

      record(longest,
          idx_src->paths[hs[ks].filecnt], hs[ks].linepos, hs[ls].linepos,
          idx_tgt->paths[ht[kt].filecnt], ht[kt].linepos, ht[lt].linepos
          );

      // cut out the chains
      hs[suppls[ks].prev].next = hs[ls].next;
      suppls[suppls[ks].prev].term = 1;
      ht[supplt[kt].prev].next = ht[lt].next;
      supplt[supplt[kt].prev].term = 1;
    }
  } while (longest >= min_region_size);
  free(dp0);
  free(dp1);
  free(suppls);
  free(supplt);
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
        if (min_region_size < 1) usage();
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
  //construct_target(&idx2, &idx1);
  //
  common_substring(&idx2, &idx1);

  cleanup_idx(&idx1);
  cleanup_idx(&idx2);

  exit(EXIT_SUCCESS);
}
