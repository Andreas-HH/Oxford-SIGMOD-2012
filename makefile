# Copyright (c) 2011 TU Dresden - Database Technology Group
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# Author: Lukas M. Maas <Lukas_Michael.Maas@mailbox.tu-dresden.de>
# 
# Current version: 1.0 (released December 14, 2011)
# 
# Version history:
#   - 1.0 Initial release (December 14, 2011) 

COMPILER = g++
CFLAGS = -Wall
LDFLAGS = 

BINDIR = ./bin
INCDIR = ./include
LIBDIR = ./lib

# The name of the library implementing the contest interface
LIBRARY = contest

# Directory related rules
$(BINDIR):
	mkdir -p $(BINDIR)

$(LIBDIR):
	mkdir -p $(LIBDIR)

# Unit tests
unittests: $(BINDIR)
	$(COMPILER) -I. -I$(INCDIR) $(CFLAGS) unittests/main.cc unittests/tests.cc unittests/test_runner.cc unittests/test_util.cc common/argument_parser.cc -L$(LIBDIR) -lpthread $(LDFLAGS) -o $(BINDIR)/unittests -l$(LIBRARY)

# The following targets may be used to run the created executables
run-unittests: unittests
	$(BINDIR)/unittests

run: run-unittests

# Cleanup all files created during the build process
clean:
	rm -rf $(BINDIR) $(LIBDIR)

# Default target
all: unittests

# Display a help page
help:
	@echo
	@echo "Build SIGMOD 2012 Programming Contest"
	@echo "============================================"
	@echo "build targets:"
	@echo "  cleanup           Delete all files created during the build process"
	@echo "  help              Display this help"
	@echo "  run-unittests     Build and execute the unit tests"
	@echo "  unittests         Build the unit tests"
	@echo
	@echo "binary output directory: $(BINDIR)"
	@echo "library directory: $(LIBDIR)"
	@echo
