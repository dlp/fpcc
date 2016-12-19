/*
 *  csig.c - based on sig.c written by Rob Pike
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <openssl/md5.h>


extern int yylex (void);
extern FILE *yyin;

int		Ntoken = 5;
int		Zerobits = 4;
unsigned long	zeromask;
int *		token;
FILE *		outfile;

void	signature(FILE*);

const char *program_name = "csig";

void usage(void)
{
  (void) fprintf(stderr, "USAGE: %s [-z zerobits] [-n chainlength]"
                         " [-o outfile] file...\n", program_name);
  (void) fprintf(stderr, "  defaults: zerobits=%d chainlength=%d\n",
      Zerobits, Ntoken);
  exit(2);
}

int main(int argc, char *argv[])
{
	FILE *f;
	int  nfiles;
	char *outname;

	outfile = stdout;
	outname = NULL;

        int c;
	while ((c = getopt(argc, argv, "z:n:o:")) != -1) {
          switch (c) {
            case 'z':
              Zerobits = atoi(optarg);
              if (Zerobits < 0 || Zerobits > 31)
                usage();
              break;
            case 'n':
              Ntoken = atoi(optarg);
              if (Ntoken <= 0)
                usage();
              break;
            case 'o':
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

	zeromask = (1<<Zerobits)-1;

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

unsigned long hash(int tok[])
{
  MD5_CTX md5;
  union {
    unsigned char digest[16];
    unsigned long h;
  } result;
  int i;

  MD5_Init(&md5);
  for (i=0; i < Ntoken; i++) {
    MD5_Update(&md5, &tok[i], sizeof tok[i]);
  }
  MD5_Final(result.digest, &md5);
  return result.h;
}

void dotoken(int x)
{
  static int ntoken = 0;
  unsigned long h;

  for (int i=Ntoken; --i > 0; )
    token[i] = token[i-1];
  token[0] = x;

  // fill the first chain
  if (++ntoken < Ntoken) return;

  h = hash(token);
  // mod-p fingerprinting -> to be replaced by winnowing
  if ((h & zeromask) == 0) {
    (void) fprintf(outfile, "%0lx\n", h>>Zerobits);
    //(void) fprintf(outfile, "%016lx %d\n", h>>Zerobits, x);
  }
}


void signature(FILE *f)
{
  token = malloc(Ntoken * sizeof(int));

  yyin = f;
  int tok;
  while ((tok = yylex()) != 0) {
    dotoken(tok);
  }
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