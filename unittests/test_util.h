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
Provides all necessary classes to build and run a set of unit tests.
*/

#include <iostream>
#include <map>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <common/macros.h>

#ifndef _UNITTESTS_TEST_UTIL_H_
#define _UNITTESTS_TEST_UTIL_H_

void CopyRecordX(Record *dst, Record *src);
void DeleteRecordX(Record **record);

// Defines a simple Tester class.
//
// Tester implements the Singleton Pattern.
class Tester{
   public:
    // Return the single instance
    static Tester& getInstance();
    
    // Register a test function under the given name
    bool RegisterTest(const char* name, void (*func)());

    // Run all registered tests
    void RunTests();

    // Assert the given condition
    void Assert(const bool condition, const char* file, const int line, const char* message);

    // Setter functions
    Tester& set_num_errors(unsigned int num_errors){num_errors_=num_errors; return *this;}

    // Getter functions
    unsigned int num_errors() const { return num_errors_; };
   
   private:
    // Private constructor (don't allow instantiation from outside)
    Tester();

    // Destructor
    ~Tester();
    
    // A map of test names and their associated test functions
    std::map<const char*, void (*)()> tests_;

    // The number of errors that occurred during the last run
    unsigned int num_errors_;

    DISALLOW_COPY_AND_ASSIGN(Tester);
};

// To invoke the registration of the new test from outside a function,
// we create a global helper variable that will be assigned the returning value of the registration function
//
// Usage:
//   TEST(TestName){
//    // Do some testing...
//   };
#define TEST(name)                                      \
  void name##_TEST_();                                    \
  bool name##_TEST_helper_ = Tester::getInstance().RegisterTest(#name,&name##_TEST_);        \
  void name##_TEST_()

// Macros used for testing
#define ASSERT_EQUALS(a,b,message) Tester::getInstance().Assert(((a)==(b)),__FILE__,__LINE__,message)
#define ASSERT_NOT_EQUAL(a,b,message) Tester::getInstance().Assert(((a)!=(b)),__FILE__,__LINE__,message)
#define ASSERT_GT(a,b,message) Tester::getInstance().Assert(((a)>(b)),__FILE__,__LINE__,message)
#define ASSERT_GEQ(a,b,message) Tester::getInstance().Assert(((a)>=(b)),__FILE__,__LINE__,message)
#define ASSERT_LT(a,b,message) Tester::getInstance().Assert(((a)<(b)),__FILE__,__LINE__,message)
#define ASSERT_LEQ(a,b,message) Tester::getInstance().Assert(((a)<=(b)),__FILE__,__LINE__,message)

#endif // _UNITTESTS_TEST_UTIL_H_
