/*
 *  csig.c - based on sig.c written by Rob Pike
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <openssl/md5.h>


#define DEFAULT_NTOKEN   5
#define DEFAULT_ZEROBITS 4

typedef uint64_t hash_t;

extern int yylex (void);
extern FILE *yyin;

int		Ntoken   = DEFAULT_NTOKEN;
static          int ntoken;
int		Zerobits = DEFAULT_ZEROBITS;
int *		token;
FILE *		outfile;

void	signature(FILE*);

const char *program_name = "csig";

void usage(void)
{
  (void) fprintf(stderr, "USAGE: %s [-z zerobits] [-n chainlength]"
                         " [-o outfile] file...\n", program_name);
  (void) fprintf(stderr, "  defaults: zerobits=%d chainlength=%d\n",
      DEFAULT_ZEROBITS, DEFAULT_NTOKEN);
  exit(2);
}

int main(int argc, char *argv[])
{
	FILE *f;
	int  nfiles;
	char *outname;
        int opt_z=0, opt_n=0, opt_o=0;

	outfile = stdout;
	outname = NULL;

        int c;
	while ((c = getopt(argc, argv, "z:n:o:")) != -1) {
          switch (c) {
            case 'z':
              if (opt_z++ > 0) usage();
              Zerobits = atoi(optarg);
              if (Zerobits < 0 || Zerobits > 31)
                usage();
              break;
            case 'n':
              if (opt_n++ > 0) usage();
              Ntoken = atoi(optarg);
              if (Ntoken <= 0)
                usage();
              break;
            case 'o':
              if (opt_o++ > 0) usage();
              outname = optarg;
              break;
            case '?':
            default:
              usage();
          }
	}

	nfiles = argc - optind;
	if (nfiles < 1) usage();

	if (outname != NULL)
		outfile = fopen(outname, "w");


	for (int i = optind; i < argc; i++) {
          f = fopen(argv[i], "r");
          if (f == NULL) {
            (void) fprintf(stderr,
                "%s: can't open %s:", program_name, argv[i]);
            perror(NULL);
            continue;
          }
          signature(f);
          (void) fclose(f);
	}
	return 0;
}

hash_t hash(int tok[])
{
  MD5_CTX md5;
  union {
    unsigned char digest[16];
    hash_t h;
  } result;
  int i;

  MD5_Init(&md5);
  for (i=0; i < Ntoken; i++) {
    MD5_Update(&md5, &tok[i], sizeof tok[i]);
  }
  MD5_Final(result.digest, &md5);
  return result.h;
}

/**
 * Get the next hash.
 *
 * Continuously reads tokens from lexer, forms n-grams,
 * and returns their hashes.
 * Return the hash for the next n-gram or 0 if no tokens are left.
 */
hash_t next_hash(void)
{
  int tok;
  while ((tok = yylex()) != 0) {
    // shift chain-window
    for (int i=Ntoken; --i > 0; )
      token[i] = token[i-1];
    token[0] = tok;

    // fill the first chain
    if (++ntoken < Ntoken) continue;

    return hash(token);
  }
  return 0;
}


/**
 * Output a given hash.
 */
void record(hash_t h)
{
  (void) fprintf(outfile, "%0lx\n", h);
}


/**
 * Perform mod-z fingerprinting
 * Return the number of fingerprints.
 */
int modulo_fp(void)
{
  hash_t h, zeromask;
  int count = 0;

  zeromask = (1<<Zerobits)-1;

  while ((h = next_hash()) != 0) {
    if ((h & zeromask) == 0) {
      record(h>>Zerobits);
      count++;
    }
  }
  return count;
}


void signature(FILE *f)
{
  token = malloc(Ntoken * sizeof(int));
  ntoken = 0;
  yyin = f;

  (void) modulo_fp();
}

#if 0
void winnow(int w) {
  // circular buffer implementing window of size w
  hash_t h[w];
  for (int i=0; i<w; ++i) h[i] = INT_MAX;
  int r = 0;      // window right end
  int min = 0;    // index of minimum hash
  // At the end of each iteration, min holds the
  // position of the rightmost minimal hash in the
  // current window.  record(x) is called only the
  // first time an instance of x is selected as the
  // rightmost minimal hash of a window.
  while (true) {
    r = (r + 1) % w;      // shift the window by one
    h[r] = next_hash();   // and add one new hash
    if (min == r) {
      // The previous minimum is no longer in this
      // window.  Scan h leftward starting from r
      // for the rightmost minimal hash.  Note min
      // starts with the index of the rightmost
      // hash.
      for(int i=(r-1)%w; i!=r; i=(i-1+w)%w)
        if (h[i] < h[min]) min = i;
      record(h[min], global_pos(min, r, w));
    } else {
      // Otherwise, the previous minimum is still in
      // this window. Compare against the new value
      // and update min if necessary.
      if (h[r] <= h[min]) {  // (*)
        min=r;
        record(h[min], global_pos(min, r, w));
      }
    }
  }
}
#endif
