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
 * Two algorithms are implemented:
 *
 * 1) String-to-String Correction (STSC)
 *    see
 *    - Walter Tichy, "The String-to-String Correction Problem with
 *      Block Moves" (1983).
 *      (http://docs.lib.purdue.edu/cgi/viewcontent.cgi?article=1377&context=cstech)
 *
 * 2) Iterated Longest Common Substring (ILCS)
 *    see
 *    - https://en.wikipedia.org/wiki/Longest_common_substring_problem#Dynamic_programming
 *    - http://stackoverflow.com/a/10067660/5949973
 *
 * Benchmarked: ILCS is about 23 times slower than STSC.
 * It would be nice to see the situation when GSTs are used for ILCS
 * instead of dynamic programming.
 *
 * (c) 2017, Daniel Prokesch <daniel.prokesch@gmail.com>
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

// Matching regions (consecutive hashes) below this value are not emitted.
// The units are number of hashes.
static int min_region_size = DEFAULT_MIN_REGION_SIZE;

// the two implemented alternative algorithms
void string_to_string(struct index *, struct index *);
void iterated_lcs(struct index *, struct index *);


void usage(void)
{
  (void) fprintf(stderr, "USAGE: %s [-l] [-m min_region_size] target source\n",
      program_name);
  (void) fprintf(stderr, "\nOptions:\n"
      "  -l                 ... use ILCS instead of the default STSC\n"
      "  -m min_region_size ... matching regions (consecutive hashes) below\n"
      "                         this value are not emitted. Default: %d\n",
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
 * Print a matching region to stdout in the format:
 * file1:start1,count1 -- file2:start2,count2
 *
 * start and count are line numbers.
 */
void record(int match_size,
    char *fname1, int beg1, int end1,
    char *fname2, int beg2, int end2)
{
  if (match_size >= min_region_size) {
    ssize_t res = printf("%s:%d,%d -- %s:%d,%d\n",
        fname1, beg1, end1-beg1, fname2, beg2, end2-beg2);
    if (res < 0) {
      error_exit("cannot output match");
    }
  }
}


int main(int argc, char *argv[])
{
  int opt_l = 0, opt_m = 0;
  int c;

  if (argc > 0) program_name = argv[0];

  while ((c = getopt(argc, argv, "lm:")) != -1) {
    switch (c) {
      case 'l':
        if (opt_l++ > 0) usage();
        break;
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

  struct index idx_source, idx_target;

  load_file(argv[optind], &idx_target);
  load_file(argv[optind + 1], &idx_source);

  // decide on an algorithm
  if (opt_l > 0) {
    iterated_lcs(&idx_source, &idx_target);
  } else {
    // default algorithm
    string_to_string(&idx_source, &idx_target);
  }

  cleanup_idx(&idx_source);
  cleanup_idx(&idx_target);

  exit(EXIT_SUCCESS);
}



///////////////////////////////////////////////////////////////////////////////
// Algorithm 1: String-to-String Correction
///////////////////////////////////////////////////////////////////////////////

/**
 * An iterator for hash_entries of a specified hash value
 */
struct hash_iter {
  // invariant: ptr points to the next element,
  // last to the last of all available hashes
  hash_entry_t *ptr, *last;
};

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
 * The String-To-String Correction algorithm works by finding maximal
 * subchains in source to "construct" target.
 *
 * In contrast to the original report, we have binary search
 * for finding prefixes in the source.
 */
void string_to_string(struct index *idx_src, struct index *idx_tgt)
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
    record(best_count,
        idx_tgt->paths[tgt->filecnt],
        tgt->linepos, tgt_end->linepos,
        idx_src->paths[best_src->filecnt],
        best_src->linepos, best_src_end->linepos
        );
    k = tgt_end->next;
  }
}


///////////////////////////////////////////////////////////////////////////////
// Algorithm 2: Iterated Longest Common Substring
///////////////////////////////////////////////////////////////////////////////

/**
 * Supplemental data element for hash entry
 */
struct hash_entry_suppl {
  int prev; // index of the previous hash in the chain
  int term; // terminator flag indicates chain boundaries
};

/**
 * Create an array of hash_entry_suppl the same size as the number of
 * hash_entries in the index, and initialize it.
 */
struct hash_entry_suppl *suppl_create(struct index *idx)
{
  struct hash_entry_suppl *suppl =
    malloc(idx->hash_cnt * sizeof(struct hash_entry_suppl));

  // iterate once through the hashes in order and
  // store the previous pointer and set the term flag
  // upon change of the filename
  for (int previ = 0, curi = idx->hashes[0].next; curi > 0;
      previ = curi, curi = idx->hashes[curi].next) {
    suppl[curi ].prev = previ;
    suppl[previ].term =
      (idx->hashes[previ].filecnt != idx->hashes[curi].filecnt);
  }
  return suppl;
}

/**
 * Unlink a subchain from the hashes chain.
 * The subchain is specified via its begin and end index.
 */
void unlink_subchain(hash_entry_t *hashes, struct hash_entry_suppl *suppl,
    int ibegin, int iend)
{
  // link the predecessor of ibegin to the successor of iend
  hashes[suppl[ibegin].prev].next = hashes[iend].next;
  // set the terminate flag for the predecessor of ibegin
  suppl[suppl[ibegin].prev].term = 1;
  // link the successor of iend to the predecessor of ibegin (prev-link)
  suppl[hashes[iend].next].prev = suppl[ibegin].prev;
}


/**
 * The Iterated Longest Common Substring algorithm will repeatedly search
 * for the longest common chain in the chain of hashes and remove it
 * from both chains.
 *
 * Removal of a chain creates boundaries that are managed via termination flags
 * in a supplemental data structure, along with prev-pointers (a doubly-linked
 * list is easier to handle).
 *
 */
void iterated_lcs(struct index *idx_src, struct index *idx_tgt)
{
  int longest;

  // actual and last row of the dynamic programming table
  int *dp0 = malloc(idx_tgt->hash_cnt * sizeof(int));
  int *dp1 = malloc(idx_tgt->hash_cnt * sizeof(int));

  // create supplemental data structures
  struct hash_entry_suppl *suppls = suppl_create(idx_src),
                          *supplt = suppl_create(idx_tgt);

  hash_entry_t *hs = idx_src->hashes, *ht = idx_tgt->hashes;
  do {
    // these indices will point to the end of a matching region in both hashes
    int ls, lt;
    longest = 0;
    for (int ks = hs[0].next; ks > 0; ks = hs[ks].next) {
      int ps = suppls[ks].prev;
      // swap pointers: dp0 gets dp1,
      // dp1 is the fresh row, its contents are discarded
      int *tmp = dp0; dp0 = dp1; dp1 = tmp;
      for (int kt = ht[0].next; kt > 0; kt = ht[kt].next) {
        int pt = supplt[kt].prev;
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

    if (longest > 0) {
      // Discover the matching region from back to front.
      // This is the real motivation for the prev-links.
      int ks = ls, kt = lt;
      for (int i = 1; i < longest; i++) {
        ks = suppls[ks].prev;
        kt = supplt[kt].prev;
      }

      record(longest,
          idx_tgt->paths[ht[kt].filecnt], ht[kt].linepos, ht[lt].linepos,
          idx_src->paths[hs[ks].filecnt], hs[ks].linepos, hs[ls].linepos
          );

      // cut out the chains
      unlink_subchain(hs, suppls, ks, ls);
      unlink_subchain(ht, supplt, kt, lt);
    }
  } while (longest > 0);

  // cleanup the dyn-prog table rows and the supplemental data structures
  free(dp0);
  free(dp1);
  free(suppls);
  free(supplt);
}


