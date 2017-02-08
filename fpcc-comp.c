/**
 * fpcc-comp.c - Compare fingerprints produced by fpcc-sig
 *
 * Author: Daniel Prokesch <daniel.prokesch@gmail.com>
 */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "common.h"

typedef struct {
  char *fname; // filename
  unsigned count; //  number of hashes
  hash_t *hashes; // pointer to array of hashes
} sig_t;

int thresh = DEFAULT_THRESHOLD;

const char *program_name = "fpcc-comp";

sig_t *new_sig(void);
void load(const char *, sig_t *);
void count(int *, int *, sig_t *, sig_t *, sig_t *);

// the global list of document's fingerprints
// - a dynamically growing array
static sig_t *siglist;
static int sl_cnt=0, sl_cap=0;

/**
 * Print a usage message to stderr and exit with EXIT_FAILURE.
 */
void usage(void)
{
  (void) fprintf(stderr,
      "USAGE: %s [-b basefile] [-c|-i] [-t threshold] sigfile1 sigfile2\n",
      program_name);
  (void) fprintf(stderr,
      "USAGE: %s [-b basefile] [-c|-i] [-t threshold] [-L filelist]\n",
      program_name);
  exit(EXIT_FAILURE);
}


/**
 * Compute the Resemblance of A and B:
 * r(A, B) = |(A n B)\C| / |(A u B)\C|
 */
inline static int resemblance(int na, int nb, int nboth, int nexcl)
{
  if (na > 0 || nb > 0) {
    // invariant: nexcl <= nX
    // therefore, following only holds if nA == nB == nexcl
    if (na + nb == 2 * nexcl) {
      // per definition; think as limit, when the base part -> whole file
      return 100;
    } else {
      // We assume multisets:
      // {x,y} u {x,z} = {x,x,y,z} => |A u B| = |A| + |B|
      return 100 * 2*(nboth - nexcl) / (na + nb - 2*nexcl);
    }
  }
  return 0;
}

/**
 * Compute the Containment of A in B:
 * c(A, B) = |(A n B)\C| / |A\C|
 */
inline static int containment(int na, int nboth, int nexcl)
{
  if (na != nexcl) {
    return 100 * (nboth - nexcl) / (na - nexcl);
  }
  return 0;
}


void print_if_threshold(const char *join,
    const char *fname1, const char *fname2,
    int value, int threshold)
{
  if (value >= threshold) {
    if (printf("%s %s %s: %d%%\n", fname1, join, fname2, value) < 0) {
      (void) fprintf(stderr, "%s: cannot print result: %s\n",
          program_name, strerror(errno));
    }
  }
}


int main(int argc, char *argv[])
{
  int opt_b=0, opt_t=0, opt_L=0, opt_c=0, opt_i=0;
  const char *filelist = NULL;
  char *basefile = NULL;

  int c;
  while ((c = getopt(argc, argv, "b:cit:L:")) != -1) {
    switch (c) {
      case 'b':
        if (opt_b++ > 0) usage();
        basefile = optarg;
        break;
      case 'c':
        if (opt_c++ > 0) usage();
        break;
      case 'i':
        if (opt_i++ > 0) usage();
        break;
      case 't':
        if (opt_t++ > 0) usage();
        thresh = parse_num(optarg);
        if (thresh < 0 || thresh > 100)
          usage();
        break;
      case 'L':
        if (opt_L++ > 0) usage();
        filelist = optarg;
        break;
      case '?':
      default:
        usage();
    }
  }
  if (opt_c + opt_i > 1) usage();

  int npargs = argc - optind;

  sig_t basesig = {.fname=NULL, .count=0, .hashes=NULL};

  if (basefile != NULL) {
    load(basefile, &basesig);
  }

  if (filelist != NULL) {
    // reading the signatures from a file containing a list of files,
    // one line each
    if (npargs != 0) usage();
    FILE *f = fopen(filelist, "r");
    if (f == NULL) {
      (void) fprintf(stderr, "%s: cannot open %s: %s\n",
          program_name, filelist, strerror(errno));
      exit(EXIT_FAILURE);
    }
    char line[LINE_MAX];
    while (fgets(line, LINE_MAX, f) != NULL) {
      line[strcspn(line, "\r\n")] = '\0';
      load(line, new_sig());
    }
    if (ferror(f) != 0) {
      (void) fprintf(stderr, "%s: error reading %s: %s\n",
          program_name, filelist, strerror(errno));
      exit(EXIT_FAILURE);
    }
  } else {
    // comparing two files only
    if (npargs != 2) usage();
    load(argv[optind], new_sig());
    load(argv[optind + 1], new_sig());
  }

  // emit a warning
  if (sl_cnt < 2) {
    (void) fprintf(stderr, "%s: nothing to compare\n", program_name);
  }

  for (int i=0; i < sl_cnt; i++) {
    for (int j=i+1; j < sl_cnt; j++) {
      int nboth, nexcl;
      count(&nboth, &nexcl, &siglist[i], &siglist[j], &basesig);

      if (opt_c > 0) {
        // csv-format output all three
        int rb = resemblance(siglist[i].count, siglist[j].count, nboth, nexcl);
        int ct1 = containment(siglist[i].count, nboth, nexcl);
        int ct2 = containment(siglist[j].count, nboth, nexcl);
        if (rb >= thresh || ct1 >= thresh || ct2 >= thresh) {
          if (printf("%s;%s;%d;%d;%d\n",
                siglist[i].fname, siglist[j].fname, rb, ct1, ct2) < 0) {
            error_exit("cannot print result");
          }
        }
      } else {
        // conventional output
        if (opt_i > 0) {
          // print containment
          print_if_threshold("in", siglist[i].fname, siglist[j].fname,
              containment(siglist[i].count, nboth, nexcl), thresh);
          print_if_threshold("in", siglist[j].fname, siglist[i].fname,
              containment(siglist[j].count, nboth, nexcl), thresh);
        } else {
          // print resemblance
          print_if_threshold("and", siglist[j].fname, siglist[i].fname,
              resemblance(siglist[i].count, siglist[j].count, nboth, nexcl),
              thresh);
        } // end if conventional output
      } // end if csv-format
    }
  }

  // free the hashes of each list item
  for (int i = 0; i < sl_cnt; i++) {
    sig_t *sig = &siglist[i];
    free(sig->fname);
    free(sig->hashes);
  }
  // free the list itself
  free(siglist);

  // free the hashes of the basefile
  free(basesig.fname);
  free(basesig.hashes);

  return 0;
}


