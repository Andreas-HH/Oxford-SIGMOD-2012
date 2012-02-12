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
# Author: Lukas M. Maas <Lukas_Michael.Maas@mailbox.tu-dresden.de>, T. Kissinger <thomas.kissinger@tu-dresden.de>
#.
# Current version: 1.0 (released December 14, 2011)
#.
# Version history:
#   - 1.0 Initial release (December 14, 2011).

#CC=gcc
#CXX=g++

CFLAGS=-O0 -Wall -g -I. -I./include -I./common
CXXFLAGS=$(CFLAGS)
LDFLAGS=-lpthread -ldb_cxx -lrt

REFIMPLO=example/BDBImpl.o example/ConnectionManager.o example/Index.o example/Iterator.o example/Util.o
IMPL=$(REFIMPLO)
PROGRAMS=unittest basedriver
COMMON=common/argument_parser.o

all: $(PROGRAMS)

UNITTESTO=unittests/main.o unittests/test_runner.o unittests/test_util.o unittests/tests.o
BASEDRIVERO=benchmark/basedriver.o

unittest: $(IMPL) $(COMMON) $(UNITTESTO)
	$(CXX) $(CXXFLAGS) -o unittest $(IMPL) $(COMMON) $(UNITTESTO) $(LDFLAGS)

basedriver: $(IMPL) $(COMMON) $(BASEDRIVERO)
	$(CXX) $(CXXFLAGS) -o basedriver $(IMPL) $(COMMON) $(BASEDRIVERO) $(LDFLAGS)


clean:
	$(RM) -R $(PROGRAMS)
	find . -name '*.o' -print | xargs rm -f 


















