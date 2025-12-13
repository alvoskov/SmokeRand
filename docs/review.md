# Existing solutions

Existing solutions:

1. [TestU01](https://doi.org/10.1145/3447773). Has a very comprehensive set of
   tests and an excellent documentation. Doesn't support 64-bit generator,
   has issues with analysis of lowest bits.
2. [PractRand](https://pracrand.sourceforge.net/). Very limited number of tests
   but they are good ones and detect flaws in a lot of PRNGs. Tests are mostly
   suggested by the PractRand author and is not described in scientific
   publications. Their advantage -- they accumulate information from a sequence
   of 1024-byte blocks and can be applied to arbitrary large samples.
3. [Ent](https://www.fourmilab.ch/random/). Only very basic tests, even 64-bit
   LCG will probably pass them.
4. [Dieharder](https://webhome.phy.duke.edu/~rgb/General/dieharder.php).
   Resembles DIEHARD, but contains more tests and uses much larger samples.
   Less sensitive than TestU01.
5. [gjrand](https://gjrand.sourceforge.net/). Has some unique and sensitive
   tests but documentation is scarce.
6. [DIEHARD](https://web.archive.org/web/20160125103112/http://stat.fsu.edu/pub/diehard/).
   A classical test suite by G. Marsaglia. TestU01 and PractRand are much more
   sensitive, but its source code contains some intesting combined PRNGs.
