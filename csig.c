/*
 *  sig.c - written by Rob Pike, modified by Loki
 *
 *  This program doesn't quite do what the specification says.
 *  It adds a '-o outfile' option which allows the user to
 *  specify which file should receive the output (rather than
 *  always going to stdout). The default is still stdout.
 *  It also doesn't use stdin as input. This was done so that
 *  the usage message would appear if the user just types 'sig'.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <openssl/md5.h>


extern int yylex (void);
extern FILE *yyin;

int		Ntoken = 5;
int		Zerobits = 4;
unsigned long	zeromask;
int *		token;
FILE *		outfile;

void	signature(FILE*);

void usage(void)
{
	fprintf(stderr, "usage: sig");
	fprintf(stderr, " [-z zerobits]");
	fprintf(stderr, " [-n chainlength]");
	fprintf(stderr, " [-o outfile]");
	fprintf(stderr, " file ...\n");

	fprintf(stderr, "defaults:");
	fprintf(stderr, " zerobits=4");
	fprintf(stderr, " chainlength=5");
	fprintf(stderr, " outfile=the screen");
	fprintf(stderr, "\n");
	exit(2);
}

int main(int argc, char *argv[])
{
	FILE *f;
	int i, start, nfiles;
	char *s, *outname;

	outfile = stdout;
	outname = NULL;

	for (start=1; start < argc; start++) {
		if (argv[start][0] != '-')
			break;
		switch (argv[start][1]) {
		case 'z':
			s = argv[++start];
			if (s == NULL)
				usage();
			Zerobits = atoi(s);
			if (Zerobits < 0 || Zerobits > 31)
				usage();
			break;
		case 'n':
			s = argv[++start];
			if (s == NULL)
				usage();
			Ntoken = atoi(s);
			if (Ntoken <= 0)
				usage();
			break;
		case 'o':
			s = argv[++start];
			if (s == NULL)
				usage();
			outname = s;
			break;
		default:
			usage();
		}
	}

	nfiles = argc - start;
	if (nfiles < 1)
		usage();

	if (outname != NULL)
		outfile = fopen(outname, "w");

	zeromask = (1<<Zerobits)-1;

	for (i=start; i < argc; i++) {
		f = fopen(argv[i], "r");
		if (f == NULL) {
			fprintf(stderr, "sig: can't open %s:", argv[i]);
			perror(NULL);
			continue;
		}
		signature(f);
		fclose(f);
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