sig_t *new_sig(void)
{
  // append to global list
  if (sl_cnt == sl_cap) {
    sl_cap += 100;
    sig_t *newlist = realloc(siglist, sl_cap*sizeof(sig_t));
    if (newlist == NULL) {
      error_exit("cannot allocate memory");
    }
    siglist = newlist;
  }
  return &siglist[sl_cnt++];
}


void load(const char *fname, sig_t *sig)
{
  FILE *f;
  uint32_t hash_count;
  hash_t *hash_buf;

  f = fopen(fname, "r");
  if (f == NULL) {
    // if we cannot open a file, we simply return without reading it
    (void) fprintf(stderr, "%s: cannot open %s: %s - skipping\n",
        program_name, fname, strerror(errno));
    return;
  }
  DBG("Reading '%s'\n", fname);

  int cnt = fread(&hash_count, sizeof hash_count, 1, f);
  if (cnt != 1) {
    (void) fprintf(stderr, "%s: error reading %s\n",
        program_name, fname);
    exit(EXIT_FAILURE);
  }
  hash_buf = malloc((hash_count - 1) * sizeof(hash_t));
  if (hash_buf == NULL) {
    error_exit("can't allocate buffer");
  }
  for (int i = 0; i < hash_count; i++) {
    hash_entry_t entry;
    if (fread(&entry, sizeof(hash_entry_t), 1, f) != 1) {
      (void) fprintf(stderr, "%s: error reading %s\n",
          program_name, fname);
      exit(EXIT_FAILURE);
    }
    // the first entry is a dummy entry, which we have to skip
    if (i > 0) {
      DBG("%016lx\n", entry.hash);
      hash_buf[i-1] = entry.hash;
    }
  }
  (void) fclose(f);

  sig->fname = strdup(fname);
  sig->count = hash_count;
  sig->hashes = hash_buf;
}

/**
 * Given two fingerprints s0 and s1, count the number of common
 * fingerprints (nboth) and the number of common fingerprints that need to be
 * excluded because they appear in sb (nexcl).
 */
void count(int *nboth, int *nexcl, sig_t *s0, sig_t *s1, sig_t *sb)
{
  int i0=0, i1=0, ib=0, lboth=0, lexcl=0;
  while (i0 < s0->count || i1 < s1->count) {
    int cmp = 0;
    if (!(i0 < s0->count)) {
      cmp = 1;
    } else if (!(i1 < s1->count)) {
      cmp = -1;
    } else {
      cmp = hash_cmp(&s0->hashes[i0], &s1->hashes[i1]);
    }
    if (cmp == 0) {
      lboth++;
      // check against the base file
      while (ib < sb->count && sb->hashes[ib] < s0->hashes[i0]) ib++;
      if (ib < sb->count && sb->hashes[ib] == s0->hashes[i0]) {
        lexcl++;
        ib++;
      }
    }
    if (cmp <= 0) i0++;
    if (cmp >= 0) i1++;
  }
  *nboth = lboth;
  *nexcl = lexcl;
}
