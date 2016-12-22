
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


Deviations from the original sig and comp
-----------------------------------------

* `sig`
  - plugged in a C lexer as frontend and using lexical token IDs as units
  - using MD5 as hashing function (but only using 64 bits)
  - implement winnowing instead of modulo 2^z as hash selection

* `comp`
  - The calculation of document resemblance is wrong, to be more precise:
    if there is a common hash in both sets, it sums up the number
    of occurences of this hash of both sets.
    As a result, e.g. the hashes like 'a a a a b' and 'a b b b b' would
    score 100% similarity.
  - comp only accepts a pair of files as argument. It was tempting to
    call comp within a `find ... -exec comp ...` command to perform an
    n-to-n comparison. However, when hitting the ARG_MAX limit, comm would be
    executed more than once with a subset of the files each time.
    You can use the provided awk script allpairs.awk to create all pairs of a
    list of files, like
    ```bash
    find ... | allpairs.awk | xargs -L1 comp -t 0
    ```
* both:
  - they read and write binary data
  - sorting is moved from comp to sig, performed before writing


Contact: Daniel Prokesch
  daniel.prokesch (at) gmail.com

[1]: http://www.cs.usyd.edu.au/~scilect/sherlock/
[2]: http://www.quut.com/c/ANSI-C-grammar-l-2011.html
[3]: https://theory.stanford.edu/~aiken/publications/papers/sigmod03.pdf
