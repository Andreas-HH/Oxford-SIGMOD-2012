/*
Copyright (c) 2011 TU Dresden - Database Technology Group

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author: Lukas M. Maas <Lukas_Michael.Maas@mailbox.tu-dresden.de>

Current version: 1.0 (released December 14, 2011)

Version history:
  - 1.0 Initial release (December 14, 2011) 
*/

/** @file
Provides a command line interface to the \ref TestRunner class.
*/

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>

#include <common/argument_parser.h>
#include "test_runner.h"

/**
 * MAIN FUNCTION
 */
int main(int argc, char** argv) {
     
  // Create a new command line argument parser
  ArgumentParser parser;
  parser.prog("unittest").description("ACM SIGMOD 2012 Programming Contest unit test runner");
  
  // Parse the command line parameters
  if(!parser.parse_args(argc, argv))
    return 1;
  
  // Create a new test runner...
  TestRunner *runner = new TestRunner();
  
  // ...and run it
  runner->run();

  return 0;
}
