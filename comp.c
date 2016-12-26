/**
 * comp.c - compare fingerprints produced by csig
 */

#include <errno.h>
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

const char *program_name = "comp";

sig_t *new_sig(void);
void load(const char *, sig_t *);
int compare(sig_t *, sig_t *, sig_t *);


// the global list of document's fingerprints
// - a dynamically growing array
static sig_t *siglist;
static int sl_cnt=0, sl_cap=0;

void usage(void)
{
  (void) fprintf(stderr,
      "USAGE: %s [-b basefile] [-t threshold] sigfile1 sigfile2\n",
      program_name);
  (void) fprintf(stderr,
      "USAGE: %s [-b basefile] [-t threshold] [-L filelist]\n",
      program_name);
  exit(EXIT_FAILURE);
}

long int parse_num(const char *s)
{
  char *eptr;
  long int res;

  res = strtol(s, &eptr, 10);
  if (*eptr != '\0') usage();
  return res;
}

int main(int argc, char *argv[])
{
  int opt_b=0, opt_t=0, opt_L=0, opt_c=0;
  const char *filelist = NULL;
  char *basefile = NULL;

  int c;
  while ((c = getopt(argc, argv, "b:ct:L:")) != -1) {
    switch (c) {
      case 'b':
        if (opt_b++ > 0) usage();
        basefile = optarg;
        break;
      case 'c':
        if (opt_c++ > 0) usage();
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

  const char *outfmt;
  if (opt_c > 0) {
    outfmt = "%s and %s: %d%%\n";
  } else {
    outfmt = "%s;%s;%d\n";
  }

  for (int i=0; i < sl_cnt; i++) {
    for (int j=i+1; j < sl_cnt; j++) {
      int percent = compare(&siglist[i], &siglist[j], &basesig);
      if (percent >= thresh) {
        if (printf(outfmt, siglist[i].fname, siglist[j].fname, percent) < 0) {
          (void) fprintf(stderr, "%s: cannot print result: %s\n",
              program_name, strerror(errno));
        }
      }
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
      (void) fprintf(stderr, "%s: cannot allocate memory\n",
          program_name);
      exit(EXIT_FAILURE);
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

  int cnt = fread(&hash_count, sizeof hash_count, 1, f);
  if (cnt != 1) {
    (void) fprintf(stderr, "%s: error reading %s: %s\n",
        program_name, fname, strerror(errno));
    exit(EXIT_FAILURE);
  }

  hash_buf = malloc(hash_count * sizeof(hash_t));
  if (hash_buf == NULL) {
    (void) fprintf(stderr, "%s: can't allocate buffer\n", program_name);
    exit(EXIT_FAILURE);
  }
  if (fread(hash_buf, sizeof(hash_t), hash_count, f) < hash_count) {
    (void) fprintf(stderr, "%s: error reading %s: %s\n",
        program_name, fname, strerror(errno));
    exit(EXIT_FAILURE);
  }
  (void) fclose(f);

  sig->fname = strdup(fname);
  sig->count = hash_count;
  sig->hashes = hash_buf;
}


int compare(sig_t *s0, sig_t *s1, sig_t *sb)
{
  int i0=0, i1=0, ib=0, nboth=0, nexcl=0;

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
      nboth++;
      // check against the base file
      while (ib < sb->count && sb->hashes[ib] < s0->hashes[i0]) ib++;
      if (ib < sb->count && sb->hashes[ib] == s0->hashes[i0]) {
        nexcl++;
        ib++;
      }
    }
    if (cmp <= 0) i0++;
    if (cmp >= 0) i1++;
  }

  // invariant: nexcl <= sX->count
  // therefore, following only holds if s0 == s1 == sb
  if (s0->count + s1->count == 2 * nexcl) {
    // per definition; think as limit, when the base part -> whole file
    return 100;
  }
  // similarity/resemblance of A,B:
  // r(A, B) = |(A n B)\C| / |(A u B)\C|
  // We assume multisets:
  // {x,y} u {x,z} = {x,x,y,z} => |A u B| = |A| + |B|
  return 100 * 2*(nboth - nexcl) / (s0->count + s1->count - 2*nexcl);
}

