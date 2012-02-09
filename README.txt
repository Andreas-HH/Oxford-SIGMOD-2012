ACM SIGMOD 2012 Programming Contest
© 2011 TU Dresden - Database Technology Group
Lukas M. Maas <Lukas_Michael.Maas@mailbox.tu-dresden.de>

All of this product's contens was created by the author of this file.

Exceptions: The ACM SIGMOD logo is property of the ACM Special Interest
Group on Management of Data (http://www.acm.org/sigmod/).


=============================================

ACM SIGMOD 2012 Programming Contest

=============================================

This readme file contains information about using the contents of this archive to build and test your contest submission.

The detailed task description of the contest can be found at: http://wwwdb.inf.tu-dresden.de/sigmod2012contest/

----------------------------------------------
-                  Contents                  -
----------------------------------------------
Included in this release are the following files and directories:
  - README.txt	                  This document
  - LICENSE.txt	                  Software license
  - CHANGES.txt                   Changes since the last release
  - makefile                      Makefile to compile the provided source code
  - doxyfile                      A configuration file that may be used to generate a clean documentation
                                  of include/contest_interface.h using Doxygen
  - include/contest_interface.h   The API that participants of the contest must implement
  - common/                       Contains source code that is used by multiple build targets
  - unittests/                    Contains source code that is only used to execute unit tests for
                                  the API defined by include/contest_interface.h
  - assets/                       Contains miscellaneous files (e.g. the official SIGMOD logo)


----------------------------------------------
-               Getting started              -
----------------------------------------------
The interface that participants of this year's SIGMOD Programming Contest must implement is
defined inside include/contest_interface.h
You can build a clean html version of the documentation using the provided doxyfile.

In order to compile the unit tests (and later the benchmark that we will provide) you need
to place your implementation, compiled as a shared library, inside the lib-directory (you
can change this directory by modifying the makefile). It has to be named libcontest.so.
If you prefer another name you can change it by editing the LIBRARY variable inside the
makefile.

All code will compile and run on most UNIX-compatible systems using the GNU toolchain
(including Linux and OS X).

For a list of all build targets available use the 'make help' command.


----------------------------------------------
-         Testing your implementation        -
----------------------------------------------
We provide a set of correctness tests that may be used to ensure that your implementation
handles common tasks in the right way.

You can compile the unit tests by using the unittests build target,
placing the generated binary (unittests) inside the bin-folder:

  make unittests

Typing:

  make run-unittests

will build and also run the unit tests.


----------------------------------------------
-       Something we would like to add       -
----------------------------------------------
You may come up with tricks to improve the performance of your implementation on
our benchmark. This is totally legitimate, as long as you do not optimize your
solution only to specifically fit our benchmark. This means that you may
optimize your solution to work best with uniform key distribution (which actually
is really common in some real-life scenarios) or add other optimization that you
think are reasonable.

If you are not sure if your optimization is legitimate, please consider asking yourself:
Would it make sense in a real-life product?

If so, it is unlikely that we will contact you and ask to remove it (we will
never disqualify teams because they optimized there solution - we just prefer
general solutions).
