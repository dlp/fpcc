
fpcc - Fingerprint C Code
=========================

A set of tools detect similarities between C source files.

To compare two source code files, first create their fingerprints with `csig`.
Then use `comp` to compare those fingerprints.

The tools are based on `sig` and `comp`, two [programs attributed to Rob
Pike][1].
As frontend/preprocessor, a C lexer is used, based on a freely available
[C grammar][2].
The set of hashes that comprises the fingerprint is selected based on
[winnowing][3].


Requirements
------------
The frontend uses a grammar, it requires `flex` and `bison`.
As hash function, the simple MD5 algorighm is used (openssl/md5.h).
In Ubuntu, to install the required packages, type
```bash
sudo apt-get install flex bison libssl-dev
```

[1]: http://www.cs.usyd.edu.au/~scilect/sherlock/
[2]: http://www.quut.com/c/ANSI-C-grammar-l-2011.html
[3]: https://theory.stanford.edu/~aiken/publications/papers/sigmod03.pdf
