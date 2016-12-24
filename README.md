
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
Another earlier paper by [Andrei Broder, "On the resemblance and containment
of documents"][4] gives more background on
document fingerprinting.


Requirements
------------

The frontend uses a grammar, it requires `flex` and `bison`.
As hash function the simple MD5 algorithm is used (openssl/md5.h).
In Ubuntu, to install the required packages, type
```bash
sudo apt-get install flex bison libssl-dev
```
To build the tools, simply type `make`.


Usage
-----

  ```
  SYNOPSIS

  csig [-n chainlength] [-w winnow] [-o outfile] file...
    defaults: chainlength=5 winnow=4

  comp [-b basefile] [-t threshold] sigfile1 sigfile2
  comp [-b basefile] [-t threshold] [-L filelist]
    defaults: threshold=0
  ```

Options: tbd

1. Create fingerprints for each source file you want to compare:
   ```bash
   $ ./csig -o mycfile1.sig mycfile1.c
   $ ./csig -o mycfile2.sig mycfile2.c
   ```
2. Compare the fingerprints:
   ```bash
   $ ./comp mycfile1.sig mycfile2.sig
   mycfile1.sig and mycfile2.sig: 34%
   ```
   You can also generate a list of signature files to perform an n-to-n
   comparison:
   ```bash
   $ ./csig -o mycfile3.sig mycfile2.c # a copy
   $ echo mycfile1.sig > mylist.txt
   $ echo mycfile2.sig >> mylist.txt
   $ echo mycfile3.sig >> mylist.txt
   $ ./comp -L mylist.txt
   mycfile1.sig and mycfile2.sig: 34%
   mycfile1.sig and mycfile3.sig: 34%
   mycfile2.sig and mycfile3.sig: 100%
   ```


Credits
-------

`sig` and `comp` can be found on a [website][1] that offers `sherlock`,
which combined and slightly modified the both tools.
The author calls himself "Loki"; I couldn't find any contact information nor
the tools elsewhere on the web.


License
-------

I release the rewritten tools under the MIT License (see LICENSE).


Deviations from the original sig and comp
-----------------------------------------

I started with the idea to take the original programs and simply plug in
a C lexer as frontend. However, the changes accumulated and I ended
rewriting the tools almost completely.
Following changes were made:

* `sig`
  - plugged in a C lexer as frontend and using lexical token IDs as units
  - using MD5 as hashing function (but only using 64 bits)
  - implement winnowing instead of modulo 2^z as hash selection

* `comp`
  - The calculation of document resemblance is wrong, to be more precise: if
    there is a common hash in both sets, it sums up the number of occurences of
    this hash of both sets.  As a result, e.g. the hashes like 'a a a a b' and
    'a b b b b' would score 100% similarity. Btw, this bug persists in
    `sherlock`.
  - comp accepts a pair of files as argument or an option specifying a file
    that contains a list of files to compare. It was tempting to call comp
    within a `find ... -exec comp ...` command to perform an n-to-n comparison.
    However, when hitting the ARG_MAX limit, comm would be executed more than
    once with a subset of the files each time.

* both:
  - they read and write binary data
  - sorting is moved from comp to sig, performed before writing


Contact: Daniel Prokesch
  daniel.prokesch (at) gmail.com

[1]: http://www.cs.usyd.edu.au/~scilect/sherlock/
[2]: http://www.quut.com/c/ANSI-C-grammar-l-2011.html
[3]: https://theory.stanford.edu/~aiken/publications/papers/sigmod03.pdf
[4]: https://pdfs.semanticscholar.org/b2ec/74c72d99b755325dc470dec2949d69cd4d57.pdf
