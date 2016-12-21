
fpcc - Fingerprint C Code
=========================

A set of tools to detect similarities between C source files.

To compare two source code files, first create their fingerprints with `csig`.
Then use `comp` to compare those fingerprints.

The tools are based on `sig` and `comp`, two [programs attributed to Rob
Pike][1].
As frontend/preprocessor, a C lexer is used, based on a freely available
[C grammar][2].  It creates n-grams from lexical tokens, i.e., their IDs as
defined by the yacc spec. This way, variable names, literal values, formatting,
etc is ignored. The scanner also ignores comments and preprocessor directives.
The set of hashes that comprises the fingerprint is selected based on
[winnowing][3].


Requirements
------------
The frontend uses a grammar, it requires `flex` and `bison`.
As hash function the simple MD5 algorithm is used (openssl/md5.h).
In Ubuntu, to install the required packages, type
```bash
sudo apt-get install flex bison libssl-dev
```

Credits
-------
I release the rewrite, csig.c, under the MIT License (see LICENSE).
At the time of writing, the similarity between csig.c and sig.c
is reported as 34% with chainlength 5 and winnow 4.


Contact: Daniel Prokesch
  daniel.prokesch (at) gmail.com

[1]: http://www.cs.usyd.edu.au/~scilect/sherlock/
[2]: http://www.quut.com/c/ANSI-C-grammar-l-2011.html
[3]: https://theory.stanford.edu/~aiken/publications/papers/sigmod03.pdf
