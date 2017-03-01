
fpcc - Fingerprint C Code
=========================

A set of tools to detect similarities between C source files.

Tool overview:

* `sig` : create fingerprints of C source files
* `idx` : create an index of the fingerprints
* `comp`: compare indices for [resemblance and containment][4], score in %
* `map` : find similar regions based on the indices
* `diff`: show similar regions given the output of `map`
* `help`: display help for a tool



To compare two source code files, first create their fingerprints with
`fpcc sig`.  Create an index with `idx` and  use `comp` to compare those
fingerprints and/or find similar regions with `map`+`diff`.


The tools are based on `sig` and `comp`, two [programs attributed to Rob
Pike][1].

As frontend/preprocessor (in `sig`), a C lexer is used, based on a freely
available [C grammar][2].  It creates n-grams from lexical tokens, i.e., their
IDs as defined by the yacc spec. This way, variable names, literal values,
formatting, etc is ignored. The scanner also ignores comments and preprocessor
directives.  The set of hashes that comprises the fingerprint is selected based
on winnowing, as described by [Schleimer, Wilkerson, Aiken, "Winnowing: Local
Algorithms for Document Fingerprinting"][3].


Requirements
------------

The frontend uses a grammar, it requires `flex` and `bison`.
As hash function the simple MD5 algorithm is used (openssl/md5.h).
For the documentation, man pages are created with `txt2man`.
In Ubuntu, to install the required packages, type
```bash
sudo apt-get install flex bison libssl-dev txt2man
```
To build the tools, simply type `make`.


Quickstart
----------

Assume you want to compute resemblance of documents.

1. Create fingerprint indices for each project you want to compare:
   ```bash
   $ fpcc sig mycfile1.c | fpcc idx -o mycfile1.sig
   $ fpcc sig mycfile2.c | fpcc idx -o mycfile2.sig
   ```
2. Compare the fingerprints:
   ```bash
   $ fpcc comp mycfile1.sig mycfile2.sig
   mycfile1.sig and mycfile2.sig: 34%
   ```
   You can also generate a list of signature files to perform an n-to-n
   comparison:
   ```bash
   $ fpcc sig mycfile2.c | fpcc idx -o mycfile3.sig # a copy
   $ echo mycfile1.sig > mylist.txt
   $ echo mycfile2.sig >> mylist.txt
   $ echo mycfile3.sig >> mylist.txt
   $ fpcc comp -L mylist.txt
   mycfile1.sig and mycfile2.sig: 34%
   mycfile1.sig and mycfile3.sig: 34%
   mycfile2.sig and mycfile3.sig: 100%
   ```


Credits
-------

`sig` and `comp` can be found on a [website][1] that offers `sherlock`,
which combined and slightly modified the both tools.
The author seems to be Lachlan "Loki" Patrick; however, the tools seem to be
unmaintained for a long time.



Deviations from the original sig and comp
-----------------------------------------

I started with the idea to take the original programs and simply plug in
a C lexer as frontend. However, the changes accumulated and I ended up
rewriting the tools almost completely.
Following changes were made:

* `sig`
  - plugged in a C lexer as frontend and using lexical token IDs as units
  - using MD5 as hashing function (but only using 64 bits)
  - implement winnowing instead of modulo 2^z as hash selection

* `comp`
  - The calculation of document resemblance was wrong, to be more precise: if
    there is a common hash in both sets, it sums up the number of occurences of
    this hash of both sets.  As a result, e.g. the hashes like 'a a a a b' and
    'a b b b b' would score 100% similarity. Btw, this bug persists in
    `sherlock`.
  - comp accepts a pair of files as argument or an option specifying a file
    that contains a list of files to compare. It was tempting to call comp
    within a `find ... -exec comp ...` command to perform an n-to-n comparison.
    However, when hitting the ARG_MAX limit, comm would be executed more than
    once with a subset of the files each time.
  - added option to ignore a set of "base hashes"

* both:
  - they read and write binary data
  - sorting is moved from comp to sig, performed before writing


Contribute
----------

In principle, all tools except `sig` (the "frontend") are source language
agnostic.  One could extend `sig` to handle more than C source code.

Reorganize and package the toolbox up, with proper configuration and
installation.


License
-------

I release the toolbox under the MIT License (see LICENSE).

---

Contact: Daniel Prokesch
  daniel.prokesch (at) gmail.com


References
----------

Saul Schleimer, Daniel S. Wilkerson, and Alex Aiken,
"Winnowing: local algorithms for document fingerprinting" (2003)
In Proceedings of the 2003 ACM SIGMOD international conference on Management
of data (SIGMOD '03)
DOI=http://dx.doi.org/10.1145/872757.872770
https://theory.stanford.edu/~aiken/publications/papers/sigmod03.pdf

Andrei Broder,
"On the Resemblance and Containment of Documents" (1997)
In Proceedings of the Compression and Complexity of Sequences 1997
(SEQUENCES '97)
https://pdfs.semanticscholar.org/b2ec/74c72d99b755325dc470dec2949d69cd4d57.pdf


Walter F. Tichy,
"The String-to-String Correction Problem with Block Moves" (1983).
Computer Science Technical Reports. Paper 378.
http://docs.lib.purdue.edu/cstech/378


---

[1]: http://www.cs.usyd.edu.au/~scilect/sherlock/
[2]: http://www.quut.com/c/ANSI-C-grammar-l-2011.html
[3]: https://theory.stanford.edu/~aiken/publications/papers/sigmod03.pdf
[4]: https://pdfs.semanticscholar.org/b2ec/74c72d99b755325dc470dec2949d69cd4d57.pdf
